%{
/* $Id: cmd_proc.y,v 1.1 2003/03/07 10:34:24 bogdan Exp $ */
/* $Name:  $ */
#include "cmd_proc.h"
#include "list.h"
#define YYERROR_VERBOSE 1 
#define YYDEBUG 1
extern hash_table_t* cmd_table;
extern int line_no;
ast_node_t* ast_temp_node;
param_list_t* param_temp_node;
index_list_t* index_temp_node;
%}

%union {
	int int_value;
	char* string;
	ast_node_t* ast_node;
	hash_table_entry_t* hash_table_entry;
	hash_table_t* hash_table;
	param_list_t* param_list;
	index_list_t* index_list;
	index_t* index;
}

%token STATEMENT_END LEFT_BRACE RIGHT_BRACE LEFT_PAREN RIGHT_PAREN 
	LEFT_BRACKET RIGHT_BRACKET IF ELSE WHILE NOT_OP PARAM_SEP COLON
	UNKNOWN_CHAR
%token <int_value> NUMBER
%token <string> IDS REALM_NAME STRING

%type <hash_table> command_block_list aaa_app_proc_list aaa_app_proc
%type <hash_table_entry> command_block aaa_app_proc_elem
%type <ast_node> statement_list statement if_statement while_statement 
		block_statement call_statement assign_statement
		index_expr expr
%type <param_list> parameter_list
%type <index_list> index_list
%type <index> atom_index

%left MEMBER_OP
%right ASSIGN_OP
%left MINUS_OP PLUS_OP
%left MULT_OP DIV_OP MOD_OP
%left OR_OP 
%left AND_OP
%left NOT_OP
%left NEG     /* Negation--unary minus */
%nonassoc EQ_OP GT_OP GT_EQ_OP LT_OP LT_EQ_OP

%start command_block_list

%%

command_block_list: command_block
	{
	insert_hash(cmd_table, $1);
	}
	| command_block_list command_block
	{
	insert_hash(cmd_table, $2);
	}
	;

command_block: REALM_NAME aaa_app_proc
	{ 
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	strncpy($$->key, $1, MAX_KEY_LEN);
	$$->value = $2;
	}
	| aaa_app_proc
	{
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	$$->key[0] = 0;
	$$->value = $1;
	}
	;

aaa_app_proc: aaa_app_proc_elem
	{
	$$ = (hash_table_t*)malloc(sizeof(hash_table_t));
	insert_hash($$, $1);
	}
	| LEFT_BRACKET aaa_app_proc_list RIGHT_BRACKET
	{
	$$ = $2;
	}
	;

aaa_app_proc_list: aaa_app_proc_elem 
	{
	$$ = (hash_table_t*)malloc(sizeof(hash_table_t));
	insert_hash($$, $1);
	}
	| aaa_app_proc_list aaa_app_proc_elem
	{
	insert_hash($1, $2);
	$$=$1;
	}
	;

aaa_app_proc_elem: IDS COLON GT_OP statement
	{
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	strncpy($$->key, $1, MAX_KEY_LEN-1);
	$$->key[strlen($$->key)] = '>';
	$$->value = $4;
	}
	|
	IDS COLON LT_OP statement
	{
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	strncpy($$->key, $1, MAX_KEY_LEN-1);
	$$->key[strlen($$->key)] = '<';
	$$->value = $4;
	}
	|
	IDS COLON statement
	{
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	strncpy($$->key, $1, MAX_KEY_LEN);
	$$->value = $3;
	}
	| statement
	{
	$$ = (hash_table_entry_t*)malloc(sizeof(hash_table_entry_t));
	$$->key[0] = 0;
	$$->value = $1;
	}
	;

statement_list: statement 
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t)); 	
	$$->st_list_n.current = $1;
	$$->type = ST_LIST_NODE;  
	INIT_LIST_HEAD(&($$->st_list_n.list_link));
	}
	|
	statement_list statement 
	{
	ast_temp_node = (ast_node_t*)malloc(sizeof(ast_node_t)); 	
	ast_temp_node->st_list_n.current = $2;
	ast_temp_node->type = ST_LIST_NODE;
	list_add(&(ast_temp_node->st_list_n.list_link), &($1->st_list_n.list_link));
	$$ = $1;
	}
	;

statement: if_statement
	| while_statement
	| call_statement
	| assign_statement
	| block_statement
	;

if_statement: IF LEFT_PAREN expr RIGHT_PAREN statement ELSE statement 
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = IF_NODE;
	$$->if_n.expr = $3;
	$$->if_n.if_branch = $5;
	$$->if_n.else_branch = $7;
	}
	| IF LEFT_PAREN expr RIGHT_PAREN statement
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = IF_NODE;
	$$->if_n.expr = $3;
	$$->if_n.if_branch = $5;
	$$->if_n.else_branch = NULL;
	}
	;

while_statement: WHILE LEFT_PAREN expr RIGHT_PAREN statement
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = WHILE_NODE;
	$$->while_n.expr = $3;
	$$->while_n.statement = $5;
	}
	;

call_statement:  IDS parameter_list STATEMENT_END
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = CALL_NODE;
	strncpy($$->call_n.module_name, $1, MAX_MODULE_NAME_LEN);
	$$->call_n.params = $2;
	}
	| IDS STATEMENT_END
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = CALL_NODE;
	strncpy($$->call_n.module_name, $1, MAX_MODULE_NAME_LEN);
	$$->call_n.params = NULL;
	}
	;

assign_statement: expr ASSIGN_OP expr STATEMENT_END
	{
	switch($1->type) {
		case STRUCT_NODE:
		case INDEX_NODE:
		case VAR_NODE:
			break;
		default:
			yyerror("bad lvalue in assignment");
	}
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = ASSIGN_NODE;
	$$->assign_n.lvalue = $1;
	$$->assign_n.expr = $3;
	}
	;

