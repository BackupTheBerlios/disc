/*
 * $Id: tcp_socket.h,v 1.1 2008/02/29 16:02:38 anomarme Exp $
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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H


#define MAX_AAA_MSG_SIZE  65536

#define CONN_SUCCESS	 1 
#define CONN_ERROR		-1
#define CONN_CLOSED		-2

/* information needed for reading messages from tcp connection */
typedef struct rd_buf
{
	unsigned int first_4bytes;
	unsigned int buf_len;
	unsigned char *buf;
} rd_buf_t;

int make_socket (uint16_t port);
int do_read( int socket, rd_buf_t *p);
void reset_read_buffer(rd_buf_t *rb);
void close_tcp_connection(void* sfd);

#endif /* TCP_SOCKET_H */

