/*
 * $Id: init_conf.c,v 1.25 2003/04/09 22:10:34 bogdan Exp $
 *
 * 2003-02-03  created by bogdan
 * 2003-03-12  converted to shm_malloc, from ser (andrei)
 * 2003-03-13  added config suport (cfg_parse_stream(), cfg_ids) (andrei)
 * 2003-04-07  moved all the above to $(topdir)/cfg_init.c  (andrei)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../str.h"
#include "init_conf.h"
#include "diameter_api.h"
#include "session.h"



/* local vars */
static int  is_lib_init = 0;



AAAReturnCode AAAClose()
{
	if (!is_lib_init) {
		fprintf(stderr,"ERROR:AAAClose: AAA library is not initialized\n");
		return AAA_ERR_NOT_INITIALIZED;
	}
	is_lib_init = 0;

	/* stop session manager */
	shutdown_session_manager();

	return AAA_ERR_SUCCESS;
}



AAAReturnCode AAAOpen()
{
	/* check if the lib is already init */
	if (is_lib_init) {
		LOG(L_ERR,"ERROR:AAAOpen: library already initialized!!\n");
		return AAA_ERR_ALREADY_INIT;
	}

	/* init the session manager */
	if (init_session_manager( 1024/*hash_size*/, 512/*shared_locks*/  )==-1)
		goto mem_error;

	/* finally DONE */
	is_lib_init = 1;

	return AAA_ERR_SUCCESS;
mem_error:
	LOG(L_ERR,"ERROR:AAAOpen: AAA library initialization failed!\n");
	AAAClose();
	return AAA_ERR_NOMEM;
}




