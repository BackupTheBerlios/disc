/*
 * $Id: cfg_init.c,v 1.6 2003/04/09 18:12:44 andrei Exp $
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
#include "route.h"





static int cfg_set_module_path (struct cfg_line* cl, void* value);
static int cfg_load_modules(struct cfg_line* cl, void* value);
static int cfg_addpair(struct cfg_line* cl, void* callback);
static int cfg_echo(struct cfg_line* cl, void* value);
static int cfg_error(struct cfg_line* cl, void* value);
static int cfg_include(struct cfg_line* cl, void* value);



/* config info (null terminated array)*/
struct cfg_def cfg_ids[]={
	{"debug",        INT_VAL,   &debug,        0                   },
	{"log_stderr",   INT_VAL,   &log_stderr,   0                   },
	{"aaa_realm",    STR_VAL,   &aaa_realm,    0                   },
	{"aaa_fqdn",     STR_VAL,   &aaa_fqdn,     0                   },
	{"listen_port",  INT_VAL,   &listen_port,  0                   },
	{"module_path",  STR_VAL,   &module_path,   cfg_set_module_path },
	{"module",       GEN_VAL,   0,              cfg_load_modules    },
	{"peer",         GEN_VAL,   add_cfg_peer,   cfg_addpair         },
	{"route",        GEN_VAL,   add_route,      cfg_addpair         },
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
					"file %s: %s\n", cfg, strerror(errno));
		goto error;
	}
	if (cfg_parse_stream(cfg_file)!=0){
		fclose(cfg_file);
		/*
		LOG(L_CRIT,"ERROR:read_config_file : reading "
					"config file(%s)\n", cfg);
		*/
		goto error;
	}
	fclose(cfg_file);
	
	return 0;
error:
	return -1;
}



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
	shm_free(s.s);
	return ret;
}



/* reads a str pair and calls callback(s1, s2) */
int cfg_addpair(struct cfg_line* cl, void* callback)
{
	int ret;
	str* s1;
	str* s2;
		
	s1=0;
	s2=0;
	if ((cl->token_no<1)||(cl->token_no>2)){
		LOG(L_CRIT, "ERROR: cfg: invalid number of "
					"parameters for peer\n");
		ret=CFG_PARAM_ERR;
		goto error;
	}
	s1=shm_malloc(sizeof(str));
	if (s1==0) goto error_mem;
	s2=shm_malloc(sizeof(str));
	if (s2==0) goto error_mem;
	memset(s2, 0, sizeof(str));
	
	ret=cfg_getstr(cl->value[0], s1);
	if (ret!=0) goto error;
	if (cl->token_no==2){
		ret=cfg_getstr(cl->value[1], s2);
		if (ret!=0) goto error;;
	}
	if (((int (*)(str*, str*))(callback))(s1, s2)!=0){
		LOG(L_CRIT, "ERROR: cfg: error adding peer\n");
		ret=CFG_RUN_ERR;
		goto error;
	}
	
	ret=CFG_OK;
error:
	if (s1) shm_free(s1);
	if (s2) shm_free(s2);
	return ret;
error_mem:
	LOG(L_CRIT, "ERROR: cfg: memory allocation error\n");
	ret=CFG_MEM_ERR;
	goto error;
}
