/*
 * Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MPD_LIB__
#define __MPD_LIB__
#ifdef WIN32
	#define __REGEX_IMPORT__ 1
	#define __W32API_USE_DLLIMPORT__ 1
#endif

#include "libmpdclient.h"
#include <regex.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MPD_NOT_CONNECTED -2
#define MPD_FAILED_STATUS -3
#define MPD_LOCK_FAILED -4
#define MPD_FAILED_STATS -5
#define MPD_ERROR -6
#define MPD_PLAYLIST_EXIST -10



/** 
 * MpdObj
 *
 *  The Main Mpd Object. Don't access any of the internal values directly, but use the provided functions.
 */ 
typedef struct _MpdObj MpdObj;

/**
 * MpdDataType
 *
 * enumeration to determine what value the MpdData structure hold. 
 * The MpdData structure can hold only one type of value, 
 * but a list of MpdData structs can hold structs with different type of values.
 * It's required to check every MpdData Structure.
 */
typedef enum _MpdDataType {
	MPD_DATA_TYPE_NONE, 		/** The MpdData structure holds no value */
	MPD_DATA_TYPE_TAG,		/** Holds an Tag String. value->tag is filled.*/
	MPD_DATA_TYPE_ARTIST,		/** Holds an Artist string. value->artist is filled. value->album is possible valid */
	MPD_DATA_TYPE_ALBUM,		/** Holds an Album string. value->album is filled. value->artist is possible valid */
	MPD_DATA_TYPE_DIRECTORY,	/** Holds an Directory String. value->directory is filled.*/
	MPD_DATA_TYPE_SONG,		/** Holds an MpdSong Structure. value->song is valid.*/
	MPD_DATA_TYPE_PLAYLIST,		/** Holds an Playlist String. value->playlist is filled.*/
	MPD_DATA_TYPE_OUTPUT_DEV	/** Holds an MpdOutputDevice structure. value->output_dev is valid.*/
} MpdDataType;

/**
 * MpdData
 *
 * A linked list in witch data is passed from libmpd to the client. 
 */
typedef struct _MpdData {
	/* Use wrapper #mpd_data_get_next to get the next item in the list.
	 * Don't use this pointer.
	 */
	struct _MpdData *next;
	/* Previous MpdData in the list */
	struct _MpdData *prev;
	/* First MpdData in the list */
	struct _MpdData *first;

	/* MpdDataType */
	MpdDataType type;

	struct {
		char *tag;
		char *artist;
		char *album;
		char *directory;
		char *playlist; /*is a path*/
		mpd_Song *song;
		mpd_OutputEntity *output_dev; /* from devices */
	}value;
}MpdData;


#include "libmpd-player.h"
#include "libmpd-status.h"
#include "libmpd-playlist.h"




/**
 * mpd_new_default 
 * 
 * Create an new #MpdObj with default settings. 
 * hostname will be set to "localhost"
 * port will be 6600
 * same as calling #mpd_new("localhost",6600,NULL)
 * 
 * returns: the new #MpdObj
 */
MpdObj * 	mpd_new_default			();



/**
 * mpd_new 
 * @hostname: The hostname to connect to
 * @port:	The port to connect to
 * @Password:	The password to use for the connection, or NULL for no password
 *
 * Create a new #MpdObj with provided settings:
 *
 * returns: the new #MpdObj
 */

MpdObj * 	mpd_new				(char *hostname, int port, char *password);
/**
 * mpd_set_hostname
 *@mi: a #MpdObj
 *@hostname: The new hostname to use
 *
 * set the hostname
 */
void 		mpd_set_hostname			(MpdObj *mi, char *hostname);
/**
 * mpd_set_password
 * @mi: a #MpdObj
 * @password: The new password to use
 * 
 * Set the password
 */
void 		mpd_set_password			(MpdObj *mi, char *password);
/**
 * mpd_set_port
 * @mi: a #MpdObj
 * @port: The port to use. (Default: 6600)
 *
 * Set the Port number
 */
void 		mpd_set_port				(MpdObj *mi, int port);
/**
 * mpd_set_connection_timeout
 * @mi: a #MpdObj
 * @timeout: A timeout (in seconds)
 *
 * Set the timeout of the connection. 
 * If allready connected the timeout of the running connection
 */
void 		mpd_set_connection_timeout		(MpdObj *mi, float timeout);
/**
 * mpd_connect
 * @mi: a #MpdObj
 *
 * Connect to the mpd daemon.
 * returns: returns 0 when successful 
 */
int 		mpd_connect				(MpdObj *mi);
/**
 * mpd_disconnect
 * @mi: The #MpdObj to disconnect
 *
 * Disconnect the current connection
 * returns: False (always)
 */
