/*
 * $Id: aaa_module.h,v 1.2 2003/03/28 20:00:37 andrei Exp $
 */
/* History:
 * --------
 *  2003-03-28  created by andrei
 */

#ifndef aaa_module_h
#define aaa_module_h



struct module_exports{
	char* name; /* module name, must be unique */
	unsigned int appid; /* application id*/
	
	int (*mod_init)();   /* module initialization function */
	void (*mod_destroy)(); /* called on exit */
	
	int (*mod_run)(int); /* called for each message */
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
