/*
 *Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
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

MpdData * mpd_playlist_sort_artist_list(MpdData *data);
MpdData * mpd_playlist_sort_tag_list(MpdData *data);

int mpd_playlist_get_playlist_length(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_player_get_playlist_length: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_player_get_playlist_length: Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mi->status->playlistLength;
}

long long mpd_playlist_get_playlist_id(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_player_get_playlist_id: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_player_get_playlist_id: Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mi->status->playlist;
}
void mpd_playlist_add(MpdObj *mi, char *path)
{
	mpd_playlist_queue_add(mi, path);
	mpd_playlist_queue_commit(mi);
}

/*******************************************************************************
 * PLAYLIST 
 */
mpd_Song * mpd_playlist_get_song(MpdObj *mi, int songid)
{
	mpd_Song *song = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_song: Not Connected\n");
		return NULL;
	}

	if(mpd_lock_conn(mi))
	{
		return NULL;
	}
	debug_printf(DEBUG_INFO, "mpd_playlist_get_song: Trying to grab song with id: %i\n", songid);
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
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_song: Failed to grab song from mpd\n");
		return NULL;
	}

	if(ent->type != MPD_INFO_ENTITY_TYPE_SONG)
	{
		mpd_freeInfoEntity(ent);
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_song: Failed to grab corect song type from mpd\n");
		return NULL;
	}
	song = mpd_songDup(ent->info.song);
	mpd_freeInfoEntity(ent);

	return song;
}


mpd_Song * mpd_playlist_get_current_song(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_current_song: Not Connected\n");
		return NULL;
	}

	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_playlist_get_current_song: Failed to check status\n");
		return NULL; 
	}

	if(mi->CurrentSong != NULL && mi->CurrentSong->id != mi->status->songid)
	{
		debug_printf(DEBUG_WARNING, "mpd_playlist_get_current_song: Current song not up2date, updating\n");
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}

	if(mi->CurrentSong == NULL)
	{
		/* TODO: this to use the geT_current_song_id function */
		mi->CurrentSong = mpd_playlist_get_song(mi, mpd_player_get_current_song_id(mi));
		if(mi->CurrentSong == NULL)
		{
			debug_printf(DEBUG_ERROR, "mpd_playlist_get_current_song: Failed to grab song\n");
			return NULL;
		}
	}
	return mi->CurrentSong;
}

