/*
 * $Id: peer.c,v 1.4 2003/03/07 19:29:19 bogdan Exp $
 *
 * 2003-02-18 created by bogdan
 *
 */


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils/dprint.h"
#include "utils/str.h"
#include "diameter_types.h"
#include "diameter_api.h"
#include "utils/aaa_lock.h"
#include "utils/str.h"
#include "utils/ip_addr.h"
#include "utils/resolve.h"
#include "tcp_shell/common.h"
#include "global.h"
#include "timer.h"
#include "sh_mutex.h"
#include "peer.h"
#include "message.h"
#include "avp.h"


#define PEER_TIMER_STEP  1
#define DELETE_TIMEOUT   2
#define RECONN_TIMEOUT   30
#define WAIT_CER_TIMEOUT 5


void peer_timer_handler(unsigned int ticks, void* param);
static struct timer *del_timer = 0;
static struct timer *reconn_timer = 0;
static struct timer *wait_cer_timer = 0;


int init_peer_manager()
{
	/* allocate the peer vector */
	peer_table = (struct p_table*)malloc( sizeof(struct p_table) );
	if (!peer_table) {
		LOG(L_ERR,"ERROR:init_peer_manager: no more free memory!\n");
		goto error;
	}
	memset( peer_table, 0, sizeof(struct p_table) );

	/* init the peer list */
	INIT_LIST_HEAD( &(peer_table->peers) );

	/* build and set the mutex */
	peer_table->mutex = create_locks(1);
	if (!peer_table->mutex) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot create lock!!\n");
		goto error;
	}

	/* create a delete timer list */
	del_timer = new_timer_list();
	if (!del_timer) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot create delete "
			"timer list!!\n");
		goto error;
	}

	/* create a reconnect timer list */
	reconn_timer = new_timer_list();
	if (!reconn_timer) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot create reconnect "
			"timer list!!\n");
		goto error;
	}

	/* create a wait CER timer list */
	wait_cer_timer = new_timer_list();
	if (!wait_cer_timer) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot create wait CER "
			"timer list!!\n");
		goto error;
	}

	/* set a delete timer function */
	if ( register_timer( peer_timer_handler, 0, PEER_TIMER_STEP)==-1 ) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot register delete timer!!\n");
		goto error;
	}

	LOG(L_INFO,"INFO:init_peer_manager: peer manager started\n");
	return 1;
error:
	LOG(L_INFO,"INFO:init_peer_manager: FAILED to start peer manager\n");
	return -1;
}




void destroy_peer_manager()
{
	struct list_head *lh, *foo;
	struct timer_link *tl, *tl_t;

	if (peer_table) {
		/* destroy all the peers */
		list_for_each_safe( lh, foo, &(peer_table->peers)) {
			/* free the peer */
			free( list_entry( lh, struct peer, all_peer_lh) );
		}
		/* destroy the mutex */
		if (peer_table->mutex)
			destroy_locks( peer_table->mutex, 1);
		/* destroy the table */
		free( peer_table );
	}

	if (del_timer) {
		/* destroy the peers from delete timer */
		tl = del_timer->first_tl.next_tl;
		while( tl!=&del_timer->last_tl){
			tl_t = tl;
			tl = tl->next_tl;
			/* free the peer */
			if (tl->payload)
				free( tl->payload );
		}
		/* destroy the list */
		destroy_timer_list( del_timer );
	}

	LOG(L_INFO,"INFO:destroy_peer_manager: peer manager stoped\n");
}



/* a new peer is created and added. The name of the peer host is given and the 
 * offset of the realm inside the host name. The host name and realm are copied
 * locally */
