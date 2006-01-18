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
/**
 * Get different state's of mpd. 
 * There is a signal (#StateChangedCallback) that indicates changed status.
 * To get this signal triggered correctly run mpd_status_update every 0.x seconds from the main loop of the program
 */



/**
 * mpd_status_check
 * @mi: a #MpdObj
 *
 * Checks if there is status information is availibe, if not availible it tries to fetch it.
 * This function is called from within libmpd, and shouldn't be called from the program.
 *
 * returns: 0 when successful
 */
int 		mpd_status_check			(MpdObj *mi);
/**
 * mpd_status_queue_update
 * @mi: a #MpdObj
 *
 * Marks the current status invalid, the next time status is needed it will be fetched from mpd.
 *
 * returns: 0 when successful
 */
int 		mpd_status_queue_update			(MpdObj *mi);
/**
 * mpd_status_update
 * @mi: a #MpdObj
 *
 * Updates the status field from mpd.
 * Call this function ever 0.x seconds from the program's main-loop to recieve signals when mpd's status has changed.
 *
 * returns: 0 when succesfull
 */
int 		mpd_status_update			(MpdObj *mi);
/**
 * mpd_status_set_volume_as_float
 * @mi: a #MpdObj
 * @fvol: an float between 0.0 and 1.0
 *
 * Set the output volume
 * returns: the new volume or a value below 0 when failed.
 */
float 		mpd_status_set_volume_as_float		(MpdObj *mi, float fvol);

/**
 * mpd_status_set_volume
 * @mi: a #MpdObj
 * @volume: a value between 0 and 100.
 *
 * Set the output volume
 *
 * returns: the new volume or < 0 when failed.
 */
int 		mpd_status_set_volume			(MpdObj *mi,int volume);
/**
 * mpd_status_get_volume:
 * @mi: a #MpdObj
 *
 * Set the audio output volume.
 *
 * returns: the volume between 0 and 100 or < 0 when failed
 */
int 		mpd_status_get_volume			(MpdObj *mi);
/**
 * mpd_status_get_bitrate
 * @mi: a #MpdObj
 *
 * get the bitrate of the current playing song in kbs. This is a constantly updating value. (for vbr songs)
 *
 * returns: bitrate in kbs
 */
int 		mpd_status_get_bitrate			(MpdObj *mi);
/**
 * mpd_status_get_samplerate
 * @mi: a #MpdObj
 *
 * get the samplerate of the current playing song in bps. 
 *
 * returns: samplerate in bps
 */
unsigned int 	mpd_status_get_samplerate			(MpdObj *mi);
/**
 * mpd_status_get_channels
 * @mi: a #MpdObj
 *
 * get the number of channels in the current playing song. This is usually only 1(mono) or 2(stereo), but this might change in the future.
 *
 * returns: number of channels
 */
int 		mpd_status_get_channels			(MpdObj *mi);
/**
 * mpd_status_get_bits
 * @mi: a #MpdObj
 *
 * get the number of bits per sample of the current playing song. 
 *
 * returns: bits per sample 
 */
int 		mpd_status_get_bits			(MpdObj *mi);

/**
 * mpd_status_get_total_song_time
 * @mi: a #MpdObj
 *
 * get the total length of the currently playing song.
 *
 * returns: time in seconds or <0 when failed.
 */
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
/**
 * mpd_status_db_is_updating
 * @mi: a #MpdObj
 *
 * Checks if mpd is updating it's music db.
 * 
 * returns: TRUE if mpd is still updating, FALSE if not.
 */
int 		mpd_status_db_is_updating		(MpdObj *mi);


#endif
