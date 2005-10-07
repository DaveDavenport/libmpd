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

/* enumeration determining what value the MpdData structure hold. 
 * it's allowed to have different type's of item's in one list.
 */
enum {
	MPD_DATA_TYPE_NONE,
	MPD_DATA_TYPE_TAG,
	MPD_DATA_TYPE_ARTIST,
	MPD_DATA_TYPE_ALBUM,
	MPD_DATA_TYPE_DIRECTORY,
	MPD_DATA_TYPE_SONG,
	MPD_DATA_TYPE_PLAYLIST,
	MPD_DATA_TYPE_OUTPUT_DEV
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

/* 
 * status commands 
 */
/* To get the function to have the  most recent info you want to call mpd_status_queue_update 
 * In a gui app. you want to call this every 0.x seconds. 
 * mpd_status_queue_update only queue's an update
 * Only when a function is called that needs status, it's fetched from mpd.
 */
int 		mpd_status_check			(MpdObj *mi);
int 		mpd_status_queue_update		(MpdObj *mi);
float 		mpd_status_set_volume_as_float	(MpdObj *mi, float fvol);
int 		mpd_status_set_volume		(MpdObj *mi,int volume);
int 		mpd_status_get_volume		(MpdObj *mi);
int 		mpd_status_get_bitrate		(MpdObj *mi);
int		mpd_status_get_total_song_time	(MpdObj *mi);
int		mpd_status_get_elapsed_song_time	(MpdObj *mi);
int		mpd_status_get_crossfade		(MpdObj *mi);
int		mpd_status_set_crossfade		(MpdObj *mi, int crossfade_time);
int		mpd_stats_update			(MpdObj *mi);

int		mpd_stats_get_total_songs		(MpdObj *mi);
int		mpd_stats_get_total_artists		(MpdObj *mi);
int		mpd_stats_get_total_albums		(MpdObj *mi);
int		mpd_stats_get_uptime			(MpdObj *mi);
int		mpd_stats_get_playtime		(MpdObj *mi);

int 		mpd_status_db_is_updating		(MpdObj *mi);

/* Player commands */
int 		mpd_player_play			(MpdObj *mi);
int 		mpd_player_play_id			(MpdObj *mi, int id);
int 		mpd_player_stop			(MpdObj *mi);
int 		mpd_player_next			(MpdObj *mi);
int 		mpd_player_prev			(MpdObj *mi);
int 		mpd_player_pause			(MpdObj *mi);
int 		mpd_player_get_state			(MpdObj *mi);
int 		mpd_player_get_current_song_id	(MpdObj *mi);
int 		mpd_player_get_current_song_pos	(MpdObj *mi);
int		mpd_player_get_repeat		(MpdObj *mi);
int		mpd_player_set_repeat		(MpdObj *mi, int repeat);
int		mpd_player_get_random		(MpdObj *mi);
int		mpd_player_set_random		(MpdObj *mi, int random);
int 		mpd_player_seek			(MpdObj *mi, int sec);

/* playlist command */
mpd_Song * 	mpd_playlist_get_song		(MpdObj *mi, int songid);
mpd_Song * 	mpd_playlist_get_current_song	(MpdObj *mi);
int 		mpd_playlist_clear			(MpdObj *mi);
int 		mpd_playlist_shuffle			(MpdObj *mi);
int 		mpd_playlist_save			(MpdObj *mi, char *name);
void 		mpd_playlist_update_dir		(MpdObj *mi, char *path);
void 		mpd_playlist_move_pos		(MpdObj *mi, int old_pos, int new_pos);
MpdData * 	mpd_playlist_get_artists		(MpdObj *mi);
MpdData *	mpd_playlist_get_albums		(MpdObj *mi, char *artist);
MpdData * 	mpd_playlist_get_directory		(MpdObj *mi,char *path);
MpdData * 	mpd_playlist_find			(MpdObj *mi, int table, char *string, int exact);

MpdData * 	mpd_playlist_get_changes		(MpdObj *mi,int old_playlist_id);
int		mpd_playlist_get_playlist_length	(MpdObj *mi);
void		mpd_playlist_add			(MpdObj *mi, char *path);
int 		mpd_playlist_delete			(MpdObj *mi,char *path);

/* mpd svn only functions 0.12.0 */
MpdData * 	mpd_playlist_get_unique_tags		(MpdObj *mi, int table,...);
MpdData *	mpd_playlist_find_adv		(MpdObj *mi,int exact, ...);
/* client side search function with best "match" option..
 * It splits the search string into tokens. (on the ' ')  every token is then matched using regex.
 * So f.e. Murder Hooker|Davis  matches songs where title/filename/artist/album contains murder and hooker or murder davis in any order.
 * Warning: This function can be slow.
 */
MpdData *	mpd_playlist_token_find		(MpdObj *mi , char *string);
/* MpdData struct functions */
int 		mpd_data_is_last			(MpdData *data);
/* new api name.. don't like the old one */
void 		mpd_data_free				(MpdData *data);
MpdData * 	mpd_data_get_next			(MpdData *data);

/* mpd ob data next will return NULL when there are no more items. it will also call free when called on the last item. */
/* if you don't want this check with mpd_data_is_last before calling get_next 
 * this allows you to make this construction: 
 *	MpdData * mpd_playlist_get_artists(..);
 * 	while(data != NULL)
 * 	{
 *
 *
 *		data = mpd_data_next(data);
 * 	}
 */
 /* withouth leaking memory  */


/* queing stuff */
void 		mpd_playlist_queue_add		(MpdObj *mi,char *path);
void 		mpd_playlist_queue_load		(MpdObj *mi,char *path);
void 		mpd_playlist_queue_delete_id		(MpdObj *mi,int id);
/* use these to commit the changes */
void 		mpd_playlist_queue_commit		(MpdObj *mi);


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
