/*
 * $Id: aaa_module.c,v 1.2 2003/03/28 19:21:27 andrei Exp $
 */
/*
 * History:
 * --------
 *  2003-03-27  created by andrei
 */

#include <ltdl.h>

#include "config.h"
#include "dprint.h"
#include "mem/shm_mem.h"
#include "aaa_module.h"


struct aaa_module* modules=0;


int init_module_loading()
{
	int ret;
	ret=lt_dlinit();
	
	if (ret){
		LOG(L_CRIT, "ERROR: init_moduled: lt_dlinit failed: %s\n",
				lt_dlerror());
		goto error;
	}
	
	ret=lt_dlsetsearchpath(MODULE_SEARCH_PATH);
	if (ret){
		LOG(L_CRIT, "ERROR: init_moduled: lt_dlinit failed: %s\n",
				lt_dlerror());
		goto error;
	}
	return ret;
error:
	lt_dlexit();
	return -1;
}



int load_module(char* name)
{
	int ret;
	lt_dlhandle handle;
	struct module_exports* e;
	char* error_msg;
	struct aaa_module* mod;
	struct aaa_module** m;
	
	ret=0;
	mod=0;
	handle=0;
	e=0;
	
	handle=lt_dlopenext(name);
	if (handle==0){
		LOG(L_CRIT, "ERROR: load_module: failed to load <%s> in <%s>\n",
					name, MODULE_SEARCH_PATH);
		goto error;
	}
	/* get module struct */
	e=(struct module_exports*)lt_dlsym(handle, "exports");
	error_msg=(char*)lt_dlerror();
	if (error_msg!=0){
		LOG(L_CRIT, "ERROR: load_module: symbol not found <%s>: %s\n",
				name, error_msg);
		goto error;
	}
	/* link it in the module list - TODO*/
	mod=shm_malloc(sizeof(struct aaa_module));
	if (mod==0){
		LOG(L_CRIT, "ERROR: load_module: memory allocation failure\n");
		ret=-2;
		goto error;
	}
	mod->path=name;
	mod->handle=handle;
	mod->exports=e;
	mod->next=0;
	for (m=&modules; *m; m=&(*m)->next){
		if ((*m)->handle==mod->handle){
			LOG(L_WARN, "WARNING: load_module: attempting to load the same"
					" module twice (%s)\n", mod->path);
			shm_free(mod);
			goto skip;
		}
		if (strcmp((*m)->exports->name, e->name)==0){
			LOG(L_CRIT, "ERROR: load_module: module name collision for %s\n",
					e->name);
			ret=-3;
			goto error;
		}
		if ((*m)->exports->appid==e->appid){
			LOG(L_CRIT, "ERROR: load_module: 2 modules with the same "
					"appid(%u): %s, %s\n",
					e->appid, (*m)->exports->name, e->name);
			ret=-4;
			goto error;
		}
	}
	*m=mod;
skip:
	return ret;
	
error:
	if(handle) lt_dlclose(handle);
	if(mod) shm_free(mod);
	return ret;
}


/*
 * Initializes all the loaded modules */
int init_modules()
{
	struct aaa_module* a;
	
	for (a=modules; a; a=a->next){
		if((a->exports)&&(a->exports->mod_init))
			if(a->exports->mod_init()!=0){
				LOG(L_CRIT, "ERROR: init_modules: module %s failed to "
						"initialize\n", a->exports->name);
				return -1;
			}
	}
	return 0;
}


/* destroys all the modules */
void destroy_modules()
{
	struct aaa_module* a;
	
	for (a=modules; a; a=a->next)
		if ((a->exports)&&(a->exports->mod_destroy)) a->exports->mod_destroy();
	modules=0;
}


/* finds a module, given an appid */
struct aaa_module* find_module(unsigned int appid)
{
	struct aaa_module* a;
	
	for (a=modules; a; a=a->next)
		if ((a->exports)&&(a->exports->appid==appid)) return a;
	return 0;
}
