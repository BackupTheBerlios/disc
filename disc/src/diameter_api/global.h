/*
 * $Id: global.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2003-02-03 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_GLOBAL_H
#define _AAA_DIAMETER_GLOBAL_H

#include "utils/str.h"
#include "hash_table.h"
#include "peer.h"


/* for logging */
extern int debug;
extern int log_stderr;

/* hash_table */
extern struct h_table *hash_table;

/* peer table */
extern struct p_table *peer_table;

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

#endif

