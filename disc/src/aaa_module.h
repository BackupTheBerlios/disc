/*
 * $Id: aaa_module.h,v 1.4 2003/04/02 19:08:52 bogdan Exp $
 */
/* History:
 * --------
 *  2003-03-28  created by andrei
 */

#ifndef aaa_module_h
#define aaa_module_h

#include "diameter_api/diameter_types.h"


struct module_exports{
	char* name; /* module name, must be unique */
	unsigned int appid; /* application id*/
	
	int (*mod_init)();   /* module initialization function */
	void (*mod_destroy)(); /* called on exit */
	
	int (*mod_run)(AAAMessage*, void*); /* called for each message */
	int (*mod_Tout)(int, void*); /* called for all TimeOut event */
};



struct aaa_module{
	char* path;
	void* handle;
	struct module_exports* exports;
	struct aaa_module* next;
};


extern struct aaa_module* modules; /* global module list */


int init_module_loading();
int load_module(char*);
int init_modules();
void destroy_modules();

#endif /*aaa_module_h*/
