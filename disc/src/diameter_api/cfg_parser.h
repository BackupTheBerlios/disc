/*
 * $Id: cfg_parser.h,v 1.2 2003/03/14 18:29:25 bogdan Exp $
 */
/*
 * History:
 * --------
 *  2003-03-13  created by andrei
 */

#ifndef  cfg_parser_h
#define cfg_parser_h

#include <stdio.h>
#include "../str.h"

#define CFG_EMPTY   0
#define CFG_COMMENT 1
#define CFG_SKIP    2
#define CFG_DEF     3
#define CFG_ERROR  -1

#define MAX_LINE_SIZE 800

struct cfg_line{
	int type;
	char* id;
	char* value;
};


enum cfg_def_types {INT_VAL, STR_VAL};

struct cfg_def{
	char* name;
	enum cfg_def_types type;
	void* value;
};


extern struct cfg_def cfg_ids[]; /* null terminated array */



int cfg_parse_line(char* line, struct cfg_line* cl);
int cfg_parse_stream(FILE* stream);
int cfg_run_def(struct cfg_line* cl);

#endif
