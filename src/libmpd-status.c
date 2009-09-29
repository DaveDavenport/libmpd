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
#include <assert.h>
#define __USE_GNU

#include <string.h>
#include <stdarg.h>
#include <config.h>
#include "debug_printf.h"
#include "libmpd.h"
#include "libmpd-internal.h"

int mpd_stats_update_real(MpdObj *mi, ChangedStatusType* what_changed);

int mpd_status_queue_update(MpdObj *mi)
{

	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mi->status != NULL)
	{
		mpd_status_free(mi->status);
		mi->status = NULL;
	}
	return MPD_OK;
}


int mpd_status_update(MpdObj *mi)
{
	ChangedStatusType what_changed=0;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}



	if(mi->status != NULL)
	{
		mpd_status_free(mi->status);
		mi->status = NULL;
	}
	mi->status = mpd_run_status(mi->connection);
	if(mi->status == NULL)
	{
		debug_printf(DEBUG_ERROR,"Failed to grab status from mpd\n");
		mpd_unlock_conn(mi);
		return MPD_STATUS_FAILED;
	}
	if(mpd_unlock_conn(mi))
	{
		debug_printf(DEBUG_ERROR, "Failed to unlock");
		return MPD_LOCK_FAILED;
	}
	/*
	 * check for changes 
	 */
	/* first save the old status */
	memcpy(&(mi->OldState), &(mi->CurrentState), sizeof(MpdServerState));

	/* playlist change */
	if(mi->CurrentState.playlistid != mpd_status_get_queue_version(mi->status))
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "Playlist has changed!");

		/* We can't trust the current song anymore. so we remove it */
		/* tags might have been updated */
		if(mi->CurrentSong != NULL)
		{
			mpd_song_free(mi->CurrentSong);
			mi->CurrentSong = NULL;
		}

		/* set MPD_CST_PLAYLIST to be changed */
		what_changed |= MPD_CST_PLAYLIST;

		if(mi->CurrentState.playlistLength == mpd_status_get_queue_length(mi->status))
		{
			// what_changed |= MPD_CST_SONGID;
		}
		/* save new id */
		mi->CurrentState.playlistid = mpd_status_get_queue_version(mi->status);
	}

	/* state change */
	if(mi->CurrentState.state != mpd_status_get_state(mi->status))
	{
		what_changed |= MPD_CST_STATE;
		mi->CurrentState.state = mpd_status_get_state(mi->status);
	}

	if(mi->CurrentState.songid != mpd_status_get_song_id(mi->status))
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "Songid has changed %i %i!", mi->OldState.songid, mpd_status_get_song_id(mi->status));

		what_changed |= MPD_CST_SONGID;
		/* save new songid */
		mi->CurrentState.songid = mpd_status_get_song_id(mi->status);

	}
	if(mi->CurrentState.songpos != mpd_status_get_song_pos(mi->status))
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "Songpos has changed %i %i!", mi->OldState.songpos, mpd_status_get_song_pos(mi->status));

		what_changed |= MPD_CST_SONGPOS;
		/* save new songid */
		mi->CurrentState.songpos = mpd_status_get_song_pos(mi->status);

	}
	if(mi->CurrentState.nextsongid != mpd_status_get_next_song_id(mi->status) || mi->CurrentState.nextsongpos != mpd_status_get_next_song_pos(mi->status))
	{
		what_changed |= MPD_CST_NEXTSONG;
		/* save new songid */
		mi->CurrentState.nextsongpos = mpd_status_get_next_song_pos(mi->status);
		mi->CurrentState.nextsongid = mpd_status_get_next_song_id(mi->status);
	}

	if(mi->CurrentState.single != mpd_status_get_single(mi->status))
	{
		what_changed |= MPD_CST_SINGLE_MODE;
		mi->CurrentState.single = mpd_status_get_single(mi->status);
	}
	if(mi->CurrentState.consume != mpd_status_get_consume(mi->status))
	{
		what_changed |= MPD_CST_CONSUME_MODE;
		mi->CurrentState.consume = mpd_status_get_consume(mi->status);
	}
	if(mi->CurrentState.repeat != mpd_status_get_repeat(mi->status))
	{
		what_changed |= MPD_CST_REPEAT;
		mi->CurrentState.repeat = mpd_status_get_repeat(mi->status);
	}
	if(mi->CurrentState.random != mpd_status_get_random(mi->status))
	{
		what_changed |= MPD_CST_RANDOM;
		mi->CurrentState.random = mpd_status_get_random(mi->status);
	}
	if(mi->CurrentState.volume != mpd_status_get_volume(mi->status))
	{
		what_changed |= MPD_CST_VOLUME;
		mi->CurrentState.volume = mpd_status_get_volume(mi->status);
	}
	if(mi->CurrentState.xfade != mpd_status_get_crossfade(mi->status))
	{
		what_changed |= MPD_CST_CROSSFADE;
		mi->CurrentState.xfade = mpd_status_get_crossfade(mi->status);
	}
	if(mi->CurrentState.totaltime != mpd_status_get_total_time(mi->status))
	{
		what_changed |= MPD_CST_TOTAL_TIME;
		mi->CurrentState.totaltime = mpd_status_get_total_time(mi->status);
	}
	if(mi->CurrentState.elapsedtime != mpd_status_get_elapsed_time(mi->status))
	{
		what_changed |= MPD_CST_ELAPSED_TIME;
		mi->CurrentState.elapsedtime = mpd_status_get_elapsed_time(mi->status);
	}

	/* Check if bitrate changed, happens with vbr encodings. */
	if(mi->CurrentState.bitrate != mpd_status_get_kbit_rate(mi->status))
	{
		what_changed |= MPD_CST_BITRATE;
		mi->CurrentState.bitrate = mpd_status_get_kbit_rate(mi->status);
	}

	/* The following 3 probly only happen on a song change, or is it possible in one song/stream? */

	struct mpd_audio_format format = { .bits = MPD_SAMPLE_FORMAT_UNDEFINED };
	const struct mpd_audio_format *f = mpd_status_get_audio_format(mi->status);
	if (f != NULL)
		format = *f;

	/* Check if the sample rate changed */
	if(mi->CurrentState.samplerate != format.sample_rate)
	{
		what_changed |= MPD_CST_AUDIOFORMAT;
		mi->CurrentState.samplerate = format.sample_rate;
	}

	/* check if the sampling depth changed */
	if(mi->CurrentState.bits != format.bits)
	{
		what_changed |= MPD_CST_AUDIOFORMAT;
		mi->CurrentState.bits = format.bits;
	}

	/* Check if the amount of audio channels changed */
	if(mi->CurrentState.channels != format.channels)
	{
		what_changed |= MPD_CST_AUDIOFORMAT;
		mi->CurrentState.channels = format.channels;
	}

	if(mpd_status_get_error(mi->status) != NULL)
	{
		what_changed |= MPD_CST_SERVER_ERROR;
		strcpy(mi->CurrentState.error, mpd_status_get_error(mi->status));
		mpd_run_clearerror(mi->connection);
	}
	else
	{
		mi->CurrentState.error[0] ='\0';
	}

	/* Check if the updating changed,
	 * If it stopped, also update the stats for the new db-time.
	 */
	if(mi->CurrentState.updatingDb != mpd_status_get_update_id(mi->status))
	{
		what_changed |= MPD_CST_UPDATING;
		if(mpd_status_get_update_id(mi->status) == 0)
		{
			mpd_stats_update_real(mi, &what_changed);
		}
		mi->CurrentState.updatingDb = mpd_status_get_update_id(mi->status);
	}


	mi->CurrentState.playlistLength = mpd_status_get_queue_length(mi->status);


	/* Detect changed outputs */
	if(!mi->has_idle)
	{
		if(mi->num_outputs >0 )
		{
			struct mpd_output *output = NULL;
			mpd_send_outputs(mi->connection);
			while (( output = mpd_recv_output(mi->connection)) != NULL)
			{
				const unsigned id = mpd_output_get_id(output);
				if(mi->num_outputs < id)
				{
					mi->num_outputs++;
					mi->output_states = realloc(mi->output_states,mi->num_outputs*sizeof(int));
					mi->output_states[mi->num_outputs] = mpd_output_get_enabled(output);
					what_changed |= MPD_CST_OUTPUT;
				}
				if(mi->output_states[id] != mpd_output_get_enabled(output))
				{
					mi->output_states[id] = mpd_output_get_enabled(output);
					what_changed |= MPD_CST_OUTPUT;
				}
				mpd_output_free(output);
			}
			mpd_response_finish(mi->connection);
		}
		else
		{
			/* if no outputs, lets fetch them */
			mpd_server_update_outputs(mi);
			if(mi->num_outputs == 0)
			{
				assert("No outputs defined? that cannot be\n");
			}
			what_changed |= MPD_CST_OUTPUT;
		}
	}else {
		int update_stats = 0;
		mpd_send_idle(mi->connection);
		enum mpd_idle idle = mpd_run_noidle(mi->connection);

		if (idle & MPD_IDLE_OUTPUT)
			what_changed |= MPD_CST_OUTPUT;

		if (idle & MPD_IDLE_DATABASE) {
			if ((what_changed & MPD_CST_DATABASE) == 0)
			{
				update_stats = 1;
			}
			what_changed |= MPD_CST_DATABASE;
		}

		if (idle & MPD_IDLE_STORED_PLAYLIST)
			what_changed |= MPD_CST_STORED_PLAYLIST;

		if (idle & MPD_IDLE_PLAYLIST)
			what_changed |= MPD_CST_PLAYLIST;

		if (idle & MPD_IDLE_STICKER)
			what_changed |= MPD_CST_STICKER;

		/* This means repeat,random, replaygain or crossface changed */
		if (idle & MPD_IDLE_OPTIONS)
			what_changed |= MPD_CST_REPLAYGAIN;

		if(update_stats) {
			mpd_stats_update_real(mi, &what_changed);
		}
	}

	/* Run the callback */
	if((mi->the_status_changed_callback != NULL) && what_changed)
	{
		mi->the_status_changed_callback( mi, what_changed, mi->the_status_changed_signal_userdata );
	}

	/* We could have lost connection again during signal handling... so before we return check again if we are connected */
	if(!mpd_check_connected(mi))
	{
		return MPD_NOT_CONNECTED;
	}
	return MPD_OK;
}

