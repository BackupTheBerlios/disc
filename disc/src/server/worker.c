/*
 * $Id: worker.c,v 1.1 2003/04/08 22:30:18 bogdan Exp $
 *
 * 2003-04-08 created by bogdan
 */



#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../timer.h"
#include "../globals.h"
#include "../msg_queue.h"
#include "../aaa_module.h"
#include "../diameter_api/diameter_api.h"
#include "../transport/trans.h"
#include "../transport/peer.h"
#include "worker.h"


static pthread_t *worker_id = 0;
static int nr_worker_threads = 0;


void *server_worker(void *attr);




int start_server_workers( int nr_workers )
{
	int i;

	worker_id = (pthread_t*)shm_malloc( nr_workers*sizeof(pthread_t));
	if (!worker_id) {
		LOG(L_ERR,"ERROR:build_workers: cannot get free memory!\n");
		return -1;
	}

	for(i=0;i<nr_workers;i++) {
		if (pthread_create( &worker_id[i], /*&attr*/ 0, &server_worker, 0)!=0){
			LOG(L_ERR,"ERROR:build_workers: cannot create worker thread\n");
			return -1;
		}
		nr_worker_threads++;
	}

	return 1;
}




void stop_server_workers()
{
	int i;

	if (worker_id) {
		for(i=0;i<nr_worker_threads;i++)
			pthread_cancel( worker_id[i]);
		shm_free( worker_id );
	}
}




int get_req_destination( AAAMessage *msg, struct peer_chain **pc )
{
	struct peer_chain *p_chaine;
	struct peer        *p;
	int                 res;

	/* is Dest-Host present? */
	if (msg->dest_host) {
		/* dest-host avp present */
		if (msg->dest_host->data.len==aaa_identity.len &&
		strncmp(msg->dest_host->data.s, aaa_identity.s, aaa_identity.len) ) {
			/* I'm the destination host */
			if (pc) *pc = 0;
			return 1;
		} else {
			/* I'm not the destination host -> am I peer with the dest-host? */
			p = lookup_peer_by_identity( &(msg->dest_host->data) );
			if (p) {
				/* the destination host is one of my peers */
				if (pc) *pc = &(p->pc);
				return 1;
			} else {
				if (!msg->dest_realm) {
					/* dest-host is not me and no dest-realm -> bogus msg */
					if (pc) *pc = 0;
					return -1;
				}
			}
		}
	} else {
		if (!msg->dest_realm) {
			/* no dest-host and no dest-realm -> it's local */
			if (pc) *pc = 0;
			return 2;
		}
	}

	/* do routing based on destination-realm AVP */
	p_chaine = 0;
	//res = run_script( &p_chaine );

	return 1;
}





void *server_worker(void *attr)
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

