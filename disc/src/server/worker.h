/*
 * $Id: worker.h,v 1.1 2003/04/08 22:30:18 bogdan Exp $
 *
 * 2003-04-08 created by bogdan
 */



#ifndef _AAA_DIAMETER_SERVER_WORKER_H_
#define _AAA_DIAMETER_SERVER_WORKER_H_

/* default number of threads acting as client workers */
#define DEAFULT_SERVER_WORKER_THREADS 4

int start_server_workers( int nr_workers );

void stop_server_workers();

#endif

