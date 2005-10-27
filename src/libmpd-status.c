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
int mpd_stats_check(MpdObj *mi);

int mpd_status_queue_update(MpdObj *mi)
{

	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_status_queue_update: Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->status != NULL)
	{                                  	
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	return FALSE;
}


int mpd_status_update(MpdObj *mi)
{
  ChangedStatusType what_changed=0;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_status_update: Where not connected\n");
		return TRUE;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_set_volume: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	
	if(mi->status != NULL)
	{
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	mpd_sendStatusCommand(mi->connection);
	mi->status = mpd_getStatus(mi->connection);
	if(mi->status == NULL)
	{
		debug_printf(DEBUG_ERROR,"mpd_status_update: Failed to grab status from mpd\n");
		mpd_unlock_conn(mi);
		return FALSE;
	}
	if(mpd_unlock_conn(mi))
	{
		return TRUE;
	}
	/*
	 * check for changes 
	 */
	/* first save the old status */
		memcpy(&(mi->OldState), &(mi->CurrentState), sizeof(MpdServerState));

	/* playlist change */
	if(mi->CurrentState.playlistid != mi->status->playlist)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_status_update: Playlist has changed!");

		/* deprecated */
		if(mi->playlist_changed != NULL)
		{
			mi->playlist_changed(mi, mi->OldState.playlistid, mi->status->playlist,mi->playlist_changed_pointer);
			if(!mpd_check_connected(mi))
			{
				return TRUE;
			}
		}


		/* We can't trust the current song anymore. so we remove it */
		/* tags might have been updated */
		if(mi->CurrentSong != NULL)
		{
			mpd_freeSong(mi->CurrentSong);
			mi->CurrentSong = NULL;
		}

    /* set MPD_CST_PLAYLIST to be changed */
		what_changed |= MPD_CST_PLAYLIST;

		/* save new id */
		mi->CurrentState.playlistid = mi->status->playlist;
	}


	/* state change */
	if(mi->CurrentState.state != mi->status->state)
	{
		/* deprecated */
		if(mi->state_changed != NULL)
		{                                                                      		
			mi->state_changed(mi, mi->OldState.state,mi->status->state,mi->state_changed_signal_pointer);
		}                                                                                           		
		what_changed |= MPD_CST_STATE;
		mi->CurrentState.state = mi->status->state;
	}

	if(mi->CurrentState.songid != mi->status->songid)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_status_update: Song has changed %i %i!", mi->OldState.songid, mi->status->songid);

		/* deprecated */
		if(mi->song_changed != NULL)
		{                                                                      		
			mi->song_changed(mi, mi->OldState.songid,mi->status->songid,mi->song_changed_signal_pointer);
		}
		what_changed |= MPD_CST_SONGID;
		/* save new songid */
		mi->CurrentState.songid = mi->status->songid;

	}


	/* deprecated */
	if(mi->status_changed != NULL)
	{                                                                      		
		mi->status_changed(mi, mi->status_changed_signal_pointer);		
	}


	if(mi->CurrentState.updatingDb != mi->status->updatingDb )
	{
		/* deprecated */
		if(mi->updating_changed != NULL)
		{
			mi->updating_changed(mi, mi->status->updatingDb,mi->updating_signal_pointer);
		}
		what_changed |= MPD_CST_UPDATING;
		if(!mi->status->updatingDb)
		{
			mpd_stats_update_real(mi, &what_changed);
		}
		mi->CurrentState.updatingDb = mi->status->updatingDb;
	}

	if(mi->the_status_changed_callback != NULL)
	{                                                                      		
		mi->the_status_changed_callback( mi, what_changed, mi->the_status_changed_signal_userdata );		
	}

	return FALSE;
}

/* returns TRUE when status is availible, when not availible and connected it tries to grab it */
int mpd_status_check(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_status_check: not connected\n");
		return FALSE;
	}
	if(mi->status == NULL)
	{
		/* try to update */
		if(mpd_status_update(mi))
		{
			debug_printf(DEBUG_INFO, "mpd_status_check: failed to update status\n");
			return FALSE;
		}
	}
	return TRUE;
}


int mpd_stats_get_total_songs(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_stats_get_total_songs: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_stats_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->stats->numberOfSongs;
}

int mpd_stats_get_total_artists(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_stats_get_total_artists: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_stats_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->stats->numberOfArtists;
}

int mpd_stats_get_total_albums(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_get_total_albums: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_stats_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_get_total_albums: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->stats->numberOfAlbums;
}


int mpd_stats_get_uptime(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_get_uptime: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_stats_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_get_uptime: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->stats->uptime;
}

