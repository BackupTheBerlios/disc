/*
 * $Id: aaa_lock.h,v 1.1 2003/03/14 15:57:16 bogdan Exp $
 *
 * 2003-01-29 created by bogdan
 *
 */

#ifndef _AAA_DIAMETER_AAA_LOCK_H
#define _AAA_DIAMETER_AAA_LOCK_H

#include "../locking.h"


gen_lock_t* create_locks(int n);

void destroy_locks( gen_lock_t *locks, int n);

#endif
