/*
 * $Id: message.c,v 1.2 2003/03/10 19:17:32 bogdan Exp $
 *
 * 2003-02-03 created by bogdan
 *
 */

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "diameter_api.h"
#include "utils/dprint.h"
#include "utils/str.h"
#include "utils/misc.h"
#include "utils/aaa_lock.h"
#include "global.h"
#include "peer.h"
#include "message.h"
#include "avp.h"
#include "session.h"
#include "trans.h"


/* local var */
struct msg_manager  *msg_mgr=0;



#define get_3bytes(_b) \
	((((unsigned int)(_b)[0])<<16)|(((unsigned int)(_b)[1])<<8)|\
	(((unsigned int)(_b)[2])))

#define get_4bytes(_b) \
	((((unsigned int)(_b)[0])<<24)|(((unsigned int)(_b)[1])<<16)|\
	(((unsigned int)(_b)[2])<<8)|(((unsigned int)(_b)[3])))

#define set_3bytes(_b,_v) \
	{(_b)[0]=((_v)&0x00ff0000)>>16;(_b)[1]=((_v)&0x0000ff00)>>8;\
	(_b)[2]=((_v)&0x000000ff);}

#define set_4bytes(_b,_v) \
	{(_b)[0]=((_v)&0xff000000)>>24;(_b)[1]=((_v)&0x00ff0000)>>16;\
	(_b)[2]=((_v)&0x0000ff00)>>8;(_b)[3]=((_v)&0x000000ff);}



/*  Inits generators for end-to-end-ID and hop-by-hop-ID
 */
int init_msg_manager()
{
	aaa_lock *locks;

	/* build a new msg_manager structure */
	msg_mgr = (struct msg_manager*)malloc( sizeof(struct msg_manager) );
	if (!msg_mgr) {
		LOG(L_ERR,"ERROR:init_msg_manager: no more free memory!\n");
		goto error;
	}

	/* init the locks */
	locks = create_locks( 2 );
	if (!locks) {
		LOG(L_ERR,"ERROR:init_msg_manager: cannot create locks!\n");
		goto error;
	}
	msg_mgr->end_to_end_lock = &(locks[0]);
	msg_mgr->hop_by_hop_lock = &(locks[1]);

	/* init end_to_end_ID and hop_by_hop_ID */
	msg_mgr->end_to_end  = ((unsigned int)time(0))<<20;
	msg_mgr->end_to_end |= ((unsigned int)rand( ))>>12;

	LOG(L_INFO,"INFO:init_msg_manager: message manager started\n");
	return 1;
error:
	LOG(L_INFO,"INFO:init_msg_manager: FAILED to start message manager!!\n");
	return -1;
}



/* Destroy the lock and free the memory used by generators
 */
void destroy_msg_manager()
{
	if (msg_mgr) {
		/* destroy the locks */
		if (msg_mgr->end_to_end_lock)
			destroy_locks( msg_mgr->end_to_end_lock , 2 );
		/* free memory */
		free( msg_mgr );
	}
	LOG(L_INFO,"INFO:destroy_msg_manager: message manager stoped\n");
}



inline AAAMsgIdentifier generate_endtoendID()
{
	unsigned int id;
	lock( msg_mgr->end_to_end_lock );
	id = msg_mgr->end_to_end++;
	unlock( msg_mgr->end_to_end_lock );
	return id;
}



/************************** MESSAGE FUNCTIONS ********************************/


int is_req_local(AAAMessage *msg, int *only_realm)
{
	AAA_AVP *avp;
	str     s;

	if (only_realm)
		*only_realm = 0;

	/* do we have avps? */
	if (!msg->avpList || !msg->avpList->head)
		goto is_local;

	/* search for the destination-host AVP */
	avp = AAAFindMatchingAVP(msg->avpList,msg->avpList->head,
			293,0,AAA_FORWARD_SEARCH);
	if (avp) {
		/* does the host match? */
		trim_lr( avp->data , s );
		if ( s.len!=aaa_identity.len || strncasecmp(s.s,aaa_identity.s,s.len) )
			/* aaa identity doesn't match*/
			goto no_match;
		goto is_local;
	} else {
		/* search for the destination-realm AVP */
		avp = AAAFindMatchingAVP(msg->avpList,msg->avpList->head,
				283,0,AAA_FORWARD_SEARCH);
		if (!avp)
			/* no host and realm -> local */
			goto is_local;
		/* is it our realm? */
		trim_lr( avp->data , s );
		if ( s.len!=aaa_realm.len || strncasecmp(s.s,aaa_realm.s,s.len) )
			/* realm doesn't match*/
			goto no_match;
		/* it's for our realm! */
		if (only_realm)
			*only_realm = 1;
		goto is_local;
	}

no_match:
	return 0;
is_local:
	return 1;
}



