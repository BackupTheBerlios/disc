/*
 * $Id: tcp_accept.c,v 1.6 2003/04/06 22:19:49 bogdan Exp $
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netinet/in.h>
#include "../dprint.h"
#include "../globals.h"
#include "peer.h"
#include "ip_addr.h"
#include "tcp_shell.h"
#include "tcp_accept.h"

#define ADDRESS_ALREADY_IN_USE 98




void *do_accept(void *arg)
{
	struct ip_addr servip;
	union sockaddr_union servaddr;
	union sockaddr_union remote;
	unsigned int server_sock4;
#ifdef USE_IPV6
	unsigned int server_sock6;
	unsigned int bind_retest;
#endif
	unsigned int server_sock;
	struct ip_addr remote_ip;
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
	FD_ZERO( &(tinfo->rd_set) );

	/* add for listening the command pipe */
	FD_SET( tinfo->cmd_pipe[0], &(tinfo->rd_set));
	max_sock = tinfo->cmd_pipe[0];

	server_sock4 = -1;

#if USE_IPV6
	server_sock6 = -1;
	bind_retest = 0;
do_bind6:
	LOG(L_INFO,"INFO: doing socket and bind for IPv6...\n");
	/* create the listening socket fpr IPv6 */
	if ((server_sock6 = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error creating server socket IPv6: %s\n",
			strerror(errno));
		goto error;
	}

	// TO DO: what was this one good for?
	option = 1;
	setsockopt(server_sock6,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

	memset( &servip, 0, sizeof(servip) );
	servip.af = AF_INET6;
	servip.len = 16;
	memcpy( &servip.u.addr, &in6addr_any, servip.len);
	init_su( &servaddr, &servip, htons(listen_port));

	if ( (bind( server_sock6, (struct sockaddr*)&servaddr,
	sockaddru_len(servaddr)))==-1) {
		LOG(L_ERR,"ERROR:do_accept: error binding server socket IPv6: %s\n",
			strerror(errno));
		goto error;
	}

	/* IMPORTANT: I have to do listen here to be able to ketch an AAIU on IPv4;
	 * bind is not sufficient because of the SO_REUSEADDR option */
	if ((listen( server_sock6, 4)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error listening on server socket IPv6: "
			"%s\n",strerror(errno));
		goto error;
	}

	if (bind_retest)
		goto bind_done;

do_bind4:
#endif
	LOG(L_INFO,"INFO: doing socket and bind for IPv4...\n");
	/* create the listening socket fpr IPv4 */
	if ((server_sock4 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error creating server socket IPv4: %s\n",
			strerror(errno));
		goto error;
	}

	// TO DO: what was this one good for?
	option = 1;
	setsockopt(server_sock4,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

	memset( &servip, 0, sizeof(servip) );
	servip.af = AF_INET;
	servip.len = 4;
	servip.u.addr32[0] = INADDR_ANY;
	init_su( &servaddr, &servip, htons(listen_port));

	if ((bind( server_sock4, (struct sockaddr*)&servaddr,
	sockaddru_len(servaddr)))==-1 ) {
#ifdef USE_IPV6
		if (errno==ADDRESS_ALREADY_IN_USE && !bind_retest) {
			/* I'm doing bind4 for the first time */
			LOG(L_WARN,"WARNING:do_accept: got AAIU for IPv4 with IPv6 on ->"
				" close IPv6 and retry\n");
			close( server_sock6 );
			server_sock6 = -1;
			bind_retest = 1;
			goto do_bind4;
		}
#endif
		LOG(L_ERR,"ERROR:do_accept: error binding server socket IPv4: %s\n",
			strerror(errno));
		goto error;
	}
#ifdef USE_IPV6
	else {
		if (bind_retest) {
			/* if a manage to re-bind4 (after an AAIU) with bind6 disable,
			 * means the OS does automaticlly bind4 when bind6; in this case
			 * i will disable the bind4 and re-bind6 */
			LOG(L_INFO,"INFO:do_accept: bind for IPv4 succeded on retesting "
				"without IPv6 -> close IPv4 and bind only IPv6\n");
			close( server_sock4 );
			server_sock4 = -1;
			goto do_bind6;
		} else {
			LOG(L_INFO,"INFO:do_accept: bind for IPv4 succeded along with"
				" bind IPv6 -> keep them both\n");
		}
	}
#endif


	/* binding part done -> do listen */

#ifdef USE_IPV6
bind_done:

	/* IPv6 socket */
	/* update the max_sock */
	DBG("DEBUG:do_accept: adding server_sock6\n");
	if (server_sock6>max_sock)
		max_sock = server_sock6;

	if (server_sock4!=-1) {
#endif

	/* IPv4 sock */
	if ((listen( server_sock4, 4)) == -1) {
		LOG(L_ERR,"ERROR:do_accept: error listening on server socket IPv4: "
			"%s\n",strerror(errno) );
		goto error;
	}

	DBG("DEBUG:do_accept: adding server_sock4\n");
	/* update the max_sock */
	if (server_sock4>max_sock)
		max_sock = server_sock4;

#ifdef USE_IPV6
	}
#endif



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

		if (
#ifdef USE_IPV6
		(FD_ISSET( server_sock6, &read_set)&&(server_sock=server_sock6)!=0) ||
		( server_sock4!=-1 &&
		(FD_ISSET( server_sock4, &read_set)&&(server_sock=server_sock4)!=0))
#else
		(FD_ISSET( server_sock4, &read_set)&&(server_sock=server_sock4)!=0)
#endif
		) {
			nready--;
			/* do accept */
			length = sizeof( union sockaddr_union);
			accept_sock = accept( server_sock, (struct sockaddr*)&remote,
				&length);
			if (accept_sock==-1) {
				/* accept failed */
				LOG(L_ERR,"ERROR:do_accept: accept failed!\n");
			} else {
				LOG(L_INFO,"INFO:do_accept: new tcp connection accepted!\n");
				/* lookup for the peer */
				su2ip_addr( &remote_ip, &remote);
				peer = lookup_peer_by_ip( &remote_ip );
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
#ifdef USE_IPV6
					FD_SET( server_sock6, &(tinfo->rd_set));
					if (server_sock4!=-1)
						FD_SET( server_sock4, &(tinfo->rd_set));
#else
					FD_SET( server_sock4, &(tinfo->rd_set));
#endif
					break;
				default:
					LOG(L_ERR,"ERROR:do_accept: unknown command "
						"code %d -> ignoring command\n",cmd.code);
			}
		}

	}

error:
	kill( 0, SIGINT );
	return 0;
}

