/*
 * $Id: msg_queue.h,v 1.2 2003/04/09 18:12:44 andrei Exp $
 *
 * 2003-03-31 created by bogdan
 */


#ifndef _AAA_DIAMETER_MSG_QUEUE_H
#define _AAA_DIAMETER_MSG_QUEUE_H

#include "str.h"
#include "transport/peer.h"


int init_msg_queue();


void destroy_msg_queue();


int put_in_queue( str *buf, struct peer *p);


int get_from_queue(str *buf, struct peer **p);

#endif
