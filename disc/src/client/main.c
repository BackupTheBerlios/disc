#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../globals.h"
#include "../aaa_core.h"
#include "worker.h"
#include "dest.h"


#define CLIENT_CFG_FILE "aaa_client.cfg"


/* thread-id of the original thread */
pthread_t main_thread;

void destroy_aaa_client();




static void sig_handler(int signo)
{
	if ( main_thread==pthread_self() ) {
		destroy_aaa_client();
		destroy_aaa_core();
		exit(0);
	}
	return;
}


int init_aaa_client()
{
	/* starts the worker threads */
	if (start_client_workers(DEAFULT_CLIENT_WORKER_THREADS)==-1 ) {
		goto error;
	}
	/* init the dest peers */
	if ( init_dest_peers()==-1 ) {
		goto error;
	}
	return 1;
error:
	printf("ERROR: cannot init the client\n");
	destroy_aaa_client();
	return -1;
}


void destroy_aaa_client()
{
	/* stop all the worker threads */
	stop_client_workers();
	/**/
	destroy_dest_peers();
}



int main(int argc, char *argv[])
{
	/* to remember which one was the original thread */
	main_thread = pthread_self();

	/* install signal handler */
	if (signal(SIGINT, sig_handler)==SIG_ERR) {
		printf("ERROR:main: cannot install signal handler\n");
		exit(-1);
	}

	/* init the aaa core */
	if ( init_aaa_core(CLIENT_CFG_FILE)==-1 )
		exit(-1);

	/* init the aaa client */
	if ( init_aaa_client()==-1 ) {
		destroy_aaa_core();
		exit(-1);
	}

	/* start running */
	start_aaa_core();

	return 0;
}
