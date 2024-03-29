/* libmpdclient
   (c)2003-2006 by Warren Dukes (warren.dukes@gmail.com)
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "libmpdclient.h"
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/param.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>

#include <glib.h>

#ifdef WIN32
#  include <ws2tcpip.h>
#  include <winsock.h>
#else
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <netdb.h>
#endif

/* (bits+1)/3 (plus the sign character) */
#define INTLEN      ((sizeof(int)       * CHAR_BIT + 1) / 3 + 1)
#define LONGLONGLEN ((sizeof(long long) * CHAR_BIT + 1) / 3 + 1)

#define COMMAND_LIST    1
#define COMMAND_LIST_OK 2

#ifndef MPD_NO_GAI
#  ifdef AI_ADDRCONFIG
#    define MPD_HAVE_GAI
#  endif
#endif

#ifndef WIN32
#include <sys/un.h>
#endif

#ifndef MSG_DONTWAIT
#  define MSG_DONTWAIT 0
#endif

#ifdef WIN32
#  define SELECT_ERRNO_IGNORE   (errno == WSAEINTR || errno == WSAEINPROGRESS)
#  define SENDRECV_ERRNO_IGNORE SELECT_ERRNO_IGNORE
#else
#  define SELECT_ERRNO_IGNORE   (errno == EINTR)
#  define SENDRECV_ERRNO_IGNORE (errno == EINTR || errno == EAGAIN)
#  define winsock_dll_error(c)  0
#  define closesocket(s)        close(s)
#  define WSACleanup()          do { /* nothing */ } while (0)
#endif

#ifdef WIN32
static int winsock_dll_error(mpd_Connection *connection)
{
	WSADATA wsaData;
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0 ||
			LOBYTE(wsaData.wVersion) != 2 ||
			HIBYTE(wsaData.wVersion) != 2 ) {
		strcpy(connection->errorStr,
		       "Could not find usable WinSock DLL.");
		connection->error = MPD_ERROR_SYSTEM;
		return 1;
	}
	return 0;
}

static int do_connect_fail(mpd_Connection *connection,
                           const struct sockaddr *serv_addr, int addrlen)
{
	int iMode = 1; /* 0 = blocking, else non-blocking */
	if (connect(connection->sock, serv_addr, addrlen) == SOCKET_ERROR)
		return 1;
	ioctlsocket(connection->sock, FIONBIO, (u_long FAR*) &iMode);
	return 0;
}
#elif defined(__solaris__) || defined(__sun__)
static int do_connect_fail(mpd_Connection *connection,
		const struct sockaddr *serv_addr, int addrlen)
{
	int flags;
	if (connect(connection->sock, serv_addr, addrlen) < 0)
		return 1;
	flags = fcntl(connection->sock, F_GETFL, 0);
	fcntl(connection->sock, F_SETFL, flags | O_NONBLOCK);
	return 0;
}
#else /* !WIN32 (sane operating systems) */
static int do_connect_fail(mpd_Connection *connection,
                           const struct sockaddr *serv_addr, int addrlen)
{
	int flags, res,valopt;
	struct timeval tv;
	fd_set myset;
	flags = fcntl(connection->sock, F_GETFL, 0);
	fcntl(connection->sock, F_SETFL, flags | O_NONBLOCK);
	/*if (connect(connection->sock, serv_addr, addrlen) < 0)
		return 1;*/
    /* Do a non blocking connect */
    res = connect(connection->sock, serv_addr, addrlen);
    if (res < 0) { 
        if (errno == EINPROGRESS) { 
            tv.tv_sec = connection->timeout.tv_sec;
            tv.tv_usec = connection->timeout.tv_usec;
            FD_ZERO(&myset); 
            FD_SET(connection->sock, &myset); 
            if (select(connection->sock+1, NULL, &myset, NULL, &tv) > 0) { 
                socklen_t lon; 
                lon = sizeof(int); 
                /* Check for errors */
                getsockopt(connection->sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
                if (valopt) { 
                    fprintf(stderr, "Error in connection() %d - %s\n", valopt, strerror(valopt)); 
                    return 1;
                } 
            } 
            else { 
                /* Connecting timed out. */
                fprintf(stderr, "Timeout or error() %d - %s\n", valopt, strerror(valopt)); 
                return 1;
            } 
        } 
        else { 
            /* Failed to connect */
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
            return 1;
        } 
    } 

	return 0;
}
#endif /* !WIN32 */

#ifdef MPD_HAVE_GAI
static int mpd_connect(mpd_Connection * connection, const char * host, int port,
                       float timeout)
{
	int error;
	char service[INTLEN+1];
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	struct addrinfo *addrinfo = NULL;

	/**
	 * Setup hints
	 */
	hints.ai_family    = AF_UNSPEC;
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_protocol  = IPPROTO_TCP;
	hints.ai_flags     = 0;
	hints.ai_addrlen   = 0;
	hints.ai_addr      = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next      = NULL;

	snprintf(service, sizeof(service), "%i", port);

	error = getaddrinfo(host, service, &hints, &addrinfo);

	if (error) {
		snprintf(connection->errorStr, MPD_ERRORSTR_MAX_LENGTH,
		         "host \"%s\" not found: %s",
		         host, gai_strerror(error));
		connection->error = MPD_ERROR_UNKHOST;
		return -1;
	}

	for (res = addrinfo; res; res = res->ai_next) {
		/* create socket */
		if (connection->sock >= 0)
			closesocket(connection->sock);
		connection->sock = socket(res->ai_family, SOCK_STREAM,
		                          res->ai_protocol);
		if (connection->sock < 0) {
			snprintf(connection->errorStr, MPD_ERRORSTR_MAX_LENGTH,
			         "problems creating socket: %s",
			         strerror(errno));
			connection->error = MPD_ERROR_SYSTEM;
			freeaddrinfo(addrinfo);
			return -1;
		}

		mpd_setConnectionTimeout(connection, timeout);

		/* connect stuff */
 		if (do_connect_fail(connection,
		                    res->ai_addr, res->ai_addrlen)) {
 			/* try the next address */
 			closesocket(connection->sock);
 			connection->sock = -1;
 			continue;
		}

		break;
	}

	freeaddrinfo(addrinfo);

	if (connection->sock < 0) {
		snprintf(connection->errorStr, MPD_ERRORSTR_MAX_LENGTH,
		         "problems connecting to \"%s\" on port %i: %s",
		         host, port, strerror(errno));
		connection->error = MPD_ERROR_CONNPORT;

		return -1;
	}

	return 0;
}
#else /* !MPD_HAVE_GAI */
static int mpd_connect(mpd_Connection * connection, const char * host, int port,
                       float timeout)
{
	struct hostent * he;
	struct sockaddr * dest;
	int destlen;
	struct sockaddr_in sin;

	if(!(he=gethostbyname(host))) {
		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
				"host \"%s\" not found",host);
		connection->error = MPD_ERROR_UNKHOST;
		return -1;
	}

	memset(&sin,0,sizeof(struct sockaddr_in));
	/*dest.sin_family = he->h_addrtype;*/
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	switch(he->h_addrtype) {
	case AF_INET:
		memcpy((char *)&sin.sin_addr.s_addr,(char *)he->h_addr,
				he->h_length);
		dest = (struct sockaddr *)&sin;
		destlen = sizeof(struct sockaddr_in);
		break;
	default:
		strcpy(connection->errorStr,"address type is not IPv4");
		connection->error = MPD_ERROR_SYSTEM;
		return -1;
		break;
	}

	if (connection->sock >= 0)
		closesocket(connection->sock);
	if((connection->sock = socket(dest->sa_family,SOCK_STREAM,0))<0) {
		strcpy(connection->errorStr,"problems creating socket");
		connection->error = MPD_ERROR_SYSTEM;
		return -1;
	}

	mpd_setConnectionTimeout(connection,timeout);

	/* connect stuff */
	if (do_connect_fail(connection, dest, destlen)) {
		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
				"problems connecting to \"%s\" on port"
				" %i",host,port);
		connection->error = MPD_ERROR_CONNPORT;
		return -1;
	}

	return 0;
}
#endif /* !MPD_HAVE_GAI */