int add_peer( struct p_table *peer_table, str *host, unsigned int realm_offset,
															unsigned int port )
{
	static struct hostent* ht;
	struct peer *p;
	int i;

	p = 0;

	if (!peer_table || !host->s || !host->len) {
		LOG(L_ERR,"ERROR:add_peer: one of the param is null!!!!\n");
		goto error;
	}

	p = (struct peer*)malloc( sizeof(struct peer) + host->len + 1 );
	if(!p) {
		LOG(L_ERR,"ERROR:add_peer: no more free memory!\n");
		goto error;
	}
	memset(p,0,sizeof(struct peer) + host->len + 1 );

	/* fill the peer structure */
	p->tl.payload = p;
	p->mutex = get_shared_mutex();
	p->aaa_host.s = (char*)p + sizeof(struct peer);
	p->aaa_host.len = host->len;
	p->aaa_realm.s = host->s + realm_offset;
	p->aaa_realm.len = host->len - realm_offset;

	/* copy the host name converted to lower case */
	for(i=0;i<host->len;i++)
		p->aaa_host.s[i] = tolower( host->s[i] );

	/* resolve the host name */
	ht = resolvehost( p->aaa_host.s );
	if (!ht) {
		LOG(L_ERR,"ERROR:add_peer: cannot resolve host \"%s\"!\n",
			p->aaa_host.s);
		goto error;
	}
	/* fill the IP_ADDR structure */
	hostent2ip_addr( &p->ip, ht, 0);
	/* set the port */
	p->port = port;

	/* get a thread to listen for this peer */
	p->fd = get_new_receive_thread();

	/* insert the peer into the list */
	lock( peer_table->mutex );
	list_add_tail( &(p->thd_peer_lh), &(peer_table->peers) );
	unlock( peer_table->mutex );

	return 1;
error:
	if (p) free(p);
	return -1;
}



void destroy_peer( struct peer *p)
{
}





void init_all_peers()
{
	struct list_head *lh;

	list_for_each( lh, &(peer_table->peers)) {
		/* open tcp connection */
		//atomic_inc( &(p->ref_cnt) );
		tcp_connect( list_entry( lh, struct peer, all_peer_lh) );
	}
}




inline int add_ce_avps(AAA_AVP_LIST **avp_list ,struct ip_addr *my_ip)
{
	AAA_AVP      *avp;

	/* Host-IP-Addres AVP */
	if ( create_avp( &avp, 257, 0, 0,
#ifdef USE_IPV6
	my_ip->u.addr, my_ip->len,
#else
	(char*)&my_ip->s_addr, 4,
#endif
	0)!=AAA_ERR_SUCCESS)
		goto error;
	if ( AAAAddAVPToList( avp_list, avp, (*avp_list)->tail )!=
	AAA_ERR_SUCCESS)
		goto avp_error;

	/* Vendor-Id AVP */
	if ( create_avp( &avp, 266, 0, 0, (char*)&vendor_id, 4, 0)!=
	AAA_ERR_SUCCESS)
		goto error;
	if ( AAAAddAVPToList( avp_list, avp, (*avp_list)->tail )!=
	AAA_ERR_SUCCESS)
		goto avp_error;

	/* Product-Name AVP */
	if ( create_avp( &avp, 269, 0, 0, product_name.s, product_name.len, 0)!=
	AAA_ERR_SUCCESS)
		goto error;
	if ( AAAAddAVPToList( avp_list, avp, (*avp_list)->tail )!=
	AAA_ERR_SUCCESS)
		goto avp_error;

	/* other avps ??  */
	//.........................

	return 1;
avp_error:
	AAAFreeAVP( &avp );
error:
	return -1;
}



int send_cer( struct peer *dst_peer, struct ip_addr *my_ip)
{
	AAAMessage   *cer_msg;

	cer_msg = 0;

	if (!dst_peer)
		goto error;

	/* build a CER */

	/* create a new request template */
	cer_msg = AAANewMessage( 257, vendor_id, 0/*sessionId*/, 
			0/*extensionId*/, 0/*request*/);
	if (!cer_msg)
		goto error;

	/* add CE specific avps */
	if (add_ce_avps( &cer_msg->avpList, my_ip)==-1)
		goto error;

	/* send the message */
	send_aaa_message( cer_msg, 0, 0, dst_peer);

	return 1;
error:
	if (cer_msg)
		AAAFreeMessage( &cer_msg );
	return -1;
}



int send_cea( struct trans *tr, unsigned int result_code, str *error_msg,
													struct ip_addr *my_ip)
{
	AAAMessage   *cea_msg;

	cea_msg = 0;
	if (!tr || !tr->in_req || !tr->in_peer)
		goto error;

	/* build a CEA */
	cea_msg = build_rpl_from_req(tr->in_req, result_code, error_msg);
	if (!cea_msg)
		goto error;

	/* is it a protocol error code? */
	if (result_code<3000 || result_code>=4000){
		/* add CE specific avps */
		if (add_ce_avps( &cea_msg->avpList, my_ip)==-1)
			goto error;
	}

