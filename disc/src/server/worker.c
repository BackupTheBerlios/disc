/*
 * $Id: worker.c,v 1.2 2003/04/09 22:10:34 bogdan Exp $
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
#include "../list.h"
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



/* REturns   0: success
 *          >0: error response
 */
int get_req_destination( AAAMessage *msg, struct peer_chain **p_chain,
												struct aaa_module **module )
{
	struct peer        *p;
	struct aaa_module  *mod;

	/* is Dest-Host present? */
	if (msg->dest_host) {
		/* dest-host avp present */
		if (!msg->dest_realm) {
			/* has dest-host but no dest-realm -> bogus msg */
			return AAA_MISSING_AVP;
		}
		/* check the dest-host */
		if (msg->dest_host->data.len==aaa_identity.len &&
		strncmp(msg->dest_host->data.s, aaa_identity.s, aaa_identity.len) ) {
			/* I'm the destination host */
			*p_chain = 0;
			*module = find_module(msg->applicationId);
			return ((*module!=0)?0:AAA_APPLICATION_UNSUPPORTED);
		} else {
			/* I'm not the destination host -> am I peer with the dest-host? */
			p = lookup_peer_by_identity( &(msg->dest_host->data) );
			if (p) {
				/* the destination host is one of my peers */
				*p_chain = &(p->pc);
				*module = 0;
				return 0;
			}
			/* do routing based on dest realm */
		}
	} else {
		if (!msg->dest_realm) {
			/* no dest-host and no dest-realm -> it's local */
			*p_chain = 0;
			*module = find_module(msg->applicationId);
			return ((*module!=0)?0:AAA_APPLICATION_UNSUPPORTED);
		}
		/* do routing based on dest realm */
	}

	/* do routing based on destination-realm AVP */
	if (msg->dest_realm->data.len==aaa_realm.len &&
	strncmp(msg->dest_realm->data.s, aaa_realm.s, aaa_realm.len) ) {
		/* I'm the destination realm */
		/* do I support the requested app_id? */
		if ( (mod=find_module(msg->applicationId))!=0) {
			/* I support the application localy */
			*p_chain = 0;
			*module = mod;
			return 0;
		} else {
			/* do I have a peer in same realm that supports this app-Id? */
			p = lookup_peer_by_realm_appid( &msg->dest_realm->data,
				msg->applicationId );
			if ( p ) {
				/* forward the request to this peer */
				*p_chain = &(p->pc);
				*module = 0;
				return 0;
			} else {
				return AAA_APPLICATION_UNSUPPORTED;
			}
		}
	} else {
		/* it's not my realm -> do routing based on script */
		*p_chain = 0;
		*module = 0;
		return 0;
	}

	return 1;
}



int get_dest_peers( AAAMessage *msg, struct peer_chain **chaine )
{
	LOG(L_ERR,"BUG:get_dest_peers: UNIMPLEMETED - we shouldn't get here!!\n");
	*chaine = 0;
	return 1;
}



void *server_worker(void *attr)
{
	str               buf;
	AAAMessage        *msg;
	AAAMessage        *ans;
	struct peer_chain *pc;
	struct aaa_module *mod;
	struct trans      *tr;
	struct peer       *in_peer;
	int               res;

	struct session *ses;

	while (1) {
		/* read a mesage from the queue */
		if (get_from_queue( &buf, &in_peer )==-1) {
			usleep(500);
			continue;
		}

		/* parse the message */
		msg = AAATranslateMessage( buf.s, buf.len );
		if (!msg) {
			LOG(L_ERR,"ERROR:server_worker: dropping message!\n");
			shm_free( buf.s );
			continue;
		}
		if (!msg->sessionId) {
			LOG(L_ERR,"ERROR:server_worker: message without sessionId AVP "
				"received -> droping!\n");
			shm_free( buf.s );
			continue;
		}

		/* process the message */
		if ( is_req( msg ) ) {
			/* request -> what should I do with it? */
			res = get_req_destination( msg, &pc, &mod );
			if (res!=0) {
				/* it was an error -> I have to send back an error answer */
				ans = AAANewMessage( msg->commandCode, msg->applicationId,
					0 /*session*/, msg );
				if (!ans) {
					LOG(L_ERR,"ERROR:server_worker: cannot create error answer"
						" back to client!\n");
				} else {
					if (AAASetMessageResultCode( ans, res)==AAA_ERR_SUCCESS ) {
						if (AAASendMessage( ans )!=AAA_ERR_SUCCESS )
							LOG(L_ERR,"ERROR:server_worker: unable to send "
								"answer back to client!\n");
					}else{
						LOG(L_ERR,"ERROR:server_worker: unable to set the "
							"error result code into the answer\n");
					}
					AAAFreeMessage( &ans );
				}
				AAAFreeMessage( &msg );
				continue;
			}
			/* build a transaction for it */
			tr = create_transaction( 0, FAKE_SESSION, in_peer);
			if (!tr) {
				LOG(L_ERR,"ERROR:server_worker: dropping request!\n");
				AAAFreeMessage( &msg );
				continue;
			}
			msg->trans = tr;
			/* process request */
			if (mod) {
				/* it's a local request */
				/* if the server is statefull, I have to call here the session
				 * state machine for the incoming request TO DO */
				msg->sId = &(msg->sessionId->data);
				/*run the handler for this module */
				((struct module_exports*)ses->app_ref)->mod_msg( msg, 0);
			} else {
				/* forward the request */
			}
		} else {
#if 0
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
#endif
		}
		/* free the mesage (along with the buffer) */
		AAAFreeMessage( &msg  );
	}

	return 0;
}

