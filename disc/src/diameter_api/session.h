/*
 * $Id: session.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2003-01-28 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_SESSION_H
#define _AAA_DIAMETER_SESSION_H


#include "diameter_types.h"
#include "utils/aaa_lock.h"
#include "hash_table.h"


/* all possible states of the session state machine */
enum {
	AAA_IDLE_STATE,
	AAA_PENDING_STATE,
	AAA_OPEN_STATE,
	AAA_DISCON_STATE
};

/* possible identities for AAA parties */
enum AAA_ENTITY {
	AAA_CLIENT,
	AAA_SERVER,
	AAA_SERVER_STATEFULL,
	AAA_SERVER_STATELESS
};

/* all possible events for the session state machines */
enum AAA_EVENTS {
	AAA_AA_RECEIVED,
	AAA_AR_RECEIVED,
	AAA_AR_SENT,
	AAA_AA_SENT,
	AAA_SESSION_TIMEOUT,
	AAA_ASR_RECEIVED,
	AAA_ASA_RECEIVED,
	AAA_ASR_SENT,
	AAA_ASA_SENT,
	AAA_RAR_RECEIVED,
	AAA_RAA_RECEIVED,
	AAA_RAR_SENT,
	AAA_RAA_SENT,
	AAA_STR_SENT,
	AAA_STA_SENT,
	AAA_STR_RECEIVED,
	AAA_STA_RECEIVED
};



/*
 * structure that contains all information needed for generating session-IDs
 */
struct session_ID_gen {
	/* vector used to store the monotonically increasing 64-bit value used
	 * in session ID generation */
	unsigned int monoton_sID[2];
	/* mutex */
	aaa_lock *mutex;
};


/*
 * encapsulates a everything about a AAA session
 */
struct session {
	/* linker into hash table; MUST be the first */
	struct h_link  linker;
	/* AAA info */
	unsigned short peer_identity;        /* is it a client, server ....? */
	AAASessionId sID;                    /* session-ID as string */
	unsigned int session_timeout;
	unsigned int request_timeout;
	unsigned int state;
};





/* builds and init all variables/structures needed for session management
 */
int init_session_manager();


/*
 */
void shutdown_session_manager();


/* parse a session-Id and retrive the hash-code and label from it
 */
int parse_sessionID(AAASessionId* ,unsigned int* ,unsigned int* );


/*
 */
int session_state_machine( struct session* , enum AAA_EVENTS event);


/* search into hash table a session, based on session-Id
 */
inline static struct session* session_lookup(struct h_table *table, 
															AAASessionId *sID)
{
	unsigned int   hash_code;
	unsigned int   label;

	if (parse_sessionID(  sID, &hash_code, &label)!=1)
		return 0;
	return (struct session*)cell_lookup
		( table, hash_code, label, SESSION_CELL_TYPE, 0);
}


#endif
