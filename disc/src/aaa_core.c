/*
 * $Id: aaa_core.c,v 1.5 2003/04/08 22:30:18 bogdan Exp $
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
#include "utils.h"
#include "msg_queue.h"
#include "aaa_module.h"
#include "cfg_init.h"
#include "route.h"



/* shared mem. size*/
unsigned int shm_mem_size=SHM_MEM_SIZE*1024*1024;

/* shm_mallocs log level */
int memlog=L_DBG;

/* default debuging level */
int debug=9;

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

/**/
unsigned int do_relay = 0;


#define AAAID_START          "aaa://"
#define AAAID_START_LEN      (sizeof(AAAID_START)-1)
#define TRANSPORT_PARAM      ";transport=tcp"
#define TRANSPORT_PARAM_LEN  (sizeof(TRANSPORT_PARAM)-1)


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

	/**/
	if (aaa_identity.s)
		shm_free(aaa_identity.s);

	/* destroy tge timer */
	destroy_timer();

	/* just for debuging */
	shm_status();
}



int init_aaa_core(char *cfg_file)
{
	void* shm_mempool;
	struct peer_entry* pl;

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
		if (add_peer(&pl->full_uri, &pl->uri.host, pl->uri.port_no)<0){
			fprintf(stderr, "ERROR: failed to add peer <%.*s>\n",
					pl->full_uri.len, pl->full_uri.s);
			goto error;
		}
	}

	/* init the message queue between transport layer and session one */
	if (init_msg_queue()==-1) {
		goto error;
	}

	return 1;
error:
	fprintf(stderr, "ERROR: cannot init the core\n");
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

