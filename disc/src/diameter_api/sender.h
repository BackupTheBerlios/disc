/*
 * $Id: sender.h,v 1.2 2003/04/08 22:30:18 bogdan Exp $
 *
 * 2003-02-07 created by bogdan
 * 2003-03-13 converted to locking.h/gen_lock_t (andrei)
 *
 */

#ifndef _AAA_DIAMETER_SENDER_H
#define _AAA_DIAMETER_SENDER_H

#include "../locking.h"
#include "../transport/peer.h"
#include "session.h"
#include "diameter_api.h"


struct send_manager {
	unsigned int  end_to_end;
	gen_lock_t    *end_to_end_lock;
};



int init_send_manager();

void destroy_send_manager();

int send_request( AAAMessage *msg );

int send_response( AAAMessage *msg);

int get_dest_peers( AAAMessage *msg, struct peer_chain **pc );

#endif

