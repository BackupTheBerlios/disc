/*
 * $Id: message.c,v 1.25 2003/04/07 15:17:51 bogdan Exp $
 *
 * 2003-02-03 created by bogdan
 * 2003-03-12 converted to use shm_malloc/shm_free (andrei)
 * 2003-03-13 converted to locking.h/gen_lock_t (andrei)
 */

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "diameter_api.h"
#include "../dprint.h"
#include "../str.h"
#include "../utils.h"
#include "../locking.h"
#include "../globals.h"
#include "../transport/peer.h"
#include "message.h"
#include "avp.h"
#include "session.h"
#include "../transport/trans.h"

#include "../mem/shm_mem.h"


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
	gen_lock_t *locks;

	/* build a new msg_manager structure */
	msg_mgr = (struct msg_manager*)shm_malloc( sizeof(struct msg_manager) );
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
		shm_free( msg_mgr );
	}
	LOG(L_INFO,"INFO:destroy_msg_manager: message manager stoped\n");
}



inline static unsigned int generate_endtoendID()
{
	unsigned int id;
	lock_get( msg_mgr->end_to_end_lock );
	id = msg_mgr->end_to_end++;
	lock_release( msg_mgr->end_to_end_lock );
	return id;
}




/************************** MESSAGE FUNCTIONS ********************************/

