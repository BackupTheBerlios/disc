/*
 * $Id: hash_table.h,v 1.3 2003/03/12 18:12:22 andrei Exp $
 *
 * 2003-01-29 created by bogdan
 *
 */



#ifndef _AAA_DIAMETER_HASH_TABLE_H
#define _AAA_DIAMETER_HASH_TABLE_H


#include "utils/aaa_lock.h"
#include "dprint.h"
#include "utils/str.h"


/*
 * number of entrys for the hash table
 */
#define HASH_SIZE  1024


/*
 * possible types of structures inserted in the hash_table
 */
enum HASH_CELL_TYPES {
	DEFAULT_CELL_TYPE = 0,
	SESSION_CELL_TYPE ,
	TRANSACTION_CELL_TYPE,
	MAX_HASH_CELL_TYPES
};


/*
 * prototype for destroy function for specific type of cell
 */
typedef void (destroy_cell_func_t)(void *);


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
	aaa_lock *mutex;
	/* each session from this entry will take a different label */
	unsigned int next_label;
};


/*
 * hash table = hash entrys + lists (session timeout)
 */
struct h_table {
	/* hash entrys */
	struct h_entry* entrys;
	/* destroy functions */
	destroy_cell_func_t* destroy_cell_func[MAX_HASH_CELL_TYPES];
	/* lists */
};

extern struct h_table *hash_table;


/* builds and inits the hash table */
struct h_table* build_htable();

/* register a destroy function for a specific cell type */
int register_destroy_func( struct h_table* , enum HASH_CELL_TYPES,
											destroy_cell_func_t *func );

/* free the hash table */
void destroy_htable( struct h_table* );

/* add a cell into the hash table on the proper hash entry */
int add_cell_to_htable(struct h_table* ,struct h_link* );

/* retursn the hash for a string */
int hash( str* );

/* remove a cell from the hash table; the session is NOT freed */
void remove_cell_from_htable_unsafe(struct h_entry* ,struct h_link* );


/* remove a cell from the hash table; the session is NOT freed */
static inline void remove_cell_from_htable(struct h_table *table ,
													struct h_link *link)
{
	struct h_entry *entry;

	entry = &(table->entrys[link->hash_code]);
	/* get the mutex for the entry */
	lock( entry->mutex );
	/* remove the entry */
	remove_cell_from_htable_unsafe( entry , link );
	/* release the mutex */
	unlock( entry->mutex );
}


/* search on an entry a cell having a given label  */
static inline struct h_link *cell_lookup(struct h_table *table,
	unsigned int hash_code, unsigned int label, unsigned int type, int rm)
{
	struct h_link *link;
	/* lock the hash entry */
	lock( table->entrys[hash_code].mutex );
	/* looks into the the sessions hash table */
	link = table->entrys[hash_code].head;
	while ( link && (link->label!=label || link->type!=type) )
		link = link->next;
	/* do I have to remove the cell from table? */
	if (rm && link)
		remove_cell_from_htable_unsafe( &(table->entrys[hash_code]), link);
	/* unlock the entry */
	unlock( table->entrys[hash_code].mutex );
	return link;
}





#endif
