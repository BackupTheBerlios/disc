/*
 * $Id: hash_table.h,v 1.6 2003/04/04 16:59:25 bogdan Exp $
 *
 * 2003-01-29 created by bogdan
 * 2003-03-13 converted to locking.h/gen_lock_t (andrei)
 * 2003-03-17 all locks removed from hash_table and functions (bogdan)
 *
 */



#ifndef _AAA_DIAMETER_HASH_TABLE_H
#define _AAA_DIAMETER_HASH_TABLE_H


#include "dprint.h"
#include "list.h"
#include "locking.h"
#include "str.h"


/*
 * link element used by al kind of different structure that wants to be linked
 * into a hash entry; 
 */
struct h_link {
	struct list_head lh;
	unsigned int     label;
	unsigned int     hash_code;
};


/*
 * an entry of the hash table; each entry has a linked list of session that
 * share the same hash code.
 */
struct h_entry {
	/* for linked list */
	struct list_head  lh;
	/* each element from this entry will take a different label */
	unsigned int next_label;
};


/*
 * hash table = hash entrys + lists (session timeout)
 */
struct h_table {
	unsigned int hash_size;
	/* hash entrys */
	struct h_entry *entrys;
	/* mutex */
	gen_lock_t *mutex;
};



/* builds and inits the hash table */
struct h_table* build_htable( unsigned int hash_size);

/* free the hash table */
void destroy_htable( struct h_table* );

/* add a cell into the hash table on the proper hash entry */
int add_cell_to_htable(struct h_table* ,struct h_link* );

/* retursn the hash for a string */
int hash( str *s, unsigned int hash_size);


/* remove a cell from the hash table */
void remove_cell_from_htable(struct h_table *table , struct h_link *link);


/* search on an entry a cell having a given label  */
static inline struct h_link *cell_lookup(struct h_table *table,
								unsigned int hash_code, unsigned int label)
{
	struct h_entry   *entry;
	struct h_link    *link;
	struct list_head *lh;

	entry = &(table->entrys[hash_code]);
	link = 0;

	lock_get( table->mutex );
	/* looks into the the sessions hash table */
	list_for_each( lh, &(entry->lh) ) {
		if ( ((struct h_link*)lh)->label==label ) {
			link = (struct h_link*)lh;
			break;
		}
	}

	lock_release( table->mutex );
	return link;
}





#endif
