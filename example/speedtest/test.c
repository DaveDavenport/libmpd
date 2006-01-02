#include <libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv)
{
	MpdObj *obj = mpd_new("localhost", 6600, NULL);
	if(!mpd_connect(obj))
//	for(i=0;i<1000;i++)
	{
		MpdData * data = mpd_playlist_find(obj, MPD_TABLE_ARTIST,"(General)", TRUE);
		while(data != NULL)
		{
			char buffer[1024];
			mpd_song_markup(buffer, 1024,"[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title% &[(%time%)]|%shortfile%", data->song);
			printf("%s\n", buffer);


			data = mpd_data_get_next(data);
		}

	}
	mpd_free(obj);
}
