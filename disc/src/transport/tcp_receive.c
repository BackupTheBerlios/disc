/*
 * $Id: tcp_receive.c,v 1.1 2003/03/14 16:05:58 bogdan Exp $
 *
 *  History:
 *  --------
 *  2003-03-07  created by bogdan
 *  2003-03-12  converted to shm_malloc/shm_free (andrei)
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "../dprint.h"
#include "../utils/ip_addr.h"
#include "../utils/list.h"
#include "../utils/str.h"
#include "../diameter_types.h"
#include "../peer.h"
#include "../message.h"
#include "receive.h"
#include "common.h"


#include "../mem/shm_mem.h"



inline int do_read( struct peer *p)
{
	unsigned char  *ptr;
	unsigned int   wanted_len, n, len;
	str s;

	if (p->buf==0) {
		wanted_len = sizeof(p->first_4bytes) - p->buf_len;
		ptr = ((unsigned char*)&(p->first_4bytes)) + p->buf_len;
	} else {
		wanted_len = p->first_4bytes - p->buf_len;
		ptr = p->buf + p->buf_len;
	}

	while( (n=read( p->sock, ptr, wanted_len))>0 ) {
		//DBG(">>>>>> read -> n=%d (expected=%d)\n",n,wanted_len);
		p->buf_len += n;
		if (n<wanted_len) {
			//DBG("only %d bytes read from %d expected\n",n,wanted_len);
			wanted_len -= n;
			ptr += n;
		} else {
			if (p->buf==0) {
				/* I just finished reading the the first 4 bytes from msg */
				len = ntohl(p->first_4bytes)&0x00ffffff;
				//DBG("message length = %d(%x)\n",len,len);
				if ( (p->buf=shm_malloc(len))==0  ) {
					LOG(L_ERR,"ERROR:do_read: no more free memory\n");
					goto error;
				}
				*((unsigned int*)p->buf) = p->first_4bytes;
				p->buf_len = sizeof(p->first_4bytes);
				p->first_4bytes = len;
				/* update the reading position and len */
				ptr = p->buf + p->buf_len;
				wanted_len = p->first_4bytes - p->buf_len;
			} else {
				/* I finished reading the whole message */
				DBG("DEBUG:do_read: whole message read (len=%d)!\n",
					p->first_4bytes);
				s.s   = p->buf;
				s.len = p->buf_len;
				/* reset the read buffer */
				p->buf = 0;
				p->buf_len = 0;
				p->first_4bytes = 0;
				/* process the mesage */
				dispatch_message( p, &s);
				break;
			}
		}
	}

	//DBG(">>>>>>>>>> n=%d, errno=%d \n",n,errno);
	if (n==0 || (n==-1 && errno!=EINTR && errno!=EAGAIN) )
		goto error;

	return 1;
error:
	return -1;
}



inline void tcp_accept(struct peer *p, unsigned int s)
{
	struct tcp_info  info;
	struct sockaddr_in local;
	unsigned int length;
	unsigned int option;

	/* get the socket non-blocking */
	option = fcntl( s, F_GETFL, 0);
	fcntl( s, F_SETFL, option | O_NONBLOCK);

	/* get the address that the socket is connected to you */
	length = sizeof(struct sockaddr_in);
	if (getsockname( s, (struct sockaddr *)&local, &length) == -1) {
		LOG(L_ERR,"ERROR:tcp_accept: getsocname failed: \"%s\"\n",
			strerror(errno));
		goto error;
	}

	info.sock  = s;
	info.local = &local.sin_addr;
	/* call the peer state machine */
	if (peer_state_machine( p, TCP_ACCEPT, &info)==-1)
		goto error;

	FD_SET( s, &p->tinfo->rd_set);
	return;
error:
	close(s);
}




inline void do_connect( struct peer *p, int sock )
{
	struct sockaddr_in local;
	struct tcp_info    info;
	unsigned int       length;

	length = sizeof(struct sockaddr_in);
	if (getsockname(sock,(struct sockaddr*)&local,&length)==-1) {
		LOG(L_ERR,"ERROR:do_conect: getsockname failed : %s\n",
			strerror(errno));
		peer_state_machine( p, TCP_CONN_FAILED,0);
		close( sock );
		return;
	}

	info.sock = sock;
	info.local = &(local.sin_addr);
	if (peer_state_machine( p, TCP_CONNECTED, &info)==-1 ) {
		/* peer state machine didn't accepted this connect */
		close(sock);
	} else {
		/* start listening on this socket */
		FD_SET( sock, &(p->tinfo->rd_set));
	}
}




inline void tcp_connect(struct peer *p)
{
	struct sockaddr_in remote;
	struct tcp_info    info;
	int option;
	int sock;

	sock = -1;

	/* create a new socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock==-1) {
		LOG(L_ERR,"ERROR:tcp_connect: cannot connect, failed to create "
				"new socket\n");
		goto error;
	}
	/* set connecting socket to non-blocking */
	option = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, option | O_NONBLOCK);

	/* remote peer */
	memset( &remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_addr   = p->ip;
	remote.sin_port   = htons( p->port );

	/* call connect non-blocking */
	if (connect(sock,(struct sockaddr*)&remote,sizeof(remote))==0) {
		/* connection creation succeedes on the spot */
		do_connect(p, sock);
	} else {
		if (errno == EINPROGRESS) {
			/* connection creation performs asynch. */
			DBG("DEBUG:do_dispatch: connecting socket %d in progress\n",sock);
			info.sock = sock;
			if (peer_state_machine( p, TCP_CONN_IN_PROG, &info)==-1)
				close(sock);
			else
				FD_SET( sock, &(p->tinfo->wr_set) );
		} else {
			DBG("DEBUG:tcp_connect: connect failed : %s\n", strerror(errno));
			goto error;
		}
	}

	return;
error:
	if (sock!=-1)
		close(sock);
	/* generate connect failed */
	peer_state_machine( p, TCP_CONN_FAILED, 0);
}