char * mpdTagItemKeys[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
	"Artist",
	"Album",
	"Title",
	"Track",
	"Name",
	"Genre",
	"Date",
	"Composer",
	"Performer",
	"Comment",
	"Disc",
	"Filename",
    "AlbumArtist",
	"Any"
};

static char * mpd_sanitizeArg(const char * arg) {
	size_t i;
	char * ret;
	register const char *c;
	register char *rc;

	/* instead of counting in that loop above, just
	 * use a bit more memory and half running time
	 */
	ret = malloc(strlen(arg) * 2 + 1);

	c = arg;
	rc = ret;
	for(i = strlen(arg)+1; i != 0; --i) {
		if(*c=='"' || *c=='\\')
			*rc++ = '\\';
		*(rc++) = *(c++);
	}

	return ret;
}

static mpd_ReturnElement * mpd_newReturnElement(const char * name, const char * value)
{
	mpd_ReturnElement* ret = g_slice_new(mpd_ReturnElement);

	ret->name = strdup(name);
	ret->value = strdup(value);

	return ret;
}

static void mpd_freeReturnElement(mpd_ReturnElement * re) {
	free(re->name);
	free(re->value);
	g_slice_free(mpd_ReturnElement, re);
}

void mpd_setConnectionTimeout(mpd_Connection * connection, float timeout) {
	connection->timeout.tv_sec = (int)timeout;
	connection->timeout.tv_usec = (int)(timeout*1e6 -
	                                    connection->timeout.tv_sec*1000000 +
					    0.5);
}

static int mpd_parseWelcome(mpd_Connection * connection, const char * host, int port,
                            char * output) {
	char * tmp;
	char * test;
	int i;

	if(strncmp(output,MPD_WELCOME_MESSAGE,strlen(MPD_WELCOME_MESSAGE))) {
		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
				"mpd not running on port %i on host \"%s\"",
				port,host);
		connection->error = MPD_ERROR_NOTMPD;
		return 1;
	}

	tmp = &output[strlen(MPD_WELCOME_MESSAGE)];

	for(i=0;i<3;i++) {
		if(tmp) connection->version[i] = strtol(tmp,&test,10);

		if (!tmp || (test[0] != '.' && test[0] != '\0')) {
			snprintf(connection->errorStr,
			         MPD_ERRORSTR_MAX_LENGTH,
			         "error parsing version number at "
			         "\"%s\"",
			         &output[strlen(MPD_WELCOME_MESSAGE)]);
			connection->error = MPD_ERROR_NOTMPD;
			return 1;
		}
		tmp = ++test;
	}

	return 0;
}

#ifndef WIN32
static int mpd_connect_un(mpd_Connection * connection,
			  const char * host, float timeout)
{
	int error, flags;
	size_t path_length;
	struct sockaddr_un saun;

	path_length = strlen(host);
	if (path_length >= sizeof(saun.sun_path)) {
		strcpy(connection->errorStr, "unix socket path is too long");
		connection->error = MPD_ERROR_UNKHOST;
		return -1;
	}

	saun.sun_family = AF_UNIX;
	memcpy(saun.sun_path, host, path_length + 1);

	connection->sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connection->sock < 0) {
		strcpy(connection->errorStr, "problems creating socket");
		connection->error = MPD_ERROR_SYSTEM;
		return -1;
	}

	mpd_setConnectionTimeout(connection, timeout);

	flags = fcntl(connection->sock, F_GETFL, 0);
	fcntl(connection->sock, F_SETFL, flags | O_NONBLOCK);

	error = connect(connection->sock, (struct sockaddr*)&saun, sizeof(saun));
	if (error < 0) {
		/* try the next address family */
		close(connection->sock);
		connection->sock = 0;

		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
			 "problems connecting to \"%s\": %s",
			 host, strerror(errno));
		connection->error = MPD_ERROR_CONNPORT;
		return -1;
	}

	return 0;
}
#endif /* WIN32 */

mpd_Connection * mpd_newConnection(const char * host, int port, float timeout) {
	int err;
	char * rt;
	char * output =  NULL;
	mpd_Connection * connection = g_slice_new0(mpd_Connection);
	struct timeval tv;
	fd_set fds;
	strcpy(connection->buffer,"");
	connection->sock = -1;
	strcpy(connection->errorStr,"");

	if (winsock_dll_error(connection))
		return connection;

#ifndef WIN32
	if (host[0] == '/')
		err = mpd_connect_un(connection, host, timeout);
	else
#endif
		err = mpd_connect(connection, host, port, timeout);
	if (err < 0)
		return connection;

	while(!(rt = strstr(connection->buffer,"\n"))) {
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock,&fds);
		if((err = select(connection->sock+1,&fds,NULL,NULL,&tv)) == 1) {
			int readed;
			readed = recv(connection->sock,
					&(connection->buffer[connection->buflen]),
					MPD_BUFFER_MAX_LENGTH-connection->buflen,0);
			if(readed<=0) {
				snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
						"problems getting a response from"
						" \"%s\" on port %i : %s",host,
						port, strerror(errno));
				connection->error = MPD_ERROR_NORESPONSE;
				return connection;
			}
			connection->buflen+=readed;
			connection->buffer[connection->buflen] = '\0';
		}
		else if(err<0) {
 			if (SELECT_ERRNO_IGNORE)
				continue;
			snprintf(connection->errorStr,
					MPD_ERRORSTR_MAX_LENGTH,
					"problems connecting to \"%s\" on port"
					" %i",host,port);
			connection->error = MPD_ERROR_CONNPORT;
			return connection;
		}
		else {
			snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
					"timeout in attempting to get a response from"
					" \"%s\" on port %i",host,port);
			connection->error = MPD_ERROR_NORESPONSE;
			return connection;
		}
	}

	*rt = '\0';
	output = strdup(connection->buffer);
	strcpy(connection->buffer,rt+1);
	connection->buflen = strlen(connection->buffer);

	if(mpd_parseWelcome(connection,host,port,output) == 0) connection->doneProcessing = 1;

	free(output);

	return connection;
}

void mpd_clearError(mpd_Connection * connection) {
	connection->error = 0;
	connection->errorStr[0] = '\0';
}

void mpd_closeConnection(mpd_Connection * connection) {
	closesocket(connection->sock);
	if(connection->returnElement) free(connection->returnElement);
	if(connection->request) free(connection->request);
	g_slice_free(mpd_Connection, connection);
	WSACleanup();
}

