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

#define MPD_O_NOT_CONNECTED -2
#define MPD_O_FAILED_STATUS -3
#define MPD_O_LOCK_FAILED -4
#define MPD_O_FAILED_STATS -5
#define MPD_O_ERROR -6
#define MPD_O_PLAYLIST_EXIST -10

#define	MPD_PLAYER_PAUSE 	3
#define	MPD_PLAYER_PLAY 	2
#define	MPD_PLAYER_STOP 	1
#define	MPD_PLAYER_UNKNOWN 	0

/* queue struct */
typedef struct _MpdQueue MpdQueue;
/* main object struct. */
typedef struct _MpdObj MpdObj;

/** enumeration to determine what value the MpdData structure hold. 
 *  And MpdData structure can hold only one type of value, but of MpdData structs can hold different type of values.
 *  It's required to check every MpdData Structure.
 */
enum MpdDataType{
	MPD_DATA_TYPE_NONE, 		/** The MpdData structure holds no value */
	MPD_DATA_TYPE_TAG,		/** Holds an Tag String. value->tag is filled.*/
	MPD_DATA_TYPE_ARTIST,		/** Holds an Artist string. value->artist is filled. value->album is possible valid */
	MPD_DATA_TYPE_ALBUM,		/** Holds an Album string. value->album is filled. value->artist is possible valid */
	MPD_DATA_TYPE_DIRECTORY,	/** Holds an Directory String. value->directory is filled.*/
	MPD_DATA_TYPE_SONG,		/** Holds an MpdSong Structure. value->song is valid.*/
	MPD_DATA_TYPE_PLAYLIST,		/** Holds an Playlist String. value->playlist is filled.*/
	MPD_DATA_TYPE_OUTPUT_DEV	/** Holds an MpdOutputDevice structure. value->output_dev is valid.*/
} MpdDataType;

/* 
 * there will be wrapper functions in the future to grab the internals.
 */
typedef struct _MpdData
{
	struct _MpdData *next;
	struct _MpdData *prev;
	struct _MpdData *first;

	/* MpdDataType */
	int type;

	struct 
	{
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

MpdObj * 	mpd_new_default			();
MpdObj * 	mpd_new				(char *hostname, int port, char *password);
void 		mpd_set_hostname			(MpdObj *mi, char *hostname);
void 		mpd_set_password			(MpdObj *mi, char *hostname);
void 		mpd_set_port				(MpdObj *mi, int port);
void 		mpd_set_connection_timeout		(MpdObj *mi, float timeout);
int 		mpd_connect				(MpdObj *mi);
int 		mpd_disconnect			(MpdObj *mi);
int 		mpd_check_connected			(MpdObj *mi);
int 		mpd_check_error			(MpdObj *mi);
void 		mpd_free				(MpdObj *mi);


/* 
 * signals 
 */
void 		mpd_signal_set_playlist_changed	(MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id,void *pointer), void *pointer);
void 		mpd_signal_set_error			(MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg, void *pointer),void *pointer);
void 		mpd_signal_set_song_changed		(MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id,void *pointer), void *pointer);
void 		mpd_signal_set_status_changed	(MpdObj *mi, void *(* status_changed)(MpdObj *mi,void *pointer), void *pointer);
void 		mpd_signal_set_state_changed 	(MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer);
void 		mpd_signal_set_disconnect		(MpdObj *mi, void *(* disconnect)(MpdObj *mi, void *pointer),void *disconnect_pointer);
void 		mpd_signal_set_connect		(MpdObj *mi, void *(* connect)(MpdObj *mi, void *pointer),void *connect_pointer);
void 		mpd_signal_set_database_changed	(MpdObj *mi, void *(* database_changed)(MpdObj *mi, void *pointer), void *pointer);
void 		mpd_signal_set_updating_changed	(MpdObj *mi, void *(* updating_changed)(MpdObj *mi,int updating, void *pointer), void *pointer);


/* MpdData struct functions */
int 		mpd_data_is_last			(MpdData *data);
void 		mpd_data_free				(MpdData *data);
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
