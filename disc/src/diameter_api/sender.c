/*
 * $Id: sender.c,v 1.5 2003/04/10 23:54:02 bogdan Exp $
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
#include "session.h"


int get_dest_peers( AAAMessage *msg, struct peer_chain **pc );


/****************************** API FUNCTIONS ********************************/


/* The following function sends a message to the server assigned to the
 * message by AAASetServer() */
AAAReturnCode  AAASendMessage(AAAMessage *msg)
{
	struct peer_chain *pc;
	struct session    *ses;
	struct trans      *tr;
	unsigned int      event;
	int               ret;

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
		/* it's a response */

		/* generate the buf from the message */
		if ( AAABuildMsgBuffer( msg )==-1 )
			goto error;

		if (!msg->no_ses) {
			ses = sId2session( msg->sId );
			/* update the session state machine */
			switch (msg->commandCode) {
				case 274: /*ASA*/ event = AAA_SENDING_ASA; break;
				case 275: /*STA*/ event = AAA_SENDING_STA; break;
				case 258: /*RAA*/ event = AAA_SENDING_RAA; break;
				default:  /*AA */ event = AAA_SENDING_AA;  break;
			}
			if (session_state_machine( ses, event, 0)!=1)
				goto error;
		}

		/* send the reply to the request incoming peer  */
		if (send_res_to_peer( &(msg->buf), (struct peer*)msg->in_peer)==-1) {
			LOG(L_ERR,"ERROR:send_aaa_response: send returned error!\n");
			if (!msg->no_ses)
				session_state_machine( ses, AAA_SEND_FAILED, 0);
			goto error;
		}
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

		/* update the session state */
		switch (msg->commandCode) {
			case 274: /*ASR*/ event = AAA_SENDING_ASR; break;
			case 275: /*STR*/ event = AAA_SENDING_STR; break;
			case 258: /*RAR*/ event = AAA_SENDING_RAR; break;
			default:  /*AR */ event = AAA_SENDING_AR;  break;
		}
		if (session_state_machine( ses, event, 0)!=1)
			goto error;

		/* generate the buf from the message */
		if ( AAABuildMsgBuffer( msg )==-1 )
			goto error;

		/* build a new outgoing transaction for this request */
		if (( tr=create_transaction(&(msg->buf), 0) )==0 ) {
			LOG(L_ERR,"ERROR:AAASendMesage: cannot create a new"
				" transaction!\n");
			goto error;
		}
		tr->ses = ses;

		/* send the request */
		for( ; pc ; pc=pc->next ) {
			if ( (ret=send_req_to_peer( tr, pc->p))!=-1) {
				/* start the timeout timer */
				add_to_timer_list( &(tr->timeout) , tr_timeout_timer ,
					get_ticks()+TR_TIMEOUT_TIMEOUT );
				break;
			}
		}
		tr->req = 0;
		if (ret==-1) {
			LOG(L_ERR,"ERROR:AAASendMessage: I wasn't able to send request\n");
			session_state_machine( ses, AAA_SEND_FAILED, 0);
			goto error;
		}
	}


	/* free the buffer */
	shm_free(msg->buf.s);
	msg->buf.s = 0;
	msg->buf.len = 0;
	return AAA_ERR_SUCCESS;
error:
	if (tr)
		destroy_transaction( tr );
	if (msg->buf.s) {
		shm_free(msg->buf.s);
		msg->buf.s = 0;
		msg->buf.len = 0;
	}
	return AAA_ERR_FAILURE;
}

