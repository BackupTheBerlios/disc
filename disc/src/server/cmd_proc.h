/* $Id: cmd_proc.h,v 1.2 2003/04/22 19:58:41 andrei Exp $ */
/*
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

/* $Name:  $ */
#ifndef cmd_proc_h
#define cmd_proc_h

#include "list.h"

#define MAX_MODULE_NAME_LEN 256
#define MAX_VAR_NAME_LEN 128
#define MAX_KEY_LEN 256
#define MAX_HASH_TABLE_ENTRIES 128
#define STR_ALLOC_BLOCK 1

/*
typedef enum {
	LEFT_BRACE,
	RIGHT_BRACE,
	LEFT_PAREN,
	RIGHT_PAREN,
	MOD_OP,
	OR_OP,
	AND_OP,
	NOT_OP,
	REALM_NAME,
	IDS,
	MODULE_NAME,
	ASSIGN_OP,
	GT_OP,
	GT_EQ_OP,
	LT_OP,
	LT_EQ_OP,
	EQ_OP,
	PLUS_OP,
	MINUS_OP,
	MULT_OP,
	DIV_OP,
	AAA_ID,
	IF,
	ELSE,
	WHILE,
	NUMBER
} token_t;
*/

typedef unsigned int(*hash_fun_t)(char*);

typedef enum {
        IF_NODE,
        WHILE_NODE,
        EXPR_NODE,
        ST_LIST_NODE,
        CALL_NODE,
        ASSIGN_NODE,
	STRUCT_NODE,
	INDEX_NODE,
        VALUE_NODE,
        VAR_NODE,
	STRING_NODE
} node_type_t;

typedef enum operator_ {
	PLUS,
	MINUS,
	MULT,
	DIV,
	MOD,
	EQ,
	GT,
	GT_EQ,
	LT,
	LT_EQ,
	NOT,
	AND,
	OR,
	NOP,
	MEMBER
} operator_t;		
       
typedef struct ast_node_ *ast_node_ptr;
typedef struct param_list_ *param_list_ptr;
typedef struct index_ *index_ptr;

typedef struct index_ {
	ast_node_ptr low;
	ast_node_ptr high;
} index_t;

typedef struct index_list_ {
	index_ptr index;
	struct list_head list_link;
} index_list_t;

typedef struct param_list_ {
        ast_node_ptr param;
        struct list_head list_link;
} param_list_t;

typedef struct if_node_ {
	ast_node_ptr expr;
        ast_node_ptr if_branch;
        ast_node_ptr else_branch;
}if_node_t;
        
typedef struct while_node_ {
	ast_node_ptr expr;
	ast_node_ptr statement;
}while_node_t;

typedef struct struct_node_ {
	ast_node_ptr structure;	
	ast_node_ptr member;
}struct_node_t;	

typedef struct index_node_ {
	ast_node_ptr vector;
	index_list_t* index;
}index_node_t;	

typedef struct expr_node_ {
	operator_t op;
	ast_node_ptr left_op;
	ast_node_ptr right_op;
}expr_node_t;
        
typedef struct st_list_node_ {
	struct list_head list_link;
	ast_node_ptr current;
}st_list_node_t;

typedef struct call_node_ {
	char module_name[MAX_MODULE_NAME_LEN];
	param_list_ptr params;
}call_node_t;

typedef struct assign_node_ {
	ast_node_ptr lvalue;
	ast_node_ptr expr;
}assign_node_t;

typedef struct value_node_ {
	int value;
}value_node_t;

typedef struct var_node_ {
	char var_name[MAX_VAR_NAME_LEN];
}var_node_t;

typedef struct str_node_ {
	char str[MAX_VAR_NAME_LEN];
}str_node_t;

typedef union node_ {
	if_node_t if_node;
	while_node_t while_node;
	expr_node_t expr_node;
	st_list_node_t st_list_node;
	call_node_t call_node;
	assign_node_t assign_node;
	struct_node_t struct_node;
	index_node_t index_node;
	value_node_t value_node;
	var_node_t var_node;
	str_node_t str_node;
} node_t;

typedef struct ast_node_ {
	node_type_t type;
	node_t node;
#define if_n node.if_node
#define while_n node.while_node
#define expr_n node.expr_node
#define st_list_n node.st_list_node
#define call_n node.call_node
#define assign_n node.assign_node
#define struct_n node.struct_node
#define index_n node.index_node
#define value_n node.value_node
#define str_n node.str_node
#define var_n node.var_node
} ast_node_t;

typedef struct hash_table_entry_ {
	struct list_head list_link;
	char *key;
	void* value;
} hash_table_entry_t;

typedef struct hash_table_ {
	hash_table_entry_t dft;
	hash_table_entry_t tbl[MAX_HASH_TABLE_ENTRIES];
	hash_fun_t fun;
} hash_table_t;

void insert_hash(
		hash_table_t*,
		hash_table_entry_t*
);		

#endif
