/*
 * $Id: route.c,v 1.4 2003/04/08 22:06:24 andrei Exp $
 */
/*
 * History:
 * --------
 *  2003-04-08  created by andrei
 */


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
	return 0;
error_mem:
	LOG(L_ERR, "ERROR: add_route: memory allocation error\n");
	return -1;
error_no_peer:
	LOG(L_ERR, "ERROR: add_route: no peer <%.*s> found\n", dst->len, dst->s);
	return -1;
}



/* returns the first match peer list or 0 on error */
/* WARNING: all this must be null terminated */
struct peer_entry* route_dest(str* realm)
{
	struct route_entry* re;
	

	for (re=route_lst; re; re=re->next){
		/*FNM_CASEFOLD | FNM_EXTMATCH - GNU extensions*/ 
		if (fnmatch(re->realm.s, realm->s, 0)==0){
			/*match */
			DBG("route_dest: match on <%s> (<%s>)\n", re->realm.s, realm->s);
			return re->peer_l;
		}
	}
	DBG("WARNING: route_dest: no route found for <%s>\n", realm->s);
	return 0;
}


