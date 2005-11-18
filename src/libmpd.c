/*
 *Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU

#include <string.h>
#include <regex.h>
#include <stdarg.h>
#include <config.h>
#include "debug_printf.h"

#include "libmpd.h"
#include "libmpd-internal.h"


#ifndef HAVE_STRNDUP
char * strndup(const char *s, size_t n)
{
	size_t nAvail;
	char *p;

	if(!s) {
		return NULL;
	}

	/*  nAvail = min( strlen(s)+1, n+1 ); */
	nAvail=((strlen(s)+1) > (n+1)) ? n+1 : strlen(s)+1;
	if(!(p=malloc(nAvail))) {
		return NULL;
	}
	memcpy(p, s, nAvail);
	p[nAvail - 1] = 0;
	return p;
}
#endif



/*************************************************************************************/
static MpdObj * mpd_create()
{
	MpdObj * mi = malloc(sizeof(MpdObj));
	if( mi == NULL )
	{
		/* should never happen on linux */
		return NULL;
	}


	/* set default values */
	mi->connected = FALSE;
	mi->port = 6600;
	mi->hostname = strdup("localhost");
	mi->password = NULL;
	mi->connection_timeout = 1.0;
	mi->connection = NULL;
	mi->status = NULL;
	mi->stats = NULL;
	mi->error = 0;
	mi->error_mpd_code = 0;
	mi->error_msg = NULL;
	mi->CurrentSong = NULL;
	/* info */
	mi->CurrentState.playlistid = -1;
	mi->CurrentState.state = -1;
	mi->CurrentState.songid = -1;
	mi->CurrentState.dbUpdateTime = 0;
	mi->CurrentState.updatingDb = 0;
	mi->CurrentState.repeat = -1;
	mi->CurrentState.random = -1;
	mi->CurrentState.volume = -2;
	mi->CurrentState.xfade	= -1;
	mi->CurrentState.totaltime = 0;
	mi->CurrentState.elapsedtime = 0;

	memcpy(&(mi->OldState), &(mi->CurrentState), sizeof(MpdServerState));


	/* signals */
/*	mi->playlist_changed = NULL;
	mi->playlist_changed_pointer = NULL;
*/	/* song */
/*	mi->song_changed = NULL;
	mi->song_changed_signal_pointer = NULL;
*/	/* status */
/*	mi->status_changed = NULL;
	mi->status_changed_signal_pointer = NULL;
*/	/* state */
/*	mi->state_changed = NULL;
	mi->state_changed_signal_pointer = NULL;
*/	/* database changed */
/*	mi->database_changed = NULL;
	mi->state_changed_signal_pointer = NULL;
*/	/* disconnect signal */
/*	mi->disconnect = NULL;
	mi->disconnect_pointer = NULL;
*/	/* connect signal */
/*	mi->connect = NULL;
	mi->connect_pointer = NULL;	
*/	/* updating db */
/*	mi->updating_changed = NULL;
	mi->updating_signal_pointer = NULL;
*/	/* error signal */
/*	mi->error_signal = NULL;
	mi->error_signal_pointer = NULL;
*/
	
	/* connection changed signal */
	mi->the_connection_changed_callback = NULL;
	mi->the_connection_changed_signal_userdata = NULL;

	/* status changed */
	mi->the_status_changed_callback = NULL;
	mi->the_status_changed_signal_userdata = NULL;	

	/* error callback */
	mi->the_error_callback = NULL;
	mi->the_error_signal_userdata  = NULL;
	/* connection is locked because where not connected */
	mi->connection_lock = TRUE;

	mi->queue = NULL;
	/* commands */
	mi->commands = NULL;
	return mi;
}

void mpd_free(MpdObj *mi)
{
	debug_printf(DEBUG_INFO, "mpd_free: destroying MpdObj object\n");
	if(mi->connected)
	{
		/* disconnect */
		mpd_disconnect(mi);
		debug_printf(DEBUG_WARNING, "mpd_free: Connection still running, disconnecting\n");
	}
	if(mi->hostname)
	{
		free(mi->hostname);
	}
	if(mi->password)
	{
		free(mi->password);
	}
	if(mi->error_msg)
	{
		free(mi->error_msg);
	}
	if(mi->connection)
	{
		/* obsolete */
		mpd_closeConnection(mi->connection);
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
	}
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
	}
	free(mi);
}

