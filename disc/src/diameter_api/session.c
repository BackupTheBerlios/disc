/*
 * $Id: session.c,v 1.9 2003/03/26 17:58:38 bogdan Exp $
 *
 * 2003-01-28  created by bogdan
 * 2003-03-12  converted to shm_malloc/shm_free (andrei)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../dprint.h"
#include "../str.h"
#include "../globals.h"
#include "../utils.h"
#include "../aaa_lock.h"
#include "../locking.h"
#include "../hash_table.h"
#include "session.h"

#include "../mem/shm_mem.h"


/* local vars */
struct session_manager  ses_mgr;   /* session-IDs manager */



/* extra functions */
struct session* create_session( unsigned short peer_id);
void destroy_session( struct session *ses);




/*
 * builds and inits all variables/structures needed for session management
 */
int init_session_manager( unsigned int ses_hash_size )
{
	/* init the session manager */
	memset( &ses_mgr, 0, sizeof(ses_mgr));

	/* init the monoton_sID vector as follows:  the high 32 bits of the 64-bit
	 * value are initialized to the time, and the low 32 bits are initialized
	 * to zero */
	ses_mgr.monoton_sID[0] = 0;
	ses_mgr.monoton_sID[1] = (unsigned int)time(0) ;
	/* create the mutex */
	ses_mgr.sID_mutex = create_locks( 1 );
	if (!ses_mgr.sID_mutex) {
		LOG(L_ERR,"ERROR:init_session_manager: cannot create lock!\n");
		goto error;
	}

	/* build the hash_table */
	ses_mgr.ses_table = build_htable( ses_hash_size );

	LOG(L_INFO,"INFO:init_session_manager: session manager started\n");
	return 1;
error:
	LOG(L_INFO,"INFO:init_session_manager: FAILED to start session manager\n");
	return -1;
}


/*
 * destroy all structures used in session management
 */
void shutdown_session_manager()
{
	/* destroy the mutex */
	if (ses_mgr.sID_mutex)
		destroy_locks( ses_mgr.sID_mutex, 1);

	/* free the hash table */
	if (ses_mgr.ses_table)
		destroy_htable( ses_mgr.ses_table );

	LOG(L_INFO,"INFO:shutdown_session_manager: session manager stoped\n");
	return;
}




/************************ SESSION_ID FUNCTIONS *******************************/

/*
 * increments the 64-biti value keept in monoto_sID vector
 */
inline void inc_64biti(int *vec_64b)
{
	vec_64b[0]++;
	vec_64b[1] += (vec_64b[0]==0);
}


/*
 * Generates a new session_ID (conforming with draft-ietf-aaa-diameter-17)
 * Returns an 1 if success or -1 if error.
 * The function is thread safe
 */
int generate_sessionID( str *sID, unsigned int end_pad_len)
{
	char *p;

	/* some checks */
	if (!sID)
		goto error;

	/* compute id's len */
	sID->len = aaa_identity.len +
		1/*;*/ + 10/*high 32 bits*/ +
		1/*;*/ + 10/*low 32 bits*/ +
		1/*;*/ + 8/*optional value*/ +
		end_pad_len;

	/* get some memory for it */
	sID->s = (char*)shm_malloc( sID->len );
	if (sID->s==0) {
		LOG(L_ERR,"ERROR:generate_sessionID: no more free memory!\n");
		goto error;
	}

	/* build the sessionID */
	p = sID->s;
	/* aaa_identity */
	memcpy( p, aaa_identity.s , aaa_identity.len);
	p += aaa_identity.len;
	*(p++) = ';';
	/* lock the mutex for accessing "sID_gen" var */
	lock_get( ses_mgr.sID_mutex );
	/* high 32 bits */
	p += int2str( ses_mgr.monoton_sID[1] , p, 10);
	*(p++) = ';';
	/* low 32 bits */
	p += int2str( ses_mgr.monoton_sID[0] , p, 10);
	/* unlock the mutex after the 64 biti value is inc */
	inc_64biti( ses_mgr.monoton_sID );
	lock_release( ses_mgr.sID_mutex );
	/* optional value*/
	*(p++) = ';';
	p += int2hexstr( rand() , p, 8);
	/* set the correct length */
	sID->len = p - sID->s;

	return 1;
error:
	return -1;
}