static void mpd_executeCommand(mpd_Connection * connection,const char * command) {
	int ret;
	struct timeval tv;
	fd_set fds;
	const char * commandPtr = command;
	int commandLen = strlen(command);

	if(!connection->doneProcessing && !connection->commandList) {
		strcpy(connection->errorStr,"not done processing current command");
		connection->error = 1;
		return;
	}

	mpd_clearError(connection);

	FD_ZERO(&fds);
	FD_SET(connection->sock,&fds);
	tv.tv_sec = connection->timeout.tv_sec;
	tv.tv_usec = connection->timeout.tv_usec;
    while((ret = select(connection->sock+1,NULL,&fds,NULL,&tv)==1) ||
			(ret==-1 && SELECT_ERRNO_IGNORE)) {
        fflush(NULL);
		ret = send(connection->sock,commandPtr,commandLen,MSG_DONTWAIT);
		if(ret<=0)
		{
			if (SENDRECV_ERRNO_IGNORE) continue;
			snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
			         "problems giving command \"%s\"",command);
			connection->error = MPD_ERROR_SENDING;
			return;
		}
		else {
			commandPtr+=ret;
			commandLen-=ret;
		}

		if(commandLen<=0) break;
	}
	if(commandLen>0) {
		perror("");
		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
		         "timeout sending command \"%s\"",command);
		connection->error = MPD_ERROR_TIMEOUT;
		return;
	}

	if(!connection->commandList) connection->doneProcessing = 0;
	else if(connection->commandList == COMMAND_LIST_OK) {
		connection->listOks++;
	}
}

static void mpd_getNextReturnElement(mpd_Connection * connection) {
	char * output = NULL;
	char * rt = NULL;
	char * name = NULL;
	char * value = NULL;
	fd_set fds;
	struct timeval tv;
	char * tok = NULL;
	int readed;
	char * bufferCheck = NULL;
	int err;
	int pos;

	if(connection->returnElement) mpd_freeReturnElement(connection->returnElement);
	connection->returnElement = NULL;

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		strcpy(connection->errorStr,"already done processing current command");
		connection->error = 1;
		return;
	}

	bufferCheck = connection->buffer+connection->bufstart;
	while(connection->bufstart>=connection->buflen ||
			!(rt = strchr(bufferCheck,'\n'))) {
		if(connection->buflen>=MPD_BUFFER_MAX_LENGTH) {
			memmove(connection->buffer,
					connection->buffer+
					connection->bufstart,
					connection->buflen-
					connection->bufstart+1);
			connection->buflen-=connection->bufstart;
			connection->bufstart = 0;
		}
		if(connection->buflen>=MPD_BUFFER_MAX_LENGTH) {
			strcpy(connection->errorStr,"buffer overrun");
			connection->error = MPD_ERROR_BUFFEROVERRUN;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
		bufferCheck = connection->buffer+connection->buflen;
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock,&fds);
		if((err = select(connection->sock+1,&fds,NULL,NULL,&tv) == 1)) {
			readed = recv(connection->sock,
					connection->buffer+connection->buflen,
					MPD_BUFFER_MAX_LENGTH-connection->buflen,
					MSG_DONTWAIT);
			if(readed<0 && SENDRECV_ERRNO_IGNORE) {
				continue;
			}
			if(readed<=0) {
				strcpy(connection->errorStr,"connection"
				       " closed");
				connection->error = MPD_ERROR_CONNCLOSED;
				connection->doneProcessing = 1;
				connection->doneListOk = 0;
				return;
			}
			connection->buflen+=readed;
			connection->buffer[connection->buflen] = '\0';
		}
		else if(err<0 && SELECT_ERRNO_IGNORE) continue;
		else {
			strcpy(connection->errorStr,"connection timeout");
			connection->error = MPD_ERROR_TIMEOUT;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
	}

	*rt = '\0';
	output = connection->buffer+connection->bufstart;
	connection->bufstart = rt - connection->buffer + 1;

	if(strcmp(output,"OK")==0) {
		if(connection->listOks > 0) {
			strcpy(connection->errorStr, "expected more list_OK's");
			connection->error = 1;
		}
		connection->listOks = 0;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;
		return;
	}

	if(strcmp(output, "list_OK") == 0) {
		if(!connection->listOks) {
			strcpy(connection->errorStr,
					"got an unexpected list_OK");
			connection->error = 1;
		}
		else {
			connection->doneListOk = 1;
			connection->listOks--;
		}
		return;
	}

	if(strncmp(output,"ACK",strlen("ACK"))==0) {
		char * test;
		char * needle;
		int val;

		strcpy(connection->errorStr, output);
		connection->error = MPD_ERROR_ACK;
		connection->errorCode = MPD_ACK_ERROR_UNK;
		connection->errorAt = MPD_ERROR_AT_UNK;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;

		needle = strchr(output, '[');
		if(!needle) return;
		val = strtol(needle+1, &test, 10);
		if(*test != '@') return;
		connection->errorCode = val;
		val = strtol(test+1, &test, 10);
		if(*test != ']') return;
		connection->errorAt = val;
		return;
	}

	tok = strchr(output, ':');
	if (!tok) return;
	pos = tok - output;
	value = ++tok;
	name = output;
	name[pos] = '\0';

	if(value[0]==' ') {
		connection->returnElement = mpd_newReturnElement(name,&(value[1]));
	}
	else {
		snprintf(connection->errorStr,MPD_ERRORSTR_MAX_LENGTH,
					"error parsing: %s:%s",name,value);
		connection->error = 1;
	}
}

void mpd_finishCommand(mpd_Connection * connection) {
	while(!connection->doneProcessing) {
		if(connection->doneListOk) connection->doneListOk = 0;
		mpd_getNextReturnElement(connection);
	}
}

static void mpd_finishListOkCommand(mpd_Connection * connection) {
	while(!connection->doneProcessing && connection->listOks &&
			!connection->doneListOk)
	{
		mpd_getNextReturnElement(connection);
	}
}

int mpd_nextListOkCommand(mpd_Connection * connection) {
	mpd_finishListOkCommand(connection);
	if(!connection->doneProcessing) connection->doneListOk = 0;
	if(connection->listOks == 0 || connection->doneProcessing) return -1;
	return 0;
}

void mpd_sendStatusCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"status\n");
}

