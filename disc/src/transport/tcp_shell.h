


#ifndef _TCP_SHELL_COMMON_H_
#define _TCP_SHELL_COMMON_H_

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "../list.h"
#include "ip_addr.h"
#include "peer.h"



struct thread_info {
	/* linker into the peer list */
	struct list_head  tl;
	/* thread  id */
	pthread_t         tid;
	/* number of peer per thread */
	unsigned int      load;
	/* commad pipe */
	unsigned int      cmd_pipe[2];
	fd_set            rd_set;
	fd_set            wr_set;
};


struct command {
	unsigned int code;
	unsigned int fd;
	struct peer  *peer;
	void         *attrs;
};


enum COMMAND_CODES {
	SHUTDOWN_CMD,             /* (cmd,-,-,-) */
	START_ACCEPT_CMD,         /* (cmd,-,-,-) */
	CONNECT_CMD,              /* (cmd,-,peer,-) */
	ACCEPTED_CMD,             /* (cmd,accepted_fd,peer,-) */
	ADD_PEER_CMD,             /* (cmd,-,peer,-) */
	INACTIVITY_CMD,           /* (cmd,-,peer,-) */
	TIMEOUT_PEER_CMD,         /* (cmd,event,peer,-) */
};



#define COMMAND_SIZE (sizeof(struct command))

#define tcp_send_unsafe( _peer_,  _buf_, _len_) \
	(write( (_peer_)->sock, (_buf_), (_len_)))



int init_tcp_shell(unsigned int nr_receivers);

void terminate_tcp_shell(void);

int get_new_receive_thread();

void start_tcp_accept();

void tcp_connect(struct peer *p);

void tcp_close( struct peer *p);

inline int static write_command(unsigned int fd_pipe, unsigned int code,
							unsigned int fd, struct peer *peer, void *attrs)
{
	struct command cmd;
	cmd.code  = code;
	cmd.fd    = fd;
	cmd.peer  = peer;
	cmd.attrs = attrs;
	return write( fd_pipe, &cmd, COMMAND_SIZE);
}

#endif