/* extract hash_code and label from sID
 */
int parse_sessionID( str *sID, unsigned int *hash_code, unsigned int *label)
{
	unsigned int  rang;
	unsigned int  u;
	char c;
	char *p;
	char *s;

	/* start from the end */
	s = sID->s;
	p = sID->s + sID->len - 1;
	/* decode the label */
	u = 0;
	rang = 0;
	while(p>=s && *p!='.') {
		if ((c=hexchar2int(*p))==-1)
			goto error;
		u += ((unsigned int)c)<<rang;
		rang += 4;
		p--;
	}
	if (p<=s || *(p--)!='.')
		goto error;
	if (label)
		*label = u;
	/* decode the hash_code */
	u = 0;
	rang = 0;
	while(p>=s && *p!='.') {
		if ((c=hexchar2int(*p))==-1)
			goto error;
		u += ((unsigned int)c)<<rang;
		rang += 4;
		p--;
	}
	if (p<=s || *(p--)!='.')
		goto error;
	if (hash_code)
		*hash_code = u;

#if 0
	/* firts we have to skip 3 times char ';' */
	for(i=0;i<3&&(p<end);p++)
		if (*p==';') i++;
	if ( i!=3 || (++p>=end) )
		goto error;
	/* after first dot starts the hash_code */
	for(;(p<end)&&(*p!='.');p++);
	if ( (p>=end) || (++p>=end))
		goto error;
	/* get the hash_code; it ends with a dot */
	for(i=0;(p<end)&&(*p!='.');p++,i++);
	if (!i || (p>=end))
		goto error;
	if (hash_code && (*hash_code = hexstr2int( p-i, i))==-1)
		goto error;
	/* get the label; it ends at of EOS */
	for(p++,i=0;(p<end);p++,i++);
	if (!i)
		goto error;
	if (label && (*label = hexstr2int( p-i, i))==-1)
		goto error;
	DBG("DEBUG: session_lookup: hash_code=%u ; label=%u\n",*hash_code,*label);
#endif

	return 1;
error:
	DBG("DEBUG:parse_sessionID: sessionID [%.*s] is not generate by us! "
		"Parse error at char [%c][%d] offset %d!\n",
		(int)sID->len, sID->s,*p,*p,p-s);
	return -1;
}




/************************** SESSION FUNCTION ********************************/


struct session* create_session( unsigned short peer_id)
{
	struct session *ses;

	/* allocates a new struct session and zero it! */
	ses = (struct session*)shm_malloc(sizeof(struct session));
	if (!ses) {
		LOG(L_ERR,"ERROR:create_session: not more free memeory!\n");
		goto error;
	}
	memset( ses, 0, sizeof(struct session));

	/* init the session */
	ses->peer_identity = peer_id;
	ses->state = AAA_IDLE_STATE;

	return ses;
error:
	return 0;
}



void destroy_session( struct session *ses)
{
	if (!ses)
		return;
	if ( ses->sID.s)
		shm_free( ses->sID.s );
	shm_free(ses);
}



