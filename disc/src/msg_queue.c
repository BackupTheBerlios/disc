/*
 * $Id: msg_queue.c,v 1.3 2003/04/14 19:23:17 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "dprint.h"
#include "aaa_lock.h"
#include "locking.h"
#include "msg_queue.h"

struct queue_unit {
	str  buf;
	struct peer *p;
};

static int msg_pipe[2];
gen_lock_t *msg_lock;

static unsigned int max_queued_size = 0;
static unsigned int max_queued_units = 0;
static unsigned int cur_queued_size = 0;
static unsigned int cur_queued_units = 0;


int init_msg_queue()
{
	if (pipe( msg_pipe )==-1) {
		LOG(L_ERR,"ERROR:init_msg_queue: cannot create pipe : %s\n",
			strerror(errno));
		goto error;
	}
	msg_lock = create_locks( 1 );
	if (!msg_lock) {
		LOG(L_ERR,"ERROR:init_msg_queue: cannot create lock\n");
		goto error;
	}
	return 1;

error:
	return -1;
}


void destroy_msg_queue()
{
	unsigned int  available;
	str           buf;
	struct peer   *p;

	LOG(L_INFO,"INFO:destroy_msg_queue: max_queued_size  = %u bytes\n",
			max_queued_size);
	LOG(L_INFO,"INFO:destroy_msg_queue: max_queued_units = %u \n",
			max_queued_units);
	destroy_locks( msg_lock, 1);

	/*empty the pipe */
	while (ioctl(msg_pipe[0],FIONREAD,&available) &&
	available>=sizeof(struct queue_unit) ) {
		get_from_queue( &buf, &p);
		DBG("DEBUG:destroy_msg_queue: dumping unused mesage from queue\n");
		shm_free( buf.s );
	}

	close(msg_pipe[0]);
	close(msg_pipe[1]);
}


int put_in_queue( str *buf, struct peer *p)
{
	struct queue_unit qu;

	qu.buf.s   = buf->s;
	qu.buf.len = buf->len;
	qu.p       = p;

	if (write( msg_pipe[1], &qu, sizeof(qu) )!=sizeof(qu) ) {
		LOG(L_ERR,"ERROR:put_in_queue: cannot write into pipe : %s\n",
			strerror(errno));
		return -1;
	}

	lock_get( msg_lock );
	cur_queued_size += buf->len;
	if (max_queued_size<cur_queued_size) max_queued_size=cur_queued_size;
	cur_queued_units++;
	if (max_queued_units<cur_queued_units) max_queued_units=cur_queued_units;
	lock_release( msg_lock );

	return 1;
}


int get_from_queue(str *buf, struct peer **p)
{
	struct queue_unit qu;

	while (read( msg_pipe[0], &qu, sizeof(qu) )!=sizeof(qu) ) {
		if (errno==EINTR)
			continue;
		LOG(L_ERR,"ERROR:put_in_queue: cannot read from pipe : %s\n",
			strerror(errno));
		return -1;
	}
	buf->s = qu.buf.s;
	buf->len = qu.buf.len;
	*p = qu.p;

	lock_get( msg_lock );
	cur_queued_size -= buf->len;
	cur_queued_units--;
	lock_release( msg_lock );


	return 1;

}





