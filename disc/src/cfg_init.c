/*
 * $Id: cfg_init.c,v 1.3 2003/04/08 18:59:52 andrei Exp $
 *
 * History:
 * --------
 * 2003-03-07  created mostly from pieces of diameter_api/init_conf.c by andrei
 */

#include <errno.h>
#include <string.h>

#include "../libltdl/ltdl.h"

#include "dprint.h"
#include "cfg_init.h"
#include "cfg_parser.h"
#include "globals.h"
#include "aaa_module.h"
#include "aaa_parse_uri.h"
#include "mem/shm_mem.h"





static int cfg_set_module_path (struct cfg_line* cl, void* value);
static int cfg_load_modules(struct cfg_line* cl, void* value);
static int cfg_addpeer(struct cfg_line* cl, void* value);
static int cfg_echo(struct cfg_line* cl, void* value);
static int cfg_error(struct cfg_line* cl, void* value);
static int cfg_include(struct cfg_line* cl, void* value);


/* peer list*/
struct cfg_peer_list* cfg_peer_lst=0;

/* config info (null terminated array)*/
struct cfg_def cfg_ids[]={
	{"debug",        INT_VAL,   &debug,        0                   },
	{"log_stderr",   INT_VAL,   &log_stderr,   0                   },
	{"aaa_realm",    STR_VAL,   &aaa_realm,    0                   },
	{"aaa_fqdn",     STR_VAL,   &aaa_fqdn,     0                   },
	{"listen_port",  INT_VAL,   &listen_port,  0                   },
	{"module_path",  STR_VAL,   &module_path,   cfg_set_module_path },
	{"module",       GEN_VAL,   0,              cfg_load_modules    },
	{"peer",         GEN_VAL,   &cfg_peer_lst,  cfg_addpeer         },
	{"echo",         GEN_VAL,   0,              cfg_echo            },
	{"_error",       GEN_VAL,   0,              cfg_error           },
	{"include",      STR_VAL,   0,              cfg_include         },
	{0,0,0,0}
};




int read_config_file( char *cfg)
{
	FILE* cfg_file;
	
	/* read the parameters from confg file */
	if ((cfg_file=fopen(cfg, "r"))==0){
		LOG(L_CRIT,"ERROR:read_config_file: reading config "
					"file: %s\n",strerror(errno));
		goto error;
	}
	if (cfg_parse_stream(cfg_file)!=0){
		fclose(cfg_file);
		LOG(L_CRIT,"ERROR:read_config_file : reading "
					"config file(%s)\n", cfg);
		goto error;
	}
	fclose(cfg_file);
	
	return 0;
error:
	return -1;
}
	/* read the config file */
	/*if ( read_config_file(configFileName)!=1)
	 *	goto error_config;
	 */



int cfg_set_module_path(struct cfg_line* cl, void* value)
{
	int ret;
	str* s;
	
	if (cl->token_no!=1){
		LOG(L_CRIT, "ERROR: too many parameters for module path\n");
		return CFG_PARAM_ERR;
	}
	s=(str*)value;
	/* get the string */
	ret=cfg_getstr(cl->value[0], s);
	if (ret!=0) goto error;
	/* modules should be already init here */
	ret=lt_dlsetsearchpath(s->s);
	if (ret){
		LOG(L_CRIT, "ERROR:cfg_set_module_path:"
					" lt_dlsetsearchpath failed: %s\n", lt_dlerror());
		ret=CFG_RUN_ERR;
		goto error;
	}
	return CFG_OK;
error:
	return ret;
}



int cfg_load_modules(struct cfg_line* cl, void* value)
{
	int ret;
	int r;
	str s;
	
	ret=CFG_PARAM_ERR;
	for (r=0; r<cl->token_no; r++){
		LOG(L_INFO, "loading module  %s...", cl->value[r]);
		ret=cfg_getstr(cl->value[r], &s);
		if (ret!=0){
			LOG(L_CRIT, "ERROR: load module: bad module name %s\n",
					cl->value[r]);
			break;
		}
		ret=load_module(s.s);
		if (ret==0) LOG(L_INFO, "ok\n");
		else LOG(L_INFO, "FAILED\n");
	}
	return ret;
}



int cfg_echo(struct cfg_line* cl, void* value)
{
	int r;
	
	for (r=0; r<cl->token_no; r++){
		LOG(L_INFO, "%s ", cl->value[r]);
	};
	LOG(L_INFO, "\n");
	return CFG_OK;
}




int cfg_error(struct cfg_line* cl, void* value)
{
	int r;
	
	for (r=0; r<cl->token_no; r++){
		LOG(L_CRIT, "%s ", cl->value[r]);
	};
	LOG(L_CRIT, "\n");
	return CFG_ERR;
}




int cfg_include(struct cfg_line* cl, void* value)
{
	str s;
	int ret;
	
	if (cl->token_no!=1){
		LOG(L_CRIT, "ERROR: too many parameters for include\n");
		return CFG_PARAM_ERR;
	}
	ret=cfg_getstr(cl->value[0], &s);
	if (ret!=0) return ret;
	DBG("-> including <%s>\n", s.s);
	ret=read_config_file(s.s);
	return ret;
}



int cfg_addpeer(struct cfg_line* cl, void* value)
{
	struct cfg_peer_list **pl;
	struct cfg_peer_list *n;
	int ret;
		
	if ((cl->token_no<1)||(cl->token_no>2)){
		LOG(L_CRIT, "ERROR: cfg_addpeer: invalid number of "
					"parameters for peer\n");
		return CFG_PARAM_ERR;
	}
	n=shm_malloc(sizeof(struct cfg_peer_list));
	if (n==0){
		LOG(L_CRIT, "ERROR: cfg_addpeer: mem. alloc. failure\n");
		return CFG_MEM_ERR;
	}
	memset(n, 0, sizeof(struct cfg_peer_list));
	ret=cfg_getstr(cl->value[0], &n->full_uri);
	if (ret!=0) return ret;
	if (aaa_parse_uri(n->full_uri.s, n->full_uri.len, &n->uri)!=0){
		LOG(L_CRIT, "ERROR: cfg_addpeer: bad uri\n");
		return CFG_PARAM_ERR;
	}
	if (cl->token_no==2){
		ret=cfg_getstr(cl->value[0], &n->alias);
		if (ret!=0) return ret;
	}
	/* append to the list*/
	pl=value;
	n->next=*pl;
	*pl=n;
	return CFG_OK;
}