int mpd_stats_get_playtime(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_stats_get_playtime: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_stats_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_get_playtime: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->stats->playTime;
}
int mpd_status_get_volume(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_status_get_volume: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_status_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING, "mpd_status_get_volume: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->status->volume;
}


int mpd_status_get_bitrate(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING,"mpd_status_get_bitrate: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_status_check(mi) || !mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->status->bitRate;
}
/* TODO: error checking might be nice? */
int mpd_status_db_is_updating(MpdObj *mi)
{
	return mi->CurrentState.updatingDb;
}


int mpd_status_get_total_song_time(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING, "mpd_status_get_total_song_time: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_WARNING, "mpd_status_get_total_song_time: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->status->totalTime;
}


int mpd_status_get_elapsed_song_time(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_get_elapsed_song_time: failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_get_elapsed_song_time: Failed to get status\n");
		return MPD_FAILED_STATUS;
	}
	return mi->status->elapsedTime;
}

int mpd_status_set_volume(MpdObj *mi,int volume)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_set_volume: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	/* making sure volume is between 0 and 100 */
	volume = (volume < 0)? 0:(volume>100)? 100:volume;

	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_set_volume: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	/* send the command */
	mpd_sendSetvolCommand(mi->connection , volume);
	mpd_finishCommand(mi->connection);
	/* check for errors */

	mpd_unlock_conn(mi);
	/* update status, because we changed it */
	mpd_status_queue_update(mi);
	/* return current volume */
	return mpd_status_get_volume(mi);
}

int mpd_status_get_crossfade(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_get_crossfade: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(!mpd_status_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_get_crossfade: Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mi->status->crossfade;
}

int mpd_status_set_crossfade(MpdObj *mi,int crossfade_time)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_set_crossfade: not connected\n");	
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_status_set_crossfade: lock failed\n");
		return MPD_LOCK_FAILED;
	}
	mpd_sendCrossfadeCommand(mi->connection, crossfade_time);
	mpd_finishCommand(mi->connection);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return FALSE;
}


float mpd_status_set_volume_as_float(MpdObj *mi, float fvol)
{
	int volume = mpd_status_set_volume(mi, (int)(fvol*100.0));
	if(volume > -1)
	{
		return (float)volume/100.0;
	}
	return (float)volume;
}

int mpd_stats_queue_update(MpdObj *mi)
{

	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_stats_queue_update: Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->stats != NULL)
	{                                  	
		mpd_freeStats(mi->stats);
		mi->stats = NULL;
	}
	return FALSE;
}

int mpd_stats_update(MpdObj *mi)
{
	mpd_stats_update_real(mi, NULL);
}

int mpd_stats_update_real(MpdObj *mi, ChangedStatusType* what_changed)
{
	ChangedStatusType what_changed_here = 0;
	if ( what_changed == NULL ) {
		/* we need to save the current state, because we're called standalone */
		memcpy(&(mi->OldState), &(mi->CurrentState), sizeof(MpdServerState));
	}
	
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_stats_update: Where not connected\n");
		return TRUE;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_set_volume: lock failed\n");
		return MPD_LOCK_FAILED;
	}

	if(mi->stats != NULL)
	{
		mpd_freeStats(mi->stats);
	}
	mpd_sendStatsCommand(mi->connection);
	mi->stats = mpd_getStats(mi->connection);
	if(mi->stats == NULL)
	{
		debug_printf(DEBUG_ERROR,"mpd_stats_update: Failed to grab stats from mpd\n");
	}
	else if(mi->stats->dbUpdateTime != mi->OldState.dbUpdateTime)
	{
		debug_printf(DEBUG_INFO, "mpd_stats_update: database updated\n");
		/* deprecated */
		if(mi->database_changed != NULL)
		{                                                                      		
			mi->database_changed(mi, mi->database_changed_signal_pointer);
		}
		what_changed_here |= MPD_CST_DATABASE;
		
		mi->CurrentState.dbUpdateTime = mi->stats->dbUpdateTime;
	}

  if (what_changed) {
    (*what_changed) |= what_changed_here;
  } else {
   	if(mi->the_status_changed_callback != NULL)
    {                                                                      		
      mi->the_status_changed_callback(mi, what_changed_here, mi->the_status_changed_signal_userdata);		
    }
  }

	if(mpd_unlock_conn(mi))
	{
		return TRUE;
	}
	return FALSE;
}


int mpd_stats_check(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_stats_check: not connected\n");
		return FALSE;
	}
	if(mi->stats == NULL)
	{
		/* try to update */
		if(mpd_stats_update(mi))
		{
			debug_printf(DEBUG_ERROR,"mpd_stats_check: failed to update status\n");
			return FALSE;
		}
	}
	return TRUE;
}