	/* send the message */
	send_aaa_message( cea_msg, tr, 0, tr->in_peer);

	return 1;
error:
	LOG(L_ERR,"ERROR:send_cea: failed to build and send cea!!!\n");
	if (cea_msg)
		AAAFreeMessage( &cea_msg );
	return -1;
}



int send_dpr( struct peer *dst_peer, unsigned int disc_cause)
{
	AAAMessage *dpr_msg;
	AAA_AVP    *avp;

	dpr_msg = 0;

	if (!dst_peer)
		goto error;

	/* build a DPR */

	/* create a new request template */
	dpr_msg = AAANewMessage( 282, vendor_id, 0/*sessionId*/,
			0/*extensionId*/, 0/*request*/);
	if (!dpr_msg)
		goto error;

	/* add Disconect-Cause AVP */
	/* Vendor-Id AVP */
	if ( create_avp( &avp, 273, 0, 0, (char*)&disc_cause, 4, 1)!=
	AAA_ERR_SUCCESS)
		goto error;
	if ( AAAAddAVPToList( &(dpr_msg->avpList), avp, dpr_msg->avpList->tail )!=
	AAA_ERR_SUCCESS) {
		AAAFreeAVP( &avp );
		goto error;
	}

	/* send the message */
	send_aaa_message( dpr_msg, 0, 0, dst_peer);

	return 1;
error:
	if (dpr_msg)
		AAAFreeMessage( &dpr_msg );
	return -1;
}



int send_dpa( struct trans *tr, unsigned int result_code, str *error_msg)
{
	AAAMessage   *dpa_msg;

	dpa_msg = 0;
	if (!tr || tr->in_req || tr->in_peer)
		goto error;

	/* build a CEA */
	dpa_msg = build_rpl_from_req(tr->in_req, result_code, error_msg);
	if (!dpa_msg)
		goto error;

	/* send the message */
	send_aaa_message( dpa_msg, tr, 0, tr->in_peer);

	return 1;
error:
	if (dpa_msg)
		AAAFreeMessage( &dpa_msg );
	return -1;
}



inline void reset_peer( struct peer *p)
{
	/* reset peer filds */
	if (p->tl.timer_list)
		rmv_from_timer_list( &(p->tl) );
	//p->conn->peer = 0;
	//p->conn = 0;
	/* do I have pending trans. in tr_timeout_timer list? */

}



