/*
 * $Id: trans.h,v 1.10 2003/04/07 15:17:51 bogdan Exp $
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

	/* request peer */
	struct peer *peer;
	/* request buffer */
	str *req;
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


void destroy_transaction( struct trans* );


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
	linker = cell_lookup_and_remove( p->trans_table, hash_code, hopbyhopID);
	if (linker)
		return get_hlink_payload( linker, struct trans, linker );
	return 0;
}


#endif
