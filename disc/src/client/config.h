/*
 * $Id: config.h,v 1.3 2003/04/01 11:35:00 bogdan Exp $
 */
/* History:
 * --------
 *  2003-03-12  created by andrei
 */

#ifndef _diameter_api_config_h
#define _diameter_api_config_h

/* size of the shared memory */
#define SHM_MEM_SIZE 1 /* in MB */

/* default listening port for tcp */
#define DEFAULT_LISTENING_PORT   1812

/* number of thash entries per table for the transaction hash_table */
#define DEFAULT_TRANS_HASH_SIZE   256

/* default number of receiving thread for tcp */
#define DEFAULT_TCP_RECEIVE_THREADS 4

/* default number of threads acting as client workers */
#define DEAFULT_CLIENT_WORKER_THREADS 4

#define VENDOR_ID   0x0000caca

#endif

