#if 1

#include <stdio.h>

#include "hash.h"
#include "list.h"

#include <mcheck.h>

struct dummy {
		int i;
		char c;
		char str[20];
};

int hash_fn_int(int *i) {
		return *i%4;
};

int hash_key_cmp_int(struct dummy *d, int *i) {
		printf ("\t compare %d with %d\n", d->i, *i);
		return ! (d->i == *i);
};

int hash_fn_chr(char *c) {
		return *c%2;
};

int hash_key_cmp_chr(struct dummy *d, char *c) {
		printf ("\t compare '%c' with '%c'\n", d->c, *c);
		return !(d->c == *c);
};

void print_dummy(void *d) {
	printf("i: %d\n", ((struct dummy *)d)->i);
	printf("c: %c\n", ((struct dummy *)d)->c);
	printf("str: %s\n", ((struct dummy *)d)->str);
};

/* 
 * HASH_TABLE(NAME, SIZE, ENTRY_TYPE, KEY_MEMBER, HASH_FN, HASH_KEY_CMP)
 * NAME: a prefix to hash_table_XXX, lookup_XXX and lookup_group_XXX that
 *       identifies the hash table;
 * SIZE: number of buckets;
 * ENTRY_TYPE: type of the data attached to a hash entry;
 * KEY_MEMBER: type of key field in the data attached to a hash entry;
 * HASH_FN: function that calculates the hash code given the key;
 * HAS_KEY_CMP: function that compares a hash entry with a key;
 *
 * Copyright (C) 2002-2003 Fhg Fokus
 *
 * This file is part of disc, a free diameter server/client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


HASH_TABLE(INT, 4, struct dummy, i, hash_fn_int, hash_key_cmp_int);	
HASH_TABLE(CHR, 2, struct dummy, c, hash_fn_chr, hash_key_cmp_chr);
struct hash_group	*h;


		
int main(void) {

	struct hash_entry	*e;
	int					key = 5;
	char				key2 = 'd';
	
	struct dummy d1, d2, d3, d4, d5;


	mtrace();
	
	d1.i = 1; d1.c = 'a'; strcpy(d1.str, "d1");
	d2.i = 2; d2.c = 'b'; strcpy(d2.str, "d2");
	d3.i = 3; d3.c = 'c'; strcpy(d3.str, "d3");
	d4.i = 4; d4.c = 'd'; strcpy(d4.str, "d4");
	d5.i = 5; d5.c = 'e'; strcpy(d5.str, "d5");
	
	/* initialization */
	init_hash_table_INT();
	init_hash_table_CHR();
	init_hash_group(&h, 2, &hash_table_INT, &hash_table_CHR);

	/* adding entries */
	add_entry_hash_group(h, &d1);
	add_entry_hash_group(h, &d2);
	add_entry_hash_group(h, &d3);
	add_entry_hash_group(h, &d4);
	add_entry_hash_group(h, &d5);


	print_hash_group(h, print_dummy);


	/* lookup */
	e = lookup_hash_group_INT(h, &key);

	printf("\n+++\n");
	print_dummy(e->data);
	printf("\n---\n");

	/* delete entry */
	del_entry_hash_group(h, e);

	print_hash_group(h, print_dummy);
	
	/* lookup */
	e = lookup_hash_group_CHR(h, &key2);

	printf("\n§§§\n");
	print_dummy(e->data);
	printf("\n$$$\n");

	/* delete entry */
	del_entry_hash_group(h, e);
	
	print_hash_group(h, print_dummy);

	/* clean up the whole crap */
	cleanup_hash_group(h, NULL);

	muntrace();
	
	return 0;
}

#endif
