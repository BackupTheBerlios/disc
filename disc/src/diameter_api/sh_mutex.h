/*
 * $Id: sh_mutex.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2003-02-26 created by bogdan
 *
 */


#include "utils/aaa_lock.h"


#ifndef _AAA_DIAMETER_SH_MUTEX
#define _AAA_DIAMETER_SH_MUTEX

int init_shared_mutexes();

void destroy_shared_mutexes();

aaa_lock *get_shared_mutex();

#endif
