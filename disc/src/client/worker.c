/*
 * $Id: worker.c,v 1.5 2003/04/07 19:51:57 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */



#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../timer.h"
#include "../msg_queue.h"
#include "../aaa_module.h"
#include "../diameter_api/diameter_api.h"
#include "../transport/trans.h"
#include "worker.h"


static pthread_t *worker_id = 0;
static int nr_worker_threads = 0;


void *client_worker(void *attr);




int start_client_workers( int nr_workers )
{
	int i;

	worker_id = (pthread_t*)shm_malloc( nr_workers*sizeof(pthread_t));
	if (!worker_id) {
		LOG(L_ERR,"ERROR:build_workers: cannot get free memory!\n");
		return -1;
	}

	for(i=0;i<nr_workers;i++) {
		if (pthread_create( &worker_id[i], /*&attr*/ 0, &client_worker, 0)!=0){
			LOG(L_ERR,"ERROR:build_workers: cannot create worker thread\n");
			return -1;
		}
		nr_worker_threads++;
	}

	return 1;
}




void stop_client_workers()
{
	int i;

	if (worker_id) {
		for(i=0;i<nr_worker_threads;i++)
			pthread_cancel( worker_id[i]);
		shm_free( worker_id );
	}
}




void *client_worker(void *attr)
{
	str            buf;
	struct peer    *peer;
	AAAMessage     *msg;
	unsigned int   code;
	struct trans   *tr;
	struct session *ses;

	while (1) {
		/* read a mesage from the queue */
		if (get_from_queue( &buf, &peer )==-1) {
			usleep(500);
			continue;
		}
		code = ((unsigned int*)buf.s)[1]&MASK_MSG_CODE;
		if ( buf.s[VER_SIZE+MESSAGE_LENGTH_SIZE]&0x80 ) {
			/* request */
			/* TO DO  - make sens only if the server is statefull */
			LOG(L_ERR,"ERROR:client_worker: request received"
				" - UNIMPLEMENTED!!\n");
			shm_free( buf.s );
		} else {
			/* response -> performe transaction lookup and remove it from 
			 * hash table */
			tr = transaction_lookup( peer,
				((unsigned int*)buf.s)[4], ((unsigned int*)buf.s)[3]);
			if (!tr) {
				LOG(L_ERR,"ERROR:client_worker: unexpected answer received "
					"(without sending any request)!\n");
				shm_free( buf.s );
				continue;
			}
			/* remember the session! - I will need it later */
			ses = tr->ses;
			/* destroy the transaction (also the timeout timer will be stop) */
			destroy_transaction( tr );

			/* parse the message */
			msg = AAATranslateMessage( buf.s, buf.len );
			if (!msg) {
				LOG(L_ERR,"ERROR:client_worker: error parsing message!\n");
				shm_free( buf.s );
				/* TODO - should I notify the modules some how?
				 * a trans timeout maybe? */
				continue;
			}
			msg->sId = &(ses->sID);

			if (code==ST_MSG_CODE) {
				/* it's a session termination answer */
				/* TO DO - only when the client will support a statefull
				 * server  */
				LOG(L_ERR,"ERROR:client_worker: STA received!! very strange!"
					" - UNIMPLEMENTED!!\n");
			} else {
				/* it's an auth answer -> change the state */
				if (session_state_machine( ses, AAA_AA_RECEIVED, msg)!=-1) {
					/* message was accepted by the session state machine -> 
					 * call the handler */
					((struct module_exports*)ses->app_ref)->
						mod_msg( msg, ses->context);
				}
			}

			/* free the mesage (along with the buffer) */
			AAAFreeMessage( &msg  );
		}
	}

	return 0;
}

