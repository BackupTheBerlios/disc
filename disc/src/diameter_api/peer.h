/*
 * $Id: peer.h,v 1.2 2003/03/07 17:20:08 bogdan Exp $
 *
 * 2003-02-18 created by bogdan
 *
 */

#ifndef _AAA_DIAMETER_PEER_H
#define _AAA_DIAMETER_PEER_H

#include "utils/str.h"
#include "utils/aaa_lock.h"
#include "utils/counter.h"
#include "utils/ip_addr.h"
#include "tcp_shell/common.h"
#include "global.h"
#include "diameter_types.h"
#include "trans.h"
#include "timer.h"


#define PEER_TO_DESTROY  1<<0


/* peer table */
extern struct p_table *peer_table;


struct peer {
	/* link to the tcp connecter */
	//struct _connection* conn;
	/* aaa specific information */
	str aaa_realm;
	str aaa_host;
	/* location of the peer as ip:port */
	unsigned int port;
	IP_ADDR ip;
	unsigned int state;
	/* linking information */
	struct peer *next;
	/* mutex for changing the status */
	aaa_lock *mutex;
	/* ref counter*/
	atomic_cnt ref_cnt;
	/* timer */
	struct timer_link tl;
	/* flags */
	unsigned char flags;
	/* command pipe */
	unsigned int fd;
};


struct p_table {
	/* numbers of peer from the list */
	unsigned int nr_peers;
	/* the head of the peer list */
	struct peer *head;
	/* the tail of the peer list */
	struct peer *tail;
	/* mutex for manipulating the list */
	aaa_lock *mutex;
};


enum AAA_PEER_EVENT {
	TCP_ACCEPT,
	TCP_CLOSE,
	TCP_EXPIRED,
	TCP_CONNECTED,
	TCP_CONN_FAILED,
	TCP_UNREACHABLE,
	CER_RECEIVED,
	CEA_RECEIVED,
	DPR_RECEIVED,
	DPA_RECEIVED,
	PEER_HANGUP,
	PEER_TIMEOUT
};


enum AAA_PEER_STATE {
	PEER_UNCONN,
	PEER_WAIT_CER,
	PEER_WAIT_CEA,
	PEER_CONN,
	PEER_WAIT_DPA
};





int init_peer_manager();

void destroy_peer_manager();

int add_peer( struct p_table *peer_table, str *host, unsigned int realm_offset,
														unsigned int port);

void init_all_peers();

int peer_state_machine( struct peer *p, enum AAA_PEER_EVENT event,
															void *info);

/* increments the ref_counter of the peer */
void static inline ref_peer(struct peer *p)
{
	atomic_inc(&(p->ref_cnt));
}


/* decrements the ref counter of the peer */
void static inline unref_peer(struct peer *p)
{
	atomic_dec(&(p->ref_cnt));
}


/* search into the peer table for the peer having the given host name */
static inline struct peer* lookup_peer_by_host( str *host )
{
	struct peer *p;

	lock( peer_table->mutex );

	p = peer_table->head;
	while(p) {
		if ( host->len==p->aaa_host.len &&
		!strncasecmp( host->s, p->aaa_host.s, host->len))
			break;
		p=p->next;
	}

	if (p)
		ref_peer( p );

	unlock( peer_table->mutex );
	return p;
}


/* search into the peer table for the peer searving the given realm */
static inline struct peer* lookup_peer_by_realm( str *realm )
{
	struct peer *p;

	lock( peer_table->mutex );

	p = peer_table->head;
	while(p) {
		if ( realm->len==p->aaa_realm.len &&
		!strncasecmp( realm->s, p->aaa_realm.s, realm->len))
			break;
		p=p->next;
	}

	if (p)
		ref_peer( p );

	unlock( peer_table->mutex );
	return p;
}


/* search into the peer table for the peer having the given IP address */
static inline struct peer* lookup_peer_fd_by_ip( struct ip_addr *ip )
{
	struct peer *p;

	lock( peer_table->mutex );

	p = peer_table->head;
	while(p) {
		if ( ip_addr_cmp(ip, &p->ip) )
			break;
		p=p->next;
	}

	if (p)
		ref_peer( p );

	unlock( peer_table->mutex );
	return p;
}




#endif