int mpd_check_error(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}
/*
	if(mi->error)
	{
		return TRUE;
	}
*/
	/* this shouldn't happen, ever */
	if(mi->connection == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_check_error: should this happen, mi->connection == NULL?");
		return TRUE;
	}

	/* TODO: map these errors in the future */
	mi->error = mi->connection->error;
	mi->error_mpd_code = mi->connection->errorCode;
	mi->error_msg = strdup(mi->connection->errorStr);

	/* Check for permission */
	/* First check for an error reported by MPD
	 * Then check what type of error mpd reported
	 */
	if(mi->error == MPD_ERROR_ACK)
	{

		debug_printf(DEBUG_ERROR,"mpd_check_error: clearing errors in mpd_Connection");
		mpd_clearError(mi->connection);
		if (mi->the_error_callback)
		{
			mi->the_error_callback(mi, mi->error_mpd_code, mi->error_msg, mi->the_error_signal_userdata );
		}
		return TRUE;
	}
	if(mi->error)
	{

		debug_printf(DEBUG_ERROR, "mpd_check_error: Following error occured: %i: code: %i msg: %s", mi->error,mi->connection->errorCode, mi->error_msg);
		mpd_disconnect(mi);
		/* deprecated */
		/*		if(mi->error_signal)
				{
				mi->error_signal(mi, mi->error, mi->error_msg,mi->error_signal_pointer);
				}
				*/
		if (mi->the_error_callback)
		{
			mi->the_error_callback(mi, mi->error, mi->error_msg, mi->the_error_signal_userdata );
		}
		return TRUE;
	}
	return FALSE;
}



int mpd_lock_conn(MpdObj *mi)
{
	/*	debug_printf(DEBUG_INFO, "mpd_lock_conn: Locking connection\n");
	*/
	if(mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_lock_conn: Failed to lock connection, already locked\n");
		return TRUE;
	}
	mi->connection_lock = TRUE;
	return FALSE;
}

int mpd_unlock_conn(MpdObj *mi)
{
	/*	debug_printf(DEBUG_INFO, "mpd_unlock_conn: unlocking connection\n");
	*/
	if(!mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_unlock_conn: Failed to unlock connection, already unlocked\n");
		return FALSE;
	}

	mi->connection_lock = FALSE;

	return mpd_check_error(mi);
}

MpdObj * mpd_new_default()
{
	debug_printf(DEBUG_INFO, "mpd_new_default: creating a new mpdInt object\n");
	return mpd_create();
}

MpdObj *mpd_new(char *hostname,  int port, char *password)
{
	MpdObj *mi = mpd_create();
	if(mi == NULL)
	{
		return NULL;
	}
	if(hostname != NULL)
	{
		mpd_set_hostname(mi, hostname);
	}
	if(port != 0)
	{
		mpd_set_port(mi, port);
	}
	if(password != NULL)
	{
		mpd_set_password(mi, password);
	}
	return mi;
}

void mpd_set_hostname(MpdObj *mi, char *hostname)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_set_hostname: mi == NULL\n");
		return;
	}

	if(mi->hostname != NULL)
	{
		free(mi->hostname);
	}
	/* possible location todo some post processing of hostname */
	mi->hostname = strdup(hostname);
}

void mpd_set_password(MpdObj *mi, char *password)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_set_password: mi == NULL\n");
		return;
	}

	if(mi->password != NULL)
	{
		free(mi->password);
	}
	/* possible location todo some post processing of password */
	mi->password = strdup(password);
}


void mpd_send_password(MpdObj *mi)
{
	if(!mi) return;
	if(mi->password && mpd_check_connected(mi))
	{
		if(mpd_lock_conn(mi))
		{
			debug_printf(DEBUG_WARNING, "mpd_set_password: failed to lock connection");
			return;
		}
		mpd_sendPasswordCommand(mi->connection, mi->password);
		mpd_finishCommand(mi->connection);	
		if(mpd_unlock_conn(mi))
		{
			debug_printf(DEBUG_ERROR, "mpd_send_password: Failed to unlock connection\n");
			return;
		}
		mpd_server_get_allowed_commands(mi);
	}
}

void mpd_set_port(MpdObj *mi, int port)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_set_port: mi == NULL\n");
		return;
	}
	mi->port = port;
}

