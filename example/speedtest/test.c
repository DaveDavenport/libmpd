#include <libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv)
{
	MpdObj *obj = mpd_new("192.150.0.111", 6600, NULL);
	if(!mpd_connect(obj))
//	for(i=0;i<1000;i++)
	{
		MpdData * data = mpd_database_get_complete(obj);
		while(data != NULL)
		{
			char buffer[1024];
			libmpd_strfsong(buffer, 1024,"[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title% &[(%time%)]|%shortfile%", data->song);
			printf("%s\n", buffer);


			data = mpd_data_get_next(data);
		}

	}
	mpd_free(obj);
}
