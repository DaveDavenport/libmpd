#include <libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv)
{
	int i=0;
	MpdObj *obj = mpd_new("192.150.0.108", 6600, NULL);
	if(!mpd_connect(obj))
	for(i=0;i<100;i++)
	{
		MpdData *data;
		data = mpd_playlist_get_changes(obj,-1);
/*		while(data != NULL)
		{
			data = mpd_data_get_next(data);
		}
        */
	}
	mpd_free(obj);
}
