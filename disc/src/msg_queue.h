/*
 * $Id: msg_queue.h,v 1.1 2003/04/01 11:35:00 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */


#ifndef _AAA_DIAMETER_MSG_QUEUE_
#define _AAA_DIAMETER_MSG_QUEUE_

#include "str.h"
#include "transport/peer.h"


int init_msg_queue();


void destroy_msg_queue();


int put_in_queue( str *buf, struct peer *p);


int get_from_queue(str *buf, struct peer **p);

#endif