void mpd_set_connection_timeout(MpdObj *mi, float timeout)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_set_connection_timeout: mi == NULL\n");
		return;
	}
	mi->connection_timeout = timeout;
	if(mpd_check_connected(mi))
	{
		/*TODO: set timeout */	
		if(mpd_lock_conn(mi))
		{
			debug_printf(DEBUG_WARNING,"mpd_set_connection_timeout: lock failed\n");
			return;
		}
		mpd_setConnectionTimeout(mi->connection, timeout);
		mpd_finishCommand(mi->connection);

		mpd_unlock_conn(mi);

	}
}

static void mpd_server_free_commands(MpdObj *mi)
{
	if(mi->commands)
	{
		int i=0;
		while(mi->commands[i].command_name)
		{                                           	
			free(mi->commands[i].command_name);
			i++;
		}
		free(mi->commands);
		mi->commands = NULL;
	}

}


void mpd_server_get_allowed_commands(MpdObj *mi)
{
	char *temp = NULL;
	int num_commands = 0;
	if(!mi) return;
	if(!mpd_check_connected(mi))return;
	if(!mpd_server_check_version(mi,0,12,0)) return;

	mpd_server_free_commands(mi);

	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING, "mpd_server_get_allowed_commands: lock failed");
		return;
	}
	mpd_sendCommandsCommand(mi->connection);
	while((temp = mpd_getNextCommand(mi->connection)))
	{
		num_commands++;
		mi->commands = realloc(mi->commands, (num_commands+1)*sizeof(MpdCommand));
		mi->commands[num_commands-1].command_name = temp;
		mi->commands[num_commands-1].enabled = TRUE;
		mi->commands[num_commands].command_name = NULL;
		mi->commands[num_commands].enabled = FALSE;
	}
	mpd_finishCommand(mi->connection);
	mpd_sendNotCommandsCommand(mi->connection);
	while((temp = mpd_getNextCommand(mi->connection)))
	{
		num_commands++;
		mi->commands = realloc(mi->commands, (num_commands+1)*sizeof(MpdCommand));
		mi->commands[num_commands-1].command_name = temp;
		mi->commands[num_commands-1].enabled = FALSE;
		mi->commands[num_commands].command_name = NULL;
		mi->commands[num_commands].enabled = FALSE;
	}
	mpd_finishCommand(mi->connection);

	mpd_unlock_conn(mi);
	return;
}



int mpd_disconnect(MpdObj *mi)
{
	/* set disconnect flag */
	mi->connected = 0;
	/* lock */
	mpd_lock_conn(mi);
	debug_printf(DEBUG_INFO, "mpd_disconnect: disconnecting\n");

	if(mi->connection)
	{
		mpd_closeConnection(mi->connection);
		mi->connection = NULL;
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
		mi->stats = NULL;
	}
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}
	mi->CurrentState.playlistid = -1;
	mi->CurrentState.state = -1;
	mi->CurrentState.songid = -1;
	mi->CurrentState.dbUpdateTime = 0;
	mi->CurrentState.updatingDb = 0;
	mi->CurrentState.repeat = -1;
	mi->CurrentState.random = -1;
	mi->CurrentState.volume = -2;
	mi->CurrentState.xfade	= -1;
	mi->CurrentState.totaltime = 0;
	mi->CurrentState.elapsedtime = 0;

	memcpy(&(mi->OldState), &(mi->CurrentState) , sizeof(MpdServerState));
	/*don't reset errors */


	if(mi->the_connection_changed_callback != NULL)
	{
		mi->the_connection_changed_callback( mi, FALSE, mi->the_connection_changed_signal_userdata );
	}
	/* deprecated */
	/*	if(mi->disconnect != NULL)
		{
		mi->disconnect(mi, mi->disconnect_pointer);
		}
		*/
	mpd_server_free_commands(mi);
	return FALSE;
}

