/*
 * $Id: sipauth.c,v 1.1 2008/02/29 16:02:38 anomarme Exp $
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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../../../dprint.h"
#include "../../../aaa_module.h"
#include "../../../mem/shm_mem.h"
#include "../../../diameter_api/diameter_api.h"

#include "sipaaa_defs.h"
#include "tcp_socket.h"

#define M_NAME	"sipauth"

extern str aaa_realm;
extern str aaa_identity;

typedef struct ctx_entry_ 
{
	int in_port;
	char serviceType;
	unsigned int sip_msg_id;
} ctx_entry_t;

static int update_message(AAAMessage *msg, AAASessionId* sID, int acc);

static int mod_init();
static void mod_destroy();
static int mod_msg(AAAMessage *msg, void *context);
static int mod_tout(int event, AAASessionId* sId, void *context);

/* parameter of the module */
int port = 3000;

static struct module_param params[]={
	{"port",  INT_TYPE, &port},
	{0,0,0} /* null terminated array */
};


struct module_exports exports = {
	"sipauth",
	AAA_CLIENT,
	AAA_APP_NASREQ,
	DOES_AUTH|DOES_ACCT,
	params,
	
	mod_init,
	mod_destroy,
	mod_msg,
	mod_tout
};


pthread_t sipauth_th;

/* this thread waits for requests from the SIP server */
void *sipauth_worker(void *attr)
{
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i, new, acc;
	struct sockaddr_in clientname;
	size_t size;
	AAAMessage* msg;
	AAASessionId* sID;
	AAA_AVP *avp=NULL;	
	ctx_entry_t *ctx;
	rd_buf_t *rb;


	/* Create the socket and set it up to accept connections. */
	sock = make_socket (port);
	if (sock<0 || listen (sock, 1) < 0)
	{
		LOG(L_ERR, M_NAME": could not create the socket\n");
		exit (1);
	}
	/* inchiderea conexiunii TCP la oprirea treadului */
	pthread_cleanup_push(close_tcp_connection, (void*)&sock);

	/* initialize the buffer to read the message */
	rb = (rd_buf_t*)shm_malloc (sizeof (rd_buf_t));
	if (!rb)
	{
		LOG(L_ERR, M_NAME": no more free memory\n");
		close(sock);
		exit(1);
	}
	
	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);
	
	DBG(M_NAME": Waiting for connections....\n");
	
	while (1)
	{
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
		{
			LOG(L_ERR, M_NAME": select function failed\n");
			return 0;
		}
		
		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; i++)
		{
			if (!FD_ISSET (i, &read_fd_set))
				continue;

			if (i == sock)
			{
				/* Connection request on original socket. */
				size = sizeof (clientname);
				new = accept (sock, (struct sockaddr *)&clientname, &size);
				if (new < 0)
				{
					LOG(L_ERR, M_NAME": accept function failed\n");
					continue;
				}
				FD_SET (new, &active_fd_set);
				continue;
			}
			
			/* Data arriving on a already-connected socket. */
			reset_read_buffer(rb);
			switch( do_read(i, rb) )
			{
				case CONN_CLOSED:
				case CONN_ERROR:
						close(i);
						FD_CLR(i, &active_fd_set);
						continue;
			}
				
			/* obtain the structure corresponding to the message */
			msg = AAATranslateMessage(rb->buf, rb->buf_len, 0);	
			if(!msg)
			{
				LOG(L_ERR, M_NAME": message structure not obtained\n");	
				continue;
			}
			ctx = (ctx_entry_t*) shm_malloc(sizeof(ctx_entry_t));
			if(!ctx)
			{
				LOG(L_ERR, M_NAME": no more free memory\n");
				goto error;
			}
			ctx->in_port = i;

			avp = AAAFindMatchingAVP(msg, NULL, AVP_SIP_MSGID,
							vendorID, AAA_FORWARD_SEARCH);
			if(!avp)
				goto error1;

			ctx->sip_msg_id = (*((unsigned int*)(avp->data.s)));	
			DBG("******* ctx->sip_msg_id=%d\n", ctx->sip_msg_id);			
			avp = AAAFindMatchingAVP(msg, NULL, AVP_Service_Type,
							vendorID, AAA_FORWARD_SEARCH);
			if(!avp)
				goto error1;

			ctx->serviceType = avp->data.s[0];	
			acc = 0;
			if(avp->data.s[0] == SIP_ACC_SERVICE ) /* ACCOUNTING */
				acc = 1;

			sID = 0;
			if (AAAStartSession( &sID, get_my_appref(), (void*)ctx)
					!=AAA_ERR_SUCCESS)
			{
				LOG(L_ERR, M_NAME": cannot create a new session\n");
				goto error1;
			}			
			if( update_message(msg, sID, acc) == 0 )
			{
				LOG(L_ERR, M_NAME": message could not be updated\n");
				goto error1;						
			}		
			
			if ( AAASendMessage( msg ) != AAA_ERR_SUCCESS )
			{
				LOG(L_ERR, M_NAME": message could not be sent\n");
				goto error1;	
			}
			continue;

error1:
			shm_free(ctx);
error:
			DBG("Error in sipauth_worker\n");
			AAAFreeMessage( &msg );

		} /* for */
	}/* end while(1) */

	pthread_cleanup_pop(0);
}


