

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include "../dprint.h"
#include "peer.h"
//#include "../globals.h"
#include "common.h"
#include "tcp_accept.h"





void *do_accept(void *arg)
{
	struct sockaddr_in  servaddr;
	struct sockaddr_in  local;
	struct sockaddr_in  remote;
	unsigned int server_sock;
	unsigned int accept_sock;
	unsigned int max_sock;
	unsigned int option;
	unsigned int length;
	struct peer  *peer;
	struct thread_info *tinfo;
	struct command cmd;
	int  nready;
	int  ncmd;
	fd_set read_set;


	/* get the pointer with info about myself */
	tinfo = (struct thread_info*)arg;

	/* create the listening socket */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error creating server socket\n");
		goto error;
	}

	// TO DO: what was this one good for?
	option = 1;
	setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &option,sizeof(option));

	memset(&local, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons( 1812/*listen_port*/  );

	if ((bind(server_sock,(struct sockaddr*)&servaddr,sizeof(servaddr)))==-1) {
		LOG(L_ERR,"ERROR:do_accept: error binding server socket\n");
		goto error;
	}
	if ((listen( server_sock, 4)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error listening on server socket\n");
		goto error;
	}

	/* prepare the fd_set for reading */
	FD_ZERO( &(tinfo->rd_set) );
	max_sock = server_sock;
	FD_SET( tinfo->cmd_pipe[0], &(tinfo->rd_set));
	if (tinfo->cmd_pipe[0]>max_sock)
		max_sock = tinfo->cmd_pipe[0];


	while(1) {
		read_set = tinfo->rd_set;
		nready = select( max_sock+1, &read_set, 0, 0, 0);
		if (nready == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				LOG(L_ERR,"ERROR:do_accept: select fails: %s\n",
					strerror(errno));
				sleep(2);
				continue;
			}
		}

		/*no timeout specified, therefore must never get into this if*/
		if (nready == 0) 
			assert(0);

		if ( FD_ISSET( server_sock, &read_set) ) {
			nready--;
			/* do accept */
			accept_sock = accept( server_sock, (struct sockaddr*)&remote,
				&length);
			if (accept_sock==-1) {
				/* accept failed */
				LOG(L_ERR,"ERROR:do_accept: accept failed!\n");
			} else {
				LOG(L_INFO,"INFO:do_accept: new tcp connection accepted!\n");
				/* lookup for the peer */
				peer = lookup_peer_by_ip( &(remote.sin_addr) );
				if (!peer) {
					LOG(L_ERR,"ERROR:do_accept: connection from an"
						" unknown peer!\n");
					close( accept_sock );
				} else {
					/* generate ACCEPT_DONE command for the peer */
					write_command(peer->fd,ACCEPTED_CMD,accept_sock,peer,0);
				}
			}
		}

		if (nready && FD_ISSET( tinfo->cmd_pipe[0], &read_set) ) {
			/* read the command */
			if ((ncmd=read( tinfo->cmd_pipe[0], &cmd, COMMAND_SIZE))
			!=COMMAND_SIZE) {
				if (ncmd==0) {
					LOG(L_CRIT,"ERROR:do_accept: reading command "
						"pipe -> it's closed!!\n");
					goto error;
				}else if (ncmd<0) {
					LOG(L_ERR,"ERROR:do_accept: reading command "
						"pipe -> %s\n",strerror(errno));
					continue;
				} else {
					LOG(L_ERR,"ERROR:do_accept: reading command "
						"pipe -> only %d\n",ncmd);
					continue;
				}
			}
			/* execute the command */
			switch (cmd.code) {
				case SHUTDOWN_CMD:
					LOG(L_INFO,"INFO:do_accept: SHUTDOWN command "
						"received-> exiting\n");
					return 0;
					break;
				case START_ACCEPT_CMD:
					LOG(L_INFO,"INFO:do_accept: START_ACCEPT command "
						"received-> start accepting connections\n");
						FD_SET( server_sock, &(tinfo->rd_set));
					break;
				default:
					LOG(L_ERR,"ERROR:do_accept: unknown command "
						"code %d -> ignoring command\n",cmd.code);
			}
		}

	}

error:
	return 0;
}

