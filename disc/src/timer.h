/*
 * $Id: timer.h,v 1.2 2003/03/14 16:54:07 bogdan Exp $
 *
 */



#ifndef _AAA_DIAMETER_TIMER_H
#define _AAA_DIAMETER_TIMER_H

#include "aaa_lock.h"
#include "locking.h"


#define TIMER_TICK 1
#define is_in_timer_list(_tl) ( (_tl)->timer_list )


typedef void (timer_function)(unsigned int ticks, void* param);


/* */
struct timer_handler{
	timer_function* timer_f;
	void* t_param;
	unsigned int interval;
	unsigned int expires;
	struct timer_handler* next;
};


/* all you need to put a cell in a timer list
   links to neighbours and timer value */
typedef struct timer_link
{
	struct timer_link *next_tl;
	struct timer_link *prev_tl;
	volatile unsigned int timeout;
	void              *payload;
	struct timer      *timer_list;
}timer_link_type ;


/* timer list: includes head, tail and protection semaphore */
typedef struct  timer
{
	struct timer_link  first_tl;
	struct timer_link  last_tl;
	gen_lock_t*          mutex;
} timer_type;




/************************* Functions to manipulate the timer *****************/

/*register a periodic timer;
 * ret: <0 on errror*/
int register_timer(timer_function f, void* param, unsigned int interval);

int init_timer();

int destroy_timer();

unsigned int get_ticks();


/******************** Functions to manipulate the timer lists ****************/

struct timer* new_timer_list();

void destroy_timer_list( struct timer* timer_list );

int add_to_timer_list( struct timer_link*, struct timer* ,unsigned int);

int rmv_from_timer_list( struct timer_link* );

struct timer_link* get_expired_from_timer_list( struct timer*, unsigned int );


#endif
