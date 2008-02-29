#include "../../src/libmpdclient.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	char *string;
	mpd_Connection * conn;
	conn = mpd_newConnection("192.150.0.108",6600,10);

	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return -1;
	}
	int i=0;
//	for(i=0;i<1000;i++)
//	{
/*/	
	mpd_sendCommandsCommand(conn);
	while (( string = mpd_getNextCommand(conn)) != NULL)
	{
		//printf("%s\n", string);
		free(string);
	}
	mpd_finishCommand(conn);
    */
//	}
//
//
//
//



    
    for(i=0;i<100;i++)
    {
        mpd_sendPlChangesCommand(conn,0); 
        mpd_InfoEntity *ent = NULL;
        while((ent = mpd_getNextInfoEntity(conn)) != NULL){
//            		printf("%s\n", ent->info.song->artist);
            mpd_freeInfoEntity(ent);
        }
    }
	mpd_closeConnection(conn);

	return 0;
}
