/*
 * $Id: route.h,v 1.1 2003/03/28 14:20:43 bogdan Exp $
 *
 * 2003-03-27 created by bogdan
 */

#ifndef _AAA_DIAMETER_ROUTE_H
#define _AAA_DIAMETER_ROUTE_H

#include "transport/peer.h"
#include "diameter_api/diameter_types.h"



/*
 * Returns:
 *    1 : the message must be forwarded to the returned peers (non NULL);
 *    2 : the mesage is for local processing;
 *    0 : bogus message
 *   -1 : error;
 */
int do_routing( AAAMessage *msg, struct peer_chaine **pc );


#endif

