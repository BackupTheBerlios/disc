/*
 * $Id: main.c,v 1.25 2003/04/09 16:34:44 andrei Exp $
 *
 */
/* History:
 * --------
 *  2003-04-09  added cmd line processing (andrei)
 */



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* isprint */
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
#include "../dprint.h"


#define CLIENT_CFG_FILE "aaa_client.cfg"


static char id[]="$Id: main.c,v 1.25 2003/04/09 16:34:44 andrei Exp $";
static char version[]= NAME " " VERSION " (" ARCH "/" OS ")" ;
static char compiled[]= __TIME__ " " __DATE__;
static char flags[]=""
#ifdef USE_IPV6
"USE_IPV6 "
#endif
#ifdef NO_DEBUG
"NO_DEBUG "
#endif
#ifdef NO_LOG
"NO_LOG "
#endif
#ifdef EXTRA_DEBUG
"EXTRA_DEBUG "
#endif
#ifdef DNS_IP_HACK
"DNS_IP_HACK "
#endif
#ifdef SHM_MEM
"SHM_MEM "
#endif
#ifdef PKG_MALLOC
"PKG_MALLOC "
#endif
#ifdef VQ_MALLOC
"VQ_MALLOC "
#endif
#ifdef F_MALLOC
"F_MALLOC "
#endif
#ifdef DBG_QM_MALLOC
"DBG_QM_MALLOC "
#endif
#ifdef FAST_LOCK
"FAST_LOCK"
#ifdef BUSY_WAIT
"-BUSY_WAIT"
#endif
#ifdef ADAPTIVE_WAIT
"-ADAPTIVE_WAIT"
#endif
#ifdef NOSMP
"-NOSMP"
#endif
" "
#endif /*FAST LOCK*/
;


static char help_msg[]= "\
Usage: " NAME " -f file   \n\
Options:\n\
    -f file     Configuration file (default " CLIENT_CFG_FILE ")\n\
    -p port     Listen on the specified port (default 1812) \n\
    -6          Disable ipv6 \n\
    -d          Debugging mode (multiple -d increase the level)\n\
    -E          Log to stderr \n\
    -V          Version number \n\
    -h          This help message \n\
"
;



/* thread-id of the original thread */
pthread_t main_thread;
static char* cfg_file=CLIENT_CFG_FILE; /* cfg file name*/

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


/* 0 on success, -1 on error */
static int process_cmd_line(int argc, char** argv)
{
	char c;
	int port;
	char* tmp;
	
	opterr=0;
	while((c=getopt(argc, argv, "f:p:6VhEd"))!=-1){
		switch(c){
			case 'f':
				cfg_file=optarg;
				break;
			case  'd':
				debug++;
				break;
			case 'p':
				port=strtol(optarg, &tmp, 10);
				if ((tmp==0)||(*tmp)){
					fprintf(stderr, "bad port number: -p %s\n", optarg);
					goto error;
				}
				break;
			case 'E':
				log_stderr=1;
				break;
			case '6':
				disable_ipv6=1;
				break;
			case 'V':
				printf("version: %s\n", version);
				printf("flags: %s\n", flags );
				printf("%s\n", id);
				printf("%s compiled on %s with %s\n", __FILE__,
							compiled, COMPILER );
				exit(0);
				break;
			case 'h':
				printf("version: %s\n", version);
				printf("%s", help_msg);
				exit(0);
				break;
			case '?':
				if (isprint(optopt))
					fprintf(stderr, "Unknown option `-%c´\n", optopt);
				else
					fprintf(stderr, "Unknown character `\\x%x´\n", optopt);
				goto error;
				break;
			case ':':
				fprintf(stderr, "Option `-%c´ requires an argument.\n",
						optopt);
				goto error;
				break;
			default:
				/* we should never reach this */
				if (isprint(c))
					fprintf(stderr, "Unknown option `-%c´\n", c);
				else
					fprintf(stderr, "Unknown option `-\\x%x´\n", c);
				goto error;
				break;
		}
	}
	return 0;
error:
	return -1;
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
	
	
	if (process_cmd_line(argc, argv)!=0) goto error;
	/* to remember which one was the original thread */
	main_thread = pthread_self();

	/* install signal handler */
	if (signal(SIGINT, sig_handler)==SIG_ERR) {
		printf("ERROR:main: cannot install signal handler\n");
		goto error;
	}

	/* init the aaa core */
	if ( init_aaa_core(cfg_file)==-1 )
		goto error;;

	/* init the aaa client */
	if ( init_aaa_client()==-1 ) {
		destroy_aaa_core();
		goto error;;
	}

	/* start running */
	start_aaa_core();

	return 0;
error:
	return -1;
}
