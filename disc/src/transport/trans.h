/*
 * $Id: trans.h,v 1.6 2003/03/17 19:10:55 bogdan Exp $
 *
 * 2003-02-11 created by bogdan
 *
 */

#ifndef _AAA_DIAMETER_TRANS_H
#define _AAA_DIAMETER_TRANS_H

#include "../str.h"
#include "../dprint.h"
#include "../hash_table.h"
#include "../timer.h"
//#include "diameter_types.h"

struct trans;

#include "peer.h"
#include "../diameter_api/session.h"


struct trans {
	/* COMMON PART */
	/* linker inside the hash table - must be the first */
	struct h_link  linker;
	/* session the request belong to - if any */
	struct session *ses;
	/* info - can be used for different purposes */
	unsigned int info;

	/* SERVER SIDE */
	/* outgoing peer ID */
	struct peer *peer;
	/* outgoing request */
	str req;
	/* timeout timer */
	struct timer_link timeout;
};

#define TRANS_SEVER   1<<0
#define TRANS_CLIENT  1<<1


#define TR_TIMEOUT_TIMEOUT   5
extern struct timer *tr_timeout_timer;


extern struct timer *tr_timer;


int init_trans_manager();


void destroy_trans_manager();


struct trans* create_transaction( str*, struct session*, struct peer*);


void destroy_transaction( void* );


/* search into hash table a transaction, based on 
 */
inline static struct trans* transaction_lookup(struct h_table *trans_table,
							unsigned int endtoendID, unsigned int hopbyhopID)
{
	str          s;
	unsigned int hash_code;

	s.s = (char*)&endtoendID;
	s.len = sizeof(endtoendID);
	hash_code = hash( &s , trans_table->hash_size );
	return (struct trans*)cell_lookup( trans_table, hash_code, hopbyhopID );
}

#endif
