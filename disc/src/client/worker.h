/*
 * $Id: worker.h,v 1.2 2003/04/08 13:29:28 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */



#ifndef _AAA_DIAMETER_CLIENT_WORKER_H_
#define _AAA_DIAMETER_CLIENT_WORKER_H_

/* default number of threads acting as client workers */
#define DEAFULT_CLIENT_WORKER_THREADS 4

int start_client_workers( int nr_workers );

void stop_client_workers();

#endif

