/*
 * $Id: trans.h,v 1.9 2003/04/04 16:59:25 bogdan Exp $
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
	struct h_link  linker;
	/* session the request belong to - if any */
	struct session *ses;
	/* info - can be used for different purposes */
	unsigned int info;

	/* outgoing request peer */
	struct peer *peer;
	/* outgoing request buffer */
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
	struct h_link *linker;
	str           s;
	unsigned int  hash_code;

	s.s = (char*)&endtoendID;
	s.len = sizeof(endtoendID);
	hash_code = hash( &s , p->trans_table->hash_size );
	linker = cell_lookup( p->trans_table, hash_code, hopbyhopID);
	if (linker) {
		remove_cell_from_htable( p->trans_table, linker );
		return ((struct trans*)((char *)(linker) -
			(unsigned long)(&((struct trans*)0)->linker)));
	}
	return 0;
}


/* search a transaction into the hash table based on endtoendID and hopbyhopID
 */
inline static struct trans* transaction_lookup_safe(struct peer *p,
							unsigned int endtoendID, unsigned int hopbyhopID)
{
	struct h_link *linker;
	str           s;
	unsigned int  hash_code;

	s.s = (char*)&endtoendID;
	s.len = sizeof(endtoendID);
	hash_code = hash( &s , p->trans_table->hash_size );
	lock_get( p->mutex );
	linker = cell_lookup( p->trans_table, hash_code, hopbyhopID );
	if (linker) {
		remove_cell_from_htable( p->trans_table, linker );
		lock_release( p->mutex );
		return ((struct trans*)((char *)(linker) -
			(unsigned long)(&((struct trans*)0)->linker)));
	}
	lock_release( p->mutex );
	return 0;
}


#endif
