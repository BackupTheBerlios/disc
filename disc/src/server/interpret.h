#ifndef ret_h
#define interpret_h

enum {
	ERR_IF,
	ERR_WHILE,
	ERR_CALL,
	ERR_ASSIGN
} err_t;	 

char *error_message[] = {
	"error in if statement",
	"error in while statement",
	"error in module call",
	"error in assignment",
	"error in block statement"
};

#define GET_OUT(decision, label) \
		if((decision)) \
			goto label;

typedef enum var_type_ {
	INT,
	STRING,
	ARRAY,
	LIST,
	AAA,
	AVP
} var_type_t;

typedef struct var_ {
	var_type_t type;
	void *value
} var_t;

typedef int_t int;
typedef string_t char*;
typedef aaa_t AAAMessage;
typedef avp_t AAA_AVP; 
typedef list_t AAA_AVP_LIST;

int
get_var(
	symtbl_t* symtbl,
	char *