mpd_Status * mpd_getStatus(mpd_Connection * connection) {
	/*mpd_executeCommand(connection,"status\n");

	if(connection->error) return NULL;*/

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	mpd_Status* status = g_slice_new0(mpd_Status);
	status->volume = -1;
	status->playlist = -1;
	status->playlistLength = -1;
	status->state = -1;
	status->nextsong = -1;
	status->nextsongid = -1;
	status->crossfade = -1;

	if(connection->error) {
		g_slice_free(mpd_Status, status);
		return NULL;
	}
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"volume")==0) {
			status->volume = atoi(re->value);
		}
		else if(strcmp(re->name,"repeat")==0) {
			status->repeat = atoi(re->value);
		}
		else if(strcmp(re->name,"single")==0) {
			status->single = atoi(re->value);
		}
		else if(strcmp(re->name,"consume")==0) {
			status->consume = atoi(re->value);
		}
		else if(strcmp(re->name,"random")==0) {
			status->random = atoi(re->value);
		}
		else if(strcmp(re->name,"playlist")==0) {
			status->playlist = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"playlistlength")==0) {
			status->playlistLength = atoi(re->value);
		}
		else if(strcmp(re->name,"bitrate")==0) {
			status->bitRate = atoi(re->value);
		}
		else if(strcmp(re->name,"state")==0) {
			if(strcmp(re->value,"play")==0) {
				status->state = MPD_STATUS_STATE_PLAY;
			}
			else if(strcmp(re->value,"stop")==0) {
				status->state = MPD_STATUS_STATE_STOP;
			}
			else if(strcmp(re->value,"pause")==0) {
				status->state = MPD_STATUS_STATE_PAUSE;
			}
			else {
				status->state = MPD_STATUS_STATE_UNKNOWN;
			}
		}
		else if(strcmp(re->name,"song")==0) {
			status->song = atoi(re->value);
		}
		else if(strcmp(re->name,"songid")==0) {
			status->songid = atoi(re->value);
		}
		else if(strcmp(re->name,"nextsong")==0) {
			status->nextsong = atoi(re->value);
		}
		else if(strcmp(re->name,"nextsongid")==0) {
			status->nextsongid = atoi(re->value);
		}
		else if(strcmp(re->name,"time")==0) {
			char * tok = strchr(re->value,':');
			/* the second strchr below is a safety check */
			if (tok && (strchr(tok,0) > (tok+1))) {
				/* atoi stops at the first non-[0-9] char: */
				status->elapsedTime = atoi(re->value);
				status->totalTime = atoi(tok+1);
			}
		}
		else if(strcmp(re->name,"error")==0) {
			status->error = strdup(re->value);
		}
		else if(strcmp(re->name,"xfade")==0) {
			status->crossfade = atoi(re->value);
		}
		else if(strcmp(re->name,"updating_db")==0) {
			status->updatingDb = atoi(re->value);
		}
		else if(strcmp(re->name,"audio")==0) {
			char * tok = strchr(re->value,':');
			if (tok && (strchr(tok,0) > (tok+1))) {
				status->sampleRate = atoi(re->value);
				status->bits = atoi(++tok);
				tok = strchr(tok,':');
				if (tok && (strchr(tok,0) > (tok+1)))
					status->channels = atoi(tok+1);
			}
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			g_slice_free(mpd_Status, status);
			return NULL;
		}
	}

	if(connection->error) {
		g_slice_free(mpd_Status, status);
		return NULL;
	}
	else if(status->state<0) {
		strcpy(connection->errorStr,"state not found");
		connection->error = 1;
		g_slice_free(mpd_Status, status);
		return NULL;
	}

	return status;
}

void mpd_freeStatus(mpd_Status * status) {
	if(status->error) free(status->error);
	g_slice_free(mpd_Status, status);
}

void mpd_sendStatsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"stats\n");
}

mpd_Stats * mpd_getStats(mpd_Connection * connection) {
	/*mpd_executeCommand(connection,"stats\n");

	if(connection->error) return NULL;*/

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	mpd_Stats* stats = g_slice_new0(mpd_Stats);

	if(connection->error) {
		mpd_freeStats(stats);
		return NULL;
	}
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"artists")==0) {
			stats->numberOfArtists = atoi(re->value);
		}
		else if(strcmp(re->name,"albums")==0) {
			stats->numberOfAlbums = atoi(re->value);
		}
		else if(strcmp(re->name,"songs")==0) {
			stats->numberOfSongs = atoi(re->value);
		}
		else if(strcmp(re->name,"uptime")==0) {
			stats->uptime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"db_update")==0) {
			stats->dbUpdateTime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"playtime")==0) {
			stats->playTime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"db_playtime")==0) {
			stats->dbPlayTime = strtol(re->value,NULL,10);
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			mpd_freeStats(stats);
			return NULL;
		}
	}

	if(connection->error) {
		mpd_freeStats(stats);
		return NULL;
	}

	return stats;
}

void mpd_freeStats(mpd_Stats * stats) {
	g_slice_free(mpd_Stats, stats);
}

mpd_SearchStats * mpd_getSearchStats(mpd_Connection * connection)
{
	if (connection->doneProcessing ||
	    (connection->listOks && connection->doneListOk)) {
		return NULL;
	}

	if (!connection->returnElement) mpd_getNextReturnElement(connection);

	if (connection->error)
		return NULL;

	mpd_ReturnElement* re;
	mpd_SearchStats* stats = g_slice_new0(mpd_SearchStats);

	while (connection->returnElement) {
		re = connection->returnElement;

		if (strcmp(re->name, "songs") == 0) {
			stats->numberOfSongs = atoi(re->value);
		} else if (strcmp(re->name, "playtime") == 0) {
			stats->playTime = strtol(re->value, NULL, 10);
		}

		mpd_getNextReturnElement(connection);
		if (connection->error) {
			mpd_freeSearchStats(stats);
			return NULL;
		}
	}

	if (connection->error) {
		mpd_freeSearchStats(stats);
		return NULL;
	}

	return stats;
}

void mpd_freeSearchStats(mpd_SearchStats * stats)
{
	g_slice_free(mpd_SearchStats, stats);
}

static void mpd_finishSong(mpd_Song * song) {
	if(song->file) free(song->file);
	if(song->artist) free(song->artist);
	if(song->album) free(song->album);
	if(song->title) free(song->title);
	if(song->track) free(song->track);
	if(song->name) free(song->name);
	if(song->date) free(song->date);
	if(song->genre) free(song->genre);
	if(song->composer) free(song->composer);
	if(song->performer) free(song->performer);
	if(song->disc) free(song->disc);
	if(song->comment) free(song->comment);
    if(song->albumartist) free(song->albumartist);
}

mpd_Song * mpd_newSong(void) {
	mpd_Song * ret = g_slice_new0(mpd_Song);
	ret->time = MPD_SONG_NO_TIME;
	ret->pos = MPD_SONG_NO_NUM;
	ret->id = MPD_SONG_NO_ID;
    ret->priority = MPD_SONG_NO_PRIORITY;;
    ret->rating = MPD_SONG_NO_RATING;
	return ret;
}

void mpd_freeSong(mpd_Song * song) {
	mpd_finishSong(song);
	g_slice_free(mpd_Song, song);
}

mpd_Song * mpd_songDup(const mpd_Song * song) {
	mpd_Song * ret = mpd_newSong();

	if(song->file) ret->file = strdup(song->file);
	if(song->artist) ret->artist = strdup(song->artist);
	if(song->album) ret->album = strdup(song->album);
	if(song->title) ret->title = strdup(song->title);
	if(song->track) ret->track = strdup(song->track);
	if(song->name) ret->name = strdup(song->name);
	if(song->date) ret->date = strdup(song->date);
	if(song->genre) ret->genre= strdup(song->genre);
	if(song->composer) ret->composer= strdup(song->composer);
	if(song->performer) ret->performer = strdup(song->performer);
	if(song->disc) ret->disc = strdup(song->disc);
	if(song->comment) ret->comment = strdup(song->comment);
    if(song->albumartist) ret->albumartist = strdup(song->albumartist);
	ret->time = song->time;
	ret->pos = song->pos;
	ret->id = song->id;
    ret->priority = song->priority;
    ret->rating = song->rating;

	return ret;
}


