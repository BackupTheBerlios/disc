/*
 * $Id: init_conf.c,v 1.12 2003/03/13 19:16:07 bogdan Exp $
 *
 * 2003-02-03  created by bogdan
 * 2003-03-12  converted to shm_malloc, from ser (andrei)
 * 2003-03-13  added config suport (cfg_parse_stream(), cfg_ids) (andrei)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "utils/str.h"
#include "globals.h"
#include "init_conf.h"
#include "diameter_types.h"
#include "dprint.h"
#include "tcp_shell/common.h"
#include "hash_table.h"
#include "timer.h"
#include "sh_mutex.h"
#include "trans.h"
#include "peer.h"
#include "session.h"
#include "message.h"

#include "mem/shm_mem.h"
#include "cfg_parser.h"
#include "config.h"



/* local vars */
static int  is_lib_init = 0;
static char *config_filename;


/* external vars */
unsigned int shm_mem_size=SHM_MEM_SIZE*1024*1024; /* shared mem. size*/
int memlog=L_DBG;                     /* shm_mallocs log level */
int debug=9;                          /* debuging level */
int log_stderr=1;                     /* use std error fro loging? */
struct h_table *hash_table;           /* hash table for sessions and trans */
struct p_table *peer_table;           /* table with peers */
str aaa_identity= {0, 0};             /* aaa identity */
str aaa_realm= {0, 0};                /* realm */
str aaa_fqdn= {0, 0 };
str product_name = {"AAA FFS",7};     /* product name */
unsigned int vendor_id = VENDOR_ID;   /* vendor id */
unsigned int listen_port = DEFAULT_LISTENING_PORT;   /* listening port */
unsigned int supported_auth_app_id =
	(1<<AAA_APP_RELAY) | (1<<AAA_APP_MOBILE_IP);
unsigned int supported_acc_app_id = 
	(1<<AAA_APP_RELAY);

/* config info */

struct cfg_def cfg_ids[]={
	{"debug",        INT_VAL,   &debug        },
	{"log_stderr",   INT_VAL,   &log_stderr   },
	{"aaa_identity", STR_VAL,   &aaa_identity },
	{"aaa_realm",    STR_VAL,   &aaa_realm    },
	{"aaa_fqdn",     STR_VAL,   &aaa_fqdn     },
	{"listen_port",  INT_VAL,   &listen_port  },
	{0,0,0}
};



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



AAAReturnCode AAAClose()
{
	if (!is_lib_init) {
		fprintf(stderr,"ERROR:AAAClose: AAA library is not initialized\n");
		return AAA_ERR_NOT_INITIALIZED;
	}
	is_lib_init = 0;

	/* close all open connections */

	/* stop the timer */
	destroy_timer();

	/* stop the socket layer (kill all threads) */
	terminate_tcp_shell();

	/* stop the peer manager */
	destroy_peer_manager();

	/* stop the message manager */
	destroy_msg_manager();

	/* stop session manager */
	shutdown_session_manager();

	/* stop the transaction manager */
	destroy_trans_manager();

	/* destroy the shared mutexes */
	destroy_shared_mutexes();

	/* destroy the hash_table */
	destroy_htable( hash_table );

	/* just for debuging */
	shm_status();

	return AAA_ERR_SUCCESS;
}



AAAReturnCode AAAOpen(char *configFileName)
{
	str peer;
	void* shm_mempool;
	FILE* cfg_file;

	/* check if the lib is already init */
	if (is_lib_init) {
		LOG(L_ERR,"ERROR:AAAOpen: library already initialized!!\n");
		return AAA_ERR_ALREADY_INIT;
	}

	/* init mallocs */
	shm_mempool=malloc(shm_mem_size);
	if (shm_mempool==0){
		LOG(L_CRIT,"ERROR:AAAOpen: intial malloc failed\n");
		return AAA_ERR_NOMEM;
	};
	if (shm_mem_init_mallocs(shm_mempool, shm_mem_size)<0){
		LOG(L_CRIT,"ERROR:AAAOpen: could not intialize shm. mallocs\n");
		return AAA_ERR_NOMEM;
	};

	/* read the config file */
	if ((cfg_file=fopen(configFileName, "r"))==0){
		LOG(L_CRIT,"ERROR:AAAOpen: reading config file: %s\n",strerror(errno));
		goto error_config;
	}
	if (cfg_parse_stream(cfg_file)!=0){
		fclose(cfg_file);
		LOG(L_CRIT,"ERROR:AAAOpen: reading config file(%s)\n", configFileName);
		goto error_config;
	}
	fclose(cfg_file);

	/* save the name of the config file  */
	config_filename = shm_malloc( sizeof(configFileName)+1 );
	if (!config_filename) {
		LOG(L_CRIT, "ERROR:AAAOpen: cannot allocate memory!\n");
		goto error_config;
	}
	strcpy( config_filename, configFileName);

	/* init the random number geberator */
	init_random_generator();

	printf("after config read: debug=%d, log_stderr=%d\n", debug, log_stderr);
	printf("identity=%.*s, fqdn=%.*s realm=%.*s\n",
				aaa_identity.len, aaa_identity.s,
				aaa_fqdn.len, aaa_fqdn.s,
				aaa_realm.len, aaa_realm.s
			);
	
	if ((aaa_identity.s==0)||(aaa_fqdn.s==0)||(aaa_realm.s==0)){
		LOG(L_CRIT, "critical config parameters missing --exiting\n");
		goto error_config;
	}

	/* init the hash_table */
	if ( (hash_table=build_htable())==0)
		goto mem_error;

	/* init the shared mutexes */
	if ( (init_shared_mutexes())==0)
		goto mem_error;

	/* init the transaction manager */
	if (init_trans_manager()==-1)
		goto mem_error;

	/* init the session manager */
	if (init_session_manager()==-1)
		goto mem_error;

	/* init the message manager */
	if (init_msg_manager()==-1)
		goto mem_error;

	/* init peer manager */
	if (init_peer_manager( )==-1)
		goto mem_error;

	/* init the socket layer */
	if (init_tcp_shell()==-1)
		goto mem_error;

	/* start the timer */
	init_timer();

	/* add the peers from config file */
	//..................
	peer.s = "ugluk.mobis.fokus.gmd.de";
	peer.len = strlen(peer.s);
	add_peer( peer_table, &peer, 7, 1812);

	/* start the tcp shell */
	start_tcp_accept();

	/* finally DONE */
	is_lib_init = 1;

	return AAA_ERR_SUCCESS;
mem_error:
	LOG(L_ERR,"ERROR:AAAOpen: AAA library initialization failed!\n");
	AAAClose();
	return AAA_ERR_NOMEM;
error_config:
		return  AAA_ERR_CONFIG;
}



const char* AAAGetDefaultConfigFileName()
{
	return config_filename;
}


