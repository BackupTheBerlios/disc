/*
 * $Id: aaa_module.h,v 1.10 2003/04/12 20:53:50 bogdan Exp $
 */
/* History:
 * --------
 *  2003-03-28  created by andrei
 */

#ifndef aaa_module_h
#define aaa_module_h

#include "diameter_api/diameter_api.h"

#define DOES_AUTH 1<<0
#define DOES_ACCT 1<<1

extern str module_path;

struct module_exports{
	char* name; /* module name, must be unique */
	unsigned int mod_type; /* module's type - server or client */
	unsigned int app_id; /* application id*/
	unsigned int flags; /* flags */
	
	int (*mod_init)();   /* module initialization function */
	void (*mod_destroy)(); /* called on exit */
	
	int (*mod_msg)(AAAMessage*, void*); /* called for each message */
	int (*mod_tout)(int, AAASessionId*, void*);  /* called for all timeout */
};



struct aaa_module{
	char* path;
	void* handle;
	struct module_exports* exports;
	int is_init;
	struct aaa_module* next;
};


extern struct aaa_module* modules; /* global module list */


int init_module_loading();
int load_module(char*);
int init_modules();
void destroy_modules();
struct aaa_module* find_module(unsigned int app_id);

#endif /*aaa_module_h*/
