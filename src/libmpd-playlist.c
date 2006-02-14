/*
 *Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
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

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU

#include <string.h>
#include <regex.h>
#include <stdarg.h>
#include <config.h>
#include "debug_printf.h"
#include "libmpd.h"
#include "libmpd-internal.h"


int mpd_playlist_get_playlist_length(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR,"Failed grabbing status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->status->playlistLength;
}

long long mpd_playlist_get_old_playlist_id(MpdObj *mi)
{
	return mi->OldState.playlistid;
}

long long mpd_playlist_get_playlist_id(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed grabbing status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->status->playlist;
}
int mpd_playlist_add(MpdObj *mi, char *path)
{
	int retv = mpd_playlist_queue_add(mi, path);
	if(retv != MPD_OK) return retv;
	return mpd_playlist_queue_commit(mi);
}

int mpd_playlist_delete_id(MpdObj *mi, int songid)
{
	int retv = mpd_playlist_queue_delete_id(mi, songid);
	if(retv != MPD_OK) return retv;
	return mpd_playlist_queue_commit(mi);
}

int mpd_playlist_delete_pos(MpdObj *mi, int songpos)
{
	int retv = mpd_playlist_queue_delete_pos(mi, songpos);
	if(retv != MPD_OK) return retv;
	return mpd_playlist_queue_commit(mi);
}
/*******************************************************************************
 * PLAYLIST
 */
mpd_Song * mpd_playlist_get_song(MpdObj *mi, int songid)
{
	mpd_Song *song = NULL;
	mpd_InfoEntity *ent = NULL;
	if(songid < 0){
		debug_printf(DEBUG_ERROR, "songid < 0 Failed");
		return NULL;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return NULL;
	}

	if(mpd_lock_conn(mi))
	{
		return NULL;
	}
	debug_printf(DEBUG_INFO, "Trying to grab song with id: %i\n", songid);
	mpd_sendPlaylistIdCommand(mi->connection, songid);
	ent = mpd_getNextInfoEntity(mi->connection);
	mpd_finishCommand(mi->connection);

	if(mpd_unlock_conn(mi))
	{
		/*TODO free entity. for now this can never happen */
		return NULL;
	}

	if(ent == NULL)
	{
		debug_printf(DEBUG_ERROR, "Failed to grab song from mpd\n");
		return NULL;
	}

	if(ent->type != MPD_INFO_ENTITY_TYPE_SONG)
	{
		mpd_freeInfoEntity(ent);
		debug_printf(DEBUG_ERROR, "Failed to grab corect song type from mpd\n");
		return NULL;
	}
	song = ent->info.song;
	ent->info.song = NULL;

	mpd_freeInfoEntity(ent);

	return song;
}


mpd_Song * mpd_playlist_get_current_song(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "Not Connected\n");
		return NULL;
	}

	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR, "Failed to check status\n");
		return NULL;
	}

	if(mi->CurrentSong != NULL && mi->CurrentSong->id != mi->status->songid)
	{
		debug_printf(DEBUG_WARNING, "Current song not up2date, updating\n");
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}
	/* only update song when playing/pasing */
	if(mi->CurrentSong == NULL && 
			(mpd_player_get_state(mi) != MPD_PLAYER_STOP && mpd_player_get_state(mi) != MPD_PLAYER_UNKNOWN))
	{
		/* TODO: this to use the geT_current_song_id function */
		mi->CurrentSong = mpd_playlist_get_song(mi, mpd_player_get_current_song_id(mi));
		if(mi->CurrentSong == NULL)
		{
			debug_printf(DEBUG_ERROR, "Failed to grab song\n");
			return NULL;
		}
	}
	return mi->CurrentSong;
}

int mpd_playlist_clear(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_sendClearCommand(mi->connection);
	mpd_finishCommand(mi->connection);
	/* hack to make it update correctly when replacing 1 song */
	mi->CurrentState.songid = -1;
	/* unlock */
	mpd_unlock_conn(mi);
	return FALSE;
}

int mpd_playlist_shuffle(MpdObj *mi)
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

	mpd_sendShuffleCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	return FALSE;

}


int mpd_playlist_move_id(MpdObj *mi, int old_id, int new_id)
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

	mpd_sendMoveIdCommand(mi->connection,old_id, new_id);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	return MPD_OK;
}

int mpd_playlist_move_pos(MpdObj *mi, int old_pos, int new_pos)
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

	mpd_sendMoveCommand(mi->connection,old_pos, new_pos);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	return MPD_OK;
}

