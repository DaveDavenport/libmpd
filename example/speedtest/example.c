#include "../../src/libmpdclient.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	char *string;
	mpd_Connection * conn;
	conn = mpd_newConnection("192.150.0.111",6600,10);


	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return -1;
	}
	int i=0;
	for(i=0;i<1000;i++)
	{
	mpd_sendListCommand(conn,MPD_TABLE_ARTIST,NULL);
	while (( string = mpd_getNextArtist(conn)) != NULL)
	{
		printf("%s\n", string);
	}
	mpd_finishCommand(conn);
	}
//	mpd_closeConnection(conn);

	return 0;
}
