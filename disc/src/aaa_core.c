/*
 * $Id: aaa_core.c,v 1.12 2003/04/13 23:01:16 andrei Exp $
 *
 * 2003-04-08 created by bogdan
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h> /* isprint */
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mem/shm_mem.h"
#include "diameter_api/diameter_api.h"
#include "transport/peer.h"
#include "transport/tcp_shell.h"
#include "server.h"
#include "client.h"
#include "globals.h"
#include "timer.h"
#include "utils.h"
#include "msg_queue.h"
#include "aaa_module.h"
#include "cfg_init.h"
#include "route.h"


#define CFG_FILE "aaa.cfg"


static char id[]="$Id: aaa_core.c,v 1.12 2003/04/13 23:01:16 andrei Exp $";
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
    -f file     Configuration file (default " CFG_FILE ")\n\
    -p port     Listen on the specified port (default 1812) \n\
    -6          Disable ipv6 \n\
    -d          Debugging mode (multiple -d increase the level)\n\
    -E          Log to stderr \n\
    -V          Version number \n\
    -h          This help message \n\
";


/* thread-id of the original thread */
pthread_t main_thread;

/* cfg file name*/
static char* cfg_file=CFG_FILE;

/* shared mem. size*/
unsigned int shm_mem_size=SHM_MEM_SIZE*1024*1024;

/* shm_mallocs log level */
int memlog=L_DBG;

/* default debuging level */
int debug=0;

/* use std error for loging - default value */
int log_stderr=1;

/* aaa identity */
str aaa_identity= {0, 0};

/* realm served */
str aaa_realm= {0, 0};

/* fqdn */
str aaa_fqdn= {0, 0 };

/* product name */
str product_name = {"AAA FokusFastServer",19};

/* vendor id */
unsigned int vendor_id = VENDOR_ID;

/* listening port */
unsigned int listen_port = DEFAULT_LISTENING_PORT;

/* if 1 do not use ipv6 */
int disable_ipv6=0;

/**/
unsigned int do_relay = 0;


/* my status - client, server, statefull server */
unsigned int my_aaa_status = AAA_UNDEFINED;



#define AAAID_START          "aaa://"
#define AAAID_START_LEN      (sizeof(AAAID_START)-1)
#define TRANSPORT_PARAM      ";transport=tcp"
#define TRANSPORT_PARAM_LEN  (sizeof(TRANSPORT_PARAM)-1)


static pthread_t *worker_id = 0;
static int nr_worker_threads = 0;

int (*send_local_request)(AAAMessage*,struct trans*);


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


void init_random_generator()
{
	unsigned int seed;
	int fd;

	/* init the random number generater by choosing a proper seed; first
	 * we try to read it from /dev/random; if it doesn't exist use a
	 * combination of current time and pid */
	seed=0;
	if ((fd=open("/dev/random", O_RDONLY))!=-1) {
		while(seed==0&&read(fd,(void*)&seed, sizeof(seed))==-1&&errno==EINTR);
		if (seed==0) {
			LOG(L_WARN,"WARNING:init_random_generator: could not read from"
				" /dev/random (%d)\n",errno);
		}
		close(fd);
	}else{
		LOG(L_WARN,"WARNING:init_random_generator: could not open "
			"/dev/random (%d)\n",errno);
	}
	seed+=getpid()+time(0);
	srand(seed);
}



int generate_aaaIdentity()
{
	char port_s[32];
	int  port_len;
	char *ptr;

	/* convert port into string */
	port_len = int2str( listen_port, port_s, 32 );

	/* compute the length */
	aaa_identity.len = AAAID_START_LEN + aaa_fqdn.len + 1/*:*/ +
		port_len + TRANSPORT_PARAM_LEN;

	/* allocate mem */
	aaa_identity.s = (char*)shm_malloc( aaa_identity.len );
	if (!aaa_identity.s) {
		LOG(L_CRIT,"ERROR:generate_aaaIdentity: no free memory -> cannot "
			"generate aaa_identity\n");
		return -1;
	}

	ptr = aaa_identity.s;
	memcpy( ptr, AAAID_START, AAAID_START_LEN );
	ptr += AAAID_START_LEN;

	memcpy( ptr, aaa_fqdn.s, aaa_fqdn.len );
	ptr += aaa_fqdn.len;

	*(ptr++) = ':';

	memcpy( ptr, port_s, port_len );
	ptr += port_len;

	memcpy( ptr, TRANSPORT_PARAM, TRANSPORT_PARAM_LEN );
	ptr += TRANSPORT_PARAM_LEN;

	LOG(L_INFO,"INFO:generate_aaaIdentity: [%.*s]\n",
		aaa_identity.len,aaa_identity.s);
	return 1;
}



int start_workers( void*(*worker)(void*), int nr_workers )
{
	int i;

	worker_id = (pthread_t*)shm_malloc( nr_workers*sizeof(pthread_t));
	if (!worker_id) {
		LOG(L_ERR,"ERROR:start_workers: cannot get free memory!\n");
		return -1;
	}

	for(i=0;i<nr_workers;i++) {
		if (pthread_create( &worker_id[i], /*&attr*/ 0, worker, 0)!=0){
			LOG(L_ERR,"ERROR:start_workers: cannot create worker thread\n");
			return -1;
		}
		nr_worker_threads++;
	}

	return 1;
}



void stop_workers()
{
	int i;

	if (worker_id) {
		for(i=0;i<nr_worker_threads;i++)
			pthread_cancel( worker_id[i]);
		shm_free( worker_id );
	}
}





