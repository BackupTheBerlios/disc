/*
 * $Id: timer.c,v 1.2 2003/03/14 16:54:07 bogdan Exp $
 *
 * 
 *  2003-03-12  converted to shm_malloc/shm_free (andrei)
 *
 */


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "dprint.h"
#include "aaa_lock.h"
#include "timer.h"

#include "mem/shm_mem.h"
#include "locking.h"



static struct timer_handler* timer_handler_list=0;
static int jiffies = 0;
static pthread_t  timer_thread = 0;

void* timer_ticker( void* );



/*register a periodic timer;
 * ret: <0 on error*/
int register_timer(timer_function f, void* param, unsigned int interval)
{
	struct timer_handler* t;

	t=shm_malloc(sizeof(struct timer_handler));
	if (t==0){
		LOG(L_ERR, "ERROR: register_timer: out of memory\n");
		goto error;
	}
	t->timer_f=f;
	t->t_param=param;
	t->interval=interval;
	t->expires=jiffies+interval;
	/* insert it into the list*/
	t->next = timer_handler_list;
	timer_handler_list = t;
	return 1;

error:
	return -1;
}



int init_timer()
{
	/* a separat thread will be run to act as a timer */
	if (pthread_create( &timer_thread, /*&attr*/ 0, &timer_ticker, 0) != 0) {
		LOG(L_ERR,"ERROR:init_timer: cannot create timer thread\n");
		return -1;
	}

	LOG(L_INFO,"INFO:init_timer: timer thread started\n");
	return 1;
}



int destroy_timer()
{
	struct timer_handler *th, *th_tmp;

	/* stop the timer thread */
	if (timer_thread) {
		if (pthread_cancel( timer_thread )!=0) {
			LOG(L_ERR,"ERROR:destroy_timer: error when canceling the"
					" timer thread\n");
			return -1;;
		}
	}

	/* clear all the timer handlers */
	th = timer_handler_list;
	while (th) {
		th_tmp = th;
		th = th->next;
		shm_free( th_tmp );
	}

	LOG(L_INFO,"INFO:destroy_timer: timer thread stoped\n");
	return 1;
}



void* timer_ticker( void *foo)
{
	struct timer_handler* t;
	unsigned int prev_jiffies;

	for(;;) {
		/* sleep */
		sleep(TIMER_TICK);
		/* call all the handlers */
		prev_jiffies=jiffies;
		jiffies+=TIMER_TICK;
		DBG("DEBUG:timer_ticker: la semnalul urmator va fi ora: %d sec\n",
				jiffies);
		/* test for overflow (if tick= 1s =>overflow in 136 years)*/
		if (jiffies<prev_jiffies){ 
			/*force expire & update every timer, a little buggy but it 
			 * happens once in 136 years :) */
			for(t=timer_handler_list;t;t=t->next){
				t->expires=jiffies+t->interval;
				t->timer_f(jiffies, t->t_param);
			}
		} else {
			for (t=timer_handler_list;t; t=t->next){
				if (jiffies>=t->expires){
					t->expires=jiffies+t->interval;
					t->timer_f(jiffies, t->t_param);
				}
			}
		}
	}
	return 0;
}


unsigned int get_ticks()
{
	return jiffies;
}





/*************************** TIMER LIST FUNCTIONS ****************************/

struct timer* new_timer_list()
{
	struct timer *new_timer;

	new_timer = 0;

	new_timer = (struct timer*)shm_malloc( sizeof(struct timer) );
	if (!new_timer) {
		LOG(L_ERR,"ERROR:new_timer_list: no more free memory!\n");
		goto error;
	}
	memset( new_timer, 0, sizeof(struct timer) );

	new_timer->mutex = create_locks( 1 );
	if (!new_timer->mutex) {
		LOG(L_ERR,"ERROR:new_timer_list: cannot create new lock!\n");
		goto error;
	}

	new_timer->first_tl.next_tl = &(new_timer->last_tl );
	new_timer->last_tl.prev_tl  = &(new_timer->first_tl );
	new_timer->first_tl.prev_tl =new_timer->last_tl.next_tl = NULL;
	new_timer->last_tl.timeout = -1;

	return new_timer;
error:
	if (new_timer)
		shm_free( new_timer);
	return 0;
}



void destroy_timer_list( struct timer* timer_list )
{
	if (timer_list) {
		/* destroy the mutex */
		if (timer_list->mutex)
			destroy_locks( timer_list->mutex, 1);
		/* free the memory */
		shm_free(timer_list);
	}
}



int add_to_timer_list( struct timer_link* tl, struct timer* timer_list,
														unsigned int timeout )
{
	/* lock the list */
	lock_get(timer_list->mutex);
	/* add the element into the list */
	tl->timeout = timeout;
	tl->prev_tl = timer_list->last_tl.prev_tl;
	tl->next_tl = & timer_list->last_tl;
	timer_list->last_tl.prev_tl = tl;
	tl->prev_tl->next_tl = tl;
	tl->timer_list = timer_list;
	/* unlock the list */
	lock_release(timer_list->mutex);

	DBG("DEBUG: add_to_timer_list[%p]: %p\n",timer_list,tl);
	return 1;
}



int rmv_from_timer_list( struct timer_link* tl )
{
	if (is_in_timer_list( tl )) {
		/* lock the list */
		lock_get(tl->timer_list->mutex);
		tl->prev_tl->next_tl = tl->next_tl;
		tl->next_tl->prev_tl = tl->prev_tl;
		tl->next_tl = 0;
		tl->prev_tl = 0;
		/* unlock the list */
		DBG("DEBUG: rmv_from_timer_list[%p]: %p\n",tl->timer_list,tl);
		lock_release(tl->timer_list->mutex);
		tl->timer_list = 0;
	}
	return 1;
}



struct timer_link* get_expired_from_timer_list( struct timer* timer_list ,
															unsigned int time)
{
	struct timer_link *tl , *end, *ret;

	/* quick check whether it is worth entering the lock */
	if (timer_list->first_tl.next_tl==&timer_list->last_tl 
			|| ( /* timer_list->first_tl.next_tl
				&& */ timer_list->first_tl.next_tl->timeout > time) )
		return NULL;

	/* the entire timer list is locked now */
	lock_get(timer_list->mutex);

	end = &timer_list->last_tl;
	tl = timer_list->first_tl.next_tl;
	while( tl!=end && tl->timeout <= time) {
		tl->timer_list = NULL;
		tl=tl->next_tl;
	}

	/* nothing to delete found */
	if (tl->prev_tl==&(timer_list->first_tl)) {
		ret = NULL;
	} else { /* we did find timers to be fired! */
		/* the detached list begins with current beginning */
		ret = timer_list->first_tl.next_tl;
		/* and we mark the end of the split list */
		tl->prev_tl->next_tl = NULL;
		/* the shortened list starts from where we suspended */
		timer_list->first_tl.next_tl = tl;	
		tl->prev_tl = & timer_list->first_tl;
	}
	/* give the list lock away */
	lock_release(timer_list->mutex);

	return ret;
}



