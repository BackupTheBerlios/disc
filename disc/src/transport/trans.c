/*
 * $Id: trans.c,v 1.7 2003/04/08 12:08:20 bogdan Exp $
 *
 * 2003-02-11  created by bogdan
 * 2003-03-12  converted to shm_malloc/shm_free (andrei)
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../hash_table.h"
#include "../diameter_api/diameter_api.h"
#include "trans.h"



struct timer *tr_timeout_timer=0;



void timeout_handler(unsigned int ticks, void* param);


int init_trans_manager()
{
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



struct trans* create_transaction(str *buf, struct session *ses, struct peer *p)
{
	struct trans *t;

	t = (struct trans*)shm_malloc(sizeof(struct trans));
	if (!t) {
		LOG(L_ERR,"ERROR:create_transaction: no more free memory!\n");
		goto error;
	}
	memset(t,0,sizeof(struct trans));

	/* link the session */
	t->ses = ses;
	/* init the timer_link for timeout */
	t->timeout.payload = t;
	/* link the outging request */
	t->req = buf;
	/* link the outgoing peer */
	t->peer = p;

	return t;
error:
	return 0;
}



void destroy_transaction( struct trans *tr )
{
	if (!tr) {
		LOG(L_ERR,"ERROR:destroy_transaction: null parameter received!\n");
		return;
	}

	/* timer */
	if (tr->timeout.timer_list)
		rmv_from_timer_list( &(tr->timeout) );

	/* free the structure */
	shm_free(tr);
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
			session_state_machine( tr->ses, AAA_SESSION_REQ_TIMEOUT, 0);
		}else{
			write_command( tr->peer->fd, TIMEOUT_PEER_CMD,
				PEER_TR_TIMEOUT, tr->peer, (void*)tr->info);
		}
		destroy_transaction( tr );
	}


}