static void mpd_finishDirectory(mpd_Directory * directory) {
	if(directory->path) free(directory->path);
}

mpd_Directory * mpd_newDirectory(void) {
	return g_slice_new0(mpd_Directory);
}

void mpd_freeDirectory(mpd_Directory * directory) {
	mpd_finishDirectory(directory);
	g_slice_free(mpd_Directory, directory);
}

mpd_Directory * mpd_directoryDup(mpd_Directory * directory) {
	mpd_Directory * ret = mpd_newDirectory();

	if(directory->path) ret->path = strdup(directory->path);

	return ret;
}

static void mpd_finishPlaylistFile(mpd_PlaylistFile * playlist) {
	if(playlist->path) free(playlist->path);
    if(playlist->mtime) free(playlist->mtime);
}

mpd_PlaylistFile * mpd_newPlaylistFile(void) {
	return g_slice_new0(mpd_PlaylistFile);
}

void mpd_freePlaylistFile(mpd_PlaylistFile * playlist) {
	mpd_finishPlaylistFile(playlist);
	g_slice_free(mpd_PlaylistFile, playlist);
}

mpd_PlaylistFile * mpd_playlistFileDup(mpd_PlaylistFile * playlist) {
	mpd_PlaylistFile * ret = mpd_newPlaylistFile();

	if(playlist->path) ret->path = strdup(playlist->path);
    if(playlist->mtime) ret->mtime = strdup(playlist->mtime);

	return ret;
}

static void mpd_finishInfoEntity(mpd_InfoEntity * entity) {
	if(entity->info.directory) {
		if(entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
			mpd_freeDirectory(entity->info.directory);
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeSong(entity->info.song);
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
			mpd_freePlaylistFile(entity->info.playlistFile);
		}
	}
}

mpd_InfoEntity * mpd_newInfoEntity(void) {
	return g_slice_new0(mpd_InfoEntity);
}

void mpd_freeInfoEntity(mpd_InfoEntity * entity) {
	mpd_finishInfoEntity(entity);
	g_slice_free(mpd_InfoEntity, entity);
}

static void mpd_sendInfoCommand(mpd_Connection * connection,const char * command) {
	mpd_executeCommand(connection,command);
}

mpd_InfoEntity * mpd_getNextInfoEntity(mpd_Connection * connection) {
	mpd_InfoEntity * entity = NULL;

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	if(connection->returnElement) {
		if(strcmp(connection->returnElement->name,"file")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_SONG;
			entity->info.song = mpd_newSong();
			entity->info.song->file =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name,
					"directory")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_DIRECTORY;
			entity->info.directory = mpd_newDirectory();
			entity->info.directory->path =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name,"playlist")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_PLAYLISTFILE;
			entity->info.playlistFile = mpd_newPlaylistFile();
			entity->info.playlistFile->path =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name, "cpos") == 0){
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_SONG;
			entity->info.song = mpd_newSong();
			entity->info.song->pos = atoi(connection->returnElement->value);
		}
		else {
			connection->error = 1;
			strcpy(connection->errorStr,"problem parsing song info");
			return NULL;
		}
	}
	else return NULL;

	mpd_getNextReturnElement(connection);
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;

		if(strcmp(re->name,"file")==0) return entity;
		else if(strcmp(re->name,"directory")==0) return entity;
		else if(strcmp(re->name,"playlist")==0) return entity;
		else if(strcmp(re->name,"cpos")==0) return entity;

		if(entity->type == MPD_INFO_ENTITY_TYPE_SONG &&
				strlen(re->value)) {
			if(strcmp(re->name,"Artist")==0) {
				if(entity->info.song->artist) {
					int length = strlen(entity->info.song->artist);
					entity->info.song->artist = realloc(entity->info.song->artist, 
					                                    length + strlen(re->value) + 3);
					strcpy(&((entity->info.song->artist)[length]), ", ");
					strcpy(&((entity->info.song->artist)[length + 2]), re->value);
				}
				else {
					entity->info.song->artist = strdup(re->value);
				}
			}
			else if(!entity->info.song->album &&
					strcmp(re->name,"Album")==0) {
				entity->info.song->album = strdup(re->value);
			}
			else if(!entity->info.song->title &&
					strcmp(re->name,"Title")==0) {
				entity->info.song->title = strdup(re->value);
			}
			else if(!entity->info.song->track &&
					strcmp(re->name,"Track")==0) {
				entity->info.song->track = strdup(re->value);
			}
			else if(!entity->info.song->name &&
					strcmp(re->name,"Name")==0) {
				entity->info.song->name = strdup(re->value);
			}
			else if(entity->info.song->time==MPD_SONG_NO_TIME &&
					strcmp(re->name,"Time")==0) {
				entity->info.song->time = atoi(re->value);
			}
			else if(entity->info.song->pos==MPD_SONG_NO_NUM &&
					strcmp(re->name,"Pos")==0) {
				entity->info.song->pos = atoi(re->value);
			}
			else if(entity->info.song->id==MPD_SONG_NO_ID &&
					strcmp(re->name,"Id")==0) {
				entity->info.song->id = atoi(re->value);
			}
            else if (strcmp(re->name, "Prio") == 0) {
                    entity->info.song->priority = atoi(re->value);
            }
			else if(!entity->info.song->date &&
					strcmp(re->name, "Date") == 0) {
				entity->info.song->date = strdup(re->value);
			}
			else if(!entity->info.song->genre &&
					strcmp(re->name, "Genre") == 0) {
				if(entity->info.song->genre) {
					int length = strlen(entity->info.song->genre);
					entity->info.song->genre = realloc(entity->info.song->genre, 
					                                   length + strlen(re->value) + 4);
					strcpy(&((entity->info.song->genre)[length]), ", ");
					strcpy(&((entity->info.song->genre)[length + 3]), re->value);
				}
				else {
					entity->info.song->genre = strdup(re->value);
				}
			}
			else if(strcmp(re->name, "Composer") == 0) {
				if(entity->info.song->composer) {
					int length = strlen(entity->info.song->composer);
					entity->info.song->composer = realloc(entity->info.song->composer, 
					                                      length + strlen(re->value) + 3);
					strcpy(&((entity->info.song->composer)[length]), ", ");
					strcpy(&((entity->info.song->composer)[length + 2]), re->value);
				}
				else {
					entity->info.song->composer = strdup(re->value);
				}
			}
			else if(strcmp(re->name, "Performer") == 0) {
				if(entity->info.song->performer) {
					int length = strlen(entity->info.song->performer);
					entity->info.song->performer = realloc(entity->info.song->performer, 
					                                       length + strlen(re->value) + 3);
					strcpy(&((entity->info.song->performer)[length]), ", ");
					strcpy(&((entity->info.song->performer)[length + 2]), re->value);
				}
				else {
					entity->info.song->performer = strdup(re->value);
				}
			}
			else if(!entity->info.song->disc &&
					strcmp(re->name, "Disc") == 0) {
				entity->info.song->disc = strdup(re->value);
			}
			else if(!entity->info.song->comment &&
					strcmp(re->name, "Comment") == 0) {
				entity->info.song->comment = strdup(re->value);
			}

			else if(!entity->info.song->albumartist &&
					strcmp(re->name, "AlbumArtist") == 0) {
				entity->info.song->albumartist = strdup(re->value);
			}
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
            if(!entity->info.playlistFile->mtime &&
                    strcmp(re->name, "Last-Modified") == 0) {
                    entity->info.playlistFile->mtime = strdup(re->value);
            }
		}

		mpd_getNextReturnElement(connection);
	}

	return entity;
}

