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

void mpd_database_update_dir(MpdObj *mi, char *path)
{
	if(path == NULL || !strlen(path))
	{
		debug_printf(DEBUG_WARNING, "path != NULL  and strlen(path) > 0 failed");
		return;
	}
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return;
	}

	mpd_sendUpdateCommand(mi->connection,path);
	mpd_finishCommand(mi->connection);
	/* I have no idea why do this ?? it even makes gmpc very very unhappy.
	 * Because it doesnt trigger an signal anymore when updating starts
	 * mi->CurrentState.updatingDb = mpd_getUpdateId(mi->connection);
	*/

	/* unlock */
	mpd_unlock_conn(mi);
	/* What I think you should do is to force a direct status updated
	 */
	mpd_status_update(mi);
	return;
}

MpdData * mpd_database_get_artists(MpdObj *mi)
{
	char *string = NULL;
	MpdData *data = NULL;
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

	mpd_sendListCommand(mi->connection,MPD_TABLE_ARTIST,NULL);
	while (( string = mpd_getNextArtist(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_TAG;
		data->tag_type = MPD_TAG_ITEM_ARTIST;
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

MpdData * mpd_database_get_albums(MpdObj *mi,char *artist)
{
	char *string = NULL;
	MpdData *data = NULL;
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

	mpd_sendListCommand(mi->connection,MPD_TABLE_ALBUM,artist);
	while (( string = mpd_getNextAlbum(mi->connection)) != NULL)
	{
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_TAG;
		data->tag_type = MPD_TAG_ITEM_ALBUM;
		data->tag = string;
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

MpdData * mpd_database_get_complete(MpdObj *mi)
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
	mpd_sendListallInfoCommand(mi->connection, "/");
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{

		if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
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
	mpd_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

MpdData *mpd_database_token_find(MpdObj *mi , char *string)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	regex_t ** strdata = NULL;
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
				data = mpd_new_data_struct_append(data);
				data->type = MPD_DATA_TYPE_SONG;
				/*
				data->song = mpd_songDup(ent->info.song);
				*/
				data->song = ent->info.song;
				ent->info.song = NULL;
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
	return mpd_data_get_first(data);
}
