/* 
 * $Id: route.h,v 1.5 2003/04/13 23:01:16 andrei Exp $
 */
/*
 *
 * History:
 * --------
 *  2003-04-08  created by andrei
 */


#ifndef route_h
#define route_h

#include "str.h"
#include "aaa_parse_uri.h"
#include "transport/peer.h"
#include "diameter_msg/diameter_msg.h"


struct peer_entry{
	struct aaa_uri uri;
	str full_uri;
	str alias;
	struct peer* peer; /* pointer to internal peer structure*/
	struct peer_entry *next;
};

extern struct peer_entry* cfg_peer_lst;



struct route_entry{
	str realm;
	struct peer_entry* peer_l; /* peer list */
	struct route_entry* next;
};

extern struct route_entry* route_lst;


int add_cfg_peer(str* uri, str* alias);
int add_route(str* realm, str* dst);
struct peer_entry* route_dest(str* realm);
int do_route(AAAMessage *msg, struct peer* in_peer);

/* destroy functions, call them to free the memory */
void destroy_cfg_peer_lst();
void destroy_route_lst();




#endif