void destroy_aaa_core()
{
	/* stop the worker threads */
	stop_workers();

	/* stop & destroy the modules */
	destroy_modules();

	/* destroy destination peers resolver */
	if (my_aaa_status==AAA_CLIENT) {
		/* something here? */
	} else {
		/* something for server ???? */
	}

	/* destroy the msg queue */
	destroy_msg_queue();

	/* close the libarary */
	AAAClose();

	/* stop the tcp layer */
	terminate_tcp_shell();

	/* destroy the transaction manager */
	destroy_trans_manager();

	/* destroy the peer manager */
	destroy_peer_manager( peer_table );

	/**/
	if (aaa_identity.s)
		shm_free(aaa_identity.s);

	/* destroy tge timer */
	destroy_timer();

	/* destroy route & peer lists */
	destroy_route_lst();
	destroy_cfg_peer_lst();

	/* free some globals*/
	if (aaa_realm.s) { shm_free(aaa_realm.s); aaa_realm.s=0; }
	if (aaa_fqdn.s) { shm_free(aaa_fqdn.s); aaa_fqdn.s=0; }

	/* just for debuging */
	shm_status();
	shm_mem_destroy();
}



int init_aaa_core(char *cfg_file)
{
	void* shm_mempool;
	struct peer_entry* pl;
	struct peer* pe;

	/* init mallocs */
	shm_mempool=malloc(shm_mem_size);
	if (shm_mempool==0){
		LOG(L_CRIT,"ERROR:main: initial malloc failed\n");
		exit(-1);
	};
	if (shm_mem_init_mallocs(shm_mempool, shm_mem_size)<0){
		LOG(L_CRIT,"ERROR:main: could not intialize shm. mallocs\n");
		exit(-1);
	};

	/* init rand() with a random as possible seed */
	init_random_generator();

	/* init modules loading */
	init_module_loading();

	/* read config file */
	if (read_config_file( cfg_file )!=0){
		/*fprintf(stderr, "bad config %s\n", cfg_file);*/
		goto error;
	}
	/* fix config stuff */
	if (listen_port==0) listen_port=DEFAULT_LISTENING_PORT;
	if (my_aaa_status==AAA_UNDEFINED) {
		LOG(L_CRIT,"ERROR:init_aaa_core: mandatory param \"aaa_status\" "
			"not found in config file\n");
		goto error;
	}
	if (my_aaa_status==AAA_SERVER)
		do_relay=1;

	/* build the aaa_identity based on FQDN and port */
	if ( generate_aaaIdentity()==-1 ) {
		goto error;
	}

	/* init the transaction manager */
	if (init_trans_manager()==-1) {
		goto error;
	}

	/* init the peer manager */
	if ( (peer_table=init_peer_manager(DEFAULT_TRANS_HASH_SIZE))==0) {
		goto error;
	}

	/* starts the transport layer - tcp */
	if (init_tcp_shell(DEFAULT_TCP_RECEIVE_THREADS)==-1) {
		goto error;
	}

	/* init the diameter library */
	if( AAAOpen("aaa_lib.cfg")!=AAA_ERR_SUCCESS ) {
		goto error;
	}

	/* add the peers from config file */
	if (cfg_peer_lst==0){
		fprintf(stderr, "ERROR: empty peer list\n");
		goto error;
	}
	for (pl=cfg_peer_lst; pl; pl=pl->next){
		if ((pe=add_peer(&pl->full_uri, &pl->uri.host, pl->uri.port_no))==0){
			fprintf(stderr, "ERROR: failed to add peer <%.*s>\n",
					pl->full_uri.len, pl->full_uri.s);
			goto error;
		}else{
			pl->peer=pe; /* remember it for do_route */
		}
	}

	/* init the message queue between transport layer and session one */
	if (init_msg_queue()==-1) {
		goto error;
	}

	/* starts the worker threads */
	if (my_aaa_status==AAA_CLIENT) {
		if (start_workers( client_worker, DEAFULT_WORKER_THREADS)==-1 )
			goto error;
	} else {
		if (start_workers( server_worker, DEAFULT_WORKER_THREADS)==-1 )
			goto error;
	}

	/* init the destestination peers resolver */
	if (my_aaa_status==AAA_CLIENT) {
		send_local_request = client_send_local_req;
	} else {
		/* something for server ???? */
		send_local_request = server_send_local_req;
	}

	return 1;
error:
	fprintf(stderr, "ERROR: cannot init the core\n");
	destroy_aaa_core();
	return -1;
}



static void sig_handler(int signo)
{
	if ( main_thread==pthread_self() ) {
		destroy_aaa_core();
		exit(0);
	}
	return;
}




int main(int argc, char *argv[])
{
	if (process_cmd_line(argc, argv)!=0)
		goto error;

	/* to remember which one was the original thread */
	main_thread = pthread_self();

	/* install signal handler */
	if (signal(SIGINT, sig_handler)==SIG_ERR) {
		printf("ERROR:main: cannot install signal handler\n");
		goto error;
	}

	/* init the aaa core */
	if ( init_aaa_core(cfg_file)==-1 )
		goto error;

	/* start the tcp shell */
	start_tcp_accept();

	/* start the modules */
	init_modules();

	/* for the last of the execution, I will act as timer */
	timer_ticker();

	/* this call is just a trick to force the linker to add this function */
	AAASendMessage(0); // !!!!!!!!!!!!!!!!!!

	return 0;
error:
	return -1;
}
