/*
 * $Id: message.c,v 1.4 2003/04/10 23:54:02 bogdan Exp $
 *
 * 2003-04-07 created by bogdan
 */

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "diameter_msg.h"


extern str aaa_identity;
extern str aaa_realm;


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

#define to_32x_len( _len_ ) \
	( (_len_)+(((_len_)&3)?4-((_len_)&3):0) )


/* from a AAAMessage structure, a buffer to be send is build
 */
AAAReturnCode AAABuildMsgBuffer( AAAMessage *msg )
{
	unsigned char *p;
	AAA_AVP       *avp;

	/* first let's comput the length of the buffer */
	msg->buf.len = AAA_MSG_HDR_SIZE; /* AAA message header size */
	/* count and add the avps */
	for(avp=msg->avpList.head;avp;avp=avp->next) {
		msg->buf.len += AVP_HDR_SIZE(avp->flags)+ to_32x_len( avp->data.len );
	}

	DBG("xxxx len=%d\n",msg->buf.len);
	/* allocate some memory */
	msg->buf.s = (unsigned char*)shm_malloc( msg->buf.len );
	if (!msg->buf.s) {
		LOG(L_ERR,"ERROR:AAABuildMsgBuffer: no more free memory!\n");
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
	((unsigned int*)p)[0] = msg->hopbyhopId;
	p += HOP_BY_HOP_IDENTIFIER_SIZE;
	/* end to end id */
	((unsigned int*)p)[0] = msg->endtoendId;
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
		set_3bytes(p, (AVP_HDR_SIZE(avp->flags)+avp->data.len) );
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
	avp = AAACreateAVP( 263, 0, 0, sessionId->s, sessionId->len,
		AVP_DONT_FREE_DATA);
	if ( !avp || AAAAddAVPToMessage(msg,avp,0)!=AAA_ERR_SUCCESS) {
		LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add Session-Id avp\n");
		if (avp) AAAFreeAVP( &avp );
		goto error;
	}
	msg->sessionId = avp;
	/* add origin host AVP */
	avp = AAACreateAVP( 264, 0, 0, aaa_identity.s, aaa_identity.len,
		AVP_DONT_FREE_DATA);
	if (!avp||AAAAddAVPToMessage(msg,avp,msg->avpList.tail)!=AAA_ERR_SUCCESS) {
		LOG(L_ERR,"ERROR:AAANewMessage: cannot create/add Origin-Host avp\n");
		if (avp) AAAFreeAVP( &avp );
		goto error;
	}
	msg->orig_host = avp;
	/* add origin realm AVP */
	avp = AAACreateAVP( 296, 0, 0, aaa_realm.s, aaa_realm.len,
		AVP_DONT_FREE_DATA);
	if (!avp||AAAAddAVPToMessage(msg,avp,msg->avpList.tail)!=AAA_ERR_SUCCESS) {
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
		/* it'a an answer -> it will have the same session Id */
		msg->sId = request->sId;
		msg->no_ses = request->no_ses;
		/* link the incoming peer to the answer */
		msg->in_peer = request->in_peer;
		/* set the P flag as in request */
		msg->flags |= request->flags&0x40;
		/**/
		msg->endtoendId = request->endtoendId;
		msg->hopbyhopId = request->hopbyhopId;

		/* add a success result-code avp ;-))) */
		avp = 0;
		code = AAA_SUCCESS;
		avp = AAACreateAVP( AVP_Result_Code, 0, 0, (char*)&code, sizeof(code),
			AVP_DUPLICATE_DATA);
		if (!avp || AAAAddAVPToMessage( msg, avp, msg->sessionId)
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
			if ( (avp=AAACloneAVP(avp_t,0))==0 || AAAAddAVPToMessage( msg, avp,
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



/* This function convert message to message structure */
AAAMessage* AAATranslateMessage( unsigned char* source, unsigned int sourceLen)
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
		if (ptr+AVP_HDR_SIZE(0x80)>source+msg_len){
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
		avp_data_len = avp_len-AVP_HDR_SIZE(avp_flags);
		/*check the data length */
		if ( source+msg_len<ptr+avp_data_len) {
			LOG(L_ERR,"ERROR:AAATranslateMessage: source buffer to short!! "
				"Cannot read a whole data for AVP!\n");
			goto error;
		}

		/* create the AVP */
		avp = AAACreateAVP( avp_code, avp_flags, avp_vendorID, ptr,
			avp_data_len, AVP_DONT_FREE_DATA);
		if (!avp)
			goto error;

		/* link the avp into aaa message to the end */
		AAAAddAVPToMessage( msg, avp, msg->avpList.tail);

		ptr += to_32x_len( avp_data_len );
	}

	/* link the buffer to the message */
	msg->buf.s = source;
	msg->buf.len = msg_len;

	//AAAPrintMessage( msg );
	return  msg;
error:
	LOG(L_ERR,"ERROR:AAATranslateMessage: message conversion droped!!\n");
	AAAFreeMessage(&msg);
	return 0;
}



/* print as debug all info contained by an aaa message + AVPs
 */
void AAAPrintMessage( AAAMessage *msg)
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
