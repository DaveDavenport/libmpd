/* libmpd (high level libmpdclient library)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU

#include <string.h>
#include <stdarg.h>
#include <config.h>
#include "debug_printf.h"
#include "libmpd.h"
#include "libmpd-internal.h"

int mpd_database_update_dir(MpdObj *mi,const char *path)
{
	if(path == NULL)
	{
		debug_printf(DEBUG_ERROR, "path != NULL failed");
		return MPD_ARGS_ERROR;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_update(mi->connection,path);
	/* I have no idea why do this ?? it even makes gmpc very very unhappy.
	 * Because it doesnt trigger an signal anymore when updating starts
	 * mi->CurrentState.updatingDb = mpd_getUpdateId(mi->connection);
	*/

	/* unlock */
	mpd_unlock_conn(mi);
	/* What I think you should do is to force a direct status updated
	 */
	mpd_status_update(mi);
	return MPD_OK;
}

MpdData * mpd_database_get_artists(MpdObj *mi)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}

	mpd_search_db_tags(mi->connection, MPD_TAG_ARTIST);
	struct mpd_pair *pair;
	while ((pair = mpd_recv_pair_tag(mi->connection, MPD_TAG_ARTIST)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_TAG;
		data->tag_type = MPD_TAG_ARTIST;
		data->tag = strdup(pair->value);
		mpd_return_pair(mi->connection, pair);
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
/*	data = mpd_misc_sort_tag_list(data);*/
	return mpd_data_get_first(data);
}

MpdData * mpd_database_get_albums(MpdObj *mi,const char *artist)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}

	mpd_search_db_tags(mi->connection, MPD_TAG_ALBUM);
	struct mpd_pair *pair;
	while ((pair = mpd_recv_pair_tag(mi->connection, MPD_TAG_ALBUM)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_TAG;
		data->tag_type = MPD_TAG_ALBUM;
		data->tag = strdup(pair->value);
		mpd_return_pair(mi->connection, pair);
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

MpdData * mpd_database_get_complete(MpdObj *mi)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}
	mpd_send_list_all_meta(mi->connection, "");
	struct mpd_song *song;
	while (( song = mpd_recv_song(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_SONG;
		data->song = song;
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

int mpd_database_delete_playlist(MpdObj *mi,const char *path)
{
	if(path == NULL)
	{
		debug_printf(DEBUG_WARNING, "path == NULL");
		return MPD_ARGS_ERROR;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_rm(mi->connection, path);

	/* unlock */
	mpd_unlock_conn(mi);
	return MPD_OK;
}

int mpd_database_save_playlist(MpdObj *mi,const char *name)
{
	if(name == NULL || !strlen(name))
	{
		debug_printf(DEBUG_WARNING, "mpd_playlist_save: name != NULL  and strlen(name) > 0 failed");
		return MPD_ARGS_ERROR;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_save: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_playlist_save: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_save(mi->connection, name);
	if(mpd_connection_get_error(mi->connection) == MPD_ERROR_SERVER && mpd_connection_get_server_error(mi->connection) == MPD_SERVER_ERROR_EXIST)
	{
		mpd_run_clearerror(mi->connection);
		mpd_unlock_conn(mi);
		return MPD_DATABASE_PLAYLIST_EXIST;

	}
	/* unlock */
	mpd_unlock_conn(mi);
	return MPD_OK;
}

/* "hack" to keep the compiler happy */
typedef int (* QsortCompare)(const void *a, const void *b);

static int compa(char **a,const char **b)
{
	char *c =*a;
	char *d =(char*)*b;
#ifndef NO_SMART_SORT
	if(!strncasecmp(c, "The ",4) && strlen(c) > 4)
	{
		c = &c[4];
	}
	if(!strncasecmp(d, "The ",4) && strlen(d) > 4)
	{
		d = &d[4];
	}
#endif

	return strcasecmp(c,d);
}

MpdData *mpd_misc_sort_tag_list(MpdData *data)
{
	char **array;
	MpdData *test;
	int i=0;
	int length=0;
	test = data = mpd_data_get_first(data);

	do{
		length++;
		test = mpd_data_get_next_real(test, FALSE);
	}while(test != NULL);
	array = malloc(length*sizeof(char*));
	test = data;

	do
	{
		array[i] = test->tag;
		test = mpd_data_get_next_real(test, FALSE);
		i++;
	}while(test != NULL);

	qsort(array,length,sizeof(char *),(QsortCompare)compa);

	/* reset list */
	test = mpd_data_get_first(data);
	i=0;
	do
	{
		test->tag = array[i];
		test = mpd_data_get_next_real(test, FALSE);
		i++;
	}while(test != NULL);
	free(array);
	return mpd_data_get_first(data);
}

/* should be called mpd_database_find */
MpdData * mpd_database_find(MpdObj *mi, enum mpd_tag_type table,const char *string, int exact)
{
	MpdData *data = NULL;
/*	MpdData *artist = NULL;
	MpdData *album = NULL;
*/
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return NULL;
	}
	mpd_search_db_songs(mi->connection, exact);
	mpd_search_add_tag_constraint(mi->connection, MPD_OPERATOR_DEFAULT, table, string);
	mpd_search_commit(mi->connection);
	struct mpd_song *song;
	while (( song = mpd_recv_song(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_SONG;
		data->song = song;
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	data = mpd_data_get_first(data);
	/* prepend the album then artists*/
/*	if(album != NULL)
	{
		if(data){
			data  = mpd_data_concatenate( album, data);
		}else{
			data = album;
		}
	}
	if(artist != NULL)
	{
		if(data) {
			album = mpd_data_concatenate( artist, data );
		}else{
			data = artist;
		}
	}                                                     	
*/
	return mpd_data_get_first(data);
}

MpdData * mpd_database_get_directory(MpdObj *mi,const char *path)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(path == NULL)
	{
		path = "/";
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return NULL;
	}

	mpd_send_list_meta(mi->connection, path);
	struct mpd_entity *entity;
	while (( entity = mpd_recv_entity(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);

		switch (mpd_entity_get_type(entity)) {
		case MPD_ENTITY_TYPE_UNKNOWN:
			break;

		case MPD_ENTITY_TYPE_DIRECTORY:
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->directory = strdup(mpd_directory_get_path(mpd_entity_get_directory(entity)));
			break;

		case MPD_ENTITY_TYPE_SONG:
			data->type = MPD_DATA_TYPE_SONG;
			data->song = mpd_song_dup(mpd_entity_get_song(entity));
			break;

		case MPD_ENTITY_TYPE_PLAYLIST:
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->playlist = mpd_playlist_dup(mpd_entity_get_playlist(entity));
			break;
		}

		mpd_entity_free(entity);
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

MpdData *mpd_database_get_playlist_content(MpdObj *mi,const char *playlist)
{
	int i=0;
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(!mpd_server_check_version(mi, 0,12,0))
	{
		debug_printf(DEBUG_WARNING, "only works with mpd higher then 0.12.0");
		return NULL;
	}
	if(mpd_server_check_command_allowed(mi, "listplaylistinfo") != MPD_SERVER_COMMAND_ALLOWED)
	{
		debug_printf(DEBUG_WARNING, "Listing playlist content not supported or allowed");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return NULL;
	}
	mpd_send_list_playlist_meta(mi->connection, playlist);
	struct mpd_song *song;
	while (( song = mpd_recv_song(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append( data );
		data->type = MPD_DATA_TYPE_SONG;
		data->song = song;
		mpd_song_set_pos(song, i);
		i++;
	}
	mpd_response_finish(mi->connection);

	if(mpd_connection_get_error(mi->connection) == MPD_ERROR_SERVER && mpd_connection_get_server_error(mi->connection) == MPD_SERVER_ERROR_NO_EXIST)
	{
		mpd_run_clearerror(mi->connection);
	}
	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}
/**
 * @param mi A #MpdObj
 * @param path an Path to a file
 *
 * Grabs the song info for a single file. Make sure you pass an url to a song
 * and not a directory, that might result in strange behauviour.
 *
 * @returns a #mpd_Song
 */
struct mpd_song * mpd_database_get_fileinfo(MpdObj *mi,const char *path)
{
	/*
	 * Check path for availibility and length
	 */
	if(path == NULL || path[0] == '\0')
	{
		debug_printf(DEBUG_ERROR, "path == NULL || strlen(path) == 0");
		return NULL;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return NULL;
	}
	/* lock, so we can work on mi->connection */
	if(mpd_lock_conn(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to lock connection");
		return NULL;
	}
	/* send the request */
	mpd_send_list_meta(mi->connection, path);
	/* get the first (and only) result */
	struct mpd_song *song = mpd_recv_song(mi->connection);
	/* finish and clean up libmpdclient */
	mpd_response_finish(mi->connection);
	/* unlock again */
	if(mpd_unlock_conn(mi))
	{
		if(song) mpd_song_free(song);
		debug_printf(DEBUG_ERROR, "Failed to unlock");
		return NULL;
	}

	if(song == NULL)
	{
		debug_printf(DEBUG_ERROR, "Failed to grab song from mpd\n");
	}

	return song;
}


void mpd_database_search_field_start(MpdObj *mi, enum mpd_tag_type field)
{
	/*
	 * Check argument
	 */
	if(mi == NULL) 
	{
		debug_printf(DEBUG_ERROR, "Argument error");
		return ;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return ;
	}
	if(!mpd_server_check_version(mi, 0,12,0))
	{
		debug_printf(DEBUG_ERROR, "Advanced field list requires mpd 0.12.0 or higher");
		return ;
	}
	/* lock, so we can work on mi->connection */
	if(mpd_lock_conn(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to lock connection");
		return ;
	}
	mpd_search_db_tags(mi->connection, field);
	/* Set search type */
	mi->search_type = MPD_SEARCH_TYPE_LIST;
	mi->search_field = field;
	/* unlock, let the error handler handle any possible error.
	 */
	mpd_unlock_conn(mi);
	return;
}







/**
 * @param mi A #MpdObj
 * @param exact a boolean indicating if the search is fuzzy or exact
 *
 * Starts a search, you can add "constraints" by calling mpd_database_search_add_constraint
 * 
 * This function requires mpd 0.12.0 or higher 
 */

void mpd_database_search_start(MpdObj *mi, int exact)
{
	/*
	 * Check argument
	 */
	if(mi == NULL || exact > 1 || exact < 0) 
	{
		debug_printf(DEBUG_ERROR, "Argument error");
		return ;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return ;
	}
	if(!mpd_server_check_version(mi, 0,12,0))
	{
		debug_printf(DEBUG_ERROR, "Advanced search requires mpd 0.12.0 or higher");
		return ;
	}
	/* lock, so we can work on mi->connection */
	if(mpd_lock_conn(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to lock connection");
		return ;
	}
	mpd_search_db_songs(mi->connection, exact);
	/* Set search type */
	mi->search_type = (exact)? MPD_SEARCH_TYPE_FIND:MPD_SEARCH_TYPE_SEARCH;
	/* unlock, let the error handler handle any possible error.
	 */
	mpd_unlock_conn(mi);
	return;
}
/**
 * @param mi A #MpdObj
 * @param field A #mpd_TagItems
 *
 * Adds a constraint to the search 
 */
void mpd_database_search_add_constraint(MpdObj *mi, enum mpd_tag_type field, const char *value)
{
	if(mi == NULL )
	{
		debug_printf(DEBUG_ERROR,"Failed to parse arguments");
		return;
	}
	if(mi->search_type == MPD_SEARCH_TYPE_NONE)
	{
		debug_printf(DEBUG_ERROR, "No search to constraint");
		return;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return ;
	}
	if(!mpd_server_check_version(mi, 0,12,0))
	{
		debug_printf(DEBUG_ERROR, "Advanced search requires mpd 0.12.0 or higher");
		return ;
	}
	/* lock, so we can work on mi->connection */
	if(mpd_lock_conn(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to lock connection");
		return ;
	}
	mpd_search_add_tag_constraint(mi->connection, MPD_OPERATOR_DEFAULT, field, (value)?value:"");
	/* unlock, let the error handler handle any possible error.
	 */
	mpd_unlock_conn(mi);
	return;
}

MpdData * mpd_database_search_commit(MpdObj *mi)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mi->search_type == MPD_SEARCH_TYPE_NONE)
	{
		debug_printf(DEBUG_ERROR, "no search in progress to commit");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}
	mpd_search_commit(mi->connection);
	if(mi->search_type == MPD_SEARCH_TYPE_LIST)
	{
		struct mpd_pair *pair;
		while ((pair = mpd_recv_pair_tag(mi->connection, mi->search_field)) != NULL)
		{
			data = mpd_new_data_struct_append(data);
			data->type = MPD_DATA_TYPE_TAG;
			data->tag_type = mi->search_field;
			data->tag = strdup(pair->value);
			mpd_return_pair(mi->connection, pair);
		}
	}
	else
	{
		struct mpd_song *song;
		while (( song = mpd_recv_song(mi->connection)) != NULL)
		{
			data = mpd_new_data_struct_append(data);
			data->type = MPD_DATA_TYPE_SONG;
			data->song = song;
		}
	}
	mpd_response_finish(mi->connection);
	/*
	 * reset search type
	 */
	mi->search_type = MPD_SEARCH_TYPE_NONE;
	mi->search_field = MPD_TAG_ARTIST;
	/* unlock */
	if(mpd_unlock_conn(mi))
	{
		debug_printf(DEBUG_ERROR, "Failed to unlock connection");
		if(data)mpd_data_free(data);
		return NULL;
	}
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

/**
 * @param mi A #MpdObj
 *
 * Starts a search, you can add "constraints" by calling mpd_database_search_add_constraint
 * to get the result call mpd_database_search_stats_commit
 * 
 * This function requires mpd 0.13.0 or higher 
 */

void mpd_database_search_stats_start(MpdObj *mi)
{
	/*
	 * Check argument
	 */
	if(mi == NULL) 
	{
		debug_printf(DEBUG_ERROR, "Argument error");
		return ;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return ;
	}
	if(!mpd_server_check_version(mi, 0,13,0))
	{
		debug_printf(DEBUG_ERROR, "Advanced search requires mpd 0.13.0 or higher");
		return ;
	}
	/* lock, so we can work on mi->connection */
	if(mpd_lock_conn(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to lock connection");
		return ;
	}
	mpd_count_db_songs(mi->connection);
	/* Set search type */
	mi->search_type = MPD_SEARCH_TYPE_STATS;
	/* unlock, let the error handler handle any possible error.
	 */
	mpd_unlock_conn(mi);
	return;
}

struct mpd_stats * mpd_database_search_stats_commit(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mi->search_type != MPD_SEARCH_TYPE_STATS)
	{
		debug_printf(DEBUG_ERROR, "no/wrong search in progress to commit");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}
	mpd_search_commit(mi->connection);

	struct mpd_stats *data = mpd_recv_stats(mi->connection);
	/* unlock */
	if(mpd_unlock_conn(mi))
	{
		debug_printf(DEBUG_ERROR, "Failed to unlock connection");
		if(data)mpd_stats_free(data);
		return NULL;
	}
	if(data == NULL)
	{
		return NULL;
	}
	return data;
}

void mpd_database_search_free_stats(MpdDBStats *data)
{
	mpd_stats_free(data);
}

void mpd_database_playlist_clear(MpdObj *mi, const char *path)
{
	if(!path )
		return;
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		return ;
	}

	mpd_run_playlist_clear(mi->connection, path);

	mpd_unlock_conn(mi);
}






void mpd_database_playlist_list_delete(MpdObj *mi, const char *path, int pos)
{
	if(!path )
		return;
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		return ;
	}

	mpd_run_playlist_delete(mi->connection, path, pos);

	mpd_unlock_conn(mi);
}
void mpd_database_playlist_list_add(MpdObj *mi, const char *path, const char *file)
{
	if(!path )
		return;
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		return ;
	}

	mpd_run_playlist_add(mi->connection, path, file);

	mpd_unlock_conn(mi);
}

MpdData * mpd_database_get_directory_recursive(MpdObj *mi, const char *path)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(path == NULL || path[0] == '\0')
	{
		debug_printf(DEBUG_ERROR, "argumant invalid\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}
	mpd_send_list_all_meta(mi->connection,path);
	struct mpd_song *song;
	while (( song = mpd_recv_song(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_SONG;
		data->song = song;
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}
void mpd_database_playlist_rename(MpdObj *mi, const char *old_name, const char *new_name)
{
	if(!new_name || !old_name)
	{
		debug_printf(DEBUG_ERROR, "old != NULL && new != NULL failed");
		return;
	}
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		return ;
	}

	mpd_run_rename(mi->connection, old_name, new_name);

	mpd_unlock_conn(mi);
}

int mpd_database_playlist_move(MpdObj *mi, const char *playlist, int old_pos, int new_pos)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_playlist_move(mi->connection, playlist, old_pos, new_pos);

	/* unlock */
	mpd_unlock_conn(mi);
	return MPD_OK;
}


MpdData * mpd_database_playlist_list(MpdObj *mi)
{
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return NULL;
	}
	if(mpd_server_check_command_allowed(mi, "listplaylists") == MPD_SERVER_COMMAND_ALLOWED)
	{
		mpd_send_list_playlists(mi->connection);
	}
	else
	{
		mpd_send_list_meta (mi->connection, "");
	}
	struct mpd_playlist *playlist;
	while (( playlist = mpd_recv_playlist(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_PLAYLIST;
		data->playlist = playlist;
	}
	mpd_response_finish(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}
