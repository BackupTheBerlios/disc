/*
 * $Id: client.h,v 1.1 2003/04/11 17:50:07 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */



#ifndef _AAA_DIAMETER_CLIENT_H_
#define _AAA_DIAMETER_CLIENT_H_

#include "diameter_msg/diameter_msg.h"

int init_all_peers_list();

void destroy_all_peers_list();

int get_all_peers( AAAMessage *msg, struct peer_chain **chaine );

void *client_worker(void *attr);

#endif