static char * mpd_getNextReturnElementNamed(mpd_Connection * connection,
		const char * name)
{
	if(connection->doneProcessing || (connection->listOks &&
				connection->doneListOk))
	{
		return NULL;
	}

	mpd_getNextReturnElement(connection);
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;

		if(strcmp(re->name,name)==0) return strdup(re->value);
		mpd_getNextReturnElement(connection);
	}

	return NULL;
}

char *mpd_getNextTag(mpd_Connection *connection, int type)
{
	if (type < 0 || type >= MPD_TAG_NUM_OF_ITEM_TYPES ||
	    type == MPD_TAG_ITEM_ANY)
		return NULL;
	if (type == MPD_TAG_ITEM_FILENAME)
		return mpd_getNextReturnElementNamed(connection, "file");
	return mpd_getNextReturnElementNamed(connection, mpdTagItemKeys[type]);
}

char * mpd_getNextArtist(mpd_Connection * connection) {
	return mpd_getNextReturnElementNamed(connection,"Artist");
}

char * mpd_getNextAlbum(mpd_Connection * connection) {
	return mpd_getNextReturnElementNamed(connection,"Album");
}

void mpd_sendPlaylistInfoCommand(mpd_Connection * connection, int songPos) {
	int len = strlen("playlistinfo")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistinfo \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlaylistIdCommand(mpd_Connection * connection, int id) {
	int len = strlen("playlistid")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistid \"%i\"\n", id);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendPlChangesCommand(mpd_Connection * connection, long long playlist) {
	int len = strlen("plchanges")+2+LONGLONGLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "plchanges \"%lld\"\n", playlist);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlChangesPosIdCommand(mpd_Connection * connection, long long playlist) {
	int len = strlen("plchangesposid")+2+LONGLONGLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "plchangesposid \"%lld\"\n", playlist);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendListallCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	int len = strlen("listall")+2+strlen(sDir)+3;
	char *string = malloc(len);
	snprintf(string, len, "listall \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendListallInfoCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	int len = strlen("listallinfo")+2+strlen(sDir)+3;
	char *string = malloc(len);
	snprintf(string, len, "listallinfo \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendLsInfoCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	int len = strlen("lsinfo")+2+strlen(sDir)+3;
	char *string = malloc(len);
	snprintf(string, len, "lsinfo \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendCurrentSongCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"currentsong\n");
}

void mpd_sendSearchCommand(mpd_Connection * connection, int table,
		const char * str)
{
	mpd_startSearch(connection, 0);
	mpd_addConstraintSearch(connection, table, str);
	mpd_commitSearch(connection);
}

void mpd_sendFindCommand(mpd_Connection * connection, int table,
		const char * str)
{
	mpd_startSearch(connection, 1);
	mpd_addConstraintSearch(connection, table, str);
	mpd_commitSearch(connection);
}

void mpd_sendListCommand(mpd_Connection * connection, int table,
		const char * arg1)
{
	char st[10];
	int len;
	char *string;
	if(table == MPD_TABLE_ARTIST) strcpy(st,"artist");
	else if(table == MPD_TABLE_ALBUM) strcpy(st,"album");
	else {
		connection->error = 1;
		strcpy(connection->errorStr,"unknown table for list");
		return;
	}
	if(arg1) {
		char * sanitArg1 = mpd_sanitizeArg(arg1);
		len = strlen("list")+1+strlen(sanitArg1)+2+strlen(st)+3;
		string = malloc(len);
		snprintf(string, len, "list %s \"%s\"\n", st, sanitArg1);
		free(sanitArg1);
	}
	else {
		len = strlen("list")+1+strlen(st)+2;
		string = malloc(len);
		snprintf(string, len, "list %s\n", st);
	}
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendAddCommand(mpd_Connection * connection, const char * file) {
	char * sFile = mpd_sanitizeArg(file);
	int len = strlen("add")+2+strlen(sFile)+3;
	char *string = malloc(len);
	snprintf(string, len, "add \"%s\"\n", sFile);
	mpd_executeCommand(connection,string);
	free(string);
	free(sFile);
}

int mpd_sendAddIdCommand(mpd_Connection *connection, const char *file)
{
	int retval = -1;
	char *sFile = mpd_sanitizeArg(file);
	int len = strlen("addid")+2+strlen(sFile)+3;
	char *string = malloc(len);

	snprintf(string, len, "addid \"%s\"\n", sFile);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sFile);

	string = mpd_getNextReturnElementNamed(connection, "Id");
	if (string) {
		retval = atoi(string);
		free(string);
	}
	
	return retval;
}

void mpd_sendDeleteCommand(mpd_Connection * connection, int songPos) {
	int len = strlen("delete")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "delete \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendDeleteIdCommand(mpd_Connection * connection, int id) {
	int len = strlen("deleteid")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "deleteid \"%i\"\n", id);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSaveCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	int len = strlen("save")+2+strlen(sName)+3;
	char *string = malloc(len);
	snprintf(string, len, "save \"%s\"\n", sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendLoadCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	int len = strlen("load")+2+strlen(sName)+3;
	char *string = malloc(len);
	snprintf(string, len, "load \"%s\"\n", sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendRmCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	int len = strlen("rm")+2+strlen(sName)+3;
	char *string = malloc(len);
	snprintf(string, len, "rm \"%s\"\n", sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendRenameCommand(mpd_Connection *connection, const char *from,
                           const char *to)
{
	char *sFrom = mpd_sanitizeArg(from);
	char *sTo = mpd_sanitizeArg(to);
	int len = strlen("rename")+2+strlen(sFrom)+3+strlen(sTo)+3;
	char *string = malloc(len);
	snprintf(string, len, "rename \"%s\" \"%s\"\n", sFrom, sTo);
	mpd_executeCommand(connection, string);
	free(string);
	free(sFrom);
	free(sTo);
}

void mpd_sendShuffleCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"shuffle\n");
}

void mpd_sendClearCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"clear\n");
}

void mpd_sendPlayCommand(mpd_Connection * connection, int songPos) {
	int len = strlen("play")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "play \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlayIdCommand(mpd_Connection * connection, int id) {
	int len = strlen("playid")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "playid \"%i\"\n", id);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendStopCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"stop\n");
}

void mpd_sendPauseCommand(mpd_Connection * connection, int pauseMode) {
	int len = strlen("pause")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "pause \"%i\"\n", pauseMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendNextCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"next\n");
}

