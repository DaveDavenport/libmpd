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

#endif
