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
//#include "worker.h"
//#include "dest.h"


#define CLIENT_CFG_FILE "aaa_server.cfg"


/* thread-id of the original thread */
pthread_t main_thread;

void destroy_aaa_server();




static void sig_handler(int signo)
{
	if ( main_thread==pthread_self() ) {
		destroy_aaa_server();
		destroy_aaa_core();
		exit(0);
	}
	return;
}


int init_aaa_server()
{
#if 0
	/* starts the worker threads */
	if (start_client_workers(DEAFULT_CLIENT_WORKER_THREADS)==-1 ) {
		goto error;
	}
	/* init the dest peers */
	if ( init_dest_peers()==-1 ) {
		goto error;
	}
#endif
	return 1;
error:
	printf("ERROR: cannot init the server\n");
	destroy_aaa_server();
	return -1;
}


void destroy_aaa_server()
{
	#if 0
	/* stop all the worker threads */
	stop_client_workers();
	/**/
	destroy_dest_peers();
	#endif

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

	/* init the aaa server */
	if ( init_aaa_server()==-1 ) {
		destroy_aaa_core();
		exit(-1);
	}

	/* start running */
	start_aaa_core();

	return 0;
}
