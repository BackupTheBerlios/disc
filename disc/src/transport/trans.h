/*
 * $Id: trans.h,v 1.8 2003/04/01 11:35:00 bogdan Exp $
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


/* search a transaction into the hash table based on endtoendID and hopbyhopID
 */
inline static struct trans* transaction_lookup(struct peer *p,
							unsigned int endtoendID, unsigned int hopbyhopID)
{
	str          s;
	unsigned int hash_code;
	struct trans *tr;

	s.s = (char*)&endtoendID;
	s.len = sizeof(endtoendID);
	hash_code = hash( &s , p->trans_table->hash_size );
	tr = (struct trans*)cell_lookup( p->trans_table, hash_code, hopbyhopID );
	if (tr)
		remove_cell_from_htable( p->trans_table, &(tr->linker) );
	return tr;
}


/* search a transaction into the hash table based on endtoendID and hopbyhopID
 */
inline static struct trans* transaction_lookup_safe(struct peer *p,
							unsigned int endtoendID, unsigned int hopbyhopID)
{
	str          s;
	unsigned int hash_code;
	struct trans *tr;

	s.s = (char*)&endtoendID;
	s.len = sizeof(endtoendID);
	hash_code = hash( &s , p->trans_table->hash_size );
	DBG("check point !!!!!!!!!!! \n");
	lock_get( p->mutex );
	tr = (struct trans*)cell_lookup( p->trans_table, hash_code, hopbyhopID );
	if (tr)
		remove_cell_from_htable( p->trans_table, &(tr->linker) );
	lock_release( p->mutex );
	return tr;
}


#endif