int 		mpd_disconnect			(MpdObj *mi);
/**
 * mpd_check_connected
 * @mi:	a #MpdObj
 *
 * Checks if #MpdObj is connected
 * returns: True when connected
 */ 
int 		mpd_check_connected			(MpdObj *mi);
/**
 * mpd_check_error
 * @mi: a #MpdObj
 *
 * Checks if there was an error
 * returns: True when there is an error
 */
int 		mpd_check_error			(MpdObj *mi);
/**
 * mpd_free
 * @mi: a #MpdObj
 *
 * Free the #MpdObj, when still connected the connection will be disconnected first
 */
void 		mpd_free				(MpdObj *mi);


/* 
 * signals 
 */

/**
 * ChangedStatusType
 *
 * Bitwise enumeration to determine what triggered the status_changed signals
 */
typedef enum _ChangedStatusType {
	MPD_CST_PLAYLIST      = 0x01,
	MPD_CST_SONG          = 0x02,
	MPD_CST_SONGID        = 0x04,
	MPD_CST_DATABASE      = 0x08,
	MPD_CST_UPDATING      = 0x10,
	MPD_CST_VOLUME        = 0x11,
	MPD_CST_TIME          = 0x12,
	MPD_CST_ELAPSED_TIME  = 0x14,
	MPD_CST_CROSSFADE     = 0x18,
	MPD_CST_RANDOM        = 0x20,
	MPD_CST_REPEAT        = 0x21,
 	MPD_CST_AUDIO         = 0x22,
	MPD_CST_STATE         = 0x24
} ChangedStatusType;


/* callback typedef's */
/**
 * StatusChangedCallback
 * @mi: a #MpdObj
 * @what: a #ChangedStatusType that determines what changed triggered the signal. This is a bitmask.
 * @userdata: user data set when the signal handler was connected.
 */
typedef void (* StatusChangedCallback)(MpdObj *mi, ChangedStatusType what, void *userdata);
typedef void (* ErrorCallback)(MpdObj *mi, int id, char *msg, void *userdata);
typedef void (* ConnectionChangedCallback)(MpdObj *mi, int connect, void *userdata);

/* new style signal connectors */
void 		mpd_signal_connect_status_changed        (MpdObj *mi, StatusChangedCallback status_changed, void *userdata);
void 		mpd_signal_connect_error                 (MpdObj *mi, ErrorCallback error, void *userdata);
void 		mpd_signal_connect_connection_changed	   (MpdObj *mi, ConnectionChangedCallback disconnect, int connected, void *userdata);

/* old style signal connectors */
void 		mpd_signal_set_playlist_changed	(MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id,void *pointer), void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_error			(MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg, void *pointer),void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_song_changed		(MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id,void *pointer), void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_status_changed	(MpdObj *mi, void *(* status_changed)(MpdObj *mi,void *pointer), void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_state_changed 	(MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_disconnect		(MpdObj *mi, void *(* disconnect)(MpdObj *mi, void *pointer),void *disconnect_pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_connect		(MpdObj *mi, void *(* connect)(MpdObj *mi, void *pointer),void *connect_pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_database_changed	(MpdObj *mi, void *(* database_changed)(MpdObj *mi, void *pointer), void *pointer) __attribute__ ((deprecated)); 
void 		mpd_signal_set_updating_changed	(MpdObj *mi, void *(* updating_changed)(MpdObj *mi,int updating, void *pointer), void *pointer) __attribute__ ((deprecated)); 


/* MpdData struct functions */
/**
 * mpd_data_is_last
 * @data: a #MpdData 
 *
 * Check's if the passed #MpdData is the last in a list
 * returns: TRUE when data is the last in the list.
 */
int 		mpd_data_is_last			(MpdData *data);
/**
 * mpd_data_free
 * @data: a #MpdData
 *
 * Free's a #MpdData List
 */
void 		mpd_data_free				(MpdData *data);
/**
 * mpd_data_get_next
 * @data: a #MpdData
 *
 * Returns the next #MpdData in the list. If it's the last item in the list, it will free the list.
 * returns: The next #MpdData or NULL
 */
MpdData * 	mpd_data_get_next			(MpdData *data);

/* Server Stuff */
MpdData * 	mpd_server_get_output_devices	(MpdObj *mi);
int 		mpd_server_set_output_device		(MpdObj *mi,int device_id,int state);
long unsigned	mpd_server_get_database_update_time	(MpdObj *mi);
int 		mpd_server_check_version		(MpdObj *mi, int major, int minor, int micro);

/* misc */
regex_t** 	mpd_misc_tokenize			(char *string);
void 		mpd_misc_tokens_free			(regex_t ** tokens);
int 		mpd_misc_get_tag_by_name		(char *name);
#endif

#ifdef __cplusplus
}
#endif