#if 0
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
	str            str_buf;

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
		if (tr->ses) {
			/* it has a session-> it's a local response */
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
		str_buf.s   = buf;
		str_buf.len = len;
		tr = create_transaction( &str_buf, 0/*ses*/, pr);
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
#endif



/* from a AAAMessage structure, a buffer to be send is build
 */
int build_buf_from_msg( AAAMessage *msg )
{
	unsigned char *p;
	unsigned int  k;
	AAA_AVP       *avp;

	/* first let's comput the length of the buffer */
	msg->buf.len = AAA_MSG_HDR_SIZE; /* AAA message header size */
	/* count and add the avps */
	for(avp=msg->avpList.head;avp;avp=avp->next) {
		msg->buf.len+=AVP_HDR_SIZE+(AVP_VENDOR_ID_SIZE*((avp->flags&0x80)!=0))
			+ to_32x_len( avp->data.len ) /*data*/;
	}

	DBG("xxxx len=%d\n",msg->buf.len);
	/* allocate some memory */
	msg->buf.s = (unsigned char*)shm_malloc( msg->buf.len );
	if (!msg->buf.s) {
		LOG(L_ERR,"ERROR:build_buf_from_msg: no more free memory!\n");
		goto error;
	}
	memset(msg->buf.s, 0, msg->buf.len);

	/* fill in the buffer */
	p = msg->buf.s;
	/* DIAMETER HEADER */
	/* message length */
	((unsigned int*)p)[0] =htonl(msg->buf.len);
	/* Diameter Version */
	*p = 1;
	p += VER_SIZE + MESSAGE_LENGTH_SIZE;
	/* command code */
	((unsigned int*)p)[0] = htonl(msg->commandCode);
	/* flags */
	*p = (unsigned char)msg->flags;
	p += FLAGS_SIZE + COMMAND_CODE_SIZE;
	/* application-ID */
	((unsigned int*)p)[0] = htonl(msg->applicationId);
	p += APPLICATION_ID_SIZE;
	/* hop by hop id */
	p += HOP_BY_HOP_IDENTIFIER_SIZE;
	/* end to end id */
	p += END_TO_END_IDENTIFIER_SIZE;

	/* AVPS */
	for(avp=msg->avpList.head;avp;avp=avp->next) {
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
		p += to_32x_len( avp->data.len );
	}

	if ((char*)p-msg->buf.s!=msg->buf.len) {
		LOG(L_ERR,"BUG: build_buf_from_msg: mismatch between len and buf!\n");
		shm_free( msg->buf.s );
		msg->buf.s = 0;
		msg->buf.len = 0;
		goto error;
	}

	return 1;
error:
	return -1;
}




/*  sends out a aaa message
 */
int send_request( AAAMessage *msg)
{
	struct peer_chaine *pc;
	unsigned int ete;
	struct trans *tr;
	str s;

	tr = 0;

	/* generate the buf from the message */
	if ( build_buf_from_msg( msg )==-1 )
		goto error;

	/* build a new transaction for this request */
	if (( tr = create_transaction( &(msg->buf),
	sId2session(msg->sId)/*ses*/, 0/*no peeer yest*/))==0 ) {
		LOG(L_ERR,"ERROR:send_aaa_request: cannot create a new"
			" transaction!\n");
		goto error;
	}

	/* generate a new end-to-end id */
	ete = generate_endtoendID();
	((unsigned int *)msg->buf.s)[4] = ete;

	pc = (struct peer_chaine*)msg->peers;

	/* calculating the hash_code (over the end-to-end Id) */
	s.s = (char*)&ete;
	s.len = END_TO_END_IDENTIFIER_SIZE;
	tr->linker.hash_code = hash( &s , pc->peer->trans_table->hash_size );

	/* send the request */
	DBG(" before send_req_to_peers!\n");
	if (send_req_to_peers(tr, (struct peer_chaine*)msg->peers)==-1) {
		LOG(L_ERR,"ERROR:send_aaa_request: send returned error!\n");
		goto error;
	}

	/* start the timeout timer */
	add_to_timer_list( &(tr->timeout) , tr_timeout_timer ,
		get_ticks()+TR_TIMEOUT_TIMEOUT );

	return 1;
error:
	if (tr) {
		destroy_transaction(tr);
	} else if (msg->buf.s) {
		shm_free( msg->buf.s );
		msg->buf.s = 0;
		msg->buf.len = 0;
	}
	return -1;
}



/* sends out a reply/response/answer
 */
int send_aaa_response( AAAMessage *msg)
{
	str *req;;

	/* generate the buf from the message */
	if ( build_buf_from_msg( msg )==-1 )
		goto error;

	/* copy the end-to-end id and hop-by-hop id */
	req = ((struct trans*)msg->trans)->req;
	((unsigned int*)msg->buf.s)[3] = ((unsigned int*)req->s)[3];
	((unsigned int*)msg->buf.s)[4] = ((unsigned int*)req->s)[4];

	/* send the message */
	if (send_res_to_peer(&(msg->buf),((struct trans*)msg->trans)->peer)==-1) {
		LOG(L_ERR,"ERROR:send_aaa_response: send returned error!\n");
	}

	/* destroy everything */
	destroy_transaction( (struct trans*)msg->trans );
	shm_free( msg->buf.s );

	return 1;
error:
	if (msg->buf.s) {
		shm_free( msg->buf.s );
		msg->buf.s = 0;
		msg->buf.len = 0;
	}
	return -1;
}



/****************************** API FUNCTIONS ********************************/


/* allocates a new AAAMessage
 */
AAAMessage *AAANewMessage(
	AAACommandCode commandCode,
	AAAApplicationId applicationId,
	AAASessionId *sessionId,
	AAAMessage *request)
{
	AAAMessage   *msg;
	AAA_AVP      *avp;
	AAA_AVP      *avp_t;
	struct trans *tr;
	unsigned int code;

	msg = 0;

	if (!sessionId) {
		LOG(L_ERR,"ERROR:AAANewMessage: param session-ID received null!!\n");
		goto error;
	}

	/* allocated a new AAAMessage structure a set it to 0 */
	msg = (AAAMessage*)shm_malloc(sizeof(AAAMessage));
	if (!msg) {
		LOG(L_ERR,"ERROR:AAANewMessage: no more free memory!!\n");
		goto error;
	}
	memset(msg,0,sizeof(AAAMessage));

	/* command code */
	msg->commandCode = commandCode;
	/* application ID */
	msg->applicationId = applicationId;

	/* add session ID */
	avp = 0;
	if ( create_avp( &avp, 263, 0, 0, sessionId->s, sessionId->len, 0)
	!=AAA_ERR_SUCCESS || AAAAddAVPToMessage( msg, avp, 0)
	!=AAA_ERR_SUCCESS ) {
		LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add Session-Id avp\n");
		if (avp) AAAFreeAVP( &avp );
		goto error;
	}
	msg->sessionId = avp;
	/* add origin host AVP */
	avp = 0;
	if (create_avp( &avp, 264, 0, 0, aaa_identity.s, aaa_identity.len, 0)
	!=AAA_ERR_SUCCESS || AAAAddAVPToMessage( msg, avp, msg->avpList.tail)
	!=AAA_ERR_SUCCESS ) {
		LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add Origin-Host avp\n");
		if (avp) AAAFreeAVP( &avp );
		goto error;
	}
	msg->orig_host = avp;
	/* add origin realm AVP */
	avp = 0;
	if (create_avp( &avp, 296, 0, 0, aaa_realm.s, aaa_realm.len, 0)
	!=AAA_ERR_SUCCESS || AAAAddAVPToMessage( msg, avp, msg->avpList.tail)
	!=AAA_ERR_SUCCESS ) {
		LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add Origin-Realm avp\n");
		if (avp) AAAFreeAVP( &avp );
		goto error;
	}
	msg->orig_realm = avp;

	if (!request) {
		/* it's a new request -> set the flag */
		msg->flags = 0x80;
		/* keep track of the session -> SendMessage will need it! */
		msg->sId = sessionId;
	} else {
		/* link the transaction the req. belong to */
		msg->trans = request->trans;
		tr = (struct trans*)request->trans;
		/* set the P flag as in request */
		msg->flags |= request->flags&0x40;

		/* add an success result-code avp ;-))) */
		avp = 0;
		code = AAA_SUCCESS;
		if (create_avp(&avp,AVP_Result_Code,0,0,(char*)&code,sizeof(code),1)
		!=AAA_ERR_SUCCESS || AAAAddAVPToMessage( msg, avp, msg->avpList.tail)
		!=AAA_ERR_SUCCESS ) {
			LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add "
				"Result-Code avp\n");
			if (avp) AAAFreeAVP( &avp );
			goto error;
		}
		msg->res_code = avp;

		/* mirror all the proxy-info avp in the same order */
		avp_t = request->avpList.head;
		while ( (avp_t=AAAFindMatchingAVP
		(request,avp_t,284,0,AAA_FORWARD_SEARCH))!=0 ) {
			if ( (avp=clone_avp(avp_t,0))==0 || AAAAddAVPToMessage( msg, avp,
			msg->avpList.tail)!=AAA_ERR_SUCCESS )
				goto error;
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
	avp = (*msg)->avpList.head;
	while (avp) {
		avp_t = avp;
		avp = avp->next;
		/*free the avp*/
		AAAFreeAVP(&avp_t);
	}

	/* free the buffer (if any) */
	if ( (*msg)->buf.s )
		shm_free( (*msg)->buf.s );

	/* free the AAA msg */
	shm_free(*msg);
	msg = 0;

done:
	return AAA_ERR_SUCCESS;
}



/* Sets tthe proper result_code into the Result-Code AVP; ths avp must already
 * exists into the reply messge */
AAAResultCode  AAASetMessageResultCode(
	AAAMessage *message,
	AAAResultCode resultCode)
{
	if ( !is_req(message) && message->res_code) {
		*((unsigned int*)(message->res_code->data.s)) = htonl(resultCode);
		return AAA_ERR_SUCCESS;
	}
	return AAA_ERR_FAILURE;
}



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
	struct peer_chaine *pc;
	struct session     *ses;
	struct trans       *tr;
	unsigned int       event;
	int                ret;

	ses = 0;
	tr  = 0;

	/* some checks */
	if (!msg)
		goto error;

	if (msg->commandCode==257||msg->commandCode==280||msg->commandCode==282) {
		LOG(L_ERR,"ERROR:AAASendMessage: you are not ALLOWED to send this"
			" type of message (%d) -> read the draft!!!\n",msg->commandCode);
		goto error;
	}

	if ( !is_req(msg) ) {
		/* if it's a response, I already have a transaction for it */
		tr = (struct trans*)msg->trans;
		/* ....and the session */
		ses = tr->ses;
		/* update the session state machine */
		switch (msg->commandCode) {
			case 274: /*ASA*/
				event = AAA_ASA_SENT;
				break;
			case 275: /*STA*/
				event = AAA_STA_SENT;
				break;
			case 258: /*RAA*/
				event = AAA_RAA_SENT;
				break;
			default:
				event = AAA_SEND_AA;
		}
		//if (session_state_machine( ses, event)!=1)
		//	goto error;
		/* generate the buf from the message */
		//if ( build_buf_from_msg( msg, &buf)==-1 )
		//	goto error;
		/* send the response */
		//if (send_aaa_response( &buf, tr)==-1)
		//	goto error;
	} else {
		/* it's a request -> get its session */
		ses = sId2session( msg->sId );
		/* where should I send this request? */
		pc = 0;
		ret = get_dest_peers( msg, &pc );
		if (ret!=1 || pc==0) {
			LOG(L_ERR,"ERROR:AAASendMessage: no outgoing peer found for msg!"
				" do_routing returned %d, pc=%p.\n",ret,pc);
			goto error;
		}
		msg->peers = pc;
		/* update the session state */
		switch (msg->commandCode) {
			case 274: /*ASR*/
				event = AAA_ASR_SENT;
				break;
			case 275: /*STR*/
				event = AAA_STR_SENT;
				break;
			case 258: /*RAR*/
				event = AAA_RAR_SENT;
				break;
			default:
				event = AAA_SEND_AR;
		}
		if (session_state_machine( ses, event, msg)!=1)
			goto error;
	}

	msg->peers = 0;
	return AAA_ERR_SUCCESS;
error:
	msg->peers = 0;
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
	msg = (AAAMessage*)shm_malloc(sizeof(AAAMessage));
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
	msg->flags = *ptr;
	ptr += FLAGS_SIZE;

	/* command code */
	msg->commandCode = get_3bytes( ptr );
	ptr += COMMAND_CODE_SIZE;

	/* application-Id */
	msg->applicationId = get_4bytes( ptr );
	ptr += APPLICATION_ID_SIZE;

	/* Hop-by-Hop-Id */
	msg->hopbyhopId = *((unsigned int*)ptr);
	ptr += HOP_BY_HOP_IDENTIFIER_SIZE;

	/* End-to-End-Id */
	msg->endtoendId = *((unsigned int*)ptr);
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

		/* link the avp into aaa message to the end */
		AAAAddAVPToMessage( msg, avp, msg->avpList.tail);

		ptr += to_32x_len( avp_data_len );
	}

	/* link the buffer to the message */
	msg->buf.s = source;
	msg->buf.len = msg_len;

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
	avp = msg->avpList.head;
	while (avp) {
		AAAConvertAVPToString(avp,buf,1024);
		DBG("\n%s\n",buf);
		avp=avp->next;
	}
}
