/*
 * $Id: message.h,v 1.12 2003/04/01 12:03:16 bogdan Exp $
 *
 * 2003-02-07 created by bogdan
 * 2003-03-13 converted to locking.h/gen_lock_t (andrei)
 *
 */

#ifndef _AAA_DIAMETER_MESSAGE_H
#define _AAA_DIAMETER_MESSAGE_H

#include "../locking.h"
#include "diameter_types.h"
#include "../transport/peer.h"
#include "session.h"
#include "../transport/trans.h"


struct msg_manager {
	unsigned int  hop_by_hop;
	gen_lock_t     *hop_by_hop_lock;
	unsigned int  end_to_end;
	gen_lock_t     *end_to_end_lock;
};


#define is_req(_msg_) \
	(((_msg_)->flags)&0x80)




int init_msg_manager();

void destroy_msg_manager();

void print_aaa_message( AAAMessage *msg);

int send_request( AAAMessage *msg );

int send_response( AAAMessage *msg);

int get_dest_peers( AAAMessage *msg, struct peer_chaine **pc );

#endif

