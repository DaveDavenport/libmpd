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

#ifndef __MPD_LIB_PLAYLIST__
#define __MPD_LIB_PLAYLIST__
/**
 * mpd_playlist_get_playlist_id
 * @mi: a #MpdObj
 *
 * Returns the id of the current playlist
 *
 * returns: a long long
 */
long long	mpd_playlist_get_playlist_id		(MpdObj *mi);

/**
 * mpd_playlist_get_old_playlist_id
 * @mi: a #MpdObj
 *
 * Returns the id of the previous playlist
 *
 * returns: a long long
 */
long long	mpd_playlist_get_old_playlist_id         (MpdObj *mi);

/**
 * mpd_playlist_get_song
 * @mi: a #MpdObj
 * @songid: a SongId
 *
 * returns the mpd_Song for playlist entry with songid.
 *
 * returns: a #mpd_Song
 */
mpd_Song *	mpd_playlist_get_song			(MpdObj *mi, int songid);

/**
 * mpd_playlist_get_current_song
 * @mi: a #MpdObj
 *
 * returns the mpd_Song for the currently playing song
 *
 * returns: a #mpd_Song
 */
mpd_Song *	mpd_playlist_get_current_song		(MpdObj *mi);
/**
 * mpd_playlist_clear
 * @mi: a #MpdObj
 *
 * Clears the playlist
 *
 * returns: 
 */
int		mpd_playlist_clear			(MpdObj *mi);
/**
 * mpd_playlist_shuffle
 * @mi: a #MpdObj
 *
 * Shuffle's the order of the playlist, this is different then playing random
 *
 * returns: 
 */
int		mpd_playlist_shuffle			(MpdObj *mi);
/**
 * mpd_playlist_save
 * @mi: a #MpdObj
 * @name: The name of the playlist
 *
 * Saves the current playlist to a file.
 *
 * returns: 0 if succesfull, #MPD_PLAYLIST_EXIST when the playlist allready exists, <0 when failed.
 */ 
int		mpd_playlist_save			(MpdObj *mi, char *name);
/**
 * mp_playlist_move_pos
 * @mi: a #MpdObj
 * @old_pos: The current position in the playlist
 * @new_pos: The new position in the playlist.
 *
 * Moves a song in the playlist. This uses the position of the song, not the id
 *
 */
void		mpd_playlist_move_pos		(MpdObj *mi, int old_pos, int new_pos);
/**
 * mpd_playlist_get_changes
 * @mi: a #MpdObj
 * @old_playlist_id: The id of the old playlist you want to get the changes with.
 *
 * Get's a list of the song that changed between the current and the old playlist
 *
 * returns: a #MpdData list
 */
MpdData *	mpd_playlist_get_changes		(MpdObj *mi,int old_playlist_id);

/**
 * mpd_playlist_get_playlist_length
 * @mi:	a #MpdObj
 *
 * returns: The number of songs in the current playlist.
 */
int		mpd_playlist_get_playlist_length	(MpdObj *mi);

/**
 * mpd_playlist_add
 * @mi: a #MpdObj
 * @path: the path of the song to be added.
 *
 * Add's a song to the playlist, use #mpd_playlist_queue_add to add multiple songs.
 */
void		mpd_playlist_add			(MpdObj *mi, char *path);
/**
 * mpd_playlist_delete
 * @mi: a #MpdObj
 * @path: the path of the playlist to be deleted.
 *
 * Deletes a playlist.
 *
 * returns: 
 */
int		mpd_playlist_delete			(MpdObj *mi,char *path);

/* mpd svn only functions 0.12.0 */
/* TODO: rewrite this */
MpdData *	mpd_playlist_get_unique_tags		(MpdObj *mi, int table,...);
MpdData *	mpd_playlist_find_adv		(MpdObj *mi,int exact, ...);

/* queing stuff */
/**
 * mpd_playlist_queue_add
 * @mi: a #MpdObj
 * @path: The path to a song to add
 *
 * This queue's an add command. The actuall add isn't done until #mpd_playlist_queue_commit is called
 *
 */
void	mpd_playlist_queue_add		(MpdObj *mi,char *path);
/**
 * mpd_playlist_queue_load
 * @mi: a #MpdObj
 * @path: The path to a playlist to load
 *
 * This queue's an load command. The actuall load isn't done until #mpd_playlist_queue_commit is called
 *
 */
void	mpd_playlist_queue_load		(MpdObj *mi,char *path);
/**
 * mpd_playlist_queue_delete_id
 * @mi: a #MpdObj
 * @id: The songid of the song you want to delete
 *
 * This queue's an delete song from playlist command. The actually delete isn't done until #mpd_playlist_queue_commit is called
 *
 */
void	mpd_playlist_queue_delete_id		(MpdObj *mi,int id);

/**
 * mpd_playlist_queue_commit
 * @mi: a #MpdObj
 * 
 * Commits the queue'd commands in a command list. This is an efficient way of doing alot of add's/removes.
 */
void	mpd_playlist_queue_commit		(MpdObj *mi);


/* database functions */

/* for backward compatibility */
void 		mpd_playlist_update_dir		(MpdObj *mi, char *path) __attribute__((deprecated));
MpdData *	mpd_playlist_get_albums		(MpdObj *mi, char *artist) __attribute__((deprecated));
MpdData *	mpd_playlist_get_artists	(MpdObj *mi) __attribute__((deprecated));
MpdData *	mpd_playlist_get_directory	(MpdObj *mi,char *path);
MpdData *	mpd_playlist_find		(MpdObj *mi, int table, char *string, int exact);
MpdData *	mpd_playlist_token_find		(MpdObj *mi, char *string) __attribute__((deprecated));

#endif
