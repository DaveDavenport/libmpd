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


/** 
 *MDP_NOT_CONNECTED
 * 
 * Action failed because there is no connection to an mpd daemon 
 */
#define MPD_NOT_CONNECTED -2 	
/**
 * MPD_FAILED_STATUS
 *
 * Failed to grab status 
 */
#define MPD_FAILED_STATUS -3
/**
 * MPD_LOCK_FAILED
 *
 * Connection is still locked
 */
#define MPD_LOCK_FAILED -4
/**
 * MPD_FAILED_STATS
 * 
 * Failed to grab status
 */
#define MPD_FAILED_STATS -5
/** 
 * MPD_ERROR
 *
 * Mpd returned an error 
 */
#define MPD_ERROR -6		
/** 
 * MPD_PLAYLIST_EXIST
 * 
 * The playlist allready extists 
 */
#define MPD_PLAYLIST_EXIST -10	


/** 
 * MpdObj
 *
 *  The Main Mpd Object. Don't access any of the internal values directly, but use the provided functions.
 */ 
typedef struct _MpdObj MpdObj;

/**
 * MpdDataType
 * @MPD_DATA_TYPE_NONE:	 The MpdData structure holds no value 
 * @MPD_DATA_TYPE_TAG: 	Holds an Tag String. value->tag is filled value->tag_type defines what type of tag.
 * @MPD_DATA_TYPE_DIRECTORY:   Holds an Directory String. value->directory is filled.
 * @MPD_DATA_TYPE_SONG:  Holds an MpdSong Structure. value->song is valid.
 * @MPD_DATA_TYPE_PLAYLIST: Holds an Playlist String. value->playlist is filled.
 * @MPD_DATA_TYPE_OUTPUT_DEV: Holds an MpdOutputDevice structure. value->output_dev is valid.
 *
 * enumeration to determine what value the MpdData structure hold. 
 * The MpdData structure can hold only one type of value, 
 * but a list of MpdData structs can hold structs with different type of values.
 * It's required to check every MpdData Structure.
 */
typedef enum _MpdDataType {
	MPD_DATA_TYPE_NONE,
	MPD_DATA_TYPE_TAG,
	MPD_DATA_TYPE_DIRECTORY,
	MPD_DATA_TYPE_SONG,
	MPD_DATA_TYPE_PLAYLIST,
	MPD_DATA_TYPE_OUTPUT_DEV
} MpdDataType;

/**
 * MpdData
 *
 * @type: A #MpdDataType defining the data it contains
 *
 * A linked list that is used to pass data from libmpd to the client. 
 */
typedef struct _MpdData {
	/* MpdDataType */
	MpdDataType type;

	union {
		struct {
			int tag_type;
			char *tag;
		};
		char *directory;
		char *playlist; /*is a path*/
		mpd_Song *song;
		mpd_OutputEntity *output_dev; /* from devices */
	};
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
	MPD_CST_PLAYLIST      = 0x0001,
	MPD_CST_SONGID        = 0x0004,
	MPD_CST_DATABASE      = 0x0008,
	MPD_CST_UPDATING      = 0x0010,
	MPD_CST_VOLUME        = 0x0020,
	MPD_CST_TOTAL_TIME    = 0x0040,
	MPD_CST_ELAPSED_TIME  = 0x0080,
	MPD_CST_CROSSFADE     = 0x0100,
	MPD_CST_RANDOM        = 0x0200,
	MPD_CST_REPEAT        = 0x0400,
 	MPD_CST_AUDIO         = 0x0800,/* not yet implemented */
	MPD_CST_STATE         = 0x1000
} ChangedStatusType;


/* callback typedef's */
/**
 * StatusChangedCallback
 * @mi: a #MpdObj
 * @what: a #ChangedStatusType that determines what changed triggered the signal. This is a bitmask.
 * @userdata: user data set when the signal handler was connected.
 * Signal that get's called when the state of mpd changed. Look #ChangedStatusType to see the possible events.
 */