int mpd_playlist_delete(MpdObj *mi,char *path)
{
	if(path == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_playlist_delete: path == NULL");
		return MPD_ERROR;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_delete: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_delete: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_sendRmCommand(mi->connection,path);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	return FALSE;
}



int mpd_playlist_clear(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_clear: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_clear: lock failed\n");
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
		debug_printf(DEBUG_WARNING,"mpd_playlist_shuffle: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_shuffle: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_sendShuffleCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	return FALSE;

}

int mpd_playlist_save(MpdObj *mi, char *name)
{
	if(name == NULL || !strlen(name))
	{
		debug_printf(DEBUG_WARNING, "mpd_playlist_save: name != NULL  and strlen(name) > 0 failed");
		return MPD_ERROR;
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

	mpd_sendSaveCommand(mi->connection,name);
	mpd_finishCommand(mi->connection);
	if(mi->connection->error == MPD_ERROR_ACK && mi->connection->errorCode == MPD_ACK_ERROR_EXIST)
	{
		mpd_clearError(mi->connection);
		mpd_unlock_conn(mi);	
		return MPD_PLAYLIST_EXIST; 

	}

	/* unlock */                                               	
	mpd_unlock_conn(mi);
	return FALSE;
}

void mpd_playlist_update_dir(MpdObj *mi, char *path)
{
	if(path == NULL || !strlen(path))
	{
		debug_printf(DEBUG_WARNING, "mpd_playlist_update_dir: path != NULL  and strlen(path) > 0 failed");
		return;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_update_dir: not connected\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_playlist_update_dir: lock failed\n");
		return;
	}

	mpd_sendUpdateCommand(mi->connection,path);
	mpd_finishCommand(mi->connection);

	/* unlock */                                               	
	mpd_unlock_conn(mi);
	return;
}


void mpd_playlist_move_pos(MpdObj *mi, int old_pos, int new_pos)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_move_pos: not connected\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_playlist_move_pos: lock failed\n");
		return;
	}

	mpd_sendMoveCommand(mi->connection,old_pos, new_pos);
	mpd_finishCommand(mi->connection);

	/* unlock */                                               	
	mpd_unlock_conn(mi);
	return;
}

MpdData * mpd_playlist_get_unique_tags(MpdObj *mi, int table,...)
{
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
	va_start(arglist, table);
	mpd_sendVListTagCommand(mi->connection,table,arglist);
	va_end(arglist);
	while (( string = mpd_getNextTag(mi->connection,table)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;

		}	
		else
		{
			data->next = mpd_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_TAG; 
		data->value.tag = string;
	}
	mpd_finishCommand(mi->connection);

	data = mpd_playlist_sort_tag_list(data);
	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}

MpdData * mpd_playlist_get_artists(MpdObj *mi)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_artists: not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_artists: lock failed\n");
		return NULL;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ARTIST,NULL);
	while (( string = mpd_getNextArtist(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;

		}	
		else
		{
			data->next = mpd_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_ARTIST; 
		data->value.artist = string;
	}
	mpd_finishCommand(mi->connection);

	data = mpd_playlist_sort_artist_list(data);
	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}



MpdData * mpd_playlist_sort_artist_list(MpdData *data)
{
	int changed = 0;
	MpdData *temp = NULL;
	MpdData *pldata = NULL, *first = NULL;
	if(data == NULL) return NULL;
	first = pldata = data->first;
	do
	{
		changed = 0;
		while(pldata != NULL && pldata->next != NULL)	
		{
			pldata->first = first;
			if(pldata->next->type != MPD_DATA_TYPE_ARTIST)
			{
				/* do nothing */
			}
			else if((pldata->type != MPD_DATA_TYPE_ARTIST && pldata->next->type == MPD_DATA_TYPE_ARTIST)||
					(pldata->next != NULL && strcasecmp(pldata->next->value.artist,pldata->value.artist) < 0 ))
			{
				/* swap them.*/
				temp = pldata->next;
				if(temp->next != NULL)
				{
					temp->next->prev = pldata;
				}
				pldata->next = temp->next;
				temp->next = pldata;
				if(pldata->prev)
				{
					pldata->prev->next = temp;				
				}   
				temp->prev = pldata->prev;
				pldata->prev = temp;

				if(first == pldata)
				{
					first = temp;		
				}
				changed = TRUE;
			}
			pldata = pldata->next;
		}
		pldata = first;
	}
	while(changed);
	return pldata;
}


MpdData * mpd_playlist_sort_tag_list(MpdData *data)
{
	int changed = 0;
	MpdData *temp = NULL;
	MpdData *pldata = NULL, *first = NULL;
	if(data == NULL) return NULL;
	first = pldata = data->first;
	do
	{
		changed = 0;
		while(pldata != NULL && pldata->next != NULL)	
		{
			pldata->first = first;
			if(pldata->next->type != MPD_DATA_TYPE_TAG)
			{
				/* do nothing */
			}
			else if((pldata->type != MPD_DATA_TYPE_TAG && pldata->next->type == MPD_DATA_TYPE_TAG)||
					(pldata->next != NULL && strcasecmp(pldata->next->value.tag,pldata->value.tag) < 0 ))
			{
				/* swap them.*/
				temp = pldata->next;
				if(temp->next != NULL)
				{
					temp->next->prev = pldata;
				}
				pldata->next = temp->next;
				temp->next = pldata;
				if(pldata->prev)
				{
					pldata->prev->next = temp;				
				}   
				temp->prev = pldata->prev;
				pldata->prev = temp;

				if(first == pldata)
				{
					first = temp;		
				}
				changed = TRUE;
			}
			pldata = pldata->next;
		}
		pldata = first;
	}
	while(changed);
	return pldata;
}



MpdData * mpd_playlist_get_albums(MpdObj *mi,char *artist)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_albums: not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_albums: lock failed\n");
		return NULL;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ALBUM,artist);
	while (( string = mpd_getNextAlbum(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_ALBUM;
		data->value.album = string;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}




MpdData * mpd_playlist_get_directory(MpdObj *mi,char *path)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_albums: not connected\n");
		return NULL;
	}
	if(path == NULL)
	{
		path = "/";
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_albums: lock failed\n");
		return NULL;
	}

	mpd_sendLsInfoCommand(mi->connection,path);
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->value.directory = strdup(ent->info.directory->path);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->value.playlist = strdup(ent->info.playlistFile->path);

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
	return data->first;
}

MpdData *mpd_playlist_token_find(MpdObj *mi , char *string)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	regex_t ** strdata = NULL;
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

	if(string == NULL || !strlen(string) )
	{
		debug_printf(DEBUG_INFO, "no string found");
		mpd_unlock_conn(mi);
		return NULL;
	}
	else{
		strdata = mpd_misc_tokenize(string);
	}	
	if(strdata == NULL)
	{
		mpd_unlock_conn(mi);
		debug_printf(DEBUG_INFO, "no split string found");
		return NULL;
	}

	mpd_sendListallInfoCommand(mi->connection, "/");
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			int i = 0;
			int match = 0;
			int loop = 1;
			for(i=0; strdata[i] != NULL && loop; i++)
			{
				match = 0;
				if(ent->info.song->file && !regexec(strdata[i],ent->info.song->file, 0, NULL, 0))
				{
					match = 1;
				}
				else if(ent->info.song->artist && !regexec(strdata[i],ent->info.song->artist, 0, NULL, 0))
				{
					match = 1;
				}
				else if(ent->info.song->title && !regexec(strdata[i],ent->info.song->title, 0, NULL, 0)) 
				{
					match = 1;
				}
				else if(ent->info.song->album && !regexec(strdata[i],ent->info.song->album, 0, NULL, 0))
				{
					match = 1;                                                   				
				}
				if(!match)
				{

					loop = 0;

				}
			}

			if(match)
			{
				if(data == NULL)
				{
					data = mpd_new_data_struct();
					data->first = data;
					data->next = NULL;
					data->prev = NULL;
				}	
				else
				{
					data->next = mpd_new_data_struct();
					data->next->first = data->first;
					data->next->prev = data;
					data = data->next;
					data->next = NULL;
				}
				data->type = MPD_DATA_TYPE_SONG;
				data->value.song = mpd_songDup(ent->info.song);				
			}
		}
		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);
	mpd_misc_tokens_free(strdata);




	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return data->first;
}

