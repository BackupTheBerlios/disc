#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"
#include "../globals.h"
#include "../mem/shm_mem.h"
#include "../timer.h"
#include "../msg_queue.h"
#include "../transport/peer.h"
#include "../transport/tcp_shell.h"
#include "../aaa_module.h"

#include "../diameter_api/diameter_api.h"

#include "../cfg_init.h"

#include "worker.h"
#include "dest.h"


#define CLIENT_CFG_FILE "aaa_client.cfg"


/* shared mem. size*/
unsigned int shm_mem_size=SHM_MEM_SIZE*1024*1024;

/* shm_mallocs log level */
int memlog=L_DBG;

/* default debuging level */
int debug=9;

/* use std error for loging - default value */
int log_stderr=1;

/* thread-id of the original thread */
pthread_t main_thread;

/* aaa identity of the client */
str aaa_identity= {0, 0};

/* realm served by this client */
str aaa_realm= {0, 0};

/* fqdn of the client */
str aaa_fqdn= {0, 0 };

/* product name */
str product_name = {"AAA FokusFastServer",19};

/* vendor id */
unsigned int vendor_id = VENDOR_ID;

/* listening port */
unsigned int listen_port = DEFAULT_LISTENING_PORT;

/**/
unsigned int do_relay = 1;


void close_aaa_client();




static void sig_handler(int signo)
{
	if ( main_thread==pthread_self() ) {
		close_aaa_client();
		exit(0);
	}
	return;
}




void init_random_generator()
{
	unsigned int seed;
	int fd;
#include <netinet/in.h>
	/* init the random number generater by choosing a proper seed; first
	 * we try to read it from /dev/random; if it doesn't exist use a
	 * combination of current time and pid */
	seed=0;
	if ((fd=open("/dev/random", O_RDONLY))!=-1) {
		while(seed==0&&read(fd,(void*)&seed, sizeof(seed))==-1&&errno==EINTR);
		if (seed==0) {
			LOG(L_WARN,"WARNING:init_session_manager: could not read from"
				" /dev/random (%d)\n",errno);
		}
		close(fd);
	}else{
		LOG(L_WARN,"WARNING:init_session_manager: could not open "
			"/dev/random (%d)\n",errno);
	}
	seed+=getpid()+time(0);
	srand(seed);
}



int init_aaa_client()
{
	str aaa_id;
	str host;

	/* init modules loading */
	init_module_loading();

	/* read config file */
	if (read_config_file(CLIENT_CFG_FILE)!=0){
		fprintf(stderr, "bad config %s\n", CLIENT_CFG_FILE);
		goto error;
	}
	/* init the transaction manager */
	if (init_trans_manager()==-1) {
		goto error;
	}

	/* init the message queue between transport layer and session one */
	if (init_msg_queue()==-1) {
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

	if (start_client_workers(DEAFULT_CLIENT_WORKER_THREADS)==-1 ) {
		goto error;
	}

	if( AAAOpen("aaa_lib.cfg")!=AAA_ERR_SUCCESS ) {
		goto error;
	}

	/* add the peers from config file */
	//..................
	host.s   = "ugluk.mobis.fokus.gmd.de";
	host.len = strlen(host.s);
	aaa_id.s   = "aaa://ugluk.mobis.fokus.gmd.de:1812;transport=tcp";
	aaa_id.len = strlen( aaa_id.s );
	add_peer( &aaa_id, &host, 1812);

	/* init the dest peers */
	if ( init_dest_peers()==-1 ) {
		goto error;
	}

	/* start the tcp shell */
	start_tcp_accept();

	/* load a module */
/*
	load_module("client/modules/print/print");
*/
	init_modules();
	
	return 1;
error:
	printf("ERROR: cannot init the client\n");
	close_aaa_client();
	return -1;
}



void close_aaa_client()
{
	/* close the libarary */
	AAAClose();

	/* destroy the modules */
	destroy_modules();

	/* stop all the worker threads */
	stop_client_workers();

	/**/
	destroy_dest_peers();

	/* stop the tcp layer */
	terminate_tcp_shell();

	/* destroy the transaction manager */
	destroy_trans_manager();

	/* destroy the peer manager */
	destroy_peer_manager( peer_table );

	/* destroy the msg queue */
	destroy_msg_queue();

	/* destroy tge timer */
	destroy_timer();

	/* just for debuging */
	shm_status();

}



int main(int argc, char *argv[])
{
	void* shm_mempool;

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

	init_random_generator();

	/* to remember which one was the original thread */
	main_thread = pthread_self();

	/* install signal handler */
	if (signal(SIGINT, sig_handler)==SIG_ERR) {
		LOG(L_ERR,"ERROR:main: cannot install signal handler\n");
		exit(-1);
	}

	/* init the aaa client */
	if ( init_aaa_client()==-1 )
		exit(-1);

	/* for the last of the execution, I will act as timer */
	timer_ticker();

	return 0;
}
