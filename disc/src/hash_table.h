/*
 * $Id: hash_table.h,v 1.1 2003/03/14 17:15:38 bogdan Exp $
 *
 * 2003-01-29 created by bogdan
 * 2003-03-13 converted to locking.h/gen_lock_t (andrei)
 *
 */



#ifndef _AAA_DIAMETER_HASH_TABLE_H
#define _AAA_DIAMETER_HASH_TABLE_H


#include "locking.h"
#include "dprint.h"
#include "str.h"
#include "aaa_lock.h"


/*
 * number of entrys for the hash table
 */
#define HASH_SIZE  1024


/*
 * link element used by al kind of different structure that wants to be linked
 * into a hash entry; 
 * IMPORTANT: this link structure MUST be INCLUDED(not refere) at the BEGININNG
 * of the structrure that uses the hash_table
 */
struct h_link {
	unsigned int  type;
	struct h_link *next;
	struct h_link *prev;
	unsigned int  label;
	unsigned int  hash_code;
};


/*
 * an entry of the hash table; each entry has a linked list of session that
 * share the same hash code.
 */
struct h_entry {
	/* anchors to the linked list */
	struct h_link *head;
	struct h_link *tail;
	/* mutex for manipulating the linked list in critical region */
	gen_lock_t *mutex;
	/* each session from this entry will take a different label */
	unsigned int next_label;
};


/*
 * hash table = hash entrys + lists (session timeout)
 */
struct h_table {
	/* hash entrys */
	struct h_entry* entrys;
};



/* builds and inits the hash table */
struct h_table* build_htable();

/* free the hash table */
void destroy_htable( struct h_table* );

/* add a cell into the hash table on the proper hash entry */
int add_cell_to_htable(struct h_table* ,struct h_link* );

/* retursn the hash for a string */
int hash( str* );

/* remove a cell from the hash table */
void remove_cell_from_htable_unsafe(struct h_entry* ,struct h_link* );


/* remove a cell from the hash table; the session is NOT freed */
static inline void remove_cell_from_htable(struct h_table *table ,
													struct h_link *link)
{
	struct h_entry *entry;

	entry = &(table->entrys[link->hash_code]);
	/* get the mutex for the entry */
	lock_get( entry->mutex );
	/* remove the entry */
	remove_cell_from_htable_unsafe( entry , link );
	/* release the mutex */
	lock_release( entry->mutex );
}


/* search on an entry a cell having a given label  */
static inline struct h_link *cell_lookup(struct h_table *table,
						unsigned int hash_code, unsigned int label, int rm)
{
	struct h_link *link;
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
	return link;
}





#endif
