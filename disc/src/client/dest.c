/*$
 * $Id: dest.c,v 1.4 2003/04/08 22:30:18 bogdan Exp $$
 *$
 * 2003-04-01 created by bogdan$
 */



#include "../mem/shm_mem.h"
#include "../dprint.h"
#include "../transport/peer.h"
#include "../diameter_msg/diameter_msg.h"


static struct peer_chain *all_peers;


int init_dest_peers()
{
	struct list_head   *lh;
	struct peer_chain *pc, *last;

	all_peers = 0;
	last = 0;

	list_for_each( lh, &(peer_table->peers) ) {
		pc = (struct peer_chain*)shm_malloc(sizeof(struct peer_chain));
		if (!pc) {
			LOG(L_ERR,"ERROR:init_dest_peers: no more free memory!\n");
			goto error;
		}
		/* add to list */
		pc->p = list_entry(lh, struct peer, all_peer_lh);
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



void destroy_dest_peers()
{
	struct peer_chain *pc, *pc_foo;

	pc = all_peers;
	while(pc) {
		pc_foo = pc;
		pc = pc->next;
		shm_free( pc_foo);
	}
}



int get_dest_peers( AAAMessage *msg, struct peer_chain **chaine )
{
	if (chaine)
		*chaine = all_peers;
	return 1;
}