int mpd_connect(MpdObj *mi)
{
	if(mi == NULL)
	{
		/* should return some spiffy error here */
		return -1;
	}
	/* reset errors */
	mi->error = 0;
	mi->error_mpd_code = 0;
	if(mi->error_msg != NULL)
	{
		free(mi->error_msg);
	}
	mi->error_msg = NULL;

	debug_printf(DEBUG_INFO, "mpd_connect: connecting\n");
	mi->CurrentState.playlistid = -1;
	mi->CurrentState.state = -1;
	mi->CurrentState.songid = -1;
	mi->CurrentState.dbUpdateTime = 0;
	mi->CurrentState.updatingDb = 0;
	mi->CurrentState.repeat = -1;
	mi->CurrentState.random = -1;
	mi->CurrentState.volume = -2;
	mi->CurrentState.xfade	= -1;
	mi->CurrentState.totaltime = 0;
	mi->CurrentState.elapsedtime = 0;

	memcpy(&(mi->OldState), &(mi->CurrentState), sizeof(MpdServerState));

	if(mi->connected)
	{
		/* disconnect */
		mpd_disconnect(mi);
	}

	if(mi->hostname == NULL)
	{
		mpd_set_hostname(mi, "localhost");
	}
	/* make sure this is true */
	if(!mi->connection_lock)
	{
		mpd_lock_conn(mi);
	}
	/* make timeout configurable */
	mi->connection = mpd_newConnection(mi->hostname,mi->port,mi->connection_timeout);
	if(mi->connection == NULL)
	{
		/* again spiffy error here */
		return -1;
	}
	/*
	 * We always connect anonymous
	 */
	/*
	   if(mi->password != NULL && strlen(mi->password) > 0)
	   {
	   mpd_sendPasswordCommand(mi->connection, mi->password);
	   mpd_finishCommand(mi->connection);

	   }
	   */

	/* set connected state */
	mi->connected = TRUE;
	if(mpd_unlock_conn(mi))
	{
		return -1;
	}

	/* get the commands we are allowed to use */
	mpd_server_get_allowed_commands(mi);


	if(mi->the_connection_changed_callback != NULL)
	{
		mi->the_connection_changed_callback( mi, TRUE, mi->the_connection_changed_signal_userdata );
	}
	/* deprecated */
	/*	if(mi->connect != NULL)
		{
		mi->connect(mi, mi->connect_pointer);
		}
		*/
	debug_printf(DEBUG_INFO, "mpd_connect: Connected to mpd");
	return 0;
}

int mpd_check_connected(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}
	return mi->connected;
}

/*
   void mpd_signal_set_connect (MpdObj *mi, void *(* connect)(MpdObj *mi, void *pointer), void *connect_pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_connect: MpdObj *mi == NULL");
   return;
   }
   mi->connect = connect;
   mi->connect_pointer = connect_pointer;
   }
   */
/* SIGNALS */
/*
   void mpd_signal_set_disconnect (MpdObj *mi, void *(* disconnect)(MpdObj *mi, void *pointer), void *disconnect_pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_disconnect: MpdObj *mi == NULL");
   return;
   }
   mi->disconnect = disconnect;
   mi->disconnect_pointer = disconnect_pointer;
   }
   */
/* SIGNALS */
void	mpd_signal_connect_status_changed        (MpdObj *mi, StatusChangedCallback status_changed, void *userdata)
{
	if(mi == NULL)
	{
		return;
	}
	mi->the_status_changed_callback = status_changed;
	mi->the_status_changed_signal_userdata = userdata;
}


void	mpd_signal_connect_error(MpdObj *mi, ErrorCallback error_callback, void *userdata)
{
	if(mi == NULL)
	{
		return;
	}
	mi->the_error_callback = error_callback;
	mi->the_error_signal_userdata = userdata;
}

void	mpd_signal_connect_connection_changed(MpdObj *mi, ConnectionChangedCallback connection_changed, void *userdata)
{
	if(mi == NULL)
	{
		return;
	}
	mi->the_connection_changed_callback = connection_changed;
	mi->the_connection_changed_signal_userdata = userdata;
}


