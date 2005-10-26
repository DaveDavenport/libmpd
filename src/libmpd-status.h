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


#ifndef __MPD_LIB_STATUS__
#define __MPD_LIB_STATUS__

/* 
 * status commands 
 */
/* To get the function to have the  most recent info you want to call mpd_status_queue_update 
 * In a gui app. you want to call this every 0.x seconds. 
 * mpd_status_queue_update only queue's an update
 * Only when a function is called that needs status, it's fetched from mpd.
 */
int 		mpd_status_check			(MpdObj *mi);
int 		mpd_status_queue_update			(MpdObj *mi);
int 		mpd_status_update			(MpdObj *mi);
float 		mpd_status_set_volume_as_float		(MpdObj *mi, float fvol);
int 		mpd_status_set_volume			(MpdObj *mi,int volume);
int 		mpd_status_get_volume			(MpdObj *mi);
int 		mpd_status_get_bitrate			(MpdObj *mi);
int		mpd_status_get_total_song_time		(MpdObj *mi);
int		mpd_status_get_elapsed_song_time	(MpdObj *mi);
int		mpd_status_get_crossfade		(MpdObj *mi);
int		mpd_status_set_crossfade		(MpdObj *mi, int crossfade_time);
int		mpd_stats_update			(MpdObj *mi);

int		mpd_stats_get_total_songs		(MpdObj *mi);
int		mpd_stats_get_total_artists		(MpdObj *mi);
int		mpd_stats_get_total_albums		(MpdObj *mi);
int		mpd_stats_get_uptime			(MpdObj *mi);
int		mpd_stats_get_playtime			(MpdObj *mi);

int 		mpd_status_db_is_updating		(MpdObj *mi);


#endif
