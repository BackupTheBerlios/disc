/* 
 * $Id: route.h,v 1.3 2003/04/08 22:06:24 andrei Exp $
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


struct peer_entry{
	struct aaa_uri uri;
	str full_uri;
	str alias;
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




#endif
