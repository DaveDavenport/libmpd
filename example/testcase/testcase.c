#include <libmpd/libmpd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv)
{
	int fdstdin = 0;
	int run = 1;
	MpdObj *obj = NULL;
	fdstdin = open("/dev/stdin", O_NONBLOCK|O_RDONLY);


	obj = mpd_new("127.0.0.1", 6600, NULL);
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
