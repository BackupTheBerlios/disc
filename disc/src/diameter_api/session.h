/*
 * $Id: session.h,v 1.8 2003/03/28 10:25:03 bogdan Exp $
 *
 * 2003-01-28 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_SESSION_H
#define _AAA_DIAMETER_SESSION_H


#include "diameter_types.h"
#include "../aaa_lock.h"
#include "../hash_table.h"
#include "../locking.h"


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
 * all session manager's info packed
 */
struct session_manager {
	/* vector used to store the monotonically increasing 64-bit value used
	 * in session ID generation */
	unsigned int monoton_sID[2];
	/* mutex */
	gen_lock_t *sID_mutex;
	/* hash_table for the sessions */
	struct h_table *ses_table;
};


/*
 * encapsulates a everything about a AAA session
 */
struct session {
	/* linker into hash table; MUST be the first */
	struct h_link  linker;
	/* AAA info */
	unsigned short peer_identity;        /* is it a client, server ....? */
	str sID;                             /* session-ID as string */
	/* context  */
	void  *context;
	AAAApplicationRef app_ref;
	/* infos */
	unsigned int session_timeout;
	unsigned int request_timeout;
	unsigned int state;
};


/* session-IDs manager */
extern struct session_manager  ses_mgr;



/* builds and init all variables/structures needed for session management
 */
int init_session_manager( unsigned int ses_hash_size );


/*
 */
void shutdown_session_manager();


/* parse a session-Id and retrive the hash-code and label from it
 */
int parse_sessionID( str* ,unsigned int* ,unsigned int* );


/*
 */
int session_state_machine( struct session* , enum AAA_EVENTS event,
															AAAMessage *msg);


/* search into hash table a session, based on session-Id
 */
inline static struct session* session_lookup(struct h_table *table,
															AAASessionId *sID)
{
	unsigned int   hash_code;
	unsigned int   label;

	if (parse_sessionID(  sID, &hash_code, &label)!=1)
		return 0;
	return (struct session*)cell_lookup( table, hash_code, label);
}


#endif
