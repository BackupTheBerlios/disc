


#ifndef _TCP_SHELL_COMMON_H_
#define _TCP_SHELL_COMMON_H_

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "ip_addr.h"
#include "../list.h"
//#include "../globals.h"
#include "peer.h"


#define NR_RECEIVE_THREADS  1
#define NR_THREADS          (1+NR_RECEIVE_THREADS)


struct thread_info {
	struct list_head  tl;
	pthread_t         tid;
	unsigned int      load;
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



int init_tcp_shell(void);

void terminate_tcp_shell(void);

int get_new_receive_thread();

void start_tcp_accept();

void tcp_connect(struct peer *p);

void tcp_close( struct peer *p);

int tcp_send( struct peer *p, unsigned char *buf, unsigned int len);

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