void tcp_close( struct peer *p)
{
	DBG("DEBUG:tcp_close: closing socket %d\n",p->sock);
	close(p->sock);
	FD_CLR( p->sock, &p->tinfo->rd_set);
	FD_CLR( p->sock, &p->tinfo->wr_set);
}




int tcp_send( struct peer *p, unsigned char *buf, unsigned int len)
{
	int ret=-1;

	lock_get( p->mutex );
	if (p->state==PEER_CONN)
		ret = write( p->sock, buf, len);
	lock_release( p->mutex);

	return ret;
}




void *do_receive(void *arg)
{
	struct list_head    *lh;
	struct list_head    peers;
	struct thread_info  *tinfo;
	struct peer         *p;
	fd_set ret_rd_set;
	fd_set ret_wr_set;
	int option;
	int length;
	int nready;
	int ncmd;
	struct command cmd;

	/* init section */
	tinfo = (struct thread_info*)arg;
	FD_ZERO( &(tinfo->rd_set) );
	FD_ZERO( &(tinfo->wr_set) );
	INIT_LIST_HEAD( &peers );

	/* add command pipe for listening */
	FD_SET( tinfo->cmd_pipe[0] , &tinfo->rd_set);

	while(1) {
		ret_rd_set = tinfo->rd_set;
		ret_wr_set = tinfo->wr_set;
		nready = select( FD_SETSIZE+1, &ret_rd_set, &ret_wr_set, 0, 0);

		if (nready == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				LOG(L_ERR,"ERROR:do_receive: dispatcher call to select fails:"
					" %s\n", strerror(errno));
				sleep(2);
				continue;
			}
		}

		/*no timeout specified, therefore must never get into this if*/
		if (nready == 0) 
			assert(0);

		if ( FD_ISSET( tinfo->cmd_pipe[0], &ret_rd_set) ) {
			nready--;
			/* read the command */
			ncmd = read( tinfo->cmd_pipe[0], &cmd, COMMAND_SIZE);
			if ( ncmd!=COMMAND_SIZE ){
				if (ncmd==0) {
					LOG(L_CRIT,"ERROR:do_receive: reading command "
						"pipe -> it's closed!!\n");
					goto error;
				}else if (ncmd<0) {
					LOG(L_ERR,"ERROR:do_receive: reading command "
						"pipe -> %s\n",strerror(errno));
				} else {
					LOG(L_ERR,"ERROR:do_receive: reading command "
						"pipe -> only %d\n",ncmd);
				}
			} else {
				/* execute the command */
				switch (cmd.code) {
					case SHUTDOWN_CMD:
						LOG(L_INFO,"INFO:do_receive: SHUTDOWN command "
							"received-> closing peer\n");
						//peer_state_machine( cmd.peer, PEER_HANGUP, 0);
						return 0;
						break;
					case ADD_PEER_CMD:
						LOG(L_INFO,"INFO:do_receive: adding new peer\n");
						list_add_tail( &cmd.peer->thd_peer_lh, &peers);
						cmd.peer->tinfo = tinfo;
					case CONNECT_CMD:
						LOG(L_INFO,"INFO:do_receive: connecting peer\n");
						tcp_connect( cmd.peer );
						break;
					case ACCEPTED_CMD:
						LOG(L_INFO,"INFO:do_receive: accept received\n");
						tcp_accept( cmd.peer , cmd.fd);
						break;
					case TIMEOUT_PEER_CMD:
						LOG(L_INFO,"INFO:do_receive: timeout received\n");
						peer_state_machine( cmd.peer, cmd.fd, 0);
						break;
					case INACTIVITY_CMD:
						LOG(L_INFO,"INFO:do_receive: inactivity detected\n");
						peer_state_machine( cmd.peer, PEER_IS_INACTIV, 0);
						break;
					default:
						LOG(L_ERR,"ERROR:do_receive: unknown command "
							"code %d -> ignoring command\n",cmd.code);
				}
			}
		}

		list_for_each( lh, &peers) {
			if (!nready--)
				break;
			p = list_entry( lh, struct peer, thd_peer_lh);

			if ( FD_ISSET( p->sock, &ret_rd_set) ) {
				option = -1;
				ioctl( p->sock, FIONREAD, &option);
				if (option==0) {
					/* FIN received */
					LOG(L_INFO,"INFO:do_receive: FIN received for socket"
						" %d.\n",p->sock);
					peer_state_machine( p, TCP_CLOSE, 0);
				} else {
					/* data received */
					if (do_read( p )==-1) {
						LOG(L_ERR,"ERROR:do_receive: error reading-> close\n");
						peer_state_machine( p, TCP_CLOSE, 0);
					}
				}
				continue;
			}

			if ( FD_ISSET( p->sock, &ret_wr_set) ) {
				DBG("DEBUG:do_receive: connect done on socket %d\n",p->sock);
				FD_CLR( p->sock, &tinfo->wr_set);
				length = sizeof(option);
				if (getsockopt( p->sock, SOL_SOCKET, SO_ERROR, &option,
				&length)==-1 || option!=0 ) {
					LOG(L_ERR,"ERROR:do_receive: getsockopt=%s ; res=%d\n",
						strerror(errno),option);
					close( p->sock );
					peer_state_machine( p, TCP_CONN_FAILED, 0);
				} else {
					do_connect( p, p->sock);
				}
			}
		}

	}/*while*/

error:
	return 0;
}




