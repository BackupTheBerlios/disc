/*
 * $Id: aaa_lock.c,v 1.1 2003/03/14 15:57:16 bogdan Exp $
 *
 * 2003-01-29  created by bogdan
 * 2003-03-12  converted to shm_malloc/shm_free (andrei)
 * 2003-03-13  converted to locking.h/gen_lock_t (andrei)
 *
 */


#include <stdlib.h>
#include "aaa_lock.h"

#include "../mem/shm_mem.h"


gen_lock_t* create_locks(int n)
{
	gen_lock_t  *locks;
	int       i;

	/* alocate the mutex variables */
	locks = 0;
	locks = (gen_lock_t*)shm_malloc( n * sizeof(gen_lock_t) );
	if (locks==0)
		goto error;

	/* init the mutexs */
	for(i=0;i<n;i++)
		if (lock_init( &locks[i])==0)
			goto error;

	return locks;
error:
	if (locks) shm_free((void*)locks);
	return 0;
}



void destroy_locks( gen_lock_t *locks, int n)
{
	int i;

	/* destroy the mutexs */
	for(i=0;i<n;i++)
		lock_destroy( &locks[i] );

	/* free the memory zone */
	shm_free( (void*)locks );
}
