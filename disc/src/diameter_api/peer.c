/*
 * $Id: peer.c,v 1.16 2003/03/11 23:02:03 bogdan Exp $
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


#define cheack_app_ids( _peer_ ) \
	( (((_peer_)->supp_acc_app_id)|(supported_acc_app_id)) || \
	(((_peer_)->supp_auth_app_id)|(supported_auth_app_id))  )


unsigned int AAA_APP_ID[ AAA_APP_MAX_ID ] = {
	0xffffffff,   /* AAA_APP_RELAY */
	0x00000000,   /* AAA_APP_DIAMETER_COMMON_MSG */
	0x00000001,   /* AAA_APP_NASREQ */
	0x00000002,   /* AAA_APP_MOBILE_IP */
	0x00000003    /* AAA_APP_DIAMETER_BASE_ACC */
};


int build_msg_buffers(struct p_table *peer_table);
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

	/* creates the msg buffers */
	if (build_msg_buffers(peer_table)==-1) {
		LOG(L_ERR,"ERROR:init_peer_manager: cannot build msg buffers!!\n");
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
			list_del( lh );
			free( list_entry( lh, struct peer, all_peer_lh) );
		}
		/* destroy the mutex */
		if (peer_table->mutex)
			destroy_locks( peer_table->mutex, 1);
		/* free the buffers */
		if (peer_table->std_req.s)
			free(peer_table->std_req.s);
		if (peer_table->std_ans.s)
			free(peer_table->std_ans.s);
		if (peer_table->ce_avp_ipv4.s)
			free(peer_table->ce_avp_ipv4.s);
		if (peer_table->ce_avp_ipv6.s)
			free(peer_table->ce_avp_ipv6.s);
		if (peer_table->dpr_avp.s)
			free(peer_table->dpr_avp.s);
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

	if (wait_cer_timer)
		destroy_timer_list( wait_cer_timer );

	if (reconn_timer)
		destroy_timer_list( reconn_timer );

	LOG(L_INFO,"INFO:destroy_peer_manager: peer manager stoped\n");
}




