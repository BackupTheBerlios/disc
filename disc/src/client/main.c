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
#include "../sh_mutex.h"
#include "../timer.h"
#include "../script.h"
#include "../transport/peer.h"
#include "../transport/tcp_shell.h"
#include "../aaa_module.h"



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

/* lsitening port */
unsigned int listen_port = DEFAULT_LISTENING_PORT;

/* supported auth. applications */
unsigned int supported_auth_app_id =
	(1<<AAA_APP_RELAY) | (1<<AAA_APP_MOBILE_IP);

/* supported acc. applications */
unsigned int supported_acc_app_id = 
	(1<<AAA_APP_RELAY) ;



void close_client();




static void sig_handler(int signo)
{
	if ( main_thread==pthread_self() ) {
		close_client();
		exit(0);
	}
	return;
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




int init_client()
{
	void* shm_mempool;

	/* init mallocs */
	shm_mempool=malloc(shm_mem_size);
	if (shm_mempool==0){
		LOG(L_CRIT,"ERROR:AAAOpen: initial malloc failed\n");
		goto error;
	};
	if (shm_mem_init_mallocs(shm_mempool, shm_mem_size)<0){
		LOG(L_CRIT,"ERROR:AAAOpen: could not intialize shm. mallocs\n");
		goto error;
	};

	init_random_generator();

	/* init the shared mutexes */
	if ( (init_shared_mutexes())==0)
		goto error;

	/**/
	main_thread = pthread_self();

	/* install signal handler */
	if (signal(SIGINT, sig_handler)==SIG_ERR) {
		LOG(L_ERR,"ERROR:init_client: cannot install signal handler\n");
		goto error;
	}

	/* read config file */
	aaa_identity.s = "aaa://fesarius.fokus.gmd.de:1812;transport=tcp";
	aaa_identity.len = strlen(aaa_identity.s);
	aaa_realm.s = "fokus.gmd.de";
	aaa_realm.len = strlen(aaa_realm.s);

	/* init the peer manager */
	if ( (peer_table=init_peer_manager(DEFAULT_TRANS_HASH_SIZE))==0) {
		goto error;
	}

	/* init the transaction manager */
	if (init_trans_manager()==-1) {
		goto error;
	}

	/* init the script */
	if ( init_script()==-1 ) {
		goto error;
	}

	/* starts the transport layer - tcp */
	if (init_tcp_shell(DEFAULT_TCP_RECEIVE_THREADS))

	//if( AAAOpen("aaa_lib.cfg")!=AAA_ERR_SUCCESS ) {
	//	return -1;
	//}

	/* start the timer */
	init_timer();

	/* init modules loading */
	init_module_loading();
	/* load a module */
	load_module("client/modules/print/print");
	init_modules();
	

	return 1;
error:
	printf("ERROR: cannot init the client\n");
	close_client();
	return -1;
}



void close_client()
{
	/* destroy the modules */
	destroy_modules();
	/* stop the timer */
	destroy_timer();

	/* stop the tcp layer */
	terminate_tcp_shell();

	/* destroy the transaction manager */
	destroy_trans_manager();

	/* destroy the peer manager */
	destroy_peer_manager( peer_table );

	/* destroy the shared mutexes */
	destroy_shared_mutexes();

	/* just for debuging */
	shm_status();

}



int main(int argc, char *argv[])
{
	str aaa_id;
	str host;

	if ( init_client()==-1 )
		exit(-1);

	/* add the peers from config file */
	//..................
	host.s   = "ugluk.mobis.fokus.gmd.de";
	host.len = strlen(host.s);
	aaa_id.s   = "aaa://ugluk.mobis.fokus.gmd.de:1812;transport=tcp";
	aaa_id.len = strlen( aaa_id.s );
	add_peer( &aaa_id, &host, 1812);

	/* start the tcp shell */
	start_tcp_accept();



	for(;;)
		sleep( 40 );

	return 0;
}
