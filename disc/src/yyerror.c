/*$Id: yyerror.c,v 1.1 2003/03/07 10:34:24 bogdan Exp $*/
/*$Name:  $*/ 
#include <stdio.h>

extern int line_no;
extern int parse_error_no;
void
yyerror(
	char *s
	)
{
	fprintf(stderr, "PARSE ERROR: %s at line: %d\n", s, line_no);
	parse_error_no++;
}	
