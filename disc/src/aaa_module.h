/*
 * $Id: aaa_module.h,v 1.1 2003/03/28 19:06:54 andrei Exp $
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

#endif /*aaa_module_h*/
