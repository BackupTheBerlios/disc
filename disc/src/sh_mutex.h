/*
 * $Id: sh_mutex.h,v 1.1 2003/03/14 15:28:59 bogdan Exp $
 *
 * 2003-02-26 created by bogdan
 *
 */


#include "utils/aaa_lock.h"
#include "locking.h"


#ifndef _AAA_DIAMETER_SH_MUTEX
#define _AAA_DIAMETER_SH_MUTEX

int init_shared_mutexes();

void destroy_shared_mutexes();

gen_lock_t *get_shared_mutex();

#endif