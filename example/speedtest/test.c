#include <libmpd/libmpd.h>
#include <stdio.h>


int main(int argc, char *argv)
{
	MpdObj *obj = mpd_new("192.150.0.111", 6600, NULL);
	if(!mpd_connect(obj))
	{
		MpdData * data = mpd_playlist_get_artists(obj);
	/*	while(data != NULL)
		{
			printf("%s\n", data->value.artist);
			data = mpd_data_get_next(data);
		}
*/
	}




}
