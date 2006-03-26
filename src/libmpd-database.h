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

/** \defgroup 80database Database
 */
/*@{*/



/**
 * @param mi A #MpdObj
 * @param artist an artist name
 *
 * Grab's a list of albums of a certain artist from mpd.
 * if artist is %NULL it grabs all albums
 *
 * @returns A #MpdData list.
 */
MpdData *	mpd_database_get_albums		(MpdObj *mi, char *artist);


/**
 * @param mi a #MpdObj
 *
 * returns a list of all availible artists.
 *
 * @returns a #MpdData list
 */
MpdData *	mpd_database_get_artists		(MpdObj *mi);


/**
 * @param mi a #MpdObj
 *
 * Get's the complete datababse, only returns songs
 *
 * @returns a #MpdData list with songs
 */

MpdData * mpd_database_get_complete(MpdObj *mi);


/**
 *@param mi A #MpdObj
 *@param path The path mpd should update.
 *
 * Force mpd to update (parts of )the database.
 *
 * @returns a #MpdError
 */
int 	mpd_database_update_dir		(MpdObj *mi, char *path);



/**
 * @param mi A #MpdObj
 * @param string The search string
 *
 * client side search function with best "match" option..
 * It splits the search string into tokens. (on the ' ')  every token is then matched using regex.
 * It not tokenize between [()].
 *
 * So f.e. Murder Hooker|Davis  matches songs where title/filename/artist/album contains murder and hooker or murder and davis in any order.
 *
 * Warning: This function can be slow.
 *
 * @returns a #MpdData list
 */
MpdData *	mpd_database_token_find		(MpdObj *mi , char *string);



/**
 * @param mi A #MpdObj
 * @param path path of the playlist
 *
 * Deletes a playlist.
 * @returns
 */

int mpd_database_delete_playlist(MpdObj *mi,char *path);



/**
 * @param mi a #MpdObj
 * @param name The name of the playlist
 *
 * Saves the current playlist to a file.
 *
 * @returns a #MpdError. #MPD_OK if succesfull,
 * #MPD_DATABASE_PLAYLIST_EXIST when the playlist allready exists.
 */ 
int		mpd_database_save_playlist			(MpdObj *mi, char *name);


/**
 * @param mi a #MpdObj
 * @param field The table field
 * @returns a #MpdData
 */
MpdData *	mpd_database_get_unique_tags		(MpdObj *mi, int field,...);
/**
 * @param mi a #MpdObj
 * @param exact 1 for exact search 0 for fuzzy
 * @returns a #MpdData
 */
MpdData *	mpd_database_find_adv		(MpdObj *mi,int exact, ...);

/**
 * @param mi a #MpdObj
 * @param table table
 * @param string string to search for
 * @param exact if #TRUE only return exact matches
 *
 * @returns a #MpdData list
 */
MpdData * mpd_database_find(MpdObj *mi, int table, char *string, int exact);

MpdData * mpd_database_get_directory(MpdObj *mi,char *path);


/**
 * @param mi a #MpdObj
 * @param playlist the playlist you need the content off.
 *
 * Only works with patched mpd.
 * Check for %mpd_server_command_allowed(mi, "listPlaylistInfo");
 *
 * @returns a #MpdData list
 */
MpdData *mpd_database_get_playlist_content(MpdObj *mi,char *playlist);
/*@}*/
#endif
