/*
 * $Id: script.c,v 1.1 2003/03/28 14:20:43 bogdan Exp $
 *
 * 2003-03-27 created by bogdan
 */


#include "mem/shm_mem.h"
#include "dprint.h"
#include "list.h"
#include "route.h"
#include "script.h"
//#include "globals.h"


static struct peer_chaine *all_peers;


int init_script()
{
	struct list_head   *lh;
	struct peer_chaine *pc, *last;

	all_peers = 0;
	last = 0;

	list_for_each( lh, &(peer_table->peers) ) {
		pc = (struct peer_chaine*)shm_malloc(sizeof(struct peer_chaine));
		if (!pc) {
			LOG(L_ERR,"ERROR:init_script:no more free memory!\n");
			goto error;
		}
		/* add to list */
		pc->peer = list_entry(lh, struct peer, all_peer_lh);
		pc->next = 0;
		if (!last) {
			all_peers = pc;
		} else {
			last->next = pc;
		}
		last = pc;
	}

	return 1;
error:
	return -1;
}



int run_script( struct peer_chaine **chaine )
{
	if (chaine)
		*chaine = all_peers;
	return 1;
}