/* new avps are added now */
static int update_message(AAAMessage *msg, AAASessionId* sessionId, int acc)
{
	AAA_AVP *avp;
	
	/* add origin realm AVP */
	avp = AAACreateAVP( AVP_Origin_Realm, 0, 0, aaa_realm.s, aaa_realm.len,
						AVP_DUPLICATE_DATA);
	if (!avp || AAAAddAVPToMessage(msg,avp,0)!=AAA_ERR_SUCCESS) 
	{
		LOG(L_ERR, M_NAME": cannot create/add Origin-Realm avp\n");
		if(avp)
			AAAFreeAVP(&avp);
		return 0;
	}
	msg->orig_realm = avp;

	/* add origin host AVP */
	avp = AAACreateAVP( AVP_Origin_Host, 0, 0, aaa_identity.s, 
						aaa_identity.len, AVP_DUPLICATE_DATA);
	if (!avp || AAAAddAVPToMessage(msg,avp,0)!=AAA_ERR_SUCCESS) 
	{
		LOG(L_ERR, M_NAME": cannot create/add Origin-Host avp\n");
		if(avp)
			AAAFreeAVP(&avp);
		return 0;
	}
	msg->orig_host = avp;

	if(!acc) /* update for auth application */
	{
		/* add Auth-Application-ID AVP */
		avp=AAACreateAVP(AVP_Auth_Application_Id, 0, 0, APP_ID_1, APP_ID_1_LEN,
						AVP_DUPLICATE_DATA);
	}
	else /* update for accounting application */
	{
		/* add Acct-Application-ID AVP */
		avp=AAACreateAVP(AVP_Acct_Application_Id, 0, 0, APP_ID_1, APP_ID_1_LEN,
						AVP_DUPLICATE_DATA);
	}
	if(!avp || AAAAddAVPToMessage(msg, avp, 0)!= AAA_ERR_SUCCESS)
	{
		LOG(L_ERR, M_NAME": AuthApplicationID AVP not added \n");
		if(avp)
			AAAFreeAVP(&avp);
		return 0;
	}

	
	/* add SessionID AVP */
	avp = AAACreateAVP( AVP_Session_Id, 0, 0, sessionId->s, sessionId->len,
						AVP_DUPLICATE_DATA);
	if ( !avp  ||  AAAAddAVPToMessage(msg, avp, 0) != AAA_ERR_SUCCESS ) 
	{
		LOG(L_ERR, M_NAME": cannot create/add SessionID avp\n");
		if(avp)
			AAAFreeAVP(&avp);
		return 0;
	}
	msg->sessionId = avp;

	msg->sId = sessionId;

	return 1;
}


int mod_init()
{
	DBG(M_NAME" module : initializing...\n");
	DBG(M_NAME" module : listening port=%d\n", port);
	
	if (pthread_create( &sipauth_th, /*&attr*/ 0, &sipauth_worker, 0)!=0)
	{
		LOG(L_ERR, M_NAME": listener thread for AAA request not created\n");
		return -1;
	}

	return 0;
}


void mod_destroy()
{
	LOG(L_NOTICE, M_NAME" module: selfdestruct triggered\n");
	pthread_cancel( sipauth_th );
}


int mod_msg(AAAMessage *msg, void *context)
{
	ctx_entry_t* myctx;
	AAA_AVP *avp;
	int n;
	
	if(!msg)
	{
		LOG(L_ERR, M_NAME": null message received\n");
		goto error;
	}
	
	myctx = (ctx_entry_t*)context;
	if(!myctx)
	{ 
		LOG(L_ERR, M_NAME": Returned session context is null\n");
		goto error;
	}		

	/* keep just the answers */
	if (is_req(msg))
		goto error;

	/* keep just the right answers */
	if ( msg->commandCode != AA_ANSWER &&
		 msg->commandCode != ACCOUNTING_ANSWER )
		goto error;

	avp = AAACreateAVP(AVP_SIP_MSGID, 0, 0, (char*)(&(myctx->sip_msg_id)), 
						sizeof(myctx->sip_msg_id), AVP_DUPLICATE_DATA);
	if ( !avp  ||  AAAAddAVPToMessage(msg, avp, 0) != AAA_ERR_SUCCESS ) 
	{
		LOG(L_ERR, M_NAME": cannot create/add SIP_MSGID avp\n");
		if(avp)
			AAAFreeAVP(&avp);
		goto error;
	}
	avp = AAACreateAVP(AVP_Service_Type, 0, 0, (char*)(&(myctx->serviceType)), 
						sizeof(myctx->serviceType), AVP_DUPLICATE_DATA);
	if ( !avp  ||  AAAAddAVPToMessage(msg, avp, 0) != AAA_ERR_SUCCESS ) 
	{
		LOG(L_ERR, M_NAME": cannot create/add Service_Type avp\n");
		if(avp)
			AAAFreeAVP(&avp);
		goto error;
	}


	if (AAABuildMsgBuffer(msg)!=AAA_ERR_SUCCESS)
	{
		LOG(L_ERR, M_NAME": message buffer not created\n");
		goto error;
	}
	/* try to write the message to the sip entity that originated the REQ*/
	while( (n=write(myctx->in_port, msg->buf.s, msg->buf.len))==-1 ) 
	{
		if (errno==EINTR)
			continue;
		LOG(L_ERR, M_NAME": write returned error: %s\n", strerror(errno));
		goto error;
	}

	if (n!=msg->buf.len) {
		LOG(L_ERR, M_NAME": write gave no error but wrote less than asked\n");
		goto error;
	}

	AAASessionTimerStart( msg->sId, 5);

	return 1;

error:
	return -1;
}


int mod_tout(int event, AAASessionId* sId, void *context)
{
	AAAEndSession(sId); 
	return 1;
}


