#include "cmd_proc.h"
#include "interpret.h"

hash_table_t* cmd_table;
int line_no;
int parse_error_no;

extern int yydebug;

void insert_hash(
	hash_table_t* table,
	hash_table_entry_t* entry
)
{}


void 
interpret(
	ast_node_ptr ast,
	symbol_table_t* symtbl,
	int *error
	)
{
	int res;
	if( NULL == ast)
		goto error;
	switch(ast->type) {
		
		case IF_NODE:
			err = ERR_IF;
			res = eval_expr(ast->if_n.expr, 
					tmp_val, 
					symtbl);
			GET_OUT(res < 0, exit_err);
			else if( res == 0 && NULL != ast->if_n.else_branch)
				res = interpret(ast->if_n.else_branch,
						symtbl,
						&err);
			else if( res > 0 )
				res = interpret(ast->if_n.if_branch, 
						symtbl,
						&err);
			GET_OUT(res < 0, exit_err);
			break;
		
		case WHILE_NODE:
			err = ERR_WHILE;
			res = eval_expr(ast->while_n.expr, 
					tmp_val, 
					symtbl);
			GET_OUT(res < 0, exit_err);
			while(res > 0) {
				res = interpret(ast->while_n.statement,
						symtbl,
						&err);
				GET_OUT(res < 0, exit_err);
				res = eval_expr(ast->while_n.expr, 
						tmp_val, 
						symtbl);
				GET_OUT(res < 0, exit_err);
			}
			break;
		
		case CALL_NODE:
			err = ERR_CALL;
			break;

		case ASSIGN_NODE:
			err = ERR_ASSIGN;
			res = eval_expr(ast->assign_n.expr,
					tmp_val,
					symtbl);
			GET_OUT(res < 0, exit_err);
			hentry = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
			switch(ast->assign_n.lvalue.type) {
				case VAR_NODE:
					res = get_var(symtbl, sname);
					GET_OUT(res < 0, exit_err);
					if( res == 0 ) {
						/* found the var in
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

						hentry->key = var_name;
						hentry->value = tmp_val;
					break;
				case STRUCT_NODE:
					res = find_struct_name(ast, &sname);
					res = get_var(symtbl, sname);
					GET_OUT(res < 0, exit_err);
					offset =  get_offset(ast, sname);


					
			}
			strncpy(hentry->key, 		
			res = insert_hash(symtbl, 

void main(void)
{
	yydebug =1;	
	yyparse();
}	
