/*
 * $Id: trans.c,v 1.3 2003/03/10 19:17:32 bogdan Exp $
 *
 * 2003-02-11 created by bogdan
 *
 */

#include <stdlib.h>
#include <string.h>
#include "utils/dprint.h"
#include "utils/counter.h"
#include "trans.h"
#include "hash_table.h"
#include "global.h"
#include "diameter_api.h"


struct timer *tr_timeout_timer=0;



void timeout_handler(unsigned int ticks, void* param);


int init_trans_manager()
{
	/* register a destroy function for transactions */
	register_destroy_func( hash_table, TRANSACTION_CELL_TYPE,
		destroy_transaction);

	/* build the timer list for transaction timeout */
	tr_timeout_timer = new_timer_list();
	if (!tr_timeout_timer) {
		LOG(L_ERR,"ERROR:init_trans_manager: cannot build timer\n");
		goto error;
	}

	/* register timer function */
	if (register_timer( timeout_handler, tr_timeout_timer, 1)==-1) {
		LOG(L_ERR,"ERROR:init_trans_manager: cannot register time handler\n");
		goto error;	}

	LOG(L_INFO,"INFO:init_trans_manager: transaction manager started!\n");
	return 1;
error:
	return -1;
}



void destroy_trans_manager()
{
	if (tr_timeout_timer)
		destroy_timer_list( tr_timeout_timer );
	LOG(L_INFO,"INFO:init_trans_manager: transaction manager stoped!\n");
}



struct trans* create_transaction( AAAMessage *msg, struct session *ses,
										struct peer *pr, unsigned char flag )
{
	struct trans *t;

	t = (struct trans*)malloc(sizeof(struct trans));
	if (!t) {
		LOG(L_ERR,"ERROR:create_transaction: no more free memory!\n");
		goto error;
	}
	memset(t,0,sizeof(struct trans));

	/* link the session */
	t->ses = ses;
	/* init the timer_link for timeout */
	t->timeout.payload = t;
	/* link request and peer */
	if (flag&TR_INCOMING_REQ) {
		/* link the incoming request */
		t->in_req = msg;
		/* link the incoming peer */
		atomic_inc( &(pr->ref_cnt) );
		t->in_peer = pr;
	} else {
		/* link the outgoing request */
		t->out_req = msg;
		/* link the outgoing peer */
		atomic_inc( &(pr->ref_cnt) );
		t->out_peer = pr;
	}

	return t;
error:
	return 0;
}



void destroy_transaction( void *vp)
{
	struct trans *t=(struct trans*)vp;

	if (!t) {
		LOG(L_ERR,"ERROR:destroy_transaction: null parameter received!\n");
		return;
	}

	/* unref the peers */
	if (t->out_peer)
		atomic_dec( &(t->out_peer->ref_cnt) );
	if (t->in_peer)
		atomic_dec( &(t->in_peer->ref_cnt) );

	/* unlink the transaction from hash_table */
	remove_cell_from_htable( hash_table, (struct h_link*)t );

	/* free the messages */
	if (t->in_req)
		AAAFreeMessage( &(t->in_req) );
	if (t->out_req)
		AAAFreeMessage( &(t->out_req) );
	/* timer */
	if (t->timeout.timer_list)
		rmv_from_timer_list( &(t->timeout) );

	/* free the structure */
	free(t);
}




void timeout_handler(unsigned int ticks, void* param)
{
	struct timer_link *tl;
	struct trans *tr;

	/* TIMEOUT TIMER LIST */
	/* get the expired transactions */
	tl = get_expired_from_timer_list( tr_timeout_timer, ticks);
	while (tl) {
		tr  = (struct trans*)tl->payload;
		DBG("DEBUG:timeout_handler: transaction %p expired!\n",tr);
		tl = tl->next_tl;
		/* process the transaction */
		if (tr->ses) {
			//session_state_machine();
		}else{
			write_command( tr->out_peer->fd, TIMEOUT_PEER_CMD,
				PEER_TR_TIMEOUT, tr->out_peer, 0);
			//peer_state_machine( tr->out_peer, PEER_TIMEOUT, 0);
		}
		destroy_transaction( tr );
	}


}
