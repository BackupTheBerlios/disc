/*
 * $Id: script.h,v 1.1 2003/03/28 14:20:43 bogdan Exp $
 *
 * 2003-03-27 created by bogdan
 */

#ifndef _AAA_DIAMETER_SCRIPT_H
#define _AAA_DIAMETER_SCRIPT_H


#include "transport/peer.h"


int init_script();

int run_script( struct peer_chaine **chaine );

#endif