/*
   void mpd_signal_set_playlist_changed (MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id,void *pointer), void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_playlist_changed: MpdObj *mi == NULL");
   return;
   }
   mi->playlist_changed = playlist_changed;
   mi->playlist_changed_pointer = pointer;
   }


   void mpd_signal_set_song_changed (MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id, void *pointer),void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_song_changed: MpdObj *mi == NULL");
   return;
   }
   mi->song_changed = song_changed;
   mi->song_changed_signal_pointer = pointer;
   }


   void mpd_signal_set_state_changed (MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_state_changed: MpdObj *mi == NULL");
   return;
   }
   mi->state_changed = state_changed;
   mi->state_changed_signal_pointer = pointer;
   }

   void mpd_signal_set_database_changed (MpdObj *mi, void *(* database_changed)(MpdObj *mi, void *pointer), void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_database_changed: MpdObj *mi == NULL");
   return;
   }
   mi->database_changed = database_changed;
   mi->database_changed_signal_pointer = pointer;
   }


   void mpd_signal_set_updating_changed (MpdObj *mi, void *(* updating_changed)(MpdObj *mi,int updating, void *pointer), void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_updating_changed: MpdObj *mi == NULL");
   return;
   }
   mi->updating_changed = updating_changed;
   mi->updating_signal_pointer = pointer;
   }

   void mpd_signal_set_status_changed (MpdObj *mi, void *(* status_changed)(MpdObj *mi, void *pointer),void *pointer)
   {
   if(mi == NULL)
   {
   debug_printf(DEBUG_ERROR, "mpd_signal_set_state_changed: MpdObj *mi == NULL");
   return;
   }
   mi->status_changed = status_changed;
   mi->status_changed_signal_pointer = pointer;
   }


   void mpd_signal_set_error (MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg,void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_signal_set_error: MpdObj *mi == NULL");
		return;
	}
	mi->error_signal = error_signal;
	mi->error_signal_pointer = pointer;
}
*/


/* more playlist */
/* MpdData Part */
MpdData *mpd_new_data_struct(MpdData_head * const head)
{
	MpdData_real* data;
	if (head->current->space_left == 0) {
		head->current->next = malloc(sizeof(MpdDataPool));
		head->current = head->current->next;
		head->current->space_left = MPD_DATA_POOL_SIZE;
		head->current->next = NULL;
	}
	data = &(head->current->pool[MPD_DATA_POOL_SIZE - head->current->space_left]);
	head->current->space_left--;
	data->type = 0;

	data->tag_type = 0;
	data->tag = NULL;
	data->song = NULL;
	data->directory = NULL;
	data->playlist = NULL;
	data->output_dev = NULL;
	data->next = NULL;
	data->prev = NULL;
	data->head = head;
	return (MpdData*)data;
}

MpdData *mpd_new_data_struct_append(MpdData  * const data)
{
	MpdData_real *data_real = (MpdData_real*)data;
	MpdData_head *head;
	if(data_real == NULL)
	{
		head = (MpdData_head*)malloc(sizeof(MpdData_head));
		head->current = head->pool = (MpdDataPool*)malloc(sizeof(MpdDataPool));
		head->pool->space_left = MPD_DATA_POOL_SIZE;
		head->pool->next = NULL;
		data_real = (MpdData_real*)mpd_new_data_struct(head);
		head->first = data_real;
	}
	else
	{
		data_real->next = (MpdData_real*)mpd_new_data_struct(data_real->head); 	
		data_real->next->prev = data_real;
		data_real = data_real->next;
		data_real->next = NULL;

	}
	return (MpdData*)data_real;
}

MpdData * mpd_data_get_first(MpdData const * const data)
{
	MpdData_real const * const data_real = (MpdData_real const * const)data;
	if(data_real != NULL)
	{
		if (data_real->head != NULL)
		{
			return (MpdData*)(mpd_data_get_head(data)->first);
		} 
		else 
		{
			return NULL;
		}
	}
	return NULL;
}


MpdData * mpd_data_get_next(MpdData * const data) 
{
	return mpd_data_get_next_real(data, TRUE);
}

MpdData * mpd_data_get_next_real(MpdData * const data, int kill_list)
{
	MpdData_real *data_real = (MpdData_real*)data;
	if (data_real != NULL) 
	{
		if (data_real->next != NULL )
		{
			return (MpdData*)data_real->next;
		}
		else		
		{
			if (kill_list) mpd_data_free((MpdData*)data_real);
			return NULL;
		}
	}
	return (MpdData*)data_real;	
}

int mpd_data_is_last(MpdData const * const data)
{
	MpdData_real const * const data_real = (MpdData_real const * const)data;
	if(data_real != NULL)
	{
		if (data_real->next == NULL)
		{
			return TRUE;
		}
	}
	return FALSE;	
}

MpdData_head *mpd_data_get_head(MpdData const * const data) {
	return ((MpdData_real*)data)->head;
}

