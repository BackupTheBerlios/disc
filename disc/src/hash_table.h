/*
 * $Id: hash_table.h,v 1.2 2003/03/17 13:09:33 bogdan Exp $
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
#include "str.h"


/*
 * link element used by al kind of different structure that wants to be linked
 * into a hash entry; 
 * IMPORTANT: this link structure MUST be INCLUDED(not refere) at the BEGININNG
 * of the structrure that uses the hash_table
 */
struct h_link {
	struct list_head lh;
	unsigned int     label;
	unsigned int      hash_code;
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
	struct h_entry* entrys;
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
						unsigned int hash_code, unsigned int label, int rm)
{
	struct h_link *link=0;
#if 0
	/* lock the hash entry */
	lock_get( table->entrys[hash_code].mutex );
	/* looks into the the sessions hash table */
	link = table->entrys[hash_code].head;
	while ( link && link->label!=label  )
		link = link->next;
	/* do I have to remove the cell from table? */
	if (rm && link)
		remove_cell_from_htable_unsafe( &(table->entrys[hash_code]), link);
	/* unlock the entry */
	lock_release( table->entrys[hash_code].mutex );
#endif
	return link;
}





#endif