void mpd_sendMoveCommand(mpd_Connection * connection, int from, int to) {
	int len = strlen("move")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "move \"%i\" \"%i\"\n", from, to);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendMoveIdCommand(mpd_Connection * connection, int id, int to) {
	int len = strlen("moveid")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "moveid \"%i\" \"%i\"\n", id, to);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSwapCommand(mpd_Connection * connection, int song1, int song2) {
	int len = strlen("swap")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "swap \"%i\" \"%i\"\n", song1, song2);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSwapIdCommand(mpd_Connection * connection, int id1, int id2) {
	int len = strlen("swapid")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "swapid \"%i\" \"%i\"\n", id1, id2);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSeekCommand(mpd_Connection * connection, int song, int seek_time) {
	int len = strlen("seek")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "seek \"%i\" \"%i\"\n", song, seek_time);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSeekIdCommand(mpd_Connection * connection, int id, int seek_time) {
	int len = strlen("seekid")+2+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "seekid \"%i\" \"%i\"\n", id, seek_time);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendUpdateCommand(mpd_Connection * connection,const char * path) {
	char * sPath = mpd_sanitizeArg(path);
	int len = strlen("update")+2+strlen(sPath)+3;
	char *string = malloc(len);
	snprintf(string, len, "update \"%s\"\n", sPath);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sPath);
}

int mpd_getUpdateId(mpd_Connection * connection) {
	char * jobid;
	int ret = 0;

	jobid = mpd_getNextReturnElementNamed(connection,"updating_db");
	if(jobid) {
		ret = atoi(jobid);
		free(jobid);
	}

	return ret;
}

void mpd_sendPrevCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"previous\n");
}

void mpd_sendSingleCommand(mpd_Connection * connection, int singleMode) {
	int len = strlen("repeat")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "single \"%i\"\n", singleMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendConsumeCommand(mpd_Connection * connection, int consumeMode) {
	int len = strlen("repeat")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "consume \"%i\"\n", consumeMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendRepeatCommand(mpd_Connection * connection, int repeatMode) {
	int len = strlen("repeat")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "repeat \"%i\"\n", repeatMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendRandomCommand(mpd_Connection * connection, int randomMode) {
	int len = strlen("random")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "random \"%i\"\n", randomMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendSetvolCommand(mpd_Connection * connection, int volumeChange) {
	int len = strlen("setvol")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "setvol \"%i\"\n", volumeChange);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendCrossfadeCommand(mpd_Connection * connection, int seconds) {
	int len = strlen("crossfade")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "crossfade \"%i\"\n", seconds);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendPasswordCommand(mpd_Connection * connection, const char * pass) {
	char * sPass = mpd_sanitizeArg(pass);
	int len = strlen("password")+2+strlen(sPass)+3;
	char *string = malloc(len);
	snprintf(string, len, "password \"%s\"\n", sPass);
	mpd_executeCommand(connection,string);
	free(string);
	free(sPass);
}

void mpd_sendCommandListBegin(mpd_Connection * connection) {
	if(connection->commandList) {
		strcpy(connection->errorStr,"already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST;
	mpd_executeCommand(connection,"command_list_begin\n");
}

void mpd_sendCommandListOkBegin(mpd_Connection * connection) {
	if(connection->commandList) {
		strcpy(connection->errorStr,"already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST_OK;
	mpd_executeCommand(connection,"command_list_ok_begin\n");
	connection->listOks = 0;
}

void mpd_sendCommandListEnd(mpd_Connection * connection) {
	if(!connection->commandList) {
		strcpy(connection->errorStr,"not in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = 0;
	mpd_executeCommand(connection,"command_list_end\n");
}

void mpd_sendOutputsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"outputs\n");
}

mpd_OutputEntity * mpd_getNextOutput(mpd_Connection * connection) {
	if(connection->doneProcessing || (connection->listOks &&
				connection->doneListOk))
	{
		return NULL;
	}

	if(connection->error) return NULL;

	mpd_OutputEntity* output = g_slice_new0(mpd_OutputEntity);
	output->id = -10;

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"outputid")==0) {
			if(output!=NULL && output->id>=0) return output;
			output->id = atoi(re->value);
		}
		else if(strcmp(re->name,"outputname")==0) {
			output->name = strdup(re->value);
		}
		else if(strcmp(re->name,"outputenabled")==0) {
			output->enabled = atoi(re->value);
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			mpd_freeOutputElement(output);
			return NULL;
		}
	}

	return output;
}

void mpd_sendEnableOutputCommand(mpd_Connection * connection, int outputId) {
	int len = strlen("enableoutput")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "enableoutput \"%i\"\n", outputId);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendDisableOutputCommand(mpd_Connection * connection, int outputId) {
	int len = strlen("disableoutput")+2+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "disableoutput \"%i\"\n", outputId);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_freeOutputElement(mpd_OutputEntity * output) {
	if(output->name)
		free(output->name);
	g_slice_free(mpd_OutputEntity, output);
}

/**
 * mpd_sendNotCommandsCommand
 * odd naming, but it gets the not allowed commands
 */

void mpd_sendNotCommandsCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "notcommands\n");
}

/**
 * mpd_sendCommandsCommand
 * odd naming, but it gets the allowed commands
 */
void mpd_sendCommandsCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "commands\n");
}

/**
 * Get the next returned command
 */
char * mpd_getNextCommand(mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "command");
}

void mpd_sendUrlHandlersCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "urlhandlers\n");
}

char * mpd_getNextHandler(mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "handler");
}

void mpd_sendTagTypesCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "tagtypes\n");
}

char * mpd_getNextTagType(mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "tagtype");
}

void mpd_startSearch(mpd_Connection *connection, int exact)
{
	if (connection->request) {
		strcpy(connection->errorStr, "search already in progress");
		connection->error = 1;
		return;
	}

	if (exact) connection->request = strdup("find");
	else connection->request = strdup("search");
}

void mpd_startStatsSearch(mpd_Connection *connection)
{
	if (connection->request) {
		strcpy(connection->errorStr, "search already in progress");
		connection->error = 1;
		return;
	}

	connection->request = strdup("count");
}

void mpd_startPlaylistSearch(mpd_Connection *connection, int exact)
{
	if (connection->request) {
		strcpy(connection->errorStr, "search already in progress");
		connection->error = 1;
		return;
	}

	if (exact) connection->request = strdup("playlistfind");
	else connection->request = strdup("playlistsearch");
}

void mpd_startFieldSearch(mpd_Connection *connection, int type)
{
	char *strtype;
	int len;

	if (connection->request) {
		strcpy(connection->errorStr, "search already in progress");
		connection->error = 1;
		return;
	}

	if (type < 0 || type >= MPD_TAG_NUM_OF_ITEM_TYPES) {
		strcpy(connection->errorStr, "invalid type specified");
		connection->error = 1;
		return;
	}

	strtype = mpdTagItemKeys[type];

	len = 5+strlen(strtype)+1;
	connection->request = malloc(len);

	snprintf(connection->request, len, "list %c%s",
	         tolower(strtype[0]), strtype+1);
}

void mpd_addConstraintSearch(mpd_Connection *connection, int type, const char *name)
{
	char *strtype;
	char *arg;
	int len;
	char *string;

	if (!connection->request) {
		strcpy(connection->errorStr, "no search in progress");
		connection->error = 1;
		return;
	}

	if (type < 0 || type >= MPD_TAG_NUM_OF_ITEM_TYPES) {
		strcpy(connection->errorStr, "invalid type specified");
		connection->error = 1;
		return;
	}

	if (name == NULL) {
		strcpy(connection->errorStr, "no name specified");
		connection->error = 1;
		return;
	}

	string = strdup(connection->request);
	strtype = mpdTagItemKeys[type];
	arg = mpd_sanitizeArg(name);

	len = strlen(string)+1+strlen(strtype)+2+strlen(arg)+2;
	connection->request = realloc(connection->request, len);
	snprintf(connection->request, len, "%s %c%s \"%s\"",
	         string, tolower(strtype[0]), strtype+1, arg);

	free(string);
	free(arg);
}

