/*
 * $Id: init_conf.c,v 1.16 2003/03/14 20:32:10 bogdan Exp $
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
#include <time.h>
#include "../str.h"
#include "../globals.h"
#include "init_conf.h"
#include "diameter_types.h"
#include "../dprint.h"
#include "../transport/common.h"
#include "../hash_table.h"
#include "../transport/trans.h"
#include "../transport/peer.h"
#include "session.h"
#include "message.h"

#include "../mem/shm_mem.h"
#include "cfg_parser.h"
//#include "config.h"



/* local vars */
static int  is_lib_init = 0;
static char *config_filename = 0;

#define VENDOR_ID  0x0000caca
#define DEFAULT_LISTENING_PORT 1812

/* external vars */
struct h_table *hash_table;           /* hash table for sessions and trans */
struct p_table *peer_table;           /* table with peers */
str aaa_identity= {0, 0};             /* aaa identity */
str aaa_realm= {0, 0};                /* realm */
str aaa_fqdn= {0, 0 };
str product_name = {"AAA FokusFastServer",19};     /* product name */
unsigned int vendor_id = VENDOR_ID;   /* vendor id */
unsigned int listen_port = DEFAULT_LISTENING_PORT;   /* listening port */
unsigned int supported_auth_app_id =
	(1<<AAA_APP_RELAY) | (1<<AAA_APP_MOBILE_IP);
unsigned int supported_acc_app_id = 
	(1<<AAA_APP_RELAY);

/* config info */
int i;
struct cfg_def cfg_ids[]={
	{"debug",        INT_VAL,   &debug        },
	{"log_stderr",   INT_VAL,   &log_stderr   },
	{"aaa_identity", STR_VAL,   &aaa_identity },
	{"aaa_realm",    STR_VAL,   &aaa_realm    },
	{"aaa_fqdn",     STR_VAL,   &aaa_fqdn     },
	{"listen_port",  INT_VAL,   &listen_port  },
	{"do_relay",     INT_VAL,   &i},
	{0,0,0}
};





int read_config_file( char *configFileName)
{
	FILE* cfg_file;

	/* read the parameters from confg file */
	if ((cfg_file=fopen(configFileName, "r"))==0){
		LOG(L_CRIT,"ERROR:AAAOpen: reading config file: %s\n",strerror(errno));
		goto error;
	}
	if (cfg_parse_stream(cfg_file)!=0){
		fclose(cfg_file);
		LOG(L_CRIT,"ERROR:AAAOpen: reading config file(%s)\n", configFileName);
		goto error;
	}
	fclose(cfg_file);

	/* check the params */
	if ((aaa_identity.s==0)||(aaa_fqdn.s==0)||(aaa_realm.s==0)){
		LOG(L_CRIT, "critical config parameters missing --exiting\n");
		goto error;
	}

	return 1;
error:
	return -1;
}



AAAReturnCode AAAClose()
{
	if (!is_lib_init) {
		fprintf(stderr,"ERROR:AAAClose: AAA library is not initialized\n");
		return AAA_ERR_NOT_INITIALIZED;
	}
	is_lib_init = 0;

	/* free the memory for config filename */
	if (config_filename)
		shm_free(config_filename);

	/* stop the socket layer (kill all threads) */
	terminate_tcp_shell();

	/* stop the peer manager */
	destroy_peer_manager();

	/* stop the message manager */
	destroy_msg_manager();

	/* stop session manager */
	shutdown_session_manager();

	return AAA_ERR_SUCCESS;
}



AAAReturnCode AAAOpen(char *configFileName)
{
	str peer;

	/* check if the lib is already init */
	if (is_lib_init) {
		LOG(L_ERR,"ERROR:AAAOpen: library already initialized!!\n");
		return AAA_ERR_ALREADY_INIT;
	}

	/* read the config file */
	if ( read_config_file(configFileName)!=1)
		goto error_config;

	/* save the name of the config file  */
	config_filename = shm_malloc( strlen(configFileName)+1 );
	if (!config_filename) {
		LOG(L_CRIT, "ERROR:AAAOpen: cannot allocate memory!\n");
		goto error_config;
	}
	strcpy( config_filename, configFileName);

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
	LOG(L_ERR,"ERROR:AAAOpen: failed to read configuration from file!\n");
	return  AAA_ERR_CONFIG;
}



const char* AAAGetDefaultConfigFileName()
{
	return config_filename;
}


