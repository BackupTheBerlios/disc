/*
 * $Id: client.c,v 1.1 2003/04/11 17:50:07 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */



#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "mem/shm_mem.h"
#include "diameter_api/diameter_api.h"
#include "transport/trans.h"
#include "transport/peer.h"
#include "dprint.h"
#include "timer.h"
#include "msg_queue.h"
#include "aaa_module.h"


static struct peer_chain *all_peers;



int init_all_peers_list()
{
	struct list_head  *lh;
	struct peer_chain *pc, *last;

	all_peers = 0;
	last = 0;

	list_for_each( lh, &(peer_table->peers) ) {
		pc = (struct peer_chain*)shm_malloc(sizeof(struct peer_chain));
		if (!pc) {
			LOG(L_ERR,"ERROR:init_dest_peers: no more free memory!\n");
			goto error;
	}
		/* add to list */
		pc->p = list_entry(lh, struct peer, all_peer_lh);
		pc->next = 0;
		if (!last) {
			all_peers = pc;
		} else {
			last->next = pc;
		}
		last = pc;
	}

	return 1;
error:
	return -1;
}



void destroy_all_peers_list()
{
	struct peer_chain *pc, *pc_foo;

	pc = all_peers;
	while(pc) {
		pc_foo = pc;
		pc = pc->next;
		shm_free( pc_foo);
	}
}



int get_all_peers( AAAMessage *msg, struct peer_chain **chaine )
{
	if (chaine)
		*chaine = all_peers;
	return 1;
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
				/* I notify the module by sending a ANSWER_TIMEOUT_EVENT */
				((struct module_exports*)ses->app_ref)->
					mod_tout( ANSWER_TIMEOUT_EVENT, &ses->sID, ses->context);
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

