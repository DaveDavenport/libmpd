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

#ifndef __MPD_LIB_DATABASE__
#define __MPD_LIB_DATABASE__
/**
 * mpd_database_get_albums
 * @mi: A #MpdObj
 * @artist: an artist name
 *
 * Grab's a list of albums of a certain artist from mpd.
 * if artist is %NULL it grabs all albums
 *
 * returns: A #MpdData list.
 */
MpdData *	mpd_database_get_albums		(MpdObj *mi, char *artist);

/**
 * mpd_database_get_artists
 * @mi: a #MpdObj
 *
 * returns a list of all availible artists.
 *
 * returns: a #MpdData list
 */
MpdData *	mpd_database_get_artists		(MpdObj *mi);


/**
 * mpd_database_get_complete
 * @mi: a #MpdObj
 *
 * Get's the complete datababse, only returns songs
 *
 * returns: a #MpdData list with songs
 */

MpdData * mpd_database_get_complete(MpdObj *mi);


/**
 * mpd_atabase_update_dir
 *@mi: A #MpdObj
 *@path: The path mpd should update.
 *
 * Force mpd to update (parts of )the database.
 *
 */
void		mpd_database_update_dir		(MpdObj *mi, char *path);

/* client side search function with best "match" option..
 * It splits the search string into tokens. (on the ' ')  every token is then matched using regex.
 * So f.e. Murder Hooker|Davis  matches songs where title/filename/artist/album contains murder and hooker or murder davis in any order.
 * Warning: This function can be slow.
 */
MpdData *	mpd_database_token_find		(MpdObj *mi , char *string);










#endif