typedef void (* StatusChangedCallback)(MpdObj *mi, ChangedStatusType what, void *userdata);
/**
 * ErrorCallback
 * @mi: a #MpdObj
 * @id: The error Code.
 * @msg: human-readable informative error message.
 * @userdata:  user data set when the signal handler was connected.
 * This signal is called when an error has occured in the communication with mpd.
 */
typedef void (* ErrorCallback)(MpdObj *mi, int id, char *msg, void *userdata);
/**
 * ConnectionChangedCallback
 * @mi: a #MpdObj
 * @connect: 1 if you are now connect, 0 if you are disconnect.
 * @userdata:  user data set when the signal handler was connected.
 * Signal is triggered when the connection state changes.
 */

typedef void (* ConnectionChangedCallback)(MpdObj *mi, int connect, void *userdata);

/* new style signal connectors */
/**
 * mpd_signal_connect_status_changed
 * @mi: a #MpdObj
 * @status_changed: a #StatusChangedCallback
 * @userdata: user data passed to the callback
 */
void 		mpd_signal_connect_status_changed        (MpdObj *mi, StatusChangedCallback status_changed, void *userdata);
/**
 * mpd_signal_connect_error
 * @mi: a #mpdObj
 * @error: a #ErrorCallback
 * @userdata: user data passed to the callback
 */
void 		mpd_signal_connect_error                 (MpdObj *mi, ErrorCallback error, void *userdata);
/**
 * mpd_signal_connect_connection_changed
 * @mi: a #mpdObj
 * @connection_changed: a #ConnectionChangedCallback
 * @userdata: user data passed to the callback
 */
void 		mpd_signal_connect_connection_changed	   (MpdObj *mi, ConnectionChangedCallback connection_changed, void *userdata);

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
 * 
 * returns: The next #MpdData or NULL
 */
MpdData * 	mpd_data_get_next			(MpdData *data);
/**
 * mpd_data_get_first
 * @data: a #MpdData
 *
 * Returns the first #MpdData in the list.
 * 
 * returns: The first #MpdData or NULL
 */
MpdData * 	mpd_data_get_first			(MpdData const *data);

/* Server Stuff */
/**
 * mpd_server_get_output_devices
 * @mi: a #MpdObj
 * 
 * Returns a list of audio output devices stored in a #MpdData list
 *
 * returns: a #MpdData
 */
MpdData * 	mpd_server_get_output_devices	(MpdObj *mi);
/**
 * mpd_server_set_output_device
 * @mi: a #MpdObj
 * @device_id: The id of the output device
 * @state: The state to change the output device to, 1 is enable, 0 is disable.
 * 
 * Enable or Disable an audio output device
 *
 * returns: 0 if successful
 */
int 		mpd_server_set_output_device		(MpdObj *mi,int device_id,int state);
/**
 * mpd_server_get_database_update_time
 * @mi: a #MpdObj
 * 
 * Get's a unix timestamp of the last time the database was updated.
 * 
 * returns: unix Timestamp
 */
long unsigned	mpd_server_get_database_update_time	(MpdObj *mi);
/**
 * mpd_server_check_version
 * @mi: a #MpdObj
 * @major: the major version number
 * @minor: the minor version number
 * @micro: the micro version number
 * 
 * Checks if the connected mpd server version is equal or higer.
 * 
 * returns: True or False
 */
int 		mpd_server_check_version		(MpdObj *mi, int major, int minor, int micro);

/* misc */
/**
 * mpd_misc_tokenize
 * @string: A NULL terminated string
 * 
 * Splits a string in tokens while keeping ()[] in tact. This can be used to match a string tokenized and with regex support agains a user defined string.
 * 
 * returns: An array of regex patterns
 */
regex_t** 	mpd_misc_tokenize			(char *string);
/**
 * mpd_misc_tokens_free
 * @tokens: an array of regex patterns.
 * 
 * Free's a list of regex patterns
 */
void 		mpd_misc_tokens_free(regex_t ** tokens);
/**
 * mpd_misc_get_tag_by_name
 * @name: a NULL terminated string
 * 
 * gets the Matching #MdpDataType matching at the string
 *
 * returns: a #MpdDataType
 */
int 		mpd_misc_get_tag_by_name		(char *name);
#endif

#ifdef __cplusplus
}
#endif
