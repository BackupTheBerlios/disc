/*
 * $Id: sender.c,v 1.2 2003/04/08 22:30:18 bogdan Exp $
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

#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../str.h"
#include "../utils.h"
#include "../locking.h"
#include "../globals.h"
#include "../transport/peer.h"
#include "../transport/trans.h"
#include "diameter_api.h"
#include "sender.h"
#include "session.h"



/* local var */
struct send_manager  send_mgr;


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
int init_send_manager()
{
	memset( &send_mgr, 0, sizeof(struct send_manager));

	/* create the lock */
	send_mgr.end_to_end_lock = create_locks( 1 );
	if (!send_mgr.end_to_end_lock) {
		LOG(L_ERR,"ERROR:inid_send_manager: cannot create lock!\n");
		goto error;
	}

	/* init end_to_end_ID and hop_by_hop_ID */
	send_mgr.end_to_end  = ((unsigned int)time(0))<<20;
	send_mgr.end_to_end |= ((unsigned int)rand( ))>>12;

	LOG(L_INFO,"INFO:init_send_manager: message manager started\n");
	return 1;
error:
	LOG(L_INFO,"INFO:init_send_manager: FAILED to start send manager!!\n");
	return -1;
}



/* Destroy the lock and free the memory used by generators
 */
void destroy_send_manager()
{
	/* destroy the lock */
	if (send_mgr.end_to_end_lock)
		destroy_locks( send_mgr.end_to_end_lock , 1 );

	LOG(L_INFO,"INFO:destroy_send_manager: send manager stoped\n");
}



inline static unsigned int generate_endtoendID()
{
	unsigned int id;
	lock_get( send_mgr.end_to_end_lock );
	id = send_mgr.end_to_end++;
	lock_release( send_mgr.end_to_end_lock );
	return id;
}




/**************************** SEND FUNCTIONS ********************************/

/*  sends out a aaa message
 */
int send_request( AAAMessage *msg)
{
	struct peer_chain *pc;
	unsigned int ete;
	struct trans *tr;
	str s;

	tr = 0;

	/* generate the buf from the message */
	if ( AAABuildMsgBuffer( msg )==-1 )
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

	pc = (struct peer_chain*)msg->peers;

	/* calculating the hash_code (over the end-to-end Id) */
	s.s = (char*)&ete;
	s.len = END_TO_END_IDENTIFIER_SIZE;
	tr->linker.hash_code = hash( &s , pc->p->trans_table->hash_size );

	/* send the request */
	DBG(" before send_req_to_peers!\n");
	if (send_req_to_peers(tr, (struct peer_chain*)msg->peers)==-1) {
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
	if ( AAABuildMsgBuffer( msg )==-1 )
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


/* The following function sends a message to the server assigned to the
 * message by AAASetServer() */
AAAReturnCode  AAASendMessage(AAAMessage *msg)
{
	struct peer_chain *pc;
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

