/*
 * $Id: print.c,v 1.3 2003/04/14 12:47:58 bogdan Exp $
 */
/*
 * Example aaa module (it does not do anything useful)
 *
 * History:
 * --------
 *  2003-03-28  created by andrei
 *  2003-04-14  updated exports to the new format (w/ mod params) (andrei)
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "../../../dprint.h"
#include "../../../aaa_module.h"
#include "../../../diameter_api/diameter_api.h"


static int mod_init();
static void mod_destroy();
static int mod_msg(AAAMessage *msg, void *context);
static int mod_tout(int event, AAASessionId* sId, void *context);


struct module_exports exports = {
	"print",
	AAA_SERVER,
	4,
	DOES_AUTH|DOES_ACCT,
	0, /* no  mod params */
	
	mod_init,
	mod_destroy,
	mod_msg,
	mod_tout
};



int mod_init()
{
	DBG("print module : initializing...\n");

	return 0;
}


void mod_destroy()
{
	DBG("\n print module: selfdestruct triggered\n");
}


int mod_msg(AAAMessage *msg, void *context)
{
	AAAMessage *ans;

	DBG("\n print module: mod_msg() called\n");
	if ( is_req(msg) ) {
		DBG(" print module: got a new request -> sending answer\n");

		if ( (ans=AAANewMessage( 456, 4, msg->sId, msg))==0)
				return -1;

		AAASetMessageResultCode( ans, 2001);
		AAASendMessage( ans );
		AAAFreeMessage( &ans );
	}
	return 1;
}


int mod_tout(int event, AAASessionId* sId, void *context)
{
	DBG("\n print module: mod_tout() called - "
		"BUG!!!! I should never get here yet!\n");
	return 1;
}


