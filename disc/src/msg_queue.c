/*
 * $Id: msg_queue.c,v 1.2 2003/04/06 22:19:48 bogdan Exp $
 *
 * 2003-03-31 created by bogdan
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "dprint.h"
#include "msg_queue.h"

struct queue_unit {
	str  buf;
	struct peer *p;
};

int msg_pipe[2];


int init_msg_queue()
{
	if (pipe( msg_pipe )==-1) {
		LOG(L_ERR,"ERROR:init_msg_queue: cannot create pipe : %s\n",
			strerror(errno));
		goto error;
	}
	return 1;

error:
	return -1;
}


void destroy_msg_queue()
{
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
	return 1;

}