int session_state_machine( struct session *ses, enum AAA_EVENTS event)
{
	static char *err_msg[]= {
		"no error"
		"event - state mismatch",
		"unknown type event",
		"unknown peer_identity type"
	};
	int error_code=0;

	DBG("DEBUG:session_state_machine: session %p, state = %d, event=%d\n",
		ses,ses->state,event);

	switch(ses->peer_identity) {
		case AAA_CLIENT:
			/* I am server, I am all the time statefull ;-) */
			break;
		case AAA_SERVER:
		case AAA_SERVER_STATELESS:
			/* I am client to a stateless server */
			switch( event ) {
				case AAA_AR_SENT:
					/* an auth request was sent */
					switch(ses->state) {
						case AAA_IDLE_STATE:
							/* store all the needed information from AR */
							//TO DO
							ses->state = AAA_PENDING_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_AA_RECEIVED:
					/* an auth answer was received */
					switch(ses->state) {
						case AAA_PENDING_STATE:
							/* check the code and grant access
							 * if error or service not provided -> STR
							 * else (if auth failed) -> clenup */
							//TO DO
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_SESSION_TIMEOUT:
					/* session timeout was generated */
					switch(ses->state) {
						case AAA_OPEN_STATE:
							/* Disconnect user/device */
							/* Destroy session ???? */
							// TO DO
							ses->state = AAA_IDLE_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				default:
					error_code = 2;
					goto error;
			}
			break;
		case AAA_SERVER_STATEFULL:
			/* I am client to a statefull server */
			switch( event ) {
				case AAA_AR_SENT:
					/* an re-auth request was sent */
					switch(ses->state) {
						case AAA_OPEN_STATE:
							/* update all the needed information from AR */
							//TO DO
							ses->state = AAA_OPEN_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_AA_RECEIVED:
					/* a re-auth answer was received */
					switch(ses->state) {
						case AAA_OPEN_STATE:
							/* provide service or , if failed, disconnnect */
							//TO DO
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_SESSION_TIMEOUT:
					/* session timeout was generated */
					switch(ses->state) {
						case AAA_OPEN_STATE:
							/* send STR */
							//TO DO
							ses->state = AAA_DISCON_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_ASR_RECEIVED:
					/* abort session request received */
					switch(ses->state) {
						case AAA_OPEN_STATE:
							/* send ASA and STR */
							// if user wants ????
							// TO DO 
							ses->state = AAA_DISCON_STATE;
							break;
						case AAA_DISCON_STATE:
							/* send ASA */
							// TO DO 
							ses->state = AAA_DISCON_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				case AAA_STA_RECEIVED:
					/* session terminate answer received  */
					switch(ses->state) {
						case AAA_DISCON_STATE:
							/* disconnect user/device */
							// destroy session ??
							// TO DO
							ses->state = AAA_IDLE_STATE;
							break;
						default:
							error_code = 1;
							goto error;
					}
					break;
				default:
					error_code = 2;
					goto error;
			}
			break;
		default:
			error_code = 3;
			goto error;
	}
	DBG("DEBUG:session_state_machine: session %p, new state = %d\n",
		ses,ses->state);

	return 1;
error:
	LOG(L_ERR,"ERROR:session_state_machine: %s : session=%p, peer_identity=%d,"
		"state=%d, event=%d\n",err_msg[error_code],
		ses,ses->peer_identity,ses->state,event);
	return -1;
}




/****************************** API FUNCTIONS ********************************/


AAAReturnCode  AAAStartSession( AAASessionId **sessionId,
								AAAApplicationRef appReference, void *context)
{
	struct session  *session;
	char            *p;

	if (!sessionId || !appReference) {
		LOG(L_ERR,"ERROR:AAAStartSession: invalid params received!\n");
		goto error;
	}

	/* build a new session structure */
	session = create_session( AAA_SERVER );
	if (!session)
		goto error;

	/* generates a new session-ID - the extra pad is used to append to 
	 * session-ID the hash-code and label of the session ".XXXXXXXX.XXXXXXXX"*/
	if (generate_sessionID( &(session->sID), 2*9 )!=1)
		goto error;

	/* compute the hash code; !!!IMPORTANT!!! -> this must happen before 
	 * inserting the session into the hash table, otherwise, the entry to
	 * insert on , will not be known */
	session->linker.hash_code = hash( &(session->sID),
		ses_mgr.ses_table->hash_size );

	/* insert the session into the hash table */
	add_cell_to_htable( ses_mgr.ses_table, (struct h_link*)session);

	/* now we have both the hash_code and the label of the session -> append
	 * them to the and of session ID */
	p = session->sID.s + session->sID.len;
	*(p++) = '.';
	p += int2hexstr( session->linker.hash_code, p, 8);
	*(p++) = '.';
	p += int2hexstr( session->linker.label, p, 8);
	session->sID.len = p - session->sID.s;

	/* link info into the session */
	session->context = context;
	session->app_ref = appReference;

	/* return the session-ID */
	*sessionId = &(session->sID);

	return AAA_ERR_SUCCESS;
error:
	return AAA_ERR_NOMEM;
}




