/*
 * $Id: globals.h,v 1.6 2003/04/09 16:34:44 andrei Exp $
 *
 * 2003-02-03 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_GLOBAL_H
#define _AAA_DIAMETER_GLOBAL_H

#include "str.h"

/* for shm_mem */
extern int memlog;
extern unsigned int shm_mem_size;

/* listening port for aaa connections */
extern unsigned int listen_port;

/* if 1 disable ipv6 */
extern int disable_ipv6;

/* aaa_identity string */
extern str aaa_identity;

/**/
extern str aaa_realm;

/**/
extern str aaa_fqdn;

/* product name */
extern str product_name;

/* vendor-id */
extern unsigned int vendor_id;

extern unsigned int do_relay;


#endif

