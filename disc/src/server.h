/*
 * $Id: server.h,v 1.1 2003/04/11 17:50:07 bogdan Exp $
 *
 * 2003-04-08 created by bogdan
 */



#ifndef _AAA_DIAMETER_SERVER_H_
#define _AAA_DIAMETER_SERVER_H_

#include "diameter_msg/diameter_msg.h"

int route_local_req( AAAMessage *msg, struct peer_chain **chaine );

void *server_worker(void *attr);

#endif

