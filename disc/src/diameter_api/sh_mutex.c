/*
 * $Id: sh_mutex.c,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2003-02-26 created by bogdan
 *
 */

#include "utils/dprint.h"
#include "utils/aaa_lock.h"
#include "utils/counter.h"


#define NR_SHARED_MUTEXES 256


static aaa_lock   *shared_mutexes=0;
static atomic_cnt  mutex_counter;



int init_shared_mutexes()
{
	/* build and set the mutexes */
	shared_mutexes = create_locks(NR_SHARED_MUTEXES);
	if (!shared_mutexes) {
		LOG(L_ERR,"ERROR:init_shared_mutexes: cannot create locks!!\n");
		goto error;
	}

	mutex_counter.value = 0;

	LOG(L_INFO,"INFO:init_shared_mutexes: shared mutexes created\n");
	return 1;
error:
	LOG(L_ERR,"ERROR:init_shared_mutexes: failed to create shared mutexes\n");
	return -1;
}



void destroy_shared_mutexes()
{
	if (shared_mutexes)
		destroy_locks( shared_mutexes, NR_SHARED_MUTEXES);
	LOG(L_INFO,"INFO:destroy_shared_mutexes: shared mutexes removed\n");
}


aaa_lock *get_shared_mutex()
{
	atomic_inc(&mutex_counter);
	return &(shared_mutexes[(mutex_counter.value)|(NR_SHARED_MUTEXES-1)]);
}