int peer_state_machine( struct peer *p, enum AAA_PEER_EVENT event,
																void *info)
{
	//accept_info    *acc_inf;
	struct trans   *tr;
	static char    *err_msg[]= {
		"no error"
		"event - state mismatch",
		"unknown type event",
		"unknown peer state"
	};
	int error_code=0;

	if (!p) {
		LOG(L_ERR,"ERROR:peer_state_machine: addressed peer is 0!!\n");
		return -1;
	}

	DBG("DEBUG:peer_state_machine: peer %p, state = %d, event=%d\n",
		p,p->state,event);

	switch (event) {
		case TCP_ACCEPT:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: accepting connection\n");
					/* if peer in reconn timer list-> take it out*/
					if (p->tl.timer_list==reconn_timer)
						rmv_from_timer_list( &(p->tl) );
					//if (p->conn)
					//	DBG("ELECTION!!!!!\n");
					/* open the tcp connection */
					//acc_inf = (accept_info*)info;
#if 0
					conn = tcp_open( p, acc_inf->socket, &(acc_inf->localaddr),
						&(acc_inf->peeraddr) );
					if (conn) {
						p->conn = conn;
						/* put the peer in wait_cer timer list */
						add_to_timer_list( &(p->tl), wait_cer_timer,
							get_ticks()+WAIT_CER_TIMEOUT);
						/* the new state */
						p->state = PEER_WAIT_CER;
					} else {
						atomic_dec( &(p->ref_cnt) );
					}
#endif
					unlock( p->mutex );
					break;
				default:
					atomic_dec( &(p->ref_cnt) );
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_CONNECTED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: connect finished\n");
					//send_cer( p, &(p->conn->localaddr) );
					p->state = PEER_WAIT_CEA;
					unlock( p->mutex );
					break;
				default:
					atomic_dec( &(p->ref_cnt) );
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_UNREACHABLE:
		case TCP_CLOSE:
		case TCP_EXPIRED:
		case PEER_TIMEOUT:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: peer already closed!\n");
					unlock( p->mutex );
					break;
				default:
					DBG("DEBUG:peer_state_machine: closing connection\n");
					if (event==TCP_CLOSE)
						;//tcp_close( p->conn );
					else
						;//tcp_close_lazy( p->conn );
					reset_peer( p );
					atomic_dec( &(p->ref_cnt) );
					/* new state */
					p->state = PEER_UNCONN;
					unlock( p->mutex );
			}
			break;
		case TCP_CONN_FAILED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: connect failed\n");
					/* put the peer in reconnect timer list for later retring
					 * only if the peer is not marked for delete, otherwise
					 * unref the peer */
					if (!p->flags&PEER_TO_DESTROY)
						add_to_timer_list( &(p->tl), reconn_timer,
							get_ticks()+RECONN_TIMEOUT);
					else
						atomic_dec( &(p->ref_cnt) );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
					atomic_dec( &(p->ref_cnt) );
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case CEA_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_CEA:
					DBG("DEBUG:peer_state_machine: CEA received\n");
					p->state = PEER_CONN;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case CER_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_CER:
					/* if peer in wait_cer timer list-> take it out*/
					if (p->tl.timer_list==wait_cer_timer)
						rmv_from_timer_list( &(p->tl) );
					tr = (struct trans*)info;
					DBG("DEBUG:peer_state_machine: CER received -> "
						"sending CEA\n");
					//send_cea( tr, 2001, 0, &(tr->in_peer->conn->localaddr));
					p->state = PEER_CONN;
					unlock( p->mutex );
					break;
				case PEER_WAIT_CEA:
					// send_cea() - can this be done outside the lock?
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case DPR_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_CONN:
					// send_dpa() can this be done outside the lock?
					//tcp_close_lazy( p->conn );
					reset_peer( p );
					atomic_dec( &(p->ref_cnt) );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case DPA_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_DPA:
					//tcp_close_lazy( p->conn );
					reset_peer( p );
					atomic_dec( &(p->ref_cnt) );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case PEER_HANGUP:
			lock( p->mutex );
			switch (p->state) {
				case PEER_CONN:
					// set peer not usable
					// send_dpr() - can this be done outside the lock?
					p->state = PEER_WAIT_DPA;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		default:
			error_code = 2;
			goto error;
	}

	DBG("DEBUG:peer_state_machine: peer %p, new state = %d\n",
		p,p->state);

	return 1;
error:
	LOG(L_ERR,"ERROR:peer_state_machine: %s : peer=%p, state=%d, event=%d\n",
		err_msg[error_code],p,p->state,event);
	return -1;
}




void peer_timer_handler(unsigned int ticks, void* param)
{
	struct timer_link *tl;
	struct peer *p;

	/* DELETE TIMER LIST */
	/* get the expired peers */
	tl = get_expired_from_timer_list( del_timer, ticks);
	/* process the peers */
	while (tl) {
		p  = tl->payload;
		tl = tl->next_tl;
		/* is the peer ref-ed anymore? */
		if ( atomic_read(&p->ref_cnt)==0 ) {
			/* delete the peer ???? */
		} else {
			/* put it back into the timer list */
			add_to_timer_list( &p->tl, del_timer, ticks+DELETE_TIMEOUT );
		}
	}

	/* RECONN TIME LIST */
	tl = get_expired_from_timer_list( reconn_timer, ticks);
	/* process the peers */
	while (tl) {
		p  = (struct peer*)tl->payload;
		tl = tl->next_tl;
		if (!p->flags&PEER_TO_DESTROY) {
			/* try again to connect the peer */
			DBG("DEBUG:peer_timer_handler: re-tring to connect peer %p \n",p);
			tcp_connect( p );
		} else {
			atomic_dec( &(p->ref_cnt) );
		}
	}

	/* WAIT_CER LIST */
	tl = get_expired_from_timer_list( wait_cer_timer, ticks);
	/* process the peers */
	while (tl) {
		p  = (struct peer*)tl->payload;
		tl = tl->next_tl;
		/* close the connection */
		DBG("DEBUG:peer_timer_handler: peer %p hasn't received a CER! ->"
			" close it!!\n",p);
		peer_state_machine( p, PEER_TIMEOUT, 0);
	}


}

