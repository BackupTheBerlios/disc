/*
 * $Id: print.c,v 1.13 2003/04/13 23:01:16 andrei Exp $
 */
/*
 * Example aaa module (it does not do anything useful)
 *
 * History:
 * --------
 *  2003-03-28  created by andrei
 *  2003-04-14  added module params (andrei)
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

static int foo=0;
static char* bar="default";

static struct module_param params[]={
	{"foo",  INT_TYPE, &foo},
	{"bar",  STR_TYPE, &bar},
	{0,0,0} /* null terminated array */
};


struct module_exports exports = {
	"print",
	AAA_CLIENT,
	4,
	DOES_AUTH|DOES_ACCT,
	params,
	
	mod_init,
	mod_destroy,
	mod_msg,
	mod_tout
};



int mod_init()
{
	DBG("print module : initializing...\n");
	DBG("print module : foo=%d, bar=%s\n", foo, bar);

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
	DBG("\n print module: mod_tout() called\n");
	if (event==SESSION_TIMEOUT_EVENT) {
		DBG(" print module: session %p expired -> destroy it\n",sId);
		AAAEndSession( sId );
	} else {
		DBG(" print module: no response to request :-( -> destroy sesion\n");
		AAAEndSession( sId );
	}
	return 1;
}