void mpd_commitSearch(mpd_Connection *connection)
{
	int len;

	if (!connection->request) {
		strcpy(connection->errorStr, "no search in progress");
		connection->error = 1;
		return;
	}

	len = strlen(connection->request)+2;
	connection->request = realloc(connection->request, len);
	connection->request[len-2] = '\n';
	connection->request[len-1] = '\0';
	mpd_sendInfoCommand(connection, connection->request);

	free(connection->request);
	connection->request = NULL;
}

/**
 * @param connection a MpdConnection
 * @param path	the path to the playlist.
 *
 * List the content, with full metadata, of a stored playlist.
 *
 */
void mpd_sendListPlaylistInfoCommand(mpd_Connection *connection,const char *path)
{
	char *arg = mpd_sanitizeArg(path);
	int len = strlen("listplaylistinfo")+2+strlen(arg)+3;
	char *query = malloc(len);
	snprintf(query, len, "listplaylistinfo \"%s\"\n", arg);
	mpd_sendInfoCommand(connection, query);
	free(arg);
	free(query);
}

/**
 * @param connection a MpdConnection
 * @param path	the path to the playlist.
 *
 * List the content of a stored playlist.
 *
 */
void mpd_sendListPlaylistCommand(mpd_Connection *connection,const char *path)
{
	char *arg = mpd_sanitizeArg(path);
	int len = strlen("listplaylist")+2+strlen(arg)+3;
	char *query = malloc(len);
	snprintf(query, len, "listplaylist \"%s\"\n", arg);
	mpd_sendInfoCommand(connection, query);
	free(arg);
	free(query);
}

void mpd_sendPlaylistClearCommand(mpd_Connection *connection,const char *path)
{
	char *sPath = mpd_sanitizeArg(path);
	int len = strlen("playlistclear")+2+strlen(sPath)+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistclear \"%s\"\n", sPath);
	mpd_executeCommand(connection, string);
	free(sPath);
	free(string);
}

void mpd_sendPlaylistAddCommand(mpd_Connection *connection,
                                const char *playlist,const char *path)
{
	char *sPlaylist = mpd_sanitizeArg(playlist);
	char *sPath = mpd_sanitizeArg(path);
	int len = strlen("playlistadd")+2+strlen(sPlaylist)+3+strlen(sPath)+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistadd \"%s\" \"%s\"\n", sPlaylist, sPath);
	mpd_executeCommand(connection, string);
	free(sPlaylist);
	free(sPath);
	free(string);
}

void mpd_sendPlaylistMoveCommand(mpd_Connection *connection,
                                 const char *playlist, int from, int to)
{
	char *sPlaylist = mpd_sanitizeArg(playlist);
	int len = strlen("playlistmove")+
	          2+strlen(sPlaylist)+3+INTLEN+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistmove \"%s\" \"%i\" \"%i\"\n",
	         sPlaylist, from, to);
	mpd_executeCommand(connection, string);
	free(sPlaylist);
	free(string);
}

void mpd_sendPlaylistDeleteCommand(mpd_Connection *connection,
                                   const char *playlist, int pos)
{
	char *sPlaylist = mpd_sanitizeArg(playlist);
	int len = strlen("playlistdelete")+2+strlen(sPlaylist)+3+INTLEN+3;
	char *string = malloc(len);
	snprintf(string, len, "playlistdelete \"%s\" \"%i\"\n", sPlaylist, pos);
	mpd_executeCommand(connection, string);
	free(sPlaylist);
	free(string);
}
void mpd_sendClearErrorCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"clearerror\n");
}


void mpd_sendGetEventsCommand(mpd_Connection *connection) {
    mpd_executeCommand(connection, "idle\nnoidle\n");
//    mpd_executeCommand(connection, "noidle\n");
}

char * mpd_getNextEvent(mpd_Connection *connection)
{
    return mpd_getNextReturnElementNamed(connection, "changed");
}

void mpd_sendListPlaylistsCommand(mpd_Connection * connection) {
    mpd_sendInfoCommand(connection, "listplaylists\n");
}

char * mpd_getNextSticker (mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "sticker");
}
void  mpd_sendGetSongSticker(mpd_Connection *connection, const char *song_path, const char *sticker)
{
    char *sSong = mpd_sanitizeArg(song_path); 
    char *sSticker = mpd_sanitizeArg(sticker); 
    int len = strlen("sticker get song ")+strlen(sSong)+3+strlen(sSticker)+4;
    char *string = malloc(len);
    snprintf(string, len, "sticker get song \"%s\" \"%s\"\n", sSong, sSticker);
    mpd_executeCommand(connection, string);
    free(string);
    free(sSong);
    free(sSticker);
}

void mpd_sendSetSongSticker(mpd_Connection *connection, const char *song_path, const char *sticker, const char *value)
{

    char *sSong = mpd_sanitizeArg(song_path); 
    char *sSticker = mpd_sanitizeArg(sticker); 
    char *sValue = mpd_sanitizeArg(value);
    int len = strlen("sticker set song ")+strlen(sSong)+3+strlen(sSticker)+3+strlen(sValue)+4;
    char *string = malloc(len);
    snprintf(string, len, "sticker set song \"%s\" \"%s\" \"%s\"\n", sSong, sSticker,sValue);
    mpd_sendInfoCommand(connection, string);
    free(string);
    free(sSong);
    free(sSticker);
    free(sValue);
}

void mpd_sendSetReplayGainMode(mpd_Connection *connection, const char *mode)
{
    char *smode= mpd_sanitizeArg(mode); 
    int len = strlen("replay_gain_mode ")+strlen(smode)+4;
    char *string = malloc(len);
    snprintf(string, len, "replay_gain_mode \"%s\"\n", smode);
    mpd_sendInfoCommand(connection, string);
    free(smode);
    free(string);
}
void mpd_sendReplayGainModeCommand(mpd_Connection *connection)
{
	mpd_executeCommand(connection, "replay_gain_status\n");
}
char *mpd_getReplayGainMode(mpd_Connection *connection)
{
    return mpd_getNextReturnElementNamed(connection, "replay_gain_mode");
}

void mpd_sendSetPrioId(mpd_Connection *connection, int id, int priority)
{
	int len = strlen("prioid ")+1+INTLEN+3+INTLEN+3;
    char *str = malloc(len);
    snprintf(str, len, "prioid \"%d\" \"%d\"\n", id, priority);
    mpd_sendInfoCommand(connection, str);
    free(str);
}
void mpd_sendSetPrio(mpd_Connection *connection, int pos, int priority)
{
	int len = strlen("prioid ")+1+INTLEN+3+INTLEN+3;
    char *str = malloc(len);
    snprintf(str, len, "prio \"%d\" \"%d\"\n", pos, priority);
    mpd_sendInfoCommand(connection, str);
    free(str);
}
