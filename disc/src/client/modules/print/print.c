/*
 * $Id: print.c,v 1.1 2003/03/28 20:00:37 andrei Exp $
 */
/*
 * Example aaa module (it does not do anything useful)
 *
 * History:
 * --------
 *  2003-03-28  created by andrei
 */

#include <stdio.h>

#include "../../../dprint.h"
#include "../../../aaa_module.h"


static int mod_init();
static void mod_destroy();
static int mod_run(int);

struct module_exports exports = {
	"print",
	0,
	
	mod_init,
	mod_destroy,
	mod_run
};




int mod_init()
{
	printf("\n print module : initializing...\n");
	return 0;
}

void mod_destroy()
{
	printf("\n print module: selfdestruct triggered\n");
}

int mod_run(int i)
{
	printf("\n print module: mod_run(%d) called\n", i);
}