MpdData *mpd_playlist_find_adv(MpdObj *mi,int exact, ...)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	va_list arglist;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_find: not connected\n");
		return NULL;
	}
	if(!mpd_server_check_version(mi, 0,12,0))
	{
		debug_printf(DEBUG_WARNING, "mpd_plalist_find_adv: only works with mpd higher then 0.12.0");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_find: lock failed\n");
		return NULL;
	}
	va_start(arglist, exact);
	if(exact)
	{
		mpd_sendVFindTagCommand(mi->connection,arglist);
	}
	else
	{
		mpd_sendVSearchTagCommand(mi->connection, arglist);
	}
	va_end(arglist);
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->value.directory = strdup(ent->info.directory->path);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;                            	
			data->value.song = mpd_songDup(ent->info.song);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->value.playlist = strdup(ent->info.playlistFile->path);
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
	return data->first;
}

MpdData * mpd_playlist_find(MpdObj *mi, int table, char *string, int exact)
{
	MpdData *data = NULL;
	MpdData *artist = NULL;
	MpdData *album = NULL;
	mpd_InfoEntity *ent = NULL;
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
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->value.directory = strdup(ent->info.directory->path);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
			if(data->value.song->artist != NULL)
			{
				int found = FALSE;
				if(artist != NULL)
				{
					MpdData *fartist = artist->first;
					do{
						if(fartist->type == MPD_DATA_TYPE_ARTIST)
						{
							if(fartist->value.artist == NULL)
							{
								printf("crap this should'nt be \n");
							}
							if(!strcmp(fartist->value.artist, data->value.song->artist))
							{
								found = TRUE;
							}
						}
						fartist = fartist->next;
					}while(fartist && !found);
				}	
				if(!found)
				{
					artist= mpd_new_data_struct_append(artist);
					artist->type = MPD_DATA_TYPE_ARTIST;
					artist->value.artist = strdup(data->value.song->artist);
				}
			}
			if(data->value.song->album != NULL)
			{
				int found = FALSE;
				if(artist != NULL)
				{
					MpdData *fartist = artist->first;
					do{
						if(fartist->type == MPD_DATA_TYPE_ALBUM)
						{
							if(fartist->value.album == NULL)
							{
								printf("crap this should'nt be \n");
							}
							if(!strcmp(fartist->value.album, data->value.song->album))
							{
								found = TRUE;
							}
						}
						fartist = fartist->next;
					}while(fartist && !found);
				}	
				if(!found)
				{
					artist= mpd_new_data_struct_append(artist);
					artist->type = MPD_DATA_TYPE_ALBUM;
					artist->value.album = strdup(data->value.song->album);
					if(data->value.song->artist)
					{
						artist->value.artist = strdup(data->value.song->artist);
					}
				}
			}

		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->value.playlist = strdup(ent->info.playlistFile->path);
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
	data = data->first;
	/* prepend the album then artists*/
	if(artist)
	{
		artist->next = data;
		data->prev = artist;
		/* make data point to the first in the list */
		data= artist->first;
		/* use the artist to iterate over it */
		/* I need to set all the -> first correct */
		artist = data;
		do{
			artist->first = data;
			artist = artist->next;
		}while(artist);
	}

	return data;
}