int  accept_local_request( AAAMessage *msg )
{
	return 1;
}




int process_msg( unsigned char *buf, unsigned int len, struct peer *pr)
{
	AAAMessage     *msg;
	AAA_AVP        *avp;
	struct trans   *tr;
	struct session *ses;
	unsigned int   event;
	int            only_realm;
	int            is_local;

	msg = 0;
	tr  = 0;

	if (!buf || !len) {
		LOG(L_ERR,"ERROR:process_msg: empty buffer received!\n");
		goto error;
	}
	DBG("****len=%d\n",len);

	/*parse the msg from buffer */
	if ( (msg=AAATranslateMessage( buf, len ))==0 )
		goto error;

	if (!is_req(msg)) {
		/* it's a response -> find its transaction and remove it from 
		 * hash table - search and remove is an atomic operation */
		tr = transaction_lookup( hash_table, msg, 1);
		if (!tr) {
			LOG(L_ERR,"ERROR:process_msg: respons received, but no"
				" transaction found!\n");
			goto error;
		}
		/* stop the timeout timer */
		//..............................
		/* is a local response? */
		if (!tr->in_req) {
			/* there is no incoming request->it's a local response */
			event = 0;
			switch (msg->commandCode) {
				case 257: /* peer event CEA */
					event += (!event)*CEA_RECEIVED;
				case 280: /* peer event DWA */
				case 282: /* peer event DPA */
					event += (!event)*DPA_RECEIVED;
					peer_state_machine( pr, event, tr);
					break;
				case 274: /* session event ASA */
					event += (!event)*AAA_ASA_RECEIVED;
				case 258: /* session event RAA */
					event += (!event)*AAA_RAA_RECEIVED;
				case 275: /* session event STA */
					event += (!event)*AAA_STA_RECEIVED;
				default:  /* session event  AA */
					event += (!event)*AAA_AA_RECEIVED;
					/* lokup up for the Session-ID AVP */
					if (!msg->avpList || !msg->avpList->head ||
					(avp=AAAFindMatchingAVP(msg->avpList,msg->avpList->head,
					263,0,AAA_FORWARD_SEARCH))==0 ) {
						LOG(L_ERR,"ERROR:process_msg: cannot find Session-ID "
							"AVP in message (%d)!\n", msg->commandCode);
						goto error;
					}
					/* lookup for the session */
					ses = session_lookup(hash_table,&(avp->data));
					if (!ses) {
						LOG(L_ERR,"ERROR:process_msg: respons received (%d), "
							"transaction found, but no session found!\n",
							msg->commandCode);
						goto error;
					}
					session_state_machine( ses, event);
			}
		} else {
			/* we have to backward the response */
			//..............
		}
		/* we are done with this transaction */
		destroy_transaction( tr );
	} else {
		/* it's a request -> make a transaction for it */
		tr = create_transaction( msg, 0/*ses*/, pr, TR_INCOMING_REQ);
		if (!tr)
			goto error;
		/* is it a local one or not? */
		is_local = is_req_local( msg, &only_realm);
		if (is_local && only_realm )
			is_local = accept_local_request( msg );
		if (is_local) {
			/* process a local request -> push the according event */
			event = 0;
			switch (msg->commandCode) {
				case 257: /* peer event CER */
					event += (!event)*CER_RECEIVED;
				case 280: /* peer event DWR */
				case 282: /* peer event DPR */
					event += (!event)*DPR_RECEIVED;
					peer_state_machine( pr, event, tr);
					break;
				case 274: /* session event ASR */
					event += (!event)*AAA_ASR_RECEIVED;
				case 258: /* session event RAR */
					event += (!event)*AAA_RAR_RECEIVED;
				case 275: /* session event STR */
					event += (!event)*AAA_STR_RECEIVED;
				default:  /* session event  AR */
					event += (!event)*AAA_AR_RECEIVED;
					/* lokup up for the Session-ID AVP */
					if (!msg->avpList || !msg->avpList->head ||
					(avp=AAAFindMatchingAVP(msg->avpList,msg->avpList->head,
					263,0,AAA_FORWARD_SEARCH))==0 ) {
						LOG(L_ERR,"ERROR:process_msg: cannot find Session-ID "
							"AVP in message (%d)!\n", msg->commandCode);
						goto error;
					}
					/* lookup for the session */
					ses = session_lookup(hash_table,&(avp->data));
					if (!ses) {
						LOG(L_ERR,"ERROR:process_msg: respons received (%d), "
							"transaction found, but no session found!\n",
							msg->commandCode);
						goto error;
					}
					tr->ses = ses;
					session_state_machine( ses, event);
			}
		} else {
			/* forward or rely the request */
			//                             
		}
	}

	return 1;
error:
	if (tr)
		destroy_transaction(tr);
	else if (msg)
		AAAFreeMessage( &msg );
	return -1;
}




