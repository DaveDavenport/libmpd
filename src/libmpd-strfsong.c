/* libmpd (high level libmpdclient library)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * Based on mpc's songToFormatedString modified for glib and ncmpc
 * (c) 2003-2004 by normalperson and Warren Dukes (shank@mercury.chem.pitt.edu)
 *              and Daniel Brown (danb@cs.utexas.edu)
 *              and Kalle Wallin (kaw@linux.se)
 *              and Qball Cow (Qball@qballcow.nl)
 
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
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include "libmpd.h"
#include "libmpd-internal.h"

static char * skip(char * p) 
{
	int stack = 0;

	while (*p != '\0') {
		if(*p == '[') stack++;
		if(*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		}
		else if(stack) {
			if(*p == ']') stack--;
		}
		else {
			if(*p == '&' || *p == '|' || *p == ']') {
				break;
			}
		}
		++p;
	}

	return p;
}

static unsigned int _strfsong(char *s, 
		unsigned int max, 
		const char *format, 
		struct mpd_song *song, 
		char **last)
{
	char *p, *end;
	char *temp;
	unsigned int n, length = 0;
	int i;
	short int found = FALSE;

	memset(s, 0, max);
	if( song==NULL )
		return 0;

	for( p=(char *) format; *p != '\0' && length<max; )
	{
		/* OR */
		if (p[0] == '|') 
		{
			++p;
			if(!found) 
			{
				memset(s, 0, max);
				length = 0;
			}
			else 
			{
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') 
		{
			++p;
			if(!found) 
			{
				p = skip(p);
			}
			else 
			{
				found = FALSE;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[')
		{
			temp = g_malloc0(max);
			if( _strfsong(temp, max, p+1, song, &p) >0 )
			{
				strncat(s, temp, max-length);
				length = strlen(s);
				found = TRUE;
			}
			g_free(temp);
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']')
		{
			if(last) *last = p+1;
			if(!found && length) 
			{
				memset(s, 0, max);
				length = 0;
			}
			return length;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '#' && p[0] != '%' && length<max)
		{
			strncat(s, p, 1);
			length++;
			++p;
			continue;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0' && length<max)
		{
			strncat(s, p+1, 1);
			length++;
			p+=2;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		temp = NULL;
		end  = p+1;
		while(*end >= 'a' && *end <= 'z')
		{
			end++;
		}
		n = end - p + 1;
		if(*end != '%')
			n--;
		else if (memcmp("%file%", p, n) == 0)
			temp = g_strdup(mpd_song_get_uri(song));
		else if (memcmp("%artist%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		else if (memcmp("%title%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
		else if (memcmp("%album%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		else if (memcmp("%track%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
		else if (memcmp("%name%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_NAME, 0));
		else if (memcmp("%date%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_DATE, 0));
		else if (memcmp("%genre%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		else if (memcmp("%performer%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0));
		else if (memcmp("%composer%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0));
		else if (memcmp("%comment%", p, n) == 0)
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_COMMENT, 0));
		else if (memcmp("%plpos%", p, n) == 0 || memcmp("%songpos%",p,n) == 0){
			temp = NULL;
			if(mpd_song_get_pos(song) >= 0){
				char str[32];
				int length;
				if((length = snprintf(str,32, "%i", mpd_song_get_pos(song))) >=0)
				{
					temp = g_strndup(str,length);
				}
			}
		}
		else if (memcmp("%shortfile%", p, n) == 0)
		{
			/*if( strstr(song->file, "://") )
			  temp = g_strdup(song->file);
			  else */{
				const char *uri = mpd_song_get_uri(song);
				int i=strlen(uri);
				int ext =i;
				int found = 0;
				char *temp2 = NULL;
				for(;i>=0 && (uri[i] != '/' && uri[i] != '\\');i--){
					if(uri[i] == '.' && !found) {
						ext = i;
						found = 1;
					}
				}
				temp2 = g_strndup(&(uri)[i+1],(gsize)(ext-i-1));
				temp = g_uri_unescape_string(temp2, "");
				g_free(temp2);
			}
		}
		else if (memcmp("%time%", p, n) == 0)
		{
			temp = NULL;
			const unsigned duration = mpd_song_get_duration(song);
			if (duration > 0) {
				char str[32];
				int length;
				if((length = snprintf(str,32, "%02d:%02d", duration/60, duration%60))>=0)
				{
					temp = g_strndup(str,length);
				}
			}
		}
		else if (memcmp("%disc%", p, n) == 0)
		{
			temp = g_strdup(mpd_song_get_tag(song, MPD_TAG_DISC, 0));
		}
		if(temp != NULL) {
			unsigned int templen = strlen(temp);
			found = TRUE;
			if( length+templen > max )
				templen = max-length;
			strncat(s, temp, templen);
			length+=templen;
			g_free(temp);
		}

		/* advance past the specifier */
		p += n;
	}

	for(i=0; i < length;i++)
	{
		if(s[i] == '_') s[i] = ' ';
	}

	if(last) *last = p;

	return length;
}

unsigned int mpd_song_markup(char *s, unsigned int max,const char *format, struct mpd_song *song)
{
    return _strfsong(s, max, format, song, NULL);
}

