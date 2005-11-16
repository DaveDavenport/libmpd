#ifndef __STRFSONG_H__
#define __STRFSONG_H__

/**
 * mpd_song_markup
 * @s:		A buffer to write the string in
 * @max:	The max length of the buffer
 * @format:	The markup string
 * @song:	A #MpdSong
 *
 * printfs a formatted string of a MpdSong
 * 
 * returns: The length of the new formatted string
 */

unsigned int mpd_song_markup(char *s, unsigned int max, const char *format, mpd_Song *song);

#endif
