/*
 * $Id: print.c,v 1.3 2003/04/02 19:08:53 bogdan Exp $
 */
/*
 * Example aaa module (it does not do anything useful)
 *
 * History:
 * --------
 *  2003-03-28  created by andrei
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "../../../dprint.h"
#include "../../../aaa_module.h"
#include "../../../diameter_api/diameter_api.h"
#include "../../../diameter_api/diameter_types.h"


static int mod_init();
static void mod_destroy();
static int mod_run(AAAMessage *msg, void *context);
static int Tout_run(int event, void *context);

struct module_exports exports = {
	"print",
	0,
	
	mod_init,
	mod_destroy,
	mod_run
};


pthread_t print_th;




void *print_worker(void *attr)
{
	AAASessionId *sID;
	AAAMessage   *req;
	AAA_AVP      *avp;

	sleep(5);

	AAAStartSession( &sID, &exports, 0);
	req = AAANewMessage( 456, 4, sID, 0);
	AAACreateAVP( &avp, AVP_Destination_Realm, 0, 0, "gmd.de", 6);
	AAAAddAVPToMessage( req, avp, req->orig_realm);
	AAASendMessage( req );

	while(1)
		sleep(100);
	return 0;
}


int mod_init()
{
	DBG("print module : initializing...\n");
	if (pthread_create( &print_th, /*&attr*/ 0, &print_worker, 0)!=0){
		LOG(L_ERR,"ERROR:mod_init: cannot create worker thread\n");
		return -1;
	}

	return 0;
}


void mod_destroy()
{
	DBG("\n print module: selfdestruct triggered\n");
	pthread_cancel( print_th );
}


int mod_run(AAAMessage *msg, void *context)
{
	printf("\n print module: mod_run() called\n");
	return 1;
}

int Tout_run(int event, void *context)
{
	return 1;
}