/* from a AAAMessage structure, a buffer to be send is build
 */
unsigned char* build_buf_from_msg( AAAMessage *msg, unsigned int *length)
{
	unsigned char *buf;
	unsigned char *p;
	unsigned int  len;
	unsigned int  k;
	AAA_AVP       *avp;

	/* first let's comput the length of the buffer */
	len = AAA_MSG_HDR_SIZE; /* AAA message header size */
	/* count and add the avps */
	if (msg->avpList && msg->avpList->head) {
		for(avp=msg->avpList->head;avp;avp=avp->next) {
			len += AVP_HDR_SIZE + (AVP_VENDOR_ID_SIZE*((avp->flags&0x80)!=0)) +
				avp->data.len /*data*/ +
				((avp->data.len&3)?4-(avp->data.len&3):0) /*padding*/;
		}
	}

	DBG("xxxx len=%d\n",len);
	/* allocate some memory */
	buf = (unsigned char*)malloc( len );
	if (!buf) {
		LOG(L_ERR,"ERROR:build_buf_from_msg: no more free memory!\n");
		goto error;
	}
	memset(buf, 0, len);

	/* fill in the buffer */
	p = buf;
	/* DIAMETER HEADER */
	/* Diameter Version */
	(*p++) = 1;
	/* message length */
	set_3bytes(p,len);
	p += 3;
	/* flags */
	(*p++) = (unsigned char)msg->flags;
	/* command code */
	set_3bytes(p,msg->commandCode);
	p += 3;
	/* application-ID */
	p +=4;
	/* hop by hop id */
	set_4bytes(p,msg->hopbyhopID);
	p +=4;
	/* end to end id */
	set_4bytes(p,msg->endtoendID);
	p +=4;

	/* AVPS */
	if (msg->avpList && msg->avpList->head) {
		for(avp=msg->avpList->head;avp;avp=avp->next) {
			/* AVP HEADER */
			/* avp code */
			set_4bytes(p,avp->code);
			p +=4;
			/* flags */
			(*p++) = (unsigned char)avp->flags;
			/* avp length */
			k = AVP_HDR_SIZE + AVP_VENDOR_ID_SIZE*((avp->flags&0x80)!=0) +
				avp->data.len;
			set_3bytes(p,k);
			p += 3;
			/* vendor id */
			if ((avp->flags&0x80)!=0) {
				set_4bytes(p,avp->vendorId);
				p +=4;
			}
			/* data */
			memcpy( p, avp->data.s, avp->data.len);
			p += avp->data.len+((avp->data.len&3)?4-(avp->data.len&3):0);
		}
	}

	if (p-buf!=len) {
		LOG(L_ERR,"BUG: build_buf_from_msg: mismatch between len and buf!\n");
		goto error;
	}

	if (length)
		*length = p-buf;

	return buf;
error:
	return 0;
}



/*  sends out a aaa message, within or not a session
 */
int send_aaa_message( AAAMessage *msg, struct trans *tr, struct session *ses,
														struct peer *dst_peer )
{
	unsigned char *buf;
	unsigned int  len;
	int ret;
	str s;

	buf = 0;

	/* find the destination peer */
	if (!dst_peer) {
		/* try find the destination-host avp */
	}