block_statement: LEFT_BRACE statement_list RIGHT_BRACE
	{
	$$ = $2;
	}
	;

/*
lhs_struct_expr: IDS 
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = VAR_NODE;
	strncpy(ast_temp_node->var_n.var_name, $1, MAX_VAR_NAME_LEN);
	}
	|
	index_expr
	{
	$$ = $1;
	}
	|
	struct_expr
	{
	$$ = $1;
	}
	;

rhs_struct_expr: IDS
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = VAR_NODE;
	strncpy(ast_temp_node->var_n.var_name, $1, MAX_VAR_NAME_LEN);
	}
	|
	index_expr
	{
	$$ = $1;
	}
	;

struct_expr: lhs_struct_expr MEMBER_OP rhs_struct_expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = STRUCT_NODE;
	ast_temp_node = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = VAR_NODE;
	strncpy(ast_temp_node->var_n.var_name, $1, MAX_VAR_NAME_LEN);
	$$->struct_n.structure = ast_temp_node;
	strncpy($$->struct_n.member, $3, MAX_VAR_NAME_LEN);
	}
	index_expr MEMBER_OP IDS
	|
	struct_expr MEMBER_OP IDS
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = STRUCT_NODE;
	strncpy($$->struct_n.member, $3, MAX_VAR_NAME_LEN);
	$$->struct_n.structure = $1;
	}
	|
	struct_expr MEMBER_OP index_expr
	{
	;
*/
atom_index: expr
	{
	$$ = (index_t*) malloc(sizeof(index_t));
	$$->low = $1;
	$$->high = NULL;
	}
	|
	expr COLON expr
	{
	$$ = (index_t*) malloc(sizeof(index_t));
	$$->low = $1;
	$$->high = $3;
	}
	;

index_list: atom_index
	{
	$$ = (index_list_t*)malloc(sizeof(index_list_t));
	$$->index = $1;
	INIT_LIST_HEAD(&($$->list_link));
	}
	|
	index_list PARAM_SEP atom_index
	{
	index_temp_node = (index_list_t*)malloc(sizeof(index_list_t));
	index_temp_node->index = $3;
	list_add(&(index_temp_node->list_link), &($1->list_link));
	$$ = $1;
	}
	;

index_expr: IDS LEFT_BRACKET index_list RIGHT_BRACKET
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = INDEX_NODE;
	ast_temp_node = (ast_node_t*)malloc(sizeof(ast_node_t));
	ast_temp_node->type = VAR_NODE;
	strncpy($$->var_n.var_name, $1, MAX_VAR_NAME_LEN);
	$$->index_n.vector = ast_temp_node;
	$$->index_n.index = $3;
	}
	| 
	index_expr LEFT_BRACKET index_list RIGHT_BRACKET
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t)); 
	$$->index_n.vector = $1;
	$$->index_n.index = $3;
	}
	;


expr: expr PLUS_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = PLUS;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr MINUS_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE; 
	$$->expr_n.op = MINUS;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	MINUS_OP expr %prec NEG
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = MINUS;
	$$->expr_n.left_op = NULL;
	$$->expr_n.right_op = $2;
	}
	|
	expr MULT_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = MULT;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr DIV_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE; 
	$$->expr_n.op = DIV;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr MOD_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type =  EXPR_NODE;
	$$->expr_n.op = MOD;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr EQ_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = EQ;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr GT_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = GT;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr GT_EQ_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = GT_EQ;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr LT_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = LT;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr LT_EQ_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = LT_EQ;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	NOT_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = NOT;
	$$->expr_n.left_op = NULL;
	$$->expr_n.right_op = $2;
	}
	|
	expr AND_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = AND;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr OR_OP expr
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = OR;
	$$->expr_n.left_op = $1;
	$$->expr_n.right_op = $3;
	}
	|
	expr MEMBER_OP expr
	{
	switch($1->type) {
		case VAR_NODE:
		case INDEX_NODE:
		case STRUCT_NODE:
			break;
		default:
			yyerror("wrong lhs of struct expr");
	}
	switch($3->type) {
		case VAR_NODE:
		case INDEX_NODE:
			break;
		default:
			yyerror("wrong rhs of struct expr");
	}
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = STRUCT_NODE;
	$$->struct_n.structure = $1;
	$$->struct_n.member = $3;
	}
	|
	index_expr
	{
	$$ = $1;
	}
	|
	LEFT_PAREN expr RIGHT_PAREN
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = EXPR_NODE;
	$$->expr_n.op = NOP;
	$$->expr_n.left_op = $2;
	$$->expr_n.right_op = NULL;
	}
	|
	NUMBER
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = VALUE_NODE;
	$$->value_n.value = $1;
	}
	|
	STRING
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = STRING_NODE;
	strncpy($$->str_n.str, $1, MAX_VAR_NAME_LEN);
	}
	|
	IDS
	{
	$$ = (ast_node_t*)malloc(sizeof(ast_node_t));
	$$->type = VAR_NODE;
	strncpy($$->var_n.var_name, $1, MAX_VAR_NAME_LEN);
	}
	;
	
	
parameter_list: expr
	{
	$$ = (param_list_t*)malloc(sizeof(param_list_t));
	$$->param = $1;
	INIT_LIST_HEAD(&($$->list_link));
	}
	| parameter_list PARAM_SEP expr 
	{
	param_temp_node = (param_list_t*)malloc(sizeof(param_list_t));
	param_temp_node->param = $3;
	list_add(&(param_temp_node->list_link), &($1->list_link));
	$$ = $1;
	}
	;
%%
#include "lex.yy.c"