/* returns TRUE when status is availible, when not availible and connected it tries to grab it */
int mpd_status_check(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mi->status == NULL)
	{
		/* try to update */
		if(mpd_status_update(mi))
		{
			debug_printf(DEBUG_INFO, "failed to update status\n");
			return MPD_STATUS_FAILED;
		}
	}
	return MPD_OK;
}


int mpd_stats_get_total_songs(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK) 
	{
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mpd_stats_get_number_of_songs(mi->stats);
}

int mpd_stats_get_total_artists(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_STATS_FAILED;
	}
	return mpd_stats_get_number_of_artists(mi->stats);
}

int mpd_stats_get_total_albums(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed to get status\n");
		return MPD_STATS_FAILED;
	}
	return mpd_stats_get_number_of_albums(mi->stats);
}


int libmpd_stats_get_uptime(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed to get status\n");
		return MPD_STATS_FAILED;
	}
	return mpd_stats_get_uptime(mi->stats);
}

int mpd_stats_get_playtime(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed to get status\n");
		return MPD_STATS_FAILED;
	}
	return mpd_stats_get_play_time(mi->stats);
}

int mpd_stats_get_db_playtime(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_stats_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed to get stats\n");
		return MPD_STATS_FAILED;
	}
	return mpd_stats_get_db_play_time(mi->stats);
}

















