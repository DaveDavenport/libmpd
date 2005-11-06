#include <libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv)
{
	int i=0;
	MpdObj *obj = mpd_new("192.150.0.111", 6600, NULL);
	if(!mpd_connect(obj))
//	for(i=0;i<1000;i++)
	{
		MpdData * data = mpd_playlist_get_artists(obj);
		while(data != NULL)
		{
			if(i%2){
				data = mpd_data_delete_item(data);
			}
			else printf("%s\n", data->tag);
			data = mpd_data_get_next(data);
			i++;
		}

	}
	mpd_free(obj);
}
