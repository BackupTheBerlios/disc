/*
 * $Id: init_conf.c,v 1.5 2003/03/11 18:06:29 bogdan Exp $
 *
 * 2003-02-03 created by bogdan
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
#include "utils/str.h"
#include "global.h"
#include "init_conf.h"
#include "diameter_types.h"
#include "utils/dprint.h"
#include "tcp_shell/common.h"
#include "hash_table.h"
#include "timer.h"
#include "sh_mutex.h"
#include "trans.h"
#include "peer.h"
#include "session.h"
#include "message.h"



/* local vars */
static int is_lib_init = 0;
static char config_filename[512];

/* external vars */
int debug;                           /* debuging level */
int log_stderr;                      /* use std error fro loging? */
struct h_table *hash_table;          /* hash table for sessions and trans */
struct p_table *peer_table;          /* table with peers */
str aaa_identity;                    /* aaa identity */
str aaa_realm;                       /* realm */
str aaa_fqdn;
str product_name = {"AAA FFS",7};    /* product name */
unsigned int vendor_id = 12345;      /* vendor id */
unsigned int supported_auth_app_id =
	(1<<AAA_APP_RELAY) | (1<<AAA_APP_MOBILE_IP);
unsigned int supported_acc_app_id = 0;




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

	is_lib_init = 0;
	return AAA_ERR_SUCCESS;
}



AAAReturnCode AAAOpen(char *configFileName)
{
	str peer;

	/* check if the lib is already init */
	if (is_lib_init) {
		LOG(L_ERR,"ERROR: AAAOpen: library already initialized!!\n");
		return AAA_ERR_ALREADY_INIT;
	}

	/* read the config file */
	if (0) {
		return  AAA_ERR_CONFIG;
	}
	strncpy( config_filename, configFileName, sizeof(config_filename)-1);
	config_filename[511] = 0;

	/**/
	aaa_identity.s = "aaa://fesarius.fokus.gmd.de:1912;transport=tcp";
	aaa_identity.len = strlen(aaa_identity.s);
	aaa_fqdn.s = aaa_identity.s + 6;
	aaa_fqdn.len = 21;
	aaa_realm.s = aaa_identity.s + 15;
	aaa_realm.len = 12 ;

	/* init the random number geberator */
	init_random_generator();

	/* init the log system */
	debug = 9;
	log_stderr = 1;

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
}



const char* AAAGetDefaultConfigFileName()
{
	return config_filename;
}