	/* if the mesasge is a request... */
	if ( is_req(msg) ) {
		tr = 0;
		/* build a transaction for it */
		if ((tr=create_transaction( msg, ses, dst_peer, TR_OUTGOING_REQ))==0) {
			LOG(L_ERR,"ERROR:send_aaa_message: cannot create a new"
				" transaction!\n");
			goto error;
		}
		/* link the transaction into the hash table ; use end-to-end-ID for
		 * calculating the hash_code */
		s.s = (char*)&(msg->endtoendID);
		s.len = sizeof(msg->endtoendID);
		tr->linker.hash_code = hash( &s );
		tr->linker.type = TRANSACTION_CELL_TYPE;
		add_cell_to_htable( hash_table, (struct h_link*)tr );
		/* the hash label is used as ho-by-hob ID */
		msg->hopbyhopID = tr->linker.label;
	} else {
		/* the message is a reply -> sen it to the peer where req came from */
		dst_peer = tr->in_peer;
	}

	/* build the buffer */
	buf = build_buf_from_msg( msg, &len);
	if (!buf)
		goto error;

	/* send the message */
	if (msg->commandCode==257||msg->commandCode==280||msg->commandCode==282)
		ret = tcp_send_unsafe( dst_peer, buf, len);
	else
		ret = tcp_send( dst_peer, buf, len);
	if (ret==-1) {
		LOG(L_ERR,"ERROR:send_aaa_message: tcp_send returned error!\n");
		goto error;
	}

	if ( is_req(msg) ) {
		/* if request -> start the timeout timer */
		add_to_timer_list( &(tr->timeout) , tr_timeout_timer ,
			get_ticks()+TR_TIMEOUT_TIMEOUT );
	} else {
		/* if response, destroy everything */
		/* destroy the transaction; this will destroy also the msg */
		destroy_transaction( tr );
	}

	free(buf);
	return 1;
error:
	if (buf)
		free(buf);
	if (tr)
		destroy_transaction(tr);
	return -1;
}



AAAMessage* build_rpl_from_req(AAAMessage *req, unsigned int result_code,
																str *err_msg)
{
	AAAMessage *rpl=0;
	AAA_AVP    *avp=0;

	/* create the reply form request */
	rpl = AAANewMessage( req->commandCode, req->vendorId, 0/*sessionId*/,
			0/*extensionId*/, req);
	if (!rpl)
		goto error;

	/* Result-Code AVP */
	if ( create_avp( &avp, 268, 0, 0, (char*)&result_code, 4, 1)!=
	AAA_ERR_SUCCESS)
		goto error;
	if ( AAAAddAVPToList( &rpl->avpList, avp, 0)!=
	AAA_ERR_SUCCESS)
		goto avp_error;

	/* is it a protocol error code? */
	if (result_code>=3000 && result_code<4000){
		/* set the error bit in msg flags */
		rpl->flags |=0x20;
	} else {
		if (err_msg && err_msg->s && err_msg->len) {
			/* Error-Message AVP */
			if ( create_avp(&avp,281,0,0,err_msg->s,err_msg->len,0)!=
			AAA_ERR_SUCCESS)
				goto error;
			if (AAAAddAVPToList(&rpl->avpList,avp,rpl->avpList->tail)!=
			AAA_ERR_SUCCESS)
				goto avp_error;
		}
	}

	return rpl;
avp_error:
	AAAFreeAVP( &avp );
error:
	AAAFreeMessage( &rpl );
	return 0;
}




/****************************** API FUNCTIONS ********************************/


/* allocates a new AAAMessage
 */
