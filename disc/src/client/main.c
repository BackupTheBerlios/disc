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
#include "../mem/shm_mem.h"
#include "../diameter_api/diameter_api.h"
#include "../sh_mutex.h"
#include "../timer.h"



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


void close_client();




static void sig_handler(int signo)
{
	printf(" ----------- SIGNAL RECEIVED (%d)(%d) -----------------\n",
			(int)pthread_self(), getpid() );
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
		LOG(L_CRIT,"ERROR:AAAOpen: intial malloc failed\n");
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

	//if( AAAOpen("aaa_lib.cfg")!=AAA_ERR_SUCCESS ) {
	//	return -1;
	//}

	/* start the timer */
	init_timer();

	return 1;
error:
	printf("ERROR: cannot init the client\n");
	return -1;
}



void close_client()
{
		/* stop the timer */
	destroy_timer();

	/* destroy the shared mutexes */
	destroy_shared_mutexes();

	/* just for debuging */
	shm_status();

}



int main(int argc, char *argv[])
{
	if ( init_client()==-1 )
		exit(-1);

	for(;;)
		sleep( 40 );

	return 0;
}
