/*
 * $Id: aaa_module.h,v 1.7 2003/04/07 21:27:52 andrei Exp $
 */
/* History:
 * --------
 *  2003-03-28  created by andrei
 */

#ifndef aaa_module_h
#define aaa_module_h

#include "diameter_msg/diameter_msg.h"

extern str module_path;

struct module_exports{
	char* name; /* module name, must be unique */
	unsigned int appid; /* application id*/
	
	int (*mod_init)();   /* module initialization function */
	void (*mod_destroy)(); /* called on exit */
	
	int (*mod_msg)(AAAMessage*, void*); /* called for each message */
	int (*mod_tout)(int, AAASessionId*, void*);  /* called for all timeout */
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