MpdData* mpd_data_concatenate( MpdData  * const first, MpdData  * const second) 
{
	MpdData_real *first_real  = (MpdData_real*)first;
	MpdData_real *second_real = (MpdData_real*)second;
	MpdData_head *first_head  = mpd_data_get_head(first);
	MpdData_head *second_head = mpd_data_get_head(second);
	MpdDataPool *pool;

	if ( first == NULL ) {
		if ( second != NULL ) 
			return (MpdData*)second_real;
		else
			return NULL;
	} else {
		if ( second == NULL )
			return (MpdData*)first_real;
	}

	/* find last element in first data list */	
	while (!mpd_data_is_last((MpdData*)first_real)) first_real = (MpdData_real*)mpd_data_get_next_real((MpdData*)first_real, FALSE);
	second_real =(MpdData_real*) mpd_data_get_first((MpdData*)second_real);

	first_real->next = second_real;
	second_real->prev = first_real;

	/* I need to set all the -> first correct */
	while (second_real)
	{
		second_real->head = first_head;
		second_real = (MpdData_real*)mpd_data_get_next_real((MpdData*)second_real, FALSE);
	} 
	/* Need to concatenate the list of MpdDataPools 
	 * Note that this is NOT efficient in any way...
	 */
	pool = first_head->current;
	pool->next = second_head->pool;
	first_head->current = second_head->current;
	free(second_head);
	return (MpdData*)first_head->first;
}

MpdData * mpd_data_delete_item(MpdData *data)
{
	MpdData_real *temp = NULL, *data_real = (MpdData_real*)data;
	if(data_real == NULL) return NULL;
	if(data_real->head->first == data_real)
	{
		temp = data_real->head->first = data_real->next;
		data_real->prev = NULL;
	}
	else
	{
		if (data_real->prev)
		{
			temp = data_real->prev->next = data_real->next;
			temp = data_real->prev;
		}
		if (data_real->next)
		{
			data_real->next->prev = data_real->prev;
			temp = data_real->next;
		}
	}


	return (MpdData *)temp;
}

void mpd_data_free(MpdData *data)
{
	MpdData_head *head;
	MpdDataPool *pool, *temp_pool;
	MpdData_real *data_real;
	unsigned int i;
	if(data == NULL)
	{
		return;
	}
	head = mpd_data_get_head(data);
	pool = head->pool;
	do {
		for (i = 0; i < MPD_DATA_POOL_SIZE - pool->space_left; i++) {
			data_real = &(pool->pool[i]);
			if (data_real->type == MPD_DATA_TYPE_SONG) {
				mpd_freeSong(data_real->song);
			} else if (data_real->type == MPD_DATA_TYPE_OUTPUT_DEV) {
				mpd_freeOutputElement(data_real->output_dev);
			} else {
				free((void*)(data_real->tag));
			}
		}
		temp_pool = pool->next;
		free (pool);
		pool = temp_pool;
	} while ( pool );
	free(head);
}

/* clean this up.. make one while loop */
static void mpd_free_queue_ob(MpdObj *mi)
{
	MpdQueue *temp = NULL;
	if(mi->queue == NULL)
	{
		return;
	}	
	mi->queue = mi->queue->first;
	while(mi->queue != NULL)
	{
		temp = mi->queue->next;

		if(mi->queue->path != NULL)
		{
			free(mi->queue->path);
		}

		free(mi->queue);
		mi->queue = temp;
	}
	mi->queue = NULL;

}

MpdQueue *mpd_new_queue_struct()
{
	MpdQueue* queue = malloc(sizeof(MpdQueue));

	queue->type = 0;
	queue->path = NULL;
	queue->id = 0;

	return queue;	
}


void mpd_queue_get_next(MpdObj *mi)
{
	if(mi->queue != NULL && mi->queue->next != NULL)
	{
		mi->queue = mi->queue->next;
	}
	else if(mi->queue->next == NULL)
	{
		mpd_free_queue_ob(mi);
		mi->queue = NULL;
	}
}

long unsigned mpd_server_get_database_update_time(MpdObj *mi)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_get_database_update: not connected\n");
		return MPD_NOT_CONNECTED;
	}
	if(!mpd_stats_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_get_database_update: Failed grabbing status\n");
		return MPD_FAILED_STATS;
	}
	return mi->stats->dbUpdateTime;
}


MpdData * mpd_server_get_output_devices(MpdObj *mi)
{
	mpd_OutputEntity *output = NULL;
	MpdData *data = NULL;
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_get_output_device: not connected\n");
		return NULL;
	}
	/* TODO: Check version */
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_output_devic: lock failed\n");
		return NULL;
	}

	mpd_sendOutputsCommand(mi->connection);
	while (( output = mpd_getNextOutput(mi->connection)) != NULL)
	{	
		data = mpd_new_data_struct_append(data);
		data->type = MPD_DATA_TYPE_OUTPUT_DEV; 
		data->output_dev = output;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return mpd_data_get_first(data);
}

