/*
 * $Id: print.c,v 1.4 2003/04/04 16:59:25 bogdan Exp $
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
static int mod_msg(AAAMessage *msg, void *context);
static int mod_tout(int event, AAASessionId* sId, void *context);


struct module_exports exports = {
	"print",
	0,
	
	mod_init,
	mod_destroy,
	mod_msg,
	mod_tout
};


pthread_t print_th;




void *print_worker(void *attr)
{
	AAASessionId *sID;
	AAAMessage   *req;
	AAA_AVP      *avp;

	sleep(5);

	AAAStartSession( &sID, get_my_appref(), 0);
	req = AAANewMessage( 456, 4, sID, 0);
	AAACreateAVP( &avp, AVP_Destination_Realm, 0, 0, "gmd.de", 6);
	AAAAddAVPToMessage( req, avp, req->orig_realm);
	AAASendMessage( req );

	AAAFreeMessage( &req );

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


int mod_msg(AAAMessage *msg, void *context)
{
	DBG("\n print module: mod_msg() called\n");
	if (msg->commandCode==456) {
		DBG(" print module: got answer on my request\n");
		AAASessionTimerStart( msg->sId, 5);
	}
	return 1;
}


int mod_tout(int event, AAASessionId* sId, void *context)
{
	DBG("\n print module: mod_tout() called\n");
	if (event==SESSION_TIMEOUT_EVENT) {
		DBG(" print module: session %p expired -> destroy it\n",sId);
		AAAEndSession( sId );
	}
	return 1;
}


