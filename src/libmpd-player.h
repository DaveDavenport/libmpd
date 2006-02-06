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

#ifndef __MPD_LIB_PLAYER__
#define __MPD_LIB_PLAYER__
typedef enum {
	MPD_PLAYER_PAUSE = MPD_STATUS_STATE_PAUSE,
	MPD_PLAYER_PLAY =  MPD_STATUS_STATE_PLAY,
	MPD_PLAYER_STOP =  MPD_STATUS_STATE_STOP,
	MPD_PLAYER_UNKNOWN = MPD_STATUS_STATE_UNKNOWN
} MpdState;

/* Player commands */
/**
 * mpd_player_play
 * @mi: a #MpdObj
 *
 * Sends mpd the play command.
 *
 * returns: 0 when successful
 */
int mpd_player_play(MpdObj * mi);
/**
 * mpd_player_play_id
 * @mi: a #MpdObj
 * @id: a songid.
 *
 * Plays the song with id
 *
 * returns: 0 when successfull
 */
int mpd_player_play_id(MpdObj * mi, int id);
/**
 * mpd_player_stop
 * @mi: a #MpdObj
 *
 * Sends mpd the stop command.
 *
 * returns: 0 when successful
 */
int mpd_player_stop(MpdObj * mi);
/**
 * mpd_player_next
 * @mi: a #MpdObj
 *
 * Sends mpd the next command.
 *
 * returns: 0 when successful
 */

int mpd_player_next(MpdObj * mi);
/**
 * mpd_player_prev
 * @mi: a #MpdObj
 *
 * Sends mpd the prev command.
 *
 * returns: 0 when successful
 */

int mpd_player_prev(MpdObj * mi);
/**
 * mpd_player_pause
 * @mi: a #MpdObj
 *
 * Sends mpd the pause command.
 *
 * returns: 0 when successful
 */

int mpd_player_pause(MpdObj * mi);
/**
 * mpd_player_get_state
 * @mi: a #MpdObj
 *
 * Returns the mpd play state (play/paused/stop)
 *
 * returns: a #MpdState
 */
int mpd_player_get_state(MpdObj * mi);
/**
 * mpd_player_get_current_song_id
 * @mi: a #MpdObj
 *
 * Returns the id of the currently playing song
 *
 * returns: the songid of the playing song
 */
int mpd_player_get_current_song_id(MpdObj * mi);
/**
 * mpd_player_get_current_song_pos
 * @mi: a #MpdObj
 *
 * Returns the position of the currently playing song in the playlist
 *
 * returns: the position of the playing song
 */
int mpd_player_get_current_song_pos(MpdObj * mi);
/**
 * mpd_player_get_repeat
 * @mi: a #MpdObj
 *
 * Get the state of repeat: 1 if enabled, 0 when disabled.
 *
 * returns: the state of repeat
 */
int mpd_player_get_repeat(MpdObj * mi);
/**
 * mpd_player_set_repeat
 * @mi: a #MpdObj
 * @repeat: New state of repeat (1 is enabled, 0 is disabled)
 *
 * Enable/disabled repeat
 *
 * returns: 0 when succesfull
 */
int mpd_player_set_repeat(MpdObj * mi, int repeat);
/**
 * mpd_player_get_random
 * @mi: a #MpdObj
 *
 * Get the state of random: 1 if enabled, 0 when disabled.
 *
 * returns: the state of random
 */

int mpd_player_get_random(MpdObj * mi);
/**
 * mpd_player_set_random
 * @mi: a #MpdObj
 * @random: New state of random (1 is enabled, 0 is disabled)
 *
 * Enable/disabled random
 *
 * returns: 0 when succesfull
 */
int mpd_player_set_random(MpdObj * mi, int random);
/**
 * mpd_player_seek
 * @mi: a #MpdObj
 * @sec: Position to seek to. (in seconds)
 *
 * Seek through the current song.
 * returns: 0 when successful
 */
int mpd_player_seek(MpdObj * mi, int sec);

#endif
