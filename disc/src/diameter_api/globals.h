/*
 * $Id: globals.h,v 1.1 2003/03/12 18:12:22 andrei Exp $
 *
 * 2003-02-03 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_GLOBAL_H
#define _AAA_DIAMETER_GLOBAL_H

#include "utils/str.h"
#include "hash_table.h"
#include "peer.h"

/* mallocs*/
extern unsigned int shm_mem_size;

/* for logging */
extern int debug;
extern int log_stderr;
extern int memlog;

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

/* */
extern unsigned int supported_auth_app_id;

/**/
unsigned int supported_acc_app_id;


#endif

