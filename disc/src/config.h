/*
 * $Id: config.h,v 1.7 2003/04/14 15:52:08 andrei Exp $
 */
/* History:
 * --------
 *  2003-03-12  created by andrei
 */

#ifndef _diameter_api_config_h
#define _diameter_api_config_h

#define MODULE_SEARCH_PATH "client/modules"

#define SHM_MEM_SIZE 1*1024*1024 /* 1 MB */

/* default listening port for tcp */
#define DEFAULT_LISTENING_PORT   1812

/* number of thash entries per table for the transaction hash_table */
#define DEFAULT_TRANS_HASH_SIZE   256

/* default number of receiving thread for tcp */
#define DEFAULT_TCP_RECEIVE_THREADS 4

/* default number of working threads */
#define DEAFULT_WORKER_THREADS  4

#define MAX_REALM_LEN 128


#define VENDOR_ID   0x0000caca

#endif

