/*
 * $Id: hash_table.c,v 1.2 2003/03/12 18:12:22 andrei Exp $
 *
 * 2003-01-29  created by bogdan
 * 2003-03-12  converted to use shm_malloc (andrei)
 *
 */

#include <stdlib.h>
#include <string.h>
#include "dprint.h"
#include "utils/str.h"
#include "hash_table.h"

#include "mem/shm_mem.h"



/* default function used for freeing unkown hash cell type */
static void default_cell_destroyer( void *link ) { shm_free(link); }



/*
 * Builds and inits the hash table; Size of table is given by HASH_SIZE define.
 */
struct h_table* build_htable( )
{
	struct h_table *table;
	aaa_lock       *entry_locks;
	int            i;

	/* inits */
	table = 0;

	/* allocates the hash table */
	table = (struct h_table*)shm_malloc( sizeof(struct h_table));
	if (!table) {
		LOG(L_ERR,"ERROR:build_htable: cannot get free memory!\n");
		goto error;
	}
	memset( table, 0, sizeof(struct h_table));

	/* allocates an array of hash entrys */
	table->entrys = (struct h_entry*)shm_malloc(HASH_SIZE*
													sizeof(struct h_entry));
	if (!table->entrys) {
		LOG(L_ERR,"ERROR:build_htable: cannot get free memory!\n");
		goto error;
	}
	memset( table->entrys, 0, HASH_SIZE*sizeof(struct h_entry));

	/* put a mutex for each entry */
	if ( (entry_locks=create_locks( HASH_SIZE ))==0) {
		LOG(L_ERR,"ERROR:build_htable: cannot create semaphores!!\n");
		goto error;
	}
	for(i=0;i<HASH_SIZE;i++) {
		table->entrys[i].mutex = &(entry_locks[i]);
		/* also init the next_label counter */
		table->entrys[i].next_label  = ((unsigned int)time(0))<<20;
		table->entrys[i].next_label |= ((unsigned int)rand( ))>>12;
	}

	/* set a default detroy cell function */
	table->destroy_cell_func[ DEFAULT_CELL_TYPE ] = default_cell_destroyer;

	LOG(L_INFO,"INFO:build_htable: hash table succesfuly built\n");
	return table;
error:
	LOG(L_INFO,"INFO:build_htable: FAILED to build hash table\n");
	return 0;
}



/*
 * Destroy the hash table: mutex are released and memory is freed
 */
void destroy_htable( struct h_table *table)
{
	int i;
	struct h_link  *link1, *link2;

	if (table) {
		/* deal with the entryes */
		if (table->entrys) {
			/* first, remove and destroy all records from table */
			for(i=0;i<HASH_SIZE;i++) {
				link1 = table->entrys[i].head;
				while(link1) {
					link2 = link1;
					link1 = link1->next;
					table->destroy_cell_func[link2->type]( (void*)link2 );
				}
			}
			/* than, destroy the mutexes.... */
			if (table->entrys[0].mutex)
				destroy_locks( table->entrys[0].mutex , HASH_SIZE );
			/* ... and free the entry's array */
			shm_free( table->entrys  );
		}
		/* at last, free the table structure */
		shm_free( table);
	}

	LOG(L_INFO,"INFO:destroy_htable: table succesfuly destroyed\n");
}



/* register a destroy function for a specific cell type */
int register_destroy_func( struct h_table *table, enum HASH_CELL_TYPES type,
													destroy_cell_func_t* func)
{
	if (type<1 || type>=MAX_HASH_CELL_TYPES) {
		LOG(L_ERR,"ERROR:register_destroy_func: invalid hash_cell_type "
			"received [%d]; domain is [1..%d]\n",type,MAX_HASH_CELL_TYPES-1);
		return 0;
	}
	table->destroy_cell_func[type] = func;
	return 1;
}



/*
 * Adds a cell into the hash table on the apropriate hash entry
 * ( it uses h_link->hash_code); also the label (unique identifier into the
 * entry link list) is set.
 * The entire operation is protected by mutex
 */
int add_cell_to_htable( struct h_table *table, struct h_link *link)
{
	struct h_entry *entry;

	if (link->hash_code<0 || link->hash_code>HASH_SIZE-1)
		return -1;

	entry = &(table->entrys[link->hash_code]);
	/* get the mutex for the entry */
	lock( entry->mutex );

	/* get a label for this session */
	link->label = entry->next_label++;
	/* insert the session into the linked list at the end */
	if (!entry->tail)
		entry->head = link;
	else
		entry->tail->next = link;
	link->prev = entry->tail;
	entry->tail = link;

	/* release the mutex */
	unlock( entry->mutex );
	return 1;
}



/*
 * Removes a cell from the hash_table; the session struct is not freed;
 * The entire operation is protected by mutex
 */
void remove_cell_from_htable_unsafe(struct h_entry *entry,struct h_link *link)
{
	/* remove the session from the linked list */
	if ( link->prev )
		link->prev->next = link->next;
	else
		entry->head = link->next;
	if ( link->next )
		link->next->prev = link->prev;
	else
		entry->tail = link->prev;
}



/*
 * Based on a string, calculates a hash code.
 */
int hash( str *s )
{
#define h_inc h+=v^(v>>3)
	char* p;
	register unsigned v;
	register unsigned h;

	h=0;
	for (p=s->s; p<=(s->s+s->len-4); p+=4){
		v=(*p<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
		h_inc;
	}
	v=0;
	for (;p<(s->s+s->len); p++){ v<<=8; v+=*p;}
	h_inc;

	h=((h)+(h>>11))+((h>>13)+(h>>23));
	return (h)&(HASH_SIZE-1);
}