MpdData * mpd_playlist_get_unique_tags(MpdObj *mi, int field,...)
{
	int table = 0;
	char *string = NULL;
	MpdData *data = NULL;
	va_list arglist;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_artists: not connected\n");
		return NULL;
	}

	if(!mpd_server_check_version(mi,0,12,0))
	{

		debug_printf(DEBUG_WARNING, "mpd_playlist_get_unique_tag:For this feature you need at least mpd version 0.12.0");
		return NULL;
	}
	if(table < 0 || table >= MPD_TAG_NUM_OF_ITEM_TYPES)
	{
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_unique_tag: Undefined table defined");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_artists: lock failed\n");
		return NULL;
	}
	mpd_startFieldSearch(mi->connection,field);
	va_start(arglist, field);
	while((table = va_arg(arglist, int)) != -1)
	{
		if(table >= 0 && table < MPD_TAG_NUM_OF_ITEM_TYPES)         	
		{
			char *str = va_arg(arglist,char *);
			mpd_addConstraintSearch(mi->connection, table, str);
		}	                                                   	
	}
	va_end(arglist);
	mpd_commitSearch(mi->connection);

	//	mpd_sendVListTagCommand(mi->connection,table,arglist);

	while (( string = mpd_getNextTag(mi->connection,field)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_TAG;
		data->tag = string;
	}
	mpd_finishCommand(mi->connection);
	
	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	data = mpd_playlist_sort_tag_list(data);
	return mpd_data_get_first(data);
}

/* "hack" to keep the compiler happy */
typedef int (* QsortCompare)(const void *a, const void *b);

static int compa(char **a, char **b)
{
	char *c =*a;
	char *d =*b;
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

MpdData *mpd_playlist_sort_tag_list(MpdData *data)
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


MpdData * mpd_playlist_get_directory(MpdObj *mi,char *path)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
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

	mpd_sendLsInfoCommand(mi->connection,path);
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->directory = ent->info.directory->path;
			ent->info.directory->path = NULL;
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->song = ent->info.song;
			ent->info.song = NULL;
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->playlist = ent->info.playlistFile->path;
			ent->info.playlistFile->path = NULL;
		}

		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}
/* TODO need a rewrite */
/* Should be named: mpd_database_find_adv */
MpdData *mpd_playlist_find_adv(MpdObj *mi,int exact, ...)
{
	int table = 0;
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	va_list arglist;
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
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return NULL;
	}
	va_start(arglist, exact);
	mpd_startSearch(mi->connection, exact);
	while((table = va_arg(arglist, int)) != -1)
	{
		if(table >= 0 && table < MPD_TAG_NUM_OF_ITEM_TYPES)
		{
			char *str = va_arg(arglist,char *);
			mpd_addConstraintSearch(mi->connection, table, str);
		}	
	}
	mpd_commitSearch(mi->connection);
	va_end(arglist);
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append( data );
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->directory = ent->info.directory->path;
			ent->info.directory->path = NULL;
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->song = ent->info.song;
			ent->info.song = NULL;
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->playlist = ent->info.playlistFile->path;
			ent->info.playlistFile->path = NULL;
		}

		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

/* should be called mpd_database_find */
MpdData * mpd_playlist_find(MpdObj *mi, int table, char *string, int exact)
{
	MpdData *data = NULL;
/*	MpdData *artist = NULL;
	MpdData *album = NULL;
*/	mpd_InfoEntity *ent = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_find: not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_find: lock failed\n");
		return NULL;
	}
	if(exact)
	{
		mpd_sendFindCommand(mi->connection,table,string);
	}
	else
	{
		mpd_sendSearchCommand(mi->connection, table,string);
	}
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		/* mpd_sendSearch|Find only returns songs */
		/*
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->directory = ent->info.directory->path;
			ent->info.directory->path = NULL;
		}
		else*/ if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->song = ent->info.song;
			ent->info.song = NULL;
			/* This is something the client can and should do */
/*			if(data->song->artist != NULL)
			{
				int found = FALSE;
				if(artist != NULL)
				{
					MpdData *fartist = mpd_data_get_first(artist);
					do{
						if( (fartist->type == MPD_DATA_TYPE_TAG) && (fartist->tag_type == MPD_TAG_ITEM_ARTIST))
						{
							if(fartist->tag == NULL)
							{
								printf("crap this should'nt be \n");
							}
							if(!strcmp(fartist->tag, data->song->artist))
							{
								found = TRUE;
							}
						}
						fartist = mpd_data_get_next_real(fartist, FALSE);
					}while(fartist && !found);
				}
				if(!found)
				{
					artist= mpd_new_data_struct_append(artist);
					artist->type = MPD_DATA_TYPE_TAG;
					artist->tag_type = MPD_TAG_ITEM_ARTIST;
					artist->tag = strdup(data->song->artist);
				}
			}
			if(data->song->album != NULL)
			{
				int found = FALSE;
				if(album != NULL)
				{
					MpdData *falbum = mpd_data_get_first(album);
					do{
						if( (falbum->type == MPD_DATA_TYPE_TAG) && (falbum->tag_type == MPD_TAG_ITEM_ALBUM))
						{
							if(falbum->tag == NULL)
							{
								printf("crap this should'nt be \n");
							}
							if(!strcmp(falbum->tag, data->song->album))
							{
								found = TRUE;
							}
						}
						falbum = mpd_data_get_next_real(falbum, FALSE);
					}while(falbum && !found);
				}
				if(!found)
				{
					album = mpd_new_data_struct_append(album);
					album->type = MPD_DATA_TYPE_TAG;
					album->tag_type = MPD_TAG_ITEM_ALBUM;
					album->tag = strdup(data->song->album);
				}
			}
*/
		}
		/*
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->playlist = ent->info.playlistFile->path;
			ent->info.playlistFile->path = NULL;
		}
		*/

		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

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


