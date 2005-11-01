#include <libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv)
{
	int i=0;
	MpdObj *obj = mpd_new("192.150.0.111", 6600, NULL);
	if(!mpd_connect(obj))
	for(i=0;i<1000;i++)
	{
		MpdData * data = mpd_playlist_get_artists(obj);
		while(data != NULL)
		{
			printf("%s\n", data->value.artist);
			data = mpd_data_get_next(data);
		}

	}
	mpd_free(obj);
}
