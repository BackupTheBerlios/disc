/*
 * $Id: sh_mutex.c,v 1.3 2003/03/13 13:07:55 andrei Exp $
 *
 * 2003-02-26 created by bogdan
 *
 */

#include "dprint.h"
#include "utils/aaa_lock.h"
#include "utils/counter.h"
#include "locking.h"


#define NR_SHARED_MUTEXES 256


static gen_lock_t   *shared_mutexes=0;
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


gen_lock_t *get_shared_mutex()
{
	atomic_inc(&mutex_counter);
	return &(shared_mutexes[(mutex_counter.value)|(NR_SHARED_MUTEXES-1)]);
}