int build_msg_buffers(struct p_table *table)
{
	int  nr_auth_app;
	int  nr_acc_app;
	char *ptr;
	int  i;

	/* standard request */
	table->std_req.len = AAA_MSG_HDR_SIZE +            /* header */
		AVP_HDR_SIZE + to_32x_len(aaa_identity.len) +  /* origin-host  */
		AVP_HDR_SIZE + to_32x_len(aaa_realm.len);      /* origin-realm */
	table->std_req.s = malloc( table->std_req.len );
	if (!table->std_req.s)
		goto error;
	ptr = table->std_req.s;
	memset( ptr, 0, table->std_req.len);
	/* diameter header */
	ptr[0] = 0x01;
	ptr[4] = 0x80;
	ptr += AAA_MSG_HDR_SIZE;
	/* origin host AVP */
	((unsigned int*)ptr)[0] = htonl(264);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + aaa_identity.len );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE;
	memcpy( ptr, aaa_identity.s, aaa_identity.len);
	ptr += to_32x_len(aaa_identity.len);
	/* origin realm AVP */
	((unsigned int*)ptr)[0] = htonl(296);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + aaa_realm.len );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE;
	memcpy( ptr, aaa_realm.s, aaa_realm.len);
	ptr += to_32x_len(aaa_realm.len);


	/* standard answer */
	table->std_ans.len = AAA_MSG_HDR_SIZE +            /* header */
		AVP_HDR_SIZE + 4 +                             /* result-code  */
		AVP_HDR_SIZE + to_32x_len(aaa_identity.len) +  /* origin-host  */
		AVP_HDR_SIZE + to_32x_len(aaa_realm.len);      /* origin-realm */
	table->std_ans.s = malloc( table->std_ans.len );
	if (!table->std_ans.s)
		goto error;
	ptr = table->std_ans.s;
	memset( ptr, 0, table->std_ans.len);
	/* diameter header */
	ptr[0] = 0x01;
	ptr[4] = 0x00;
	ptr += AAA_MSG_HDR_SIZE;
	/* result code AVP */
	((unsigned int*)ptr)[0] = htonl(268);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE + 4;
	/* origin host AVP */
	((unsigned int*)ptr)[0] = htonl(264);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + aaa_identity.len );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE;
	memcpy( ptr, aaa_identity.s, aaa_identity.len);
	ptr += to_32x_len(aaa_identity.len);
	/* origin realm AVP */
	((unsigned int*)ptr)[0] = htonl(296);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + aaa_realm.len );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE;
	memcpy( ptr, aaa_realm.s, aaa_realm.len);
	ptr += to_32x_len(aaa_realm.len);


	/* CE avps IPv4 */
	nr_auth_app = nr_acc_app = 0;
	for(i=0;i<AAA_APP_MAX_ID;i++) {
		if ((1<<i)&supported_auth_app_id)
			nr_auth_app++;
		if ((1<<i)&supported_acc_app_id)
			nr_acc_app++;
	}
	/* build the avps */
	table->ce_avp_ipv4.len =
		AVP_HDR_SIZE + 4 +                             /* host-ip-address  */
		AVP_HDR_SIZE + 4 +                             /* vendor-id  */
		AVP_HDR_SIZE + to_32x_len(product_name.len) +  /* product-name */
		nr_auth_app*(AVP_HDR_SIZE + 4) +               /* auth-app-id */
		nr_acc_app*(AVP_HDR_SIZE + 4);                 /* acc-app-id */
	table->ce_avp_ipv4.s = malloc( table->ce_avp_ipv4.len );
	if (!table->ce_avp_ipv4.s)
		goto error;
	ptr = table->ce_avp_ipv4.s;
	memset( ptr, 0, table->ce_avp_ipv4.len);
	/* host-ip-address AVP */
	((unsigned int*)ptr)[0] = htonl(257);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE + 4;
	/* vendor-id AVP */
	((unsigned int*)ptr)[0] = htonl(266);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE;
	((unsigned int*)ptr)[0] = htonl( vendor_id );
	ptr += 4;
	/* product-name AVP */
	((unsigned int*)ptr)[0] = htonl(269);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + product_name.len );
	ptr += AVP_HDR_SIZE;
	memcpy( ptr, product_name.s, product_name.len);
	ptr += to_32x_len(product_name.len);
	/* auth-app-id AVP */
	for(i=0;i<AAA_APP_MAX_ID;i++)
		if ((1<<i)&supported_auth_app_id) {
			((unsigned int*)ptr)[0] = htonl(258);
			((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
			ptr[4] = 1<<6;
			ptr += AVP_HDR_SIZE;
			((unsigned int*)ptr)[0] = htonl( AAA_APP_ID[i] );
			ptr += 4;
		}
	/* acc-app-id AVP */
	for(i=0;i<AAA_APP_MAX_ID;i++)
		if ((1<<i)&supported_acc_app_id) {
			((unsigned int*)ptr)[0] = htonl(259);
			((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
			ptr[4] = 1<<6;
			ptr += AVP_HDR_SIZE;
			((unsigned int*)ptr)[0] = htonl( AAA_APP_ID[i] );
			ptr += 4;
		}


	/* CE avps IPv6 */
	table->ce_avp_ipv6.len = table->ce_avp_ipv4.len + 12;
	table->ce_avp_ipv6.s = malloc( table->ce_avp_ipv6.len );
	if (!table->ce_avp_ipv6.s)
		goto error;
	ptr = table->ce_avp_ipv6.s;
	memset( ptr, 0, table->ce_avp_ipv6.len);
	/* copy */
	memcpy( ptr, table->ce_avp_ipv4.s, AVP_HDR_SIZE + 4);
	ptr += AVP_HDR_SIZE + 16;
	memcpy( ptr, table->ce_avp_ipv4.s+AVP_HDR_SIZE+4,
		table->ce_avp_ipv4.len-AVP_HDR_SIZE-4);


	/* DPR avp */
	table->dpr_avp.len = AVP_HDR_SIZE + 4; /* disconnect cause  */
	table->dpr_avp.s = malloc( table->dpr_avp.len );
	if (!table->dpr_avp.s)
		goto error;
	ptr = table->dpr_avp.s;
	memset( ptr, 0, table->dpr_avp.len);
	/* disconnect cause AVP */
	((unsigned int*)ptr)[0] = htonl(273);
	((unsigned int*)ptr)[1] = htonl( AVP_HDR_SIZE + 4 );
	ptr[4] = 1<<6;
	ptr += AVP_HDR_SIZE + 4;

	return 1;
error:
	LOG(L_ERR,"ERROR:build_msg_buffers: no more free memory\n");
	return -1;
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
	list_add_tail( &(p->all_peer_lh), &(peer_table->peers) );
	unlock( peer_table->mutex );

	/* give the peer to the designated thread */
	write_command( p->fd, ADD_PEER_CMD, 0, p, 0);

	return 1;
error:
	if (p) free(p);
	return -1;
}




void destroy_peer( struct peer *p)
{
}




int send_cer( struct peer *dst_peer)
{
	char *ptr;
	str cer;

	cer.len = peer_table->std_req.len + peer_table->ce_avp_ipv4.len;
	cer.s = malloc( cer.len );
	if (!cer.s) {
		LOG(L_ERR,"ERROR:send_cer: no more free memory\n");
		goto error;
	}
	ptr = cer.s;
	/**/
	memcpy( ptr, peer_table->std_req.s, peer_table->std_req.len );
	((unsigned int*)ptr)[0] |= htonl( cer.len );
	((unsigned int*)ptr)[1] |= CE_MSG_CODE;
	ptr += peer_table->std_req.len;
	/**/
	memcpy( ptr, peer_table->ce_avp_ipv4.s, peer_table->ce_avp_ipv4.len );
	memcpy( ptr + AVP_HDR_SIZE, &dst_peer->local_ip, sizeof(struct ip_addr) );

	/* send the buffer */
	return send_aaa_request( &cer, 0, dst_peer);
error:
	return -1;
}




int send_cea( struct trans *tr, unsigned int result_code)
{
	char *ptr;
	str cea;

	cea.len = peer_table->std_ans.len + peer_table->ce_avp_ipv4.len;
	cea.s = malloc( cea.len );
	if (!cea.s) {
		LOG(L_ERR,"ERROR:send_cer: no more free memory\n");
		goto error;
	}
	ptr = cea.s;
	/**/
	memcpy( ptr, peer_table->std_ans.s, peer_table->std_ans.len );
	((unsigned int*)ptr)[0] |= htonl( cea.len );
	((unsigned int*)ptr)[1] |= CE_MSG_CODE;
	((unsigned int*)ptr)[(AAA_MSG_HDR_SIZE+AVP_HDR_SIZE)>>2] =
		htonl( result_code );
	ptr += peer_table->std_ans.len;
	/**/
	memcpy( ptr, peer_table->ce_avp_ipv4.s, peer_table->ce_avp_ipv4.len );
	memcpy( ptr + AVP_HDR_SIZE, &tr->peer->local_ip, sizeof(struct ip_addr) );


	/* send the buffer */
	return send_aaa_response( &cea, tr);

	return 1;
error:
	return -1;
}



int process_ce( struct peer *p, str *buf , int is_req)
{
	static unsigned int rpl_pattern = 0x0000003f;
	static unsigned int req_pattern = 0x0000003e;
	unsigned int mask = 0;
	unsigned int n;
	char *ptr;
	char *foo;
	int  i;

	for_all_AVPS_do_switch( buf , foo , ptr ) {
		case 268: /* result_code */
			set_AVP_mask( mask, 0);
			n = ntohl( ((unsigned int *)ptr)[2] );
			if (n!=AAA_SUCCESS) {
				LOG(L_ERR,"ERROR:process_ce: CEA has a non-success "
					"code : %d\n",n);
				goto error;
			}
			break;
		case 264: /* orig host */
			set_AVP_mask( mask, 1);
			break;
		case 296: /* orig realm */
			set_AVP_mask( mask, 2);
			break;
		case 257: /* host ip address */
			set_AVP_mask( mask, 3);
			break;
		case 266: /* vendor ID */
			set_AVP_mask( mask, 4);
			break;
		case 269: /*product name */
			set_AVP_mask( mask, 5);
			break;
		case 259: /*acc app id*/
			n = ntohl( ((unsigned int *)ptr)[2] );
			for(i=0;i<AAA_APP_MAX_ID;i++)
				if (n==AAA_APP_ID[i]) {
					p->supp_acc_app_id |= (1<<i);
					break;
				}
			LOG(L_WARN,"WARNING:process_ce: unknown acc-app-id %d\n",n);
			break;
		case 258: /*auth app id*/
			n = ntohl( ((unsigned int *)ptr)[2] );
			for(i=0;i<AAA_APP_MAX_ID;i++)
				if (n==AAA_APP_ID[i]) {
					p->supp_auth_app_id |= (1<<i);
					break;
				}
			LOG(L_WARN,"WARNING:process_ce: unknown auth_app-id %d\n",n);
			break;
	}

	if ( mask!=(is_req?req_pattern:rpl_pattern) ) {
		LOG(L_ERR,"ERROR:process_ce: ce(a|r) has missing avps(%x<>%x)!!\n",
			(is_req?req_pattern:rpl_pattern),mask);
		goto error;
	}

	free( buf->s );
	return 1;
error:
	free( buf->s );
	return -1;
}



int send_dwr( struct peer *dst_peer)
{
	char *ptr;
	str dwr;

	dwr.len = peer_table->std_req.len ;
	dwr.s = malloc( dwr.len );
	if (!dwr.s) {
		LOG(L_ERR,"ERROR:send_dwr: no more free memory\n");
		goto error;
	}
	ptr = dwr.s;
	/**/
	memcpy( ptr, peer_table->std_req.s, peer_table->std_req.len );
	((unsigned int*)ptr)[0] |= htonl( dwr.len );
	((unsigned int*)ptr)[1] |= DW_MSG_CODE;

	/* send the buffer */
	return send_aaa_request( &dwr, 0, dst_peer);
error:
	return -1;
}



int send_dwa( struct trans *tr, unsigned int result_code)
{
	char *ptr;
	str dwa;

	dwa.len = peer_table->std_ans.len;
	dwa.s = malloc( dwa.len );
	if (!dwa.s) {
		LOG(L_ERR,"ERROR:send_dwa: no more free memory\n");
		goto error;
	}
	ptr = dwa.s;
	/**/
	memcpy( ptr, peer_table->std_ans.s, peer_table->std_ans.len );
	((unsigned int*)ptr)[0] |= htonl( dwa.len );
	((unsigned int*)ptr)[1] |= DW_MSG_CODE;
	((unsigned int*)ptr)[(AAA_MSG_HDR_SIZE+AVP_HDR_SIZE)>>2] =
		htonl( result_code );

	ptr += peer_table->std_ans.len;

	/* send the buffer */
	return send_aaa_response( &dwa, tr);

	return 1;
error:
	return -1;
}



int process_dw( struct peer *p, str *buf , int is_req)
{
	static unsigned int rpl_pattern = 0x00000007;
	static unsigned int req_pattern = 0x00000006;
	unsigned int mask = 0;
	unsigned int n;
	char *ptr;
	char *foo;

	for_all_AVPS_do_switch( buf , foo , ptr ) {
		case 268: /* result_code */
			set_AVP_mask( mask, 0);
			n = ntohl( ((unsigned int *)ptr)[2] );
			if (n!=AAA_SUCCESS) {
				LOG(L_ERR,"ERROR:process_ce: DWA has a non-success "
					"code : %d\n",n);
				goto error;
			}
			break;
		case 264: /* orig host */
			set_AVP_mask( mask, 1);
			break;
		case 296: /* orig realm */
			set_AVP_mask( mask, 2);
			break;
	}

	if ( mask!=(is_req?req_pattern:rpl_pattern) ) {
		LOG(L_ERR,"ERROR:process_dw: dw(a|r) has missing avps(%x<>%x)!!\n",
			(is_req?req_pattern:rpl_pattern),mask);
		goto error;
	}

	free( buf->s );
	return 1;
error:
	free( buf->s );
	return -1;
}




#if 0
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
	//send_aaa_message( dpr_msg, 0, 0, dst_peer);

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
	//if (!tr || tr->in_req || tr->in_peer)
	//	goto error;

	/* build a CEA */
	//dpa_msg = build_rpl_from_req(tr->in_req, result_code, error_msg);
	if (!dpa_msg)
		goto error;

	/* send the message */
	//send_aaa_message( dpa_msg, tr, 0, tr->peer);

	return 1;
error:
	if (dpa_msg)
		AAAFreeMessage( &dpa_msg );
	return -1;
}
#endif



void dispatch_message( struct peer *p, str *buf)
{
	struct trans *tr;
	unsigned int code;
	int          event;

	/* get message code */
	code = ((unsigned int*)buf->s)[1]&MASK_MSG_CODE;
	/* check the message code */
	switch ( code ) {
		case CE_MSG_CODE:
			event = CER_RECEIVED;
			break;
		case DW_MSG_CODE:
			event = DWR_RECEIVED;
			break;
		case DP_MSG_CODE:
			event = DPR_RECEIVED;
			break;
		default:
			/* it's a session message*/
			LOG(L_ERR,"UNIMPLEMENTED: message for a session arrived ->"
				" discard it!!\n");
			free( buf->s );
			return;
	}

	/* is request or reply? */
	if (buf->s[VER_SIZE+MESSAGE_LENGTH_SIZE]&0x80) {
		/* request */
		tr = create_transaction( buf, 0/*ses*/, p);
		if (tr) {
			if (peer_state_machine( p, event, tr)==-1)
				destroy_transaction( tr );
		} else {
			free( buf->s );
		}
	} else {
		/* response -> find its transaction and remove it from 
		 * hash table (search and remove is an atomic operation) */
		tr = transaction_lookup( ((unsigned int*)buf->s)[4],
			((unsigned int*)buf->s)[3], 1);
		if (!tr) {
			LOG(L_ERR,"ERROR:dispatch_message: respons received, but no"
				" transaction found!\n");
			free( buf->s );
		} else {
			/* destroy the transaction along with the originator request */
			destroy_transaction( tr );
			/*make from a request event a response event */
			event++;
			/* call the peer machine */
			peer_state_machine( p, event, buf );
		}
	}
}



inline void reset_peer( struct peer *p)
{
	/* reset peer filds */
	if (p->tl.timer_list)
		rmv_from_timer_list( &(p->tl) );
	p->sock = -1;
	/**/
	p->flags &= !PEER_CONN_IN_PROG;
	if (!p->flags&PEER_TO_DESTROY)
		add_to_timer_list( &(p->tl), reconn_timer,
			get_ticks()+RECONN_TIMEOUT);
	/**/
	p->first_4bytes = 0;
	p->buf_len = 0;
	if (p->buf)
		free(p->buf);
	p->buf = 0;
	/**/
	p->supp_acc_app_id  = 0;
	p->supp_auth_app_id = 0;
}




int peer_state_machine( struct peer *p, enum AAA_PEER_EVENT event, void *ptr)
{
	struct tcp_info *info;
	static char     *err_msg[]= {
		"no error",
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
					else if (p->flags&PEER_CONN_IN_PROG) {
						tcp_close( p );
					}
					/* update the peer */
					info = (struct tcp_info*)ptr;
					p->sock = info->sock;
					memcpy( &p->local_ip, info->local, sizeof(struct ip_addr));
					/* put the peer in wait_cer timer list */
					add_to_timer_list( &(p->tl), wait_cer_timer,
						get_ticks()+WAIT_CER_TIMEOUT);
					/* the new state */
					p->state = PEER_WAIT_CER;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_CONNECTED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: connect finished ->"
						" send CER\n");
					/* update the peer */
					p->flags &= !PEER_CONN_IN_PROG;
					info = (struct tcp_info*)ptr;
					p->sock = info->sock;
					memcpy( &p->local_ip, info->local, sizeof(struct ip_addr));
					/* send cer */
					if (send_cer( p )!=-1) {
						/* new state */
						p->state = PEER_WAIT_CEA;
					} else {
						tcp_close( p );
						reset_peer( p );
					}
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_CONN_IN_PROG:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: connect in progress\n");
					/* update the peer */
					info = (struct tcp_info*)ptr;
					p->sock = info->sock;
					p->flags |= PEER_CONN_IN_PROG;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_CONN_FAILED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: connect failed\n");
					reset_peer( p );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case TCP_CLOSE:
			lock( p->mutex );
			DBG("DEBUG:peer_state_machine: closing connection\n");
			tcp_close( p );
			reset_peer( p );
			/* new state */
			p->state = PEER_UNCONN;
			unlock( p->mutex );
			break;
		case PEER_CER_TIMEOUT:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_CER:
					DBG("DEBUG:peer_state_machine: CER not received!\n");
					tcp_close( p );
					reset_peer( p );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case PEER_RECONN_TIMEOUT:
			lock( p->mutex );
			switch (p->state) {
				case PEER_UNCONN:
					DBG("DEBUG:peer_state_machine: reconnecting peer\n");
					if (p->flags&PEER_CONN_IN_PROG)
						tcp_close( p );
					write_command( p->fd, CONNECT_CMD, 0, p, 0);
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					DBG("DEBUG:peer_state_machine: no reason to reconnect\n");
			}
			break;
		case PEER_TR_TIMEOUT:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_CEA:
				case PEER_WAIT_DPA:
					DBG("DEBUG:peer_state_machine: transaction timeout\n");
					tcp_close( p );
					reset_peer( p );
					p->state = PEER_UNCONN;
					unlock( p->mutex );
					break;
				default:
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
					if ( process_ce( p, (str*)ptr, 0 )==-1 ) {
						tcp_close( p );
						reset_peer( p );
						p->state = PEER_UNCONN;
					} else {
						p->state = PEER_CONN;
					}
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
					/* if peer in wait_cer timer list-> take it out */
					if (p->tl.timer_list==wait_cer_timer)
						rmv_from_timer_list( &(p->tl) );
				case PEER_WAIT_CEA:
					DBG("DEBUG:peer_state_machine: CER received -> "
						"sending CEA\n");
					if (process_ce( p, &(((struct trans*)ptr)->req), 1 )==-1) {
						LOG(L_ERR,"ERROR:peer_state_machine: bad CER !!\n");
						send_cea( (struct trans*)ptr, AAA_MISSING_AVP);
						tcp_close( p );
						reset_peer( p );
						p->state = PEER_UNCONN;
					} else if (cheack_app_ids(p)) {
						send_cea( (struct trans*)ptr, AAA_SUCCESS);
						p->state = PEER_CONN;
					} else {
						LOG(L_ERR,"ERROR:peer_state_machine: no common app\n");
						send_cea((struct trans*)ptr,AAA_NO_COMMON_APPLICATION);
						tcp_close( p );
						reset_peer( p );
						p->state = PEER_UNCONN;
					}
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case DWR_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_CONN:
				case PEER_WAIT_DWA:
				case PEER_WAIT_DPA:
					DBG("DEBUG:peer_state_machine: DWR received -> "
						"sending DWA\n");
					if (process_dw( p, &(((struct trans*)ptr)->req), 1 )==-1) {
						LOG(L_ERR,"ERROR:peer_state_machine: bad DWR !!\n");
						send_dwa( (struct trans*)ptr, AAA_MISSING_AVP);
					} else {
						send_dwa( (struct trans*)ptr, AAA_SUCCESS);
					}
					unlock( p->mutex );
					break;
				default:
					unlock( p->mutex );
					error_code = 1;
					goto error;
			}
			break;
		case DWA_RECEIVED:
			lock( p->mutex );
			switch (p->state) {
				case PEER_WAIT_DWA:
					p->state = PEER_CONN;
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
			write_command( p->fd, TIMEOUT_PEER_CMD, PEER_RECONN_TIMEOUT, p, 0);
		}
	}

	/* WAIT_CER LIST */
	tl = get_expired_from_timer_list( wait_cer_timer, ticks);
	/* process the peers */
	while (tl) {
		p  = (struct peer*)tl->payload;
		tl = tl->next_tl;
		/* close the connection */
		DBG("DEBUG:peer_timer_handler: peer %p hasn't received CER yet!\n",p);
		write_command( p->fd, TIMEOUT_PEER_CMD, PEER_CER_TIMEOUT, p, 0);
	}


}

