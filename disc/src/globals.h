/*
 * $Id: globals.h,v 1.2 2003/03/14 20:32:10 bogdan Exp $
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

/* for logging */
extern int debug;
extern int log_stderr;

/* listening port for aaa connections */
extern unsigned int listen_port;

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

