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
long long	mpd_playlist_get_playlist_id		(MpdObj *mi);
long long      mpd_playlist_get_old_playlist_id         (MpdObj *mi);
mpd_Song * 	mpd_playlist_get_song			(MpdObj *mi, int songid);
mpd_Song * 	mpd_playlist_get_current_song		(MpdObj *mi);
int 		mpd_playlist_clear			(MpdObj *mi);
int 		mpd_playlist_shuffle			(MpdObj *mi);
int 		mpd_playlist_save			(MpdObj *mi, char *name);
void 		mpd_playlist_update_dir		(MpdObj *mi, char *path);
void 		mpd_playlist_move_pos		(MpdObj *mi, int old_pos, int new_pos);
MpdData * 	mpd_playlist_get_artists		(MpdObj *mi);
MpdData *	mpd_playlist_get_albums		(MpdObj *mi, char *artist);
MpdData * 	mpd_playlist_get_directory		(MpdObj *mi,char *path);
MpdData * 	mpd_playlist_find			(MpdObj *mi, int table, char *string, int exact);

MpdData * 	mpd_playlist_get_changes		(MpdObj *mi,int old_playlist_id);
int		mpd_playlist_get_playlist_length	(MpdObj *mi);
void		mpd_playlist_add			(MpdObj *mi, char *path);
int 		mpd_playlist_delete			(MpdObj *mi,char *path);

/* mpd svn only functions 0.12.0 */
MpdData * 	mpd_playlist_get_unique_tags		(MpdObj *mi, int table,...);
MpdData *	mpd_playlist_find_adv		(MpdObj *mi,int exact, ...);
/* client side search function with best "match" option..
 * It splits the search string into tokens. (on the ' ')  every token is then matched using regex.
 * So f.e. Murder Hooker|Davis  matches songs where title/filename/artist/album contains murder and hooker or murder davis in any order.
 * Warning: This function can be slow.
 */
MpdData *	mpd_playlist_token_find		(MpdObj *mi , char *string);

/* mpd ob data next will return NULL when there are no more items. it will also call free when called on the last item. */
/* if you don't want this check with mpd_data_is_last before calling get_next 
 * this allows you to make this construction: 
 *	MpdData * mpd_playlist_get_artists(..);
 * 	while(data != NULL)
 * 	{
 *
 *
 *		data = mpd_data_next(data);
 * 	}
 */
 /* withouth leaking memory  */


/* queing stuff */
void 		mpd_playlist_queue_add		(MpdObj *mi,char *path);
void 		mpd_playlist_queue_load		(MpdObj *mi,char *path);
void 		mpd_playlist_queue_delete_id		(MpdObj *mi,int id);
/* use these to commit the changes */
void 		mpd_playlist_queue_commit		(MpdObj *mi);



/**
 * mpd_database_get_complete
 * @mi: a #MpdObj
 *
 * Get's the complete datababse, only returns songs
 *
 * returns: a #MpdData list with songs
 */

MpdData * mpd_database_get_complete(MpdObj *mi);












#endif
