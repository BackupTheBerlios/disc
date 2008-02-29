/*
 * $Id: tcp_socket.c,v 1.1 2008/02/29 16:02:38 anomarme Exp $
 *
 * Copyright (C) 2002-2003 Fhg Fokus
 *
 * This file is part of disc, a free diameter server/client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "../../../dprint.h"
#include "../../../str.h"
#include "../../../mem/shm_mem.h"
#include "../../../diameter_msg/diameter_msg.h"

#include "tcp_socket.h"

#define M_NAME	"acc"

int make_socket (uint16_t port)
{
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	  return -1;
	
	/* Give the socket a name. */
	name.sin_family = PF_INET;
	name.sin_port = htons (port);
	name.sin_addr.s_addr= INADDR_ANY;
	
	if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
	  return -1;

	return sock;
}

void reset_read_buffer(rd_buf_t *rb)
{
	rb->first_4bytes = 0;
	rb->buf_len = 0;
	rb->buf	= 0;
}

/* read from a socket, an AAA message buffer */
int do_read( int socket, rd_buf_t *p)
{
	unsigned char  *ptr;
	unsigned int   wanted_len, len;
	int n;

	if (p->buf==0)
	{
		wanted_len = sizeof(p->first_4bytes) - p->buf_len;
		ptr = ((unsigned char*)&(p->first_4bytes)) + p->buf_len;
	}
	else
	{
		wanted_len = p->first_4bytes - p->buf_len;
		ptr = p->buf + p->buf_len;
	}

	while( (n=recv( socket, ptr, wanted_len, MSG_DONTWAIT ))>0 ) 
	{
//		DBG("DEBUG:do_read (sock=%d)  -> n=%d (expected=%d)\n",
//			p->sock,n,wanted_len);
		p->buf_len += n;
		if (n<wanted_len)
		{
			//DBG("only %d bytes read from %d expected\n",n,wanted_len);
			wanted_len -= n;
			ptr += n;
		}
		else 
		{
			if (p->buf==0)
			{
				/* I just finished reading the the first 4 bytes from msg */
				len = ntohl(p->first_4bytes)&0x00ffffff;
				if (len<AAA_MSG_HDR_SIZE || len>MAX_AAA_MSG_SIZE)
				{
					LOG(L_ERR,"ERROR:do_read (sock=%d): invalid message "
						"length read %u (%x)\n", socket, len, p->first_4bytes);
					goto error;
				}
				//DBG("message length = %d(%x)\n",len,len);
				if ( (p->buf=shm_malloc(len))==0  )
				{
					LOG(L_ERR,"ERROR:do_read: no more free memory\n");
					goto error;
				}
				*((unsigned int*)p->buf) = p->first_4bytes;
				p->buf_len = sizeof(p->first_4bytes);
				p->first_4bytes = len;
				/* update the reading position and len */
				ptr = p->buf + p->buf_len;
				wanted_len = p->first_4bytes - p->buf_len;
			}
			else
			{
				/* I finished reading the whole message */
				DBG("DEBUG:do_read (sock=%d): whole message read (len=%d)!\n",
					socket, p->first_4bytes);
				return CONN_SUCCESS;
			}
		}
	}

	if (n==0)
	{
		LOG(L_INFO,"INFO:do_read (sock=%d): FIN received\n", socket);
		return CONN_CLOSED;
	}
	if ( n==-1 && errno!=EINTR && errno!=EAGAIN )
	{
		LOG(L_ERR,"ERROR:do_read (sock=%d): n=%d , errno=%d (%s)\n",
			socket, n, errno, strerror(errno));
		goto error;
	}
error:
	return CONN_ERROR;
}





void close_tcp_connection(void* sfd)
{
	DBG(M_NAME": TCP connection closed:%d\n", *((int*)sfd));

	shutdown(*((int*)sfd), 2);
}
