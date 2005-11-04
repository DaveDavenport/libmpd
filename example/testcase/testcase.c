#include <libmpd/libmpd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define RED "\x1b[31;01m"
#define DARKRED "\x1b[31;06m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32;06m"
#define YELLOW "\x1b[33;06m"


void status_changed(MpdObj *mi, ChangedStatusType what)
{
/*1*/	if(what&MPD_CST_SONGID)
	{
		mpd_Song *song = mpd_playlist_get_current_song(mi);
		printf(GREEN"Song:"RESET" %s - %s\n", song->artist, song->title);
	}
/*2*/	if(what&MPD_CST_SONG)
	{
		printf(GREEN"song pos changed"RESET"\n");
	}
/*3*/	if(what&MPD_CST_STATE)
	{
		printf(GREEN"State:"RESET);
		switch(mpd_player_get_state(mi))
		{
			case MPD_PLAYER_PLAY:
				printf("Playing\n");
				break;
			case MPD_PLAYER_PAUSE:
				printf("Paused\n");
				break;
			case MPD_PLAYER_STOP:
				printf("Stopped\n");
				break;
			default:
				break;
		}
	}
/*4*/	if(what&MPD_CST_REPEAT){
		printf(GREEN"Repeat:"RESET" %s\n", mpd_player_get_repeat(mi)? "On":"Off");
	}
/*5*/	if(what&MPD_CST_RANDOM){
		printf(GREEN"Random:"RESET" %s\n", mpd_player_get_random(mi)? "On":"Off");
	}
/*6*/	if(what&MPD_CST_VOLUME){
		printf(GREEN"Volume:"RESET" %03i%%\n", 
				mpd_status_get_volume(mi));
	}
/*7*/	if(what&MPD_CST_CROSSFADE){
		printf(GREEN"X-Fade:"RESET" %i sec.\n",
				mpd_status_get_crossfade(mi));
	}
/*8*/	if(what&MPD_CST_UPDATING)
	{
		if(mpd_status_db_is_updating(mi))
		{
			printf(GREEN"Started updating DB"RESET"\n");
		}
		else
		{
			printf(GREEN"Updating DB finished"RESET"\n");
		}
	}
/*9*/	if(what&MPD_CST_DATABASE)
	{
		printf(GREEN"Databased changed"RESET"\n");
	}
/*10*/	if(what&MPD_CST_PLAYLIST)
	{
		printf(GREEN"Playlist changed"RESET"\n");
	}
	/* not yet implemented signals */
/*11*/	if(what&MPD_CST_AUDIO){
		printf(GREEN"Audio Changed"RESET"\n");
	}
/*12*/	if(what&MPD_CST_TIME){
		printf(GREEN"Total song time changed"RESET"\n");
	}                                             	
/*13*/	if(what&MPD_CST_ELAPSED_TIME){
		printf(GREEN"Time elapsed changed"RESET"\n");
	}
}

int main(int argc, char *argv)
{
	int fdstdin = 0;
	int run = 1;
	MpdObj *obj = NULL;
	fdstdin = open("/dev/stdin", O_NONBLOCK|O_RDONLY);
	obj = mpd_new("127.0.0.1", 6600, NULL);
	mpd_signal_connect_status_changed(obj,(StatusChangedCallback)status_changed, NULL);
	if(!mpd_connect(obj))
	{
		char buffer[3];
		memset(buffer, '\0', 3);
		do{		
			if(read(fdstdin, buffer, 1) > 0)
			{
				switch(buffer[0])
				{
					case '\n':
						break;
					case 'b':
					       mpd_player_next(obj);
				       		break;
					case 'z':
						mpd_player_prev(obj);
						break;
					case 'x':
						mpd_player_play(obj);
						break;
					case 'c':
						mpd_player_pause(obj);
						break;
					case 'v':
						mpd_player_stop(obj);
						break;	
					case 'q':
						run = 0;
						printf("Quitting....\n");
						break;
					case 'r':
						mpd_player_set_repeat(obj, !mpd_player_get_repeat(obj));
						break;
					case 's':
						mpd_player_set_random(obj, !mpd_player_get_random(obj));
						break;
					default:
						printf("buffer: %s\n", buffer);
				}

			}	

			mpd_status_update(obj);
			memset(buffer, '\0', 3);
		}while(!usleep(500000) &&  run);
	}
	mpd_free(obj);
}