int mpd_server_set_output_device(MpdObj *mi,int device_id,int state)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_set_output_device: not connected\n");	
		return MPD_NOT_CONNECTED;
	}
	if(mpd_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_set_output_device: lock failed\n");
		return MPD_LOCK_FAILED;
	}
	if(state)
	{
		mpd_sendEnableOutputCommand(mi->connection, device_id);
	}
	else
	{
		mpd_sendDisableOutputCommand(mi->connection, device_id);
	}	
	mpd_finishCommand(mi->connection);

	mpd_unlock_conn(mi);
	mpd_status_queue_update(mi);
	return FALSE;
}

int mpd_server_check_version(MpdObj *mi, int major, int minor, int micro)
{
	if(!mpd_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_server_check_version: not connected\n");	
		return FALSE;                                     	
	}
	if(major > mi->connection->version[0]) return FALSE;
	if(mi->connection->version[0] > major) return TRUE;
	if(minor > mi->connection->version[1]) return FALSE;
	if(mi->connection->version[1] > minor) return TRUE;
	if(micro > mi->connection->version[2]) return FALSE;
	if(mi->connection->version[2] > micro) return TRUE; 	
	return TRUE;
}	

int mpd_server_check_command_allowed(MpdObj *mi, const char *command)
{
	int i;
	if(!mi || !command) return MPD_SERVER_COMMAND_ERROR;
	/* when we are connected to a mpd server that doesn't support commands and not commands
	 * feature. (like mpd 0.11.5) allow everything
	 */
	if(!mpd_server_check_version(mi, 0,12,0)) return MPD_SERVER_COMMAND_ALLOWED;
	/*
	 * Also when somehow we failted to get commands
	 */
	if(mi->commands == NULL) return MPD_SERVER_COMMAND_ALLOWED;



	for(i=0;mi->commands[i].command_name;i++)
	{
		if(!strcasecmp(mi->commands[i].command_name, command))
			return mi->commands[i].enabled;
	}
	return MPD_SERVER_COMMAND_NOT_SUPPORTED;
}

/** MISC **/

regex_t ** mpd_misc_tokenize(char *string)
{
	regex_t ** result = NULL; 	/* the result with tokens 		*/
	int i = 0;		/* position in string 			*/
	int br = 0;		/* number for open ()[]'s		*/
	int bpos = 0;		/* begin position of the cur. token 	*/

	int tokens=0;
	if(string == NULL) return NULL;
	for(i=0; i < strlen(string)+1;i++)
	{
		/* check for opening  [( */
		if(string[i] == '(' || string[i] == '[' || string[i] == '{') br++;
		/* check closing */
		else if(string[i] == ')' || string[i] == ']' || string[i] == '}') br--;
		/* if multiple spaces at begin of token skip them */
		else if(string[i] == ' ' && !(i-bpos))bpos++;
		/* if token end or string end add token to list */
		else if((string[i] == ' ' && !br) || string[i] == '\0')
		{
			char * temp=NULL;
			result = (regex_t **)realloc(result,(tokens+2)*sizeof(regex_t *));
			result[tokens] = malloc(sizeof(regex_t));
			temp = (char *)strndup((const char *)&string[bpos], i-bpos);
			if(regcomp(result[tokens], temp, REG_EXTENDED|REG_ICASE|REG_NOSUB))
			{
				result[tokens+1] = NULL;
				mpd_misc_tokens_free(result);
				return NULL;
			}
			free(temp);
			result[tokens+1] = NULL;
			bpos = i+1;                                         
			tokens++;
		}

	}
	return result;
}

void mpd_misc_tokens_free(regex_t ** tokens)
{
	int i=0;
	if(tokens == NULL) return;
	for(i=0;tokens[i] != NULL;i++)
	{
		regfree(tokens[i]);
		free(tokens[i]);
	}
	free(tokens);
}

int mpd_misc_get_tag_by_name(char *name)
{
	int i;
	if(name == NULL)
	{
		return -1;
	}
	for(i=0; i < MPD_TAG_NUM_OF_ITEM_TYPES; i++)
	{
		if(!strcasecmp(mpdTagItemKeys[i], name))
		{
			return i;
		}
	}
	return -1;
}
