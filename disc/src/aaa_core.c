/*
 * $Id: aaa_core.c,v 1.1 2003/04/08 13:29:28 bogdan Exp $
 *
 * 2003-04-08 created by bogdan
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "diameter_api/diameter_api.h"
#include "transport/peer.h"
#include "transport/tcp_shell.h"
#include "globals.h"
#include "mem/shm_mem.h"
#include "timer.h"
#include "msg_queue.h"
#include "aaa_module.h"
#include "cfg_init.h"



/* shared mem. size*/
unsigned int shm_mem_size=SHM_MEM_SIZE*1024*1024;

/* shm_mallocs log level */
int memlog=L_DBG;

/* default debuging level */
int debug=9;

/* use std error for loging - default value */
int log_stderr=1;

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
unsigned int do_relay = 0;





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



void destroy_aaa_core()
{
	/* stop & destroy the modules */
	destroy_modules();

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

	/* destroy tge timer */
	destroy_timer();

	/* just for debuging */
	shm_status();
}



int init_aaa_core(char *cfg_file)
{
	str aaa_id;
	str host;
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

	/* init rand() with a random as possible seed */
	init_random_generator();

	/* init modules loading */
	init_module_loading();

	/* read config file */
	if (read_config_file( cfg_file )!=0){
		fprintf(stderr, "bad config %s\n", cfg_file);
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
	//..................
	host.s   = "ugluk.mobis.fokus.gmd.de";
	host.len = strlen(host.s);
	aaa_id.s   = "aaa://ugluk.mobis.fokus.gmd.de:1812;transport=tcp";
	aaa_id.len = strlen( aaa_id.s );
	add_peer( &aaa_id, &host, 1812);

	/* init the message queue between transport layer and session one */
	if (init_msg_queue()==-1) {
		goto error;
	}

	return 1;
error:
	printf("ERROR: cannot init the core\n");
	destroy_aaa_core();
	return -1;
}



int start_aaa_core()
{
	/* start the tcp shell */
	start_tcp_accept();

	/* start the modules */
	init_modules();

	/* for the last of the execution, I will act as timer */
	timer_ticker();

	return 1;
}