AAAMessage *AAANewMessage(
	AAACommandCode commandCode,
	AAAVendorId vendorId,
	AAASessionId *sessionId,
	AAAExtensionId extensionId,
	AAAMessage *request)
{
	AAAMessage *msg;
	AAA_AVP    *avp;
	AAA_AVP    *avp_t;

	msg = 0;

	/* allocated a new AAAMessage structure a set it to 0 */
	msg = (AAAMessage*)malloc(sizeof(AAAMessage));
	if (!msg) {
		LOG(L_ERR,"ERROR:AAANewMessage: no more free memory!!\n");
		goto error;
	}
	memset(msg,0,sizeof(AAAMessage));

	/* command code */
	msg->commandCode = commandCode;
	/* vendor ID */
	msg->vendorId = vendorId;

	if (!request) {
		/* it's a new request -> set the flag */
		msg->flags = 0x80;
		/* add session ID */
		if (sessionId) {
			if (AAACreateAVP( &avp, 263, 0, 0, sessionId->s, sessionId->len)
			!=AAA_ERR_SUCCESS || AAAAddAVPToList( &(msg->avpList), avp, 0)
			!=AAA_ERR_SUCCESS )
				goto error;
		}
		/* generate an end-to-end identifier */
		msg->endtoendID = generate_endtoendID();
		/* add origin host AVP */
		if (AAACreateAVP( &avp, 264, 0, 0, aaa_identity.s, aaa_identity.len)
		!=AAA_ERR_SUCCESS || AAAAddAVPToList( &(msg->avpList), avp,
		msg->avpList?msg->avpList->tail:0)!=AAA_ERR_SUCCESS )
			goto error;
		/* add origin realm AVP */
		if (AAACreateAVP( &avp, 296, 0, 0, aaa_realm.s, aaa_realm.len)
		!=AAA_ERR_SUCCESS || AAAAddAVPToList( &(msg->avpList), avp,
		msg->avpList->tail)!=AAA_ERR_SUCCESS )
			goto error;
	} else {
		/* set the P flag as in request */
		msg->flags |= request->flags&0x40;
		/* copy the end-to-end id from request to response */
		msg->endtoendID = request->endtoendID;
		/* copy the hop-by-hop id from request to response */
		msg->hopbyhopID = request->hopbyhopID;
		/* look into request for a session ID avp */
		if (request->avpList && request->avpList->head)
			avp = AAAFindMatchingAVP(request->avpList,request->avpList->head,
			263,0,AAA_FORWARD_SEARCH);
		if (avp) {
			/* clone the AVP from request... */
			if ( (avp=clone_avp(avp,0))==0) {
				LOG(L_ERR,"ERROR:AAANewMessage: cannot clone "
					"session-Id avp!!\n");
				goto error;
			}
			/* ...and insert it into response */
			AAAAddAVPToList( &(msg->avpList), avp, 0);
		} else {
			/* create a new session-Id avp */
			if (sessionId && (AAACreateAVP( &avp, 263, 0, 0, sessionId->s,
			sessionId->len)!=AAA_ERR_SUCCESS || AAAAddAVPToList
			( &(msg->avpList), avp, 0)!=AAA_ERR_SUCCESS) )
				goto error;
		}
		/* add origin host AVP */
		if (AAACreateAVP( &avp, 264, 0, 0, aaa_identity.s, aaa_identity.len)
		!=AAA_ERR_SUCCESS || AAAAddAVPToList( &(msg->avpList), avp,
		(msg->avpList)?(msg->avpList->tail):0)!=AAA_ERR_SUCCESS )
			goto error;
		/* mirror all the proxy-info avp in the same order */
		if (request->avpList && request->avpList->head) {
			avp_t = request->avpList->head;
			while ( (avp_t=AAAFindMatchingAVP
			(request->avpList,avp_t,284,0,AAA_FORWARD_SEARCH))!=0 ) {
				if ( (avp=clone_avp(avp_t,0))==0 || AAAAddAVPToList
				( &(msg->avpList), avp,msg->avpList?msg->avpList->tail:0)!=
				AAA_ERR_SUCCESS )
					goto error;
			}
		}
	}

	return msg;
error:
	LOG(L_ERR,"ERROR:AAANewMessage: failed to create a new AAA message!\n");
	AAAFreeMessage(&msg);
	return 0;
}



/* frees a message allocated through AAANewMessage()
 */
AAAReturnCode  AAAFreeMessage(AAAMessage **msg)
{
	AAA_AVP *avp_t;
	AAA_AVP *avp;

	/* param check */
	if (!msg || !(*msg))
		goto done;

	/* free the avp list */
	if ( (*msg)->avpList ) {
		avp = (*msg)->avpList->head;
		while (avp) {
			avp_t = avp;
			avp = avp->next;
			/*free the avp*/
			AAAFreeAVP(&avp_t);
		}
	}

	/* what about this proxyAVP ????????? */

	/* free the buffer if any */
	if ( (*msg)->orig_buf.s )
		free( (*msg)->orig_buf.s );

	/* free the AAA msg */
	free(*msg);
	msg = 0;

done:
	return AAA_ERR_SUCCESS;
}



/** This function decapsulates an encapsulated AVP, and populates the
   list with the correct pointers. */
/*AAAResultCode  AAASetMessageResultCode(
	AAAMessage *message,
	AAAResultCode resultCode)
{
}*/



/** This function sets the server to which the message is sent */
/*AAAReturnCode   AAASetServer(
	AAAMessage *message,
	IP_ADDR host)
{
}*/



