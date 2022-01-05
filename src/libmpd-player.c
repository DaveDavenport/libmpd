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

enum mpd_state mpd_player_get_state(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_STATE_UNKNOWN;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed to get status\n");
		return MPD_STATE_UNKNOWN;
	}
	return mpd_status_get_state(mi->status);
}
int mpd_player_get_next_song_id(MpdObj *mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_ERROR, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	/* check if in valid state */
	if (mpd_player_get_state(mi) != MPD_STATE_PLAY &&
			mpd_player_get_state(mi) != MPD_STATE_PAUSE) {
		return MPD_PLAYER_NOT_PLAYING;
	}
	/* just to be sure check */
	if (!mpd_status_get_queue_length(mi->status)) {
		return MPD_PLAYLIST_EMPTY;
	}
	return mpd_status_get_next_song_id(mi->status);
}
int mpd_player_get_next_song_pos(MpdObj *mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_ERROR, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	/* check if in valid state */
	if (mpd_player_get_state(mi) != MPD_STATE_PLAY &&
	    mpd_player_get_state(mi) != MPD_STATE_PAUSE) {
		return MPD_PLAYER_NOT_PLAYING;
	}
	/* just to be sure check */
	if (!mpd_status_get_queue_length(mi->status)) {
		return MPD_PLAYLIST_EMPTY;
	}
	return mpd_status_get_next_song_pos(mi->status);
}
int mpd_player_get_current_song_id(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_ERROR, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	/* check if in valid state */
	if (mpd_player_get_state(mi) != MPD_STATE_PLAY &&
			mpd_player_get_state(mi) != MPD_STATE_PAUSE) {
		return MPD_PLAYER_NOT_PLAYING;
	}
	/* just to be sure check */
	if (!mpd_status_get_queue_length(mi->status)) {
		return MPD_PLAYLIST_EMPTY;
	}
	return mpd_status_get_song_id(mi->status);
}

int mpd_player_get_current_song_pos(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi)!= MPD_OK) {
		debug_printf(DEBUG_ERROR, "Failed to get status\n");
		return MPD_STATUS_FAILED;
	}
	/* check if in valid state */
	if (mpd_player_get_state(mi) != MPD_STATE_PLAY &&
			mpd_player_get_state(mi) != MPD_STATE_PAUSE) {
		return MPD_PLAYER_NOT_PLAYING;
	}
	/* just to be sure check */
	if (!mpd_status_get_queue_length(mi->status)) {
		return MPD_PLAYLIST_EMPTY;
	}
	return mpd_status_get_song_pos(mi->status);
}

int mpd_player_play_id(MpdObj * mi, int id)
{
	debug_printf(DEBUG_INFO, "trying to play id: %i\n", id);
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_play_id(mi->connection, id);

	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}

int mpd_player_play(MpdObj * mi)
{
	return mpd_player_play_id(mi, -1);
}

int mpd_player_stop(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_stop(mi->connection);

	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}

int mpd_player_next(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_next(mi->connection);

	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}

int mpd_player_prev(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	mpd_run_previous(mi->connection);

	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}


int mpd_player_pause(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	if (mpd_player_get_state(mi) == MPD_STATE_PAUSE) {
		mpd_run_pause(mi->connection, false);
	} else if (mpd_player_get_state(mi) == MPD_STATE_PLAY) {
		mpd_run_pause(mi->connection, true);
	}


	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}

int mpd_player_seek(MpdObj * mi, int sec)
{
	int cur_song = mpd_player_get_current_song_pos(mi);
	if (cur_song < 0) {
		debug_printf(DEBUG_ERROR, "mpd_player_get_current_song_pos returned error\n");
		return cur_song;
	}
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}

	debug_printf(DEBUG_INFO, "seeking in song %i to %i sec\n", cur_song, sec);

	mpd_run_seek_pos(mi->connection, cur_song, sec);

	mpd_unlock_conn(mi);
	if (mpd_status_update(mi)) {
		return MPD_STATUS_FAILED;
	}
	return MPD_OK;
}

int mpd_player_get_consume(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mpd_status_get_consume(mi->status);
}
int mpd_player_set_single(MpdObj * mi, int single)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}
	mpd_run_single(mi->connection, single);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return MPD_OK;
}
int mpd_player_get_single(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mpd_status_get_single(mi->status);
}
int mpd_player_set_consume(MpdObj * mi, int consume)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}
	mpd_run_consume(mi->connection, consume);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return MPD_OK;
}


int mpd_player_get_repeat(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mpd_status_get_repeat(mi->status);
}


int mpd_player_set_repeat(MpdObj * mi, int repeat)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}
	mpd_run_repeat(mi->connection, repeat);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return MPD_OK;
}



int mpd_player_get_random(MpdObj * mi)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_status_check(mi) != MPD_OK) {
		debug_printf(DEBUG_WARNING, "Failed grabbing status\n");
		return MPD_NOT_CONNECTED;
	}
	return mpd_status_get_random(mi->status);
}


int mpd_player_set_random(MpdObj * mi, int random)
{
	if (!mpd_check_connected(mi)) {
		debug_printf(DEBUG_WARNING, "not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if (mpd_lock_conn(mi)) {
		debug_printf(DEBUG_WARNING, "lock failed\n");
		return MPD_LOCK_FAILED;
	}
	mpd_run_random(mi->connection, random);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return MPD_OK;
}
