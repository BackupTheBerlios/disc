/*
 * $Id: peer.h,v 1.7 2003/03/10 19:17:32 bogdan Exp $
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
#include "utils/list.h"
#include "global.h"
#include "diameter_types.h"
#include "trans.h"
#include "timer.h"

struct peer;
#include "tcp_shell/common.h"


#define PEER_TO_DESTROY    1<<0
#define PEER_CONN_IN_PROG  1<<1

/* peer table */
extern struct p_table *peer_table;


struct tcp_info {
	struct ip_addr *local;
	unsigned int   sock;
};


struct peer {
	/* aaa specific information */
	str aaa_realm;
	str aaa_host;
	/* location of the peer as ip:port */
	unsigned int port;
	IP_ADDR ip;
	/* local ip*/
	IP_ADDR local_ip;
	unsigned int state;
	/* linking information */
	struct list_head  all_peer_lh;
	struct list_head  thd_peer_lh;
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
	/* thread the peer belong to */
	struct thread_info *tinfo;
	/* socket */
	int sock;
	/* information needed for reading messages */
	unsigned int  first_4bytes;
	unsigned int  buf_len;
	unsigned char *buf;
};


struct p_table {
	/* numbers of peer from the list */
	unsigned int nr_peers;
	/* the peer list */
	struct list_head peers ;
	/* mutex for manipulating the list */
	aaa_lock *mutex;
	/* buffers used for a fater CE, WD, DP creation */
	str std_req;
	str std_ans;
	str ce_avp_ipv4;
	str ce_avp_ipv6;
	str dpr_avp;
};


enum AAA_PEER_EVENT {
	TCP_ACCEPT,         /*  0 */
	TCP_CONNECTED,      /*  1 */
	TCP_CONN_IN_PROG,   /*  2 */
	TCP_CONN_FAILED,    /*  3 */
	TCP_CLOSE,          /*  4 */
	CER_RECEIVED,       /*  5 */
	CEA_RECEIVED,       /*  6 */
	DPR_RECEIVED,       /*  7 */
	DPA_RECEIVED,       /*  8 */
	PEER_HANGUP,        /*  9 */
	PEER_TR_TIMEOUT,    /* 10 */
	PEER_CER_TIMEOUT,   /* 11 */
	PEER_RECONN_TIMEOUT /* 12 */
};


enum AAA_PEER_STATE {
	PEER_UNCONN,
	PEER_CONN,
	PEER_WAIT_CER,
	PEER_WAIT_CEA,
	PEER_WAIT_DPA
};





int init_peer_manager();

void destroy_peer_manager();

int add_peer( struct p_table *peer_table, str *host, unsigned int realm_offset,
														unsigned int port);

void init_all_peers();

int peer_state_machine( struct peer *p, enum AAA_PEER_EVENT event, void *info);

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
	struct list_head *lh;
	struct peer *p;

	lock( peer_table->mutex );

	list_for_each( lh, &(peer_table->peers)) {
		p = list_entry( lh, struct peer, all_peer_lh);
		if ( host->len==p->aaa_host.len &&
		!strncasecmp( host->s, p->aaa_host.s, host->len)) {
			ref_peer( p );
			break;
		}
	}

	unlock( peer_table->mutex );
	return p;
}


/* search into the peer table for the peer searving the given realm */
static inline struct peer* lookup_peer_by_realm( str *realm )
{
	struct list_head *lh;
	struct peer *p;

	lock( peer_table->mutex );

	list_for_each( lh, &(peer_table->peers)) {
		p = list_entry( lh, struct peer, all_peer_lh);
		if ( realm->len==p->aaa_realm.len &&
		!strncasecmp( realm->s, p->aaa_realm.s, realm->len)) {
			ref_peer( p );
			break;
		}
	}

	unlock( peer_table->mutex );
	return p;
}


/* search into the peer table for the peer having the given IP address */
static inline struct peer* lookup_peer_by_ip( struct ip_addr *ip )
{
	struct list_head *lh;
	struct peer *p;

	lock( peer_table->mutex );

	list_for_each( lh, &(peer_table->peers)) {
		p = list_entry( lh, struct peer, all_peer_lh);
		if ( ip_addr_cmp(ip, &p->ip) ) {
			ref_peer( p );
			break;
		}
	}

	unlock( peer_table->mutex );
	return p;
}




#endif