MpdData * mpd_playlist_get_changes(MpdObj *mi,int old_playlist_id)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_changes: not connected\n");
		return NULL;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_get_changes: lock failed\n");
		return NULL;
	}

	if(old_playlist_id == -1)
	{
		debug_printf(DEBUG_INFO,"mpd_playlist_get_changes: get fresh playlist\n");
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
			if(data == NULL)
			{
				data = mpd_new_data_struct();
				data->first = data;
				data->next = NULL;
				data->prev = NULL;
			}	
			else
			{
				data->next = mpd_new_data_struct();
				data->next->first = data->first;
				data->next->prev = data;
				data = data->next;
				data->next = NULL;
			}
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
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
	return data->first;
}


void mpd_playlist_queue_add(MpdObj *mi,char *path)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_add: not connected\n");
		return;
	}
	if(path == NULL)
	{
		return;
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
}

void mpd_playlist_queue_load(MpdObj *mi,char *path)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_add: not connected\n");
		return;
	}
	if(path == NULL)
	{
		return;
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
}


void mpd_playlist_queue_commit(MpdObj *mi)
{
	if(mi->queue == NULL)
	{
		return;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_add: not connected\n");
		return;
	}                                                      	
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_find: lock failed\n");
		return ;
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
		mpd_queue_get_next(mi);
	}
	mpd_sendCommandListEnd(mi->connection);
	mpd_finishCommand(mi->connection);
	mpd_unlock_conn(mi);
}
void mpd_playlist_queue_delete_id(MpdObj *mi,int id)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_playlist_add: not connected\n");
		return;
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
}