int libmpd_status_get_volume(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mpd_status_get_volume(mi->status);
}


int mpd_status_get_bitrate(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->CurrentState.bitrate;
}

int mpd_status_get_channels(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->CurrentState.channels;
}

unsigned int mpd_status_get_samplerate(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->CurrentState.samplerate;
}

int mpd_status_get_bits(MpdObj *mi)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_WARNING,"failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mi->CurrentState.bits;
}

char * mpd_status_get_mpd_error(MpdObj *mi)
{
    if(mi->CurrentState.error[0] != '\0')
    {
        return strdup(mi->CurrentState.error);
    }
    return NULL;
}

int mpd_status_db_is_updating(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING, "mpd_check_connected failed.\n");
		return FALSE;
	}
	return mi->CurrentState.updatingDb;
}


int mpd_status_get_total_song_time(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "failed to check mi == NULL\n");
		return MPD_ARGS_ERROR;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mpd_status_get_total_time(mi->status);
}


int mpd_status_get_elapsed_song_time(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"failed to check mi == NULL\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	return mpd_status_get_elapsed_time(mi->status);
}

int mpd_status_set_volume(MpdObj *mi,int volume)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	/* making sure volume is between 0 and 100 */
	volume = (volume < 0)? 0:(volume>100)? 100:volume;

	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	/* send the command */
	mpd_run_set_volume(mi->connection, volume);
	/* check for errors */

	mpd_unlock_conn(mi);
	/* update status, because we changed it */
	mpd_status_queue_update(mi);
	/* return current volume */
	return libmpd_status_get_volume(mi);
}

int libmpd_status_get_crossfade(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_status_check(mi) != MPD_OK)
	{
		debug_printf(DEBUG_WARNING,"Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mpd_status_get_crossfade(mi->status);
}

int mpd_status_set_crossfade(MpdObj *mi,int crossfade_time)
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
	mpd_run_crossfade(mi->connection, crossfade_time);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return MPD_OK;
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

int mpd_stats_update(MpdObj *mi)
{
	return mpd_stats_update_real(mi, NULL);
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
		debug_printf(DEBUG_INFO,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"lock failed\n");
		return MPD_LOCK_FAILED;
	}

	if(mi->stats != NULL)
	{
		mpd_stats_free(mi->stats);
	}
	mi->stats = mpd_run_stats(mi->connection);
	if(mi->stats == NULL)
	{
		debug_printf(DEBUG_ERROR,"Failed to grab stats from mpd\n");
	}
	else if(mpd_stats_get_db_update_time(mi->stats) != mi->OldState.dbUpdateTime)
	{
		debug_printf(DEBUG_INFO, "database updated\n");
		what_changed_here |= MPD_CST_DATABASE;

		mi->CurrentState.dbUpdateTime = mpd_stats_get_db_update_time(mi->stats);
	}

	if (what_changed) {
		(*what_changed) |= what_changed_here;
	} else {
		if((mi->the_status_changed_callback != NULL) & what_changed_here)
		{
			mi->the_status_changed_callback(mi, what_changed_here, mi->the_status_changed_signal_userdata);
		}
	}

	if(mpd_unlock_conn(mi))
	{
		debug_printf(DEBUG_ERROR, "unlock failed");
		return MPD_LOCK_FAILED;
	}
	return MPD_OK;
}


int mpd_stats_check(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(mi->stats == NULL)
	{
		/* try to update */
		if(mpd_stats_update(mi))
		{
			debug_printf(DEBUG_ERROR,"failed to update status\n");
			return MPD_STATUS_FAILED;
		}
	}
	return MPD_OK;
}
