/*
 * $Id: cfg_parser.h,v 1.3 2003/04/08 18:59:52 andrei Exp $
 */
/*
 * History:
 * --------
 *  2003-03-13  created by andrei
 *  2003-04-07  added cfg_callback (andrei)
 */

#ifndef  cfg_parser_h
#define cfg_parser_h

#include <stdio.h>
#include "str.h"

#define CFG_EMPTY   0
#define CFG_COMMENT 1
#define CFG_SKIP    2
#define CFG_DEF     3
#define CFG_ERROR  -1

#define MAX_LINE_SIZE 800

#define CFG_TOKENS 10 /* max numbers of tokens on a line */

enum cfg_errors { CFG_OK=0, CFG_ERR=-1, CFG_PARAM_ERR=-2, CFG_RUN_ERR=-3,
                  CFG_MEM_ERR };

struct cfg_line{
	int type;
	char* id;
	char* value[CFG_TOKENS];
	int token_no; /* number of value token parsed */
};


enum cfg_def_types {INT_VAL, STR_VAL, GEN_VAL };
typedef    int (*cfg_callback)(struct cfg_line*, void* value) ;

struct cfg_def{
	char* name;
	enum cfg_def_types type;
	void* value;
	cfg_callback c;
};


extern struct cfg_def cfg_ids[]; /* null terminated array */



int cfg_parse_line(char* line, struct cfg_line* cl);
int cfg_parse_stream(FILE* stream);
int cfg_run_def(struct cfg_line* cl);

int cfg_getstr(char* p, str* r);
int cfg_getint(char* p, int* i);

#endif
