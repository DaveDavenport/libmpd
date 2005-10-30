#ifndef __MPD_INTERNAL_LIB_
#define __MPD_INTERNAL_LIB_

#include "libmpdclient.h"

/* queue struct */
typedef struct _MpdQueue MpdQueue;
typedef struct _MpdServerState {
	/* information needed to detect changes on mpd's side */
	long long 	playlistid;
	int 		songid;
	int 		state;
	unsigned long	dbUpdateTime;	
	int 		updatingDb;
	int		random;
	int		repeat;
	int		volume;
	int		xfade;
} MpdServerState;

typedef struct _MpdObj {
	/* defines if we are connected */
	/* This should be made true if and only if the connection is up and running */
	short int 	connected;
	/* information needed to connect to mpd */
	char 		*hostname;
	int 		port;
	char 		*password;
	float 		connection_timeout;

	/* mpd's structures */
	mpd_Connection 	*connection;
	mpd_Status 	*status;
	mpd_Stats 	*stats;
	mpd_Song 	*CurrentSong;

	/* used to store/detect serverside status changes */
	MpdServerState CurrentState;
	MpdServerState OldState;
 
	/* new style signals */
	/* error signal */
	ErrorCallback the_error_callback;
	void *the_error_signal_userdata;
	/* song status changed */
	StatusChangedCallback the_status_changed_callback;
	void *the_status_changed_signal_userdata;
	/* (dis)connect signal */
	ConnectionChangedCallback the_connection_changed_callback;
	void *the_connection_changed_signal_userdata;
        
	/* old style signals */
	void *(* playlist_changed)(struct _MpdObj *mi, int old_playlist_id, int new_playlist_id, void *pointer);	
	void *playlist_changed_pointer;
	/* error signal */
	void *(* error_signal)(struct _MpdObj *mi, int id, char *msg, void *pointer);	
	void *error_signal_pointer;
	/* song change */
	void *(* song_changed)(struct _MpdObj *mi, int old_song_id, int new_song_id, void *pointer);	
	void *song_changed_signal_pointer;                                                     	
	/* song status changed */
	void *(* status_changed)(struct _MpdObj *mi, void *pointer);	
	void *status_changed_signal_pointer;                                                     	
	/* song status changed */
	void *(* state_changed)(struct _MpdObj *mi,int old_state,int new_state, void *pointer);	
	void *state_changed_signal_pointer;                                                     	

	/* disconnect signal */
	void *(* disconnect) (struct _MpdObj *mi, void *pointer);
	void *disconnect_pointer;

	/* connect signal */
	void *(* connect) (struct _MpdObj *mi, void *pointer);
	void *connect_pointer;

	/* error message */
	int error;
	char *error_msg;	

	/* song datab update */
	void *(* database_changed)(struct _MpdObj *mi,void *pointer);	
	void *database_changed_signal_pointer;                                                     	

	void *(* updating_changed)(struct _MpdObj *mi, int updating,void *pointer);
	void *updating_signal_pointer;



	/* internal values */
	/* this "locks" the connections. so we can't have to commands competing with eachother */
	short int connection_lock;

	/* queue */
	MpdQueue *queue;

}_MpdObj;


enum {
	MPD_QUEUE_ADD,
	MPD_QUEUE_LOAD,
	MPD_QUEUE_DELETE_ID
} MpdQueueType;

typedef struct _MpdQueue { 
	struct _MpdQueue *next;
	struct _MpdQueue *prev;
	struct _MpdQueue *first;

	/* what item to queue, (add/load/remove)*/
	int type;
	/* for adding files/load playlist/adding streams */
	char *path;
	/* for removing */
	int id;
}_MpdQueue;

/* Internal Queue struct functions */
MpdQueue *	mpd_new_queue_struct			();

/* Internal Data struct functions */
MpdData *	mpd_new_data_struct			();
MpdData *	mpd_new_data_struct_append		(MpdData *data);


#ifndef HAVE_STRNDUP
char * 		strndup					(const char *s, size_t n);
#endif


#endif
