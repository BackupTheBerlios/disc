/*
 * $Id: route.c,v 1.9 2003/04/14 14:36:38 andrei Exp $
 */
/*
 * History:
 * --------
 *  2003-04-08  created by andrei
 *  2003-04-13  added do_route (andrei)
 */


#include "config.h"
#include "globals.h"
#include "route.h"
#include "dprint.h"
#include "mem/shm_mem.h"
#include <fnmatch.h>


/* peer list*/
struct peer_entry* cfg_peer_lst=0;
struct route_entry* route_lst=0;


/* tmp is a temporary iterator */
#define LIST_APPEND(list, elem, tmp, nxt) \
	do{ \
		if ((list)==0) (list)=(elem); \
		else{ \
			for((tmp)=(list); (tmp)->nxt; (tmp)=(tmp)->nxt); \
			(tmp)->nxt=(elem); \
		} \
	}while(0)



/* return -1 on error, 0 on success */
int add_cfg_peer(str* uri, str* alias)
{
	struct peer_entry *n;
	struct peer_entry *t;
	
	/* check if  peer or alias already present */
	for(t=cfg_peer_lst; t; t=t->next){
		if ((alias)&&(t->alias.len==alias->len)&&
			(strncasecmp(t->alias.s, alias->s, alias->len)==0)){
			LOG(L_ERR, "ERROR: a peer with alias <%.*s> already"
					" added (<%.*s>)\n", alias->len, alias->s, 
					t->full_uri.len, t->full_uri.s);
			return -1;
		}
		if ((uri->len==t->full_uri.len)&&
				(strncasecmp(t->full_uri.s, uri->s, uri->len)==0)){
			LOG(L_ERR, "ERROR: peer <%.*s> already exists\n",
					uri->len, uri->s);
			return -1;
		}
	}
	n=shm_malloc(sizeof(struct peer_entry));
	if (n==0){
		LOG(L_ERR, "ERROR: add_cfg_peer: mem. alloc failure\n");
		return -1;
	}
	memset(n, 0, sizeof(struct peer_entry));
	n->full_uri=*uri;
	n->alias=*alias;
	if (aaa_parse_uri(n->full_uri.s, n->full_uri.len, &n->uri)!=0){
		LOG(L_ERR, "ERROR: add_cfg_peer: bad uri\n");
		return -1;
	}
	LIST_APPEND(cfg_peer_lst, n, t, next);
	return 0;
}



/* return -1 on error, 0 on success */
int add_route(str* realm, str* dst)
{
	struct route_entry* re;
	struct peer_entry* pe;
	struct peer_entry* new_pe;
	struct peer_entry*  pi;
	struct route_entry* ri;
	int ret;
	

	if (my_aaa_status<AAA_SERVER) goto error_client;
	/* find coreponding peer, try alias match, full uri(id) match and
	 *  host match */
	for (pe=cfg_peer_lst; pe; pe=pe->next){
		if ((pe->alias.len==dst->len)&&
				(strncasecmp(pe->alias.s, dst->s, dst->len)==0))
			break;
		if ((pe->full_uri.len==dst->len)&&
				(strncasecmp(pe->full_uri.s, dst->s, dst->len)==0))
			break;
		/* should we keep this? it's pretty ambiguos */
		if ((pe->uri.host.len==dst->len)&&
				(strncasecmp(pe->uri.host.s, dst->s, dst->len)==0))
			break;
	}
	if (pe==0) goto error_no_peer;
	new_pe=shm_malloc(sizeof(struct peer_entry));
	if (new_pe==0) goto error_mem;
	/* prepare the new peerlist entry */
	*new_pe=*pe;
	new_pe->next=0;
	
	/* see if route for realm already exists */
	for (re=route_lst; re; re=re->next){
		if ((re->realm.len==realm->len)&&
				(strncasecmp(re->realm.s, realm->s, realm->len)==0))
			break;
	}
	if (re==0){
		/* not found -> create new one */
		re=shm_malloc(sizeof(struct route_entry));
		if (re==0) goto error_mem;
		memset(re,0, sizeof(struct route_entry));
		re->realm=*realm;
	}
	
	/* append to the route_entry peer list */
	LIST_APPEND(re->peer_l, new_pe, pi, next);
	/* append the route entry to the main list */
	LIST_APPEND(route_lst, re, ri, next);
	/* we don't need dst.s anymore => free it */
	
	ret=0;
end:
	if (dst->s) {shm_free(dst->s);dst->s=0;}
	return ret;
error_mem:
	LOG(L_ERR, "ERROR: add_route: memory allocation error\n");
	ret=-1;
	goto end;
error_no_peer:
	LOG(L_ERR, "ERROR: add_route: no peer <%.*s> found\n", dst->len, dst->s);
	ret=-1;
	goto end;
error_client:
	LOG(L_CRIT, "ERROR: routes allowed only in server mode "
			" (set aaa_status to server)\n");
	ret=-1;
	goto end;
}