/* The following function sends a message to the server assigned to the
 * message by AAASetServer() */
AAAReturnCode  AAASendMessage(AAAMessage *msg)
{
	struct session *ses;
	struct trans   *tr;
	struct peer    *dst_peer;
	AAA_AVP        *avp;
	unsigned int   event;

	/* some checks */
	if (!msg)
		goto error;

	ses = 0;
	avp = 0;
	tr  = 0;
	dst_peer = 0;

	/* if it's a response, I have to find its transaction */
	if ( !is_req(msg) ) {
		/* it should have end-to-end-ID and hop-by-hop-ID set */
		tr = transaction_lookup( hash_table, msg,0);
		if (!tr) {
			LOG(L_ERR,"ERROR:AAASendMessage: cannot send a response that"
				" doesn't belong to any transaction!\n");
			goto error;
		}
		/* also get the incoming peer for its request */
		dst_peer = tr->in_peer;
	}

	/* for the req/res that require session -> get their session for update */
	if (msg->commandCode!=271 && msg->commandCode!=257 &&
	msg->commandCode!=280 && msg->commandCode!=282) {
		/* get the session-Id AVP */
		if (!msg->avpList || !msg->avpList->head || (avp = AAAFindMatchingAVP
		(msg->avpList,msg->avpList->head,263,0,AAA_FORWARD_SEARCH))==0) {
			LOG(L_ERR,"ERROR:AAASendMessage: cannot find Session-ID AVP!\n");
			goto error;
		}

		/* search the session the msg belong to by the session-ID string */
		ses = session_lookup( hash_table, &(avp->data));
		if (!ses) {
			LOG(L_ERR,"ERROR:AAASendMessage: you attempt to send a mesage "
				"that doesn't belong to any session!!\n");
			goto error;
		}

		/* update the session state */
		switch (msg->commandCode) {
			case 274: /*ASR or ASA*/
				event = is_req(msg)?AAA_ASR_SENT:AAA_ASA_SENT;
				break;
			case 275: /*STR or STA*/
				event = is_req(msg)?AAA_STR_SENT:AAA_STA_SENT;
				break;
			case 258: /*RAR or RAA*/
				event = is_req(msg)?AAA_RAR_SENT:AAA_RAA_SENT;
				break;
			default:
				event = is_req(msg)?AAA_AR_SENT:AAA_AA_SENT;
		}
		if (session_state_machine( ses, event)!=1)
			goto error;
	}

	/* send the message */
	if (send_aaa_message( msg, tr, ses , dst_peer)!=1)
		goto error;

	return AAA_ERR_SUCCESS;
error:
	return AAA_ERR_FAILURE;
}



/** The following function sends an accounting message to an accounting
   server */
/*AAAReturnCode  AAASendAcctRequest(
	AAASessionId *aaaSessionId,
	AAAExtensionId extensionId,
	AAA_AVP_LIST *acctAvpList,
	AAAAcctMessageType msgType)
{
}*/



/** The following function will return the security characteristics of
    the current connection. */
/*AAASecurityStatus  AAAGetPeerSecurityStatus(IP_ADDR remoteHost)
{
}*/



