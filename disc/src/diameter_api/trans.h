/*
 * $Id: trans.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2003-02-11 created by bogdan
 *
 */

#ifndef _AAA_DIAMETER_TRANS_H
#define _AAA_DIAMETER_TRANS_H

#include "utils/str.h"
#include "utils/dprint.h"
#include "hash_table.h"
#include "timer.h"
#include "diameter_types.h"

struct trans;

#include "peer.h"
#include "session.h"


struct trans {
	/* COMMON PART */
	/* linker inside the hash table - must be the first */
	struct h_link  linker;
	/* session the request belong to - if any */
	struct session *ses;

	/* CLIENT SIDE */
	/* incoming peer ID */
	struct peer *in_peer;
	/* incoming request */
	AAAMessage *in_req;

	/* SERVER SIDE */
	/* outgoing peer ID */
	struct peer *out_peer;
	/* outgoing request */
	AAAMessage *out_req;
	/* timeout timer */
	struct timer_link timeout;
};

#define TR_INCOMING_REQ  1<<0
#define TR_OUTGOING_REQ  1<<1


#define TR_TIMEOUT_TIMEOUT   5
extern struct timer *tr_timeout_timer;


extern struct timer *tr_timer;


int init_trans_manager();


void destroy_trans_manager();


struct trans* create_transaction(AAAMessage*, struct session*, struct peer*,
															unsigned char);


void destroy_transaction( void* );


/* search into hash table a transaction, based on 
 */
inline static struct trans* transaction_lookup(struct h_table *table,
										AAAMessage *msg, int rm)
{
	str          s;
	unsigned int hash_code;

	s.s = (char*)&msg->endtoendID;
	s.len = sizeof(msg->endtoendID);
	hash_code = hash( &s );
	return (struct trans*)cell_lookup
		( table, hash_code, msg->hopbyhopID, TRANSACTION_CELL_TYPE, rm);
}

#endif
