/*
 * $Id: client.h,v 1.2 2003/04/12 20:53:50 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */



#ifndef _AAA_DIAMETER_CLIENT_H_
#define _AAA_DIAMETER_CLIENT_H_

#include "diameter_msg/diameter_msg.h"
#include "transport/trans.h"

int client_send_local_req( AAAMessage *msg, struct trans *tr );

void *client_worker(void *attr);

#endif