/* This function convert message to message structure */
AAAMessage* AAATranslateMessage( unsigned char* source, size_t sourceLen )
{
	unsigned char *ptr;
	AAAMessage    *msg;
	unsigned char version;
	unsigned int  msg_len;
	AAA_AVP       *avp;
	unsigned int  avp_code;
	unsigned char avp_flags;
	unsigned int  avp_len;
	unsigned int  avp_vendorID;
	unsigned int  avp_data_len;

	//int AVPHeaderLength = AVP_CODE_SIZE+AVP_FLAGS_SIZE+AVP_LENGTH_SIZE;

	/* check the params */
	if( !source || !sourceLen || sourceLen<AAA_MSG_HDR_SIZE) {
		LOG(L_ERR,"ERROR:AAATranslateMessage: invalid buffered received!\n");
		goto error;
	}

	/* inits */
	msg = 0;
	avp = 0;
	ptr = source;

	/* alloc a new message structure */
	msg = (AAAMessage*)malloc(sizeof(AAAMessage));
	if (!msg) {
		LOG(L_ERR,"ERROR:AAATranslateMessage: no more free memory!!\n");
		goto error;
	}
	memset(msg,0,sizeof(AAAMessage));

	/* get the version */
	version = (unsigned char)*ptr;
	ptr += VER_SIZE;
	if (version!=1) {
		LOG(L_ERR,"ERROR:AAATranslateMessage: invalid version [%d]in "
			"AAA msg\n",version);
		goto error;
	}

	/* message length */
	msg_len = get_3bytes( ptr );
	ptr += MESSAGE_LENGTH_SIZE;
	if (msg_len>sourceLen) {
		LOG(L_ERR,"ERROR:AAATranslateMessage: AAA message len [%d] bigger then"
			" buffer len [%d]\n",msg_len,sourceLen);
		goto error;
	}

	/* command flags */
	msg->flags = (unsigned char)*ptr;
	ptr += FLAGS_SIZE;

	/* command code */
	msg->commandCode = get_3bytes( ptr );
	ptr += COMMAND_CODE_SIZE;

	/* application-Id/vendor-Id */
	msg->vendorId = get_4bytes( ptr );
	ptr += VENDOR_ID_SIZE;

	/* Hop-by-Hop-Id */
	msg->hopbyhopID = get_4bytes( ptr );
	ptr += HOP_BY_HOP_IDENTIFIER_SIZE;

	/* End-to-End-Id */
	msg->endtoendID = get_4bytes( ptr );
	ptr += END_TO_END_IDENTIFIER_SIZE;

	/* start decoding the AVPS */
	while (ptr < source+msg_len) {
		if (ptr+AVP_HDR_SIZE>source+msg_len){
			LOG(L_ERR,"ERROR:AAATranslateMessage: source buffer to short!! "
				"Cannot read the whole AVP header!\n");
			goto error;
		}
		/* avp code */
		avp_code = get_4bytes( ptr );
		ptr += AVP_CODE_SIZE;
		/* avp flags */
		avp_flags = (unsigned char)*ptr;
		ptr += AVP_FLAGS_SIZE;
		/* avp length */
		avp_len = get_3bytes( ptr );
		ptr += AVP_LENGTH_SIZE;
		if (avp_len<1) {
			LOG(L_ERR,"ERROR:AAATranslateMessage: invalid AVP len [%d]\n",
				avp_len);
			goto error;
		}
		/* avp vendor-ID */
		avp_vendorID = 0;
		if (avp_flags&AAA_AVP_FLAG_VENDOR_SPECIFIC) {
			avp_vendorID = get_4bytes( ptr );
			ptr += AVP_VENDOR_ID_SIZE;
		}
		/* data length */
		avp_data_len = avp_len-AVP_HDR_SIZE-
			AVP_VENDOR_ID_SIZE*((avp_flags&AAA_AVP_FLAG_VENDOR_SPECIFIC)!=0);
		/*check the data length */
		if ( source+msg_len<ptr+avp_data_len) {
			LOG(L_ERR,"ERROR:AAATranslateMessage: source buffer to short!! "
				"Cannot read a whole data for AVP!\n");
			goto error;
		}

		/* create the AVP */
		if (create_avp( &avp, avp_code, avp_flags, avp_vendorID,
		ptr, avp_data_len, 0/*don't free data*/)!=AAA_ERR_SUCCESS) {
			goto error;
		}

		ptr += avp_data_len + ((avp_data_len&3)?4-(avp_data_len&3):0);
		/* link the avp into aaa message to the end */
		AAAAddAVPToList( &(msg->avpList), avp,
			(msg->avpList)?(msg->avpList->tail):0);
	}

	/* link the buffer to the message */
	msg->orig_buf.s = source;
	msg->orig_buf.len = msg_len;

	print_aaa_message( msg );
	return  msg;
error:
	LOG(L_ERR,"ERROR:AAATranslateMessage: message conversion droped!!\n");
	AAAFreeMessage(&msg);
	return 0;
}



/* print as debug all info contained by an aaa message + AVPs
 */
void print_aaa_message( AAAMessage *msg)
{
	char    buf[1024];
	AAA_AVP *avp;

	/* print msg info */
	DBG("DEBUG: AAA_MESSAGE - %p\n",msg);
	DBG("\tCode = %u\n",msg->commandCode);
	DBG("\tFlags = %x\n",msg->flags);

	/*print the AVPs */
	if (msg->avpList) {
		avp = msg->avpList->head;
		while (avp) {
			AAAConvertAVPToString(avp,buf,1024);
			DBG("\n%s\n",buf);
			avp=avp->next;
		}
	}
}
