/*
 * $Id: worker.c,v 1.3 2003/04/10 21:40:03 bogdan Exp $
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

#define I_AM_FOREIGN_SERVER      1
#define I_AM_NOT_FOREIGN_SERVER  0


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
		// TO DO
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



inline void send_error_reply(AAAMessage *msg, unsigned int response_code)
{
	AAAMessage *ans;

	ans = AAANewMessage( msg->commandCode, msg->applicationId,
		0 /*session*/, msg );
	if (!ans) {
		LOG(L_ERR,"ERROR:send_error_reply: cannot create error answer"
			" back to client!\n");
	} else {
		if (AAASetMessageResultCode( ans, response_code)==AAA_ERR_SUCCESS ) {
			if (AAASendMessage( ans )!=AAA_ERR_SUCCESS )
				LOG(L_ERR,"ERROR:send_error_reply: unable to send "
					"answer back to client!\n");
		}else{
			LOG(L_ERR,"ERROR:send_error_reply: unable to set the "
				"error result code into the answer\n");
		}
		AAAFreeMessage( &ans );
	}
}



void *server_worker(void *attr)
{
	str               buf;
	AAAMessage        *msg;
	struct peer_chain *pc;
	struct aaa_module *mod;
	struct trans      *tr;
	struct peer       *in_peer;
	int               res;

	while (1) {
		/* read a mesage from the queue */
		if (get_from_queue( &buf, &in_peer )==-1) {
			usleep(500);
			continue;
		}

		/* process the message */
		if ( is_req( msg ) ) {
			/* request*/
			msg = AAATranslateMessage( buf.s, buf.len );
			if (!msg) {
				LOG(L_ERR,"ERROR:server_worker: dropping message!\n");
				shm_free( buf.s );
				continue;
			}
			if (!msg->sessionId || !msg->orig_host || !msg->orig_realm ||
			!msg->dest_realm) {
				LOG(L_ERR,"ERROR:server_worker: message without sessionId/"
					"OriginHost/OriginRealm/DestinationRealm AVP received "
					"-> droping!\n");
				send_error_reply( msg, AAA_MISSING_AVP);
				AAAFreeMessage( &msg );
				continue;
			}
			/*-> what should I do with it? */
			res = get_req_destination( msg, &pc, &mod );
			if (res!=0) {
				/* it was an error -> I have to send back an error answer */
				send_error_reply( msg, res);
				AAAFreeMessage( &msg );
				continue;
			}
			/* process the request */
			if (mod) {
				/* it's a local request */
				/* if the server is statefull, I have to call here the session
				 * state machine for the incoming request TO DO */
				msg->sId = &(msg->sessionId->data);
				/*run the handler for this module */
				mod->exports->mod_msg( msg, 0);
			} else {
				/* forward the request -> build a transaction for it */
				tr = create_transaction( &(msg->buf), in_peer);
				if (!tr) {
					LOG(L_ERR,"ERROR:server_worker: dropping request!\n");
					AAAFreeMessage( &msg );
					continue;
				}
				/* remember the received hop_by_hop-Id */
				tr->orig_hopbyhopId = msg->hopbyhopId;
				/* am I Foreign server for this request? */
				if ( msg->orig_host->data.len==in_peer->aaa_identity.len &&
				!strncmp(msg->orig_host->data.s,in_peer->aaa_identity.s,
				in_peer->aaa_identity.len) )
					tr->info = I_AM_FOREIGN_SERVER;
				else
					tr->info = I_AM_NOT_FOREIGN_SERVER;
				/* send it out */
				for(;pc;pc=pc->next) {
					if ( (res=send_req_to_peer( tr, pc->p))!=-1) {
						/* success -> start the timeout timer */
						add_to_timer_list( &(tr->timeout) , tr_timeout_timer ,
							get_ticks()+TR_TIMEOUT_TIMEOUT );
						break;
					}
				}
				tr->req = 0;
				if (res==-1) {
					LOG(L_ERR,"ERROR:server_worker: unable to forward "
						"request\n");
					send_error_reply( msg, AAA_UNABLE_TO_DELIVER);
				}
			}
			/* free the mesage (along with the buffer) */
			AAAFreeMessage( &msg  );
		} else {
			/* response -> performe transaction lookup and remove it from 
			 * hash table */
			tr = transaction_lookup(in_peer,msg->endtoendId,msg->hopbyhopId);
			if (!tr) {
				LOG(L_ERR,"ERROR:server_worker: unexpected answer received "
					"(without sending any request)!\n");
				shm_free( buf.s );
				continue;
			}
			/* stop the transaction timeout */
			rmv_from_timer_list( &(tr->timeout) );
			/* parse the reply */
			msg = AAATranslateMessage( buf.s, buf.len );
			if (!msg) {
				LOG(L_ERR,"ERROR:server_worker: dropping message!\n");
				shm_free( buf.s );
				continue;
			}
			if (tr->in_peer) {
				/* I have to forward the reply downstream */
				msg->hopbyhopId = tr->orig_hopbyhopId;
				((unsigned int*)msg->buf.s)[3] = tr->orig_hopbyhopId;
				/* am I Foreign server for this reply? */
				if (tr->info==I_AM_FOREIGN_SERVER) {
					mod = find_module(msg->applicationId);
					if (mod)
						mod->exports->mod_msg( msg, 0);
				}
				/* send the rely */
				if (send_res_to_peer( &msg->buf, tr->in_peer)==-1) {
					LOG(L_ERR,"ERROR:server_worker: forwarding reply "
						"failed\n");
				}
			} else {
				/* look like being a reply to a local request */
				LOG(L_ERR,"BUG:server_worker: some reply to a local request "
					"received; but I don't send requests!!!!\n");
			}
			/* destroy the transaction */
			destroy_transaction( tr );
			/* free the mesage (along with the buffer) */
			AAAFreeMessage( &msg  );
		}
	}

	return 0;
}

