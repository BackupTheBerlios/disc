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
						 * symbol table */
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