MpdData * mpd_playlist_get_changes(MpdObj *mi,int old_playlist_id)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
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

	if(old_playlist_id == -1)
	{
		debug_printf(DEBUG_INFO,"get fresh playlist\n");
		mpd_sendPlaylistIdCommand(mi->connection, -1);
	}
	else
	{
		mpd_sendPlChangesCommand (mi->connection, old_playlist_id);
	}

	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{
		if(ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data = mpd_new_data_struct_append(data);
			data->type = MPD_DATA_TYPE_SONG;
			data->song = ent->info.song;
			ent->info.song = NULL;
		}
		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	if(mpd_unlock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_changes: unlock failed.\n");
		mpd_data_free(data);
		return NULL;
	}
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}


int mpd_playlist_queue_add(MpdObj *mi,char *path)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(path == NULL)
	{
		debug_printf(DEBUG_ERROR, "path != NULL Failed");
		return MPD_ARGS_ERROR;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}
	else
	{
		mi->queue->next = mpd_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_ADD;
	mi->queue->path = strdup(path);
	return MPD_OK;
}

int mpd_playlist_queue_load(MpdObj *mi,char *path)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(path == NULL)
	{
		debug_printf(DEBUG_ERROR, "path != NULL Failed");
		return MPD_ARGS_ERROR;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}
	else
	{
		mi->queue->next = mpd_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_LOAD;
	mi->queue->path = strdup(path);
	return MPD_OK;
}


int mpd_playlist_queue_commit(MpdObj *mi)
{
	if(mi->queue == NULL)
	{
		debug_printf(DEBUG_WARNING,"mi->queue is empty");
		return MPD_PLAYLIST_QUEUE_EMPTY;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"lock failed\n");
		return  MPD_LOCK_FAILED;
	}
	mpd_sendCommandListBegin(mi->connection);
	/* get first item */
	mi->queue = mi->queue->first;
	while(mi->queue != NULL)
	{
		if(mi->queue->type == MPD_QUEUE_ADD)
		{
			if(mi->queue->path != NULL)
			{
				mpd_sendAddCommand(mi->connection, mi->queue->path);
			}
		}
		else if(mi->queue->type == MPD_QUEUE_LOAD)
		{
			if(mi->queue->path != NULL)
			{
				mpd_sendLoadCommand(mi->connection, mi->queue->path);
			}
		}
		else if (mi->queue->type == MPD_QUEUE_DELETE_ID)
		{
			if(mi->queue->id >= 0)
			{
				mpd_sendDeleteIdCommand(mi->connection, mi->queue->id);
			}
		}
		else if (mi->queue->type == MPD_QUEUE_DELETE_POS)
		{                                                                      		
			if(mi->queue->id >= 0)
			{
				mpd_sendDeleteCommand(mi->connection, mi->queue->id);
			}
		}


		mpd_queue_get_next(mi);
	}
	mpd_sendCommandListEnd(mi->connection);
	mpd_finishCommand(mi->connection);
	mpd_unlock_conn(mi);
	return MPD_OK;
}
int mpd_playlist_queue_delete_id(MpdObj *mi,int id)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}
	else
	{
		mi->queue->next = mpd_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_DELETE_ID;
	mi->queue->id = id;
	mi->queue->path = NULL;
	return MPD_OK;
}

int mpd_playlist_queue_delete_pos(MpdObj *mi,int songpos)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_add: not connected\n");
		return MPD_NOT_CONNECTED;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}
	else
	{
		mi->queue->next = mpd_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_DELETE_POS;
	mi->queue->id = songpos;
	mi->queue->path = NULL;
	return MPD_OK;
}

/* deprecated stuff */
int mpd_playlist_update_dir(MpdObj *mi, char *path){ return mpd_database_update_dir(mi,path);}
MpdData * mpd_playlist_get_albums(MpdObj *mi, char *artist) { return mpd_database_get_albums(mi,artist);}
MpdData * mpd_playlist_get_artists(MpdObj *mi) { return mpd_database_get_artists(mi);}
MpdData * mpd_playlist_token_find(MpdObj *mi, char *string) {return mpd_database_token_find(mi,string);}
int mpd_playlist_delete(MpdObj *mi, char  *path) {return mpd_database_delete_playlist(mi, path);}
int mpd_playlist_save(MpdObj *mi, char  *path) {return mpd_database_save_playlist(mi, path);}


