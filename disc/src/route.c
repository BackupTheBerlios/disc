/*
 * $Id: route.c,v 1.2 2003/03/28 20:27:24 bogdan Exp $
 *
 * 2003-03-27 created by bogdan
 */


#include "mem/shm_mem.h"
#include "dprint.h"
#include "route.h"
#include "script.h"
#include "globals.h"



int do_routing( AAAMessage *msg, struct peer_chaine **pc )
{
	struct peer_chaine *p_chaine;
	struct peer        *p;
	int                 res;

	/* is Dest-Host present? */
	if (msg->dest_host) {
		/* dest-host avp present */
		if (msg->dest_host->data.len==aaa_identity.len &&
		strncmp(msg->dest_host->data.s, aaa_identity.s, aaa_identity.len) ) {
			/* I'm the destination host */
			if (pc) *pc = 0;
			return 2;
		} else {
			/* I'm not the destination host -> am I peer with the dest-host? */
			p = lookup_peer_by_identity( &(msg->dest_host->data) );
			if (p) {
				/* the destination host is one of my peers */
				p_chaine = (struct peer_chaine *)shm_malloc
					( sizeof(struct peer_chaine) );
				if ( !p_chaine ) {
					LOG(L_ERR,"ERROR:do_routing: no more free memory!\n");
					return -1;
				}
				p_chaine->peer = p;
				p_chaine->next = 0;
				if (pc) *pc = p_chaine;
				return 1;
			} else {
				if (!msg->dest_realm) {
					/* dest-host is not me and no dest-realm -> bogus msg */
					if (pc) *pc = 0;
					return 0;
				}
			}
		}
	} else {
		if (!msg->dest_realm) {
			/* no dest-host and no dest-realm -> it's local */
			if (pc) *pc = 0;
			return 2;
		}
	}

	/* do routing based on destination-realm AVP */
	p_chaine = 0;
	res = run_script( &p_chaine );
	switch (res) {
		case -1:
			/* error */
			return -1;
			break;
		case 0:
			/* bogus message */
			return 0;
			break;
		case 1:
			/* forward to a chaine of peers */
			if (pc) *pc = p_chaine;
			return 1;
			break;
		case 2:
			/* local */
			return 1;
			break;
	}

	return 1;
}