/* returns the first match peer list or 0 on error */
/* WARNING: all this must be null terminated */
struct peer_entry* route_dest(str* dst_realm)
{
	struct route_entry* re;
	char realm [MAX_REALM_LEN+1]; /* needed for null termination*/
	
	if (dst_realm->len>MAX_REALM_LEN){
		LOG(L_ERR,"ERROR: route_dest: realm too big (%d)\n", dst_realm->len);
		return 0;
	}
	memcpy(realm, dst_realm->s, dst_realm->len);
	realm[dst_realm->len]=0; /* null terminate  */
		
	for (re=route_lst; re; re=re->next){
		/*FNM_CASEFOLD | FNM_EXTMATCH - GNU extensions*/ 
		if (fnmatch(re->realm.s, realm, 0)==0){ /* re->realm.s is 0 terminated*/
			/*match */
			DBG("route_dest: match on <%s> (<%s>)\n",
				re->realm.s, realm);
			return re->peer_l;
		}
	}
	DBG("WARNING: route_dest: no route found for <%s>\n", realm);
	
	return 0;
}



/* send the msg according to the routing table
 * returns 0 on success, <0 on error */
int do_route(AAAMessage *msg, struct peer *in_p)
{
	struct peer_entry* pl;
	struct trans* tr;
	
	pl=route_dest(&in_p->aaa_realm);
	if (pl==0) goto noroute;
	/* transaction stuff */
	tr=create_transaction(&(msg->buf), in_p);
	if (tr==0) goto error_transaction;
	update_forward_transaction_from_msg(tr, msg, in_p);
	/* try to send it to the first peer in the route */
	for(;pl;pl=pl->next){
		if (pl->peer){
			if (send_req_to_peer(tr, pl->peer)<0){
				LOG(L_WARN, "WARNING: do_route: unable to send to %.*s\n",
						pl->full_uri.len, pl->full_uri.s);
				continue; /*try next peer*/
			}else{
				DBG("do_route: sending msg to %.*s\n", pl->full_uri.len,
						pl->full_uri.s);
				goto end;
			}
		}
	}
	/* if we are here => all sends failed */
	/* cleanup */
	destroy_transaction(tr);
noroute:
	LOG(L_ERR, "ERROR: do_route: dropping message, no peers/route found\n");
	return -1;
end:
	return 1;
error_transaction:
	LOG(L_ERR, "ERROR: do_route: unable to create transaction\n");
	return -1;
}



void destroy_cfg_peer_lst()
{
	struct peer_entry* p;
	struct peer_entry* r;
	
	p=cfg_peer_lst; 
	cfg_peer_lst=0;
	r=0;
	for(; p; p=r){
		r=p->next;
		if (p->full_uri.s){
			shm_free(p->full_uri.s);
			p->full_uri.s=0; p->full_uri.len=0;
		}
		if (p->alias.s){
			shm_free(p->alias.s);
			p->alias.s=0; p->alias.len=0;
		}
		shm_free(p);
	}
}



void destroy_route_lst()
{
	struct route_entry* p;
	struct route_entry* r;
	
	struct peer_entry* pe;
	struct peer_entry* re;
	
	p=route_lst;
	route_lst=0;
	r=0;
	for(; p; p=r){
		r=p->next;
		for(pe=p->peer_l,re=0; pe; pe=re){
			re=pe->next;
			shm_free(pe);
		}
		p->peer_l=0;
		if (p->realm.s){
			shm_free(p->realm.s);
			p->realm.s=0; p->realm.len=0;
		}
		shm_free(p);
	}
}



