%{
/* $Id: cmd_proc.lex,v 1.2 2003/04/22 19:58:41 andrei Exp $ */
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

#include "cmd_proc.h"

#define REALLOC_STRING \
	if(str_len > buf_len) { \
                buf_len*=2; \
                if(NULL == realloc(yylval.string, buf_len)) { \
                        free(yylval.string); \
                        return UNKNOWN_CHAR; \
                } \
        }
static char* string_buf_ptr;
static int str_len;
static buf_len;
%}

LEFT_BRACE "{"
RIGHT_BRACE "}"
LEFT_PAREN "(" 
RIGHT_PAREN ")" 
LEFT_BRACKET "["
RIGHT_BRACKET "]"
STATEMENT_END ";"
PARAM_SEP ","
MOD_OP "%"
DOT \x2e 
DASH \x2d
UNDERSCORE "_"
NOT_OP "!"
AND_OP "&"
OR_OP "|"
ASSIGN_OP "="
GT_OP ">"
GT_EQ_OP ">="
LT_OP "<"
LT_EQ_OP "<="
EQ_OP "=="
PLUS_OP "+"
MINUS_OP "-"
MULT_OP "*"
DIV_OP "/"
MEMBER_OP "->"
COLON ":"


DIGIT [0-9]
NUMBER {DIGIT}+
CHAR [a-z]|[A-Z]
ALNUM {CHAR}|{DIGIT}
DASHED_ALNUM {CHAR}|{DIGIT}|{DASH}
UNDERSCORE_ALNUM {CHAR}|{DIGIT}|{UNDERSCORE}
REALM_ATOM {ALNUM}{DASHED_ALNUM}*
REALM {REALM_ATOM}({DOT}{REALM_ATOM})+
IDS {UNDERSCORE}*{ALNUM}{UNDERSCORE_ALNUM}*

AAA_HEADER_VER "aaa.ver" 
AAA_HEADER_LEN "aaa.len" 
AAA_HEADER_REQ "aaa.req" 
AAA_HEADER_PRX "aaa.prx" 
AAA_HEADER_ERR "aaa.err" 
AAA_HEADER_RTR "aaa.rtr" 
AAA_HEADER_CMD "aaa.cmd" 
AAA_HEADER_APP "aaa.app" 
AAA_HEADER_HOP "aaa.hop" 
AAA_HEADER_END "aaa.end" 
AVP_HEADER_CODE "avp.code"
AVP_HEADER_VEND "avp.vend"
AVP_HEADER_MAND "avp.mand"
AVP_HEADER_ENCR "avp.encr"
AVP_HEADER_LEN "avp.len"
AVP_HEADER_VID "avp.vid"

IF "if"
ELSE "else"
WHILE "while"

%x comment
%x str
%option noyywrap

%%

{LEFT_BRACE} {
	return LEFT_BRACE;
}	

{RIGHT_BRACE} {
	return RIGHT_BRACE;
}	

{LEFT_PAREN} {
	return LEFT_PAREN;
}	

{RIGHT_PAREN} {
	return RIGHT_PAREN;
}

{LEFT_BRACKET} {
	return LEFT_BRACKET;
}

{RIGHT_BRACKET} {
	return RIGHT_BRACKET;
}	

{STATEMENT_END} {
	return STATEMENT_END;
}

{MOD_OP} {
	return MOD_OP;
}

{OR_OP} {
	return OR_OP;
}	

{AND_OP} {
	return AND_OP;
}	

{NOT_OP} {	
	return NOT_OP;
}



{ASSIGN_OP} {
	return ASSIGN_OP;
}	

{GT_OP} {
	return GT_OP;
}

{GT_EQ_OP} {
	return GT_EQ_OP;
}

{LT_OP} {
	return LT_OP;
}	

{LT_EQ_OP} {
	return LT_EQ_OP;
}	

{EQ_OP} {
	return EQ_OP;
}

{PLUS_OP} {
	return PLUS_OP;
}	

{MINUS_OP} {
	return MINUS_OP;
}	

{MULT_OP} {
	return MULT_OP;
}	

{DIV_OP} {
	return DIV_OP;
}	

{MEMBER_OP} {
	return MEMBER_OP;
}	

{IF} {
	return IF;
}

{ELSE} {
	return ELSE;
}	

{WHILE} {
	return WHILE;
}	

{NUMBER} {
	yylval.int_value = atoi(yytext);
	return NUMBER;
}	

{COLON} {
	return COLON;
}	

{PARAM_SEP} {
	return PARAM_SEP;
}	

{IDS} {
	yylval.string = (char*)malloc(yyleng);
	strncpy(yylval.string, yytext, yyleng);
	return IDS;
}


{REALM} {
	yylval.string = (char*)malloc(yyleng);
	strncpy(yylval.string, yytext, yyleng);
	return REALM_NAME;
}	


\"      {
	BEGIN(str);
	yylval.string = (char*)malloc(STR_ALLOC_BLOCK); 
	if(NULL == yylval.string)
		return UNKNOWN_CHAR;
	string_buf_ptr = yylval.string; 
	str_len = 0;
	buf_len = STR_ALLOC_BLOCK;
}
     
<str>\" { /* saw closing quote - all done */
       BEGIN(INITIAL);
       REALLOC_STRING;
       *string_buf_ptr = '\0';
       /* return string constant token type and
       * value to parser
       */
	return STRING;
}
     
<str>\n	{
       /* error - unterminated string constant */
       /* generate error message */
	free(yylval.string);
	return UNKNOWN_CHAR;
}
     
<str>\\[0-7]{1,3} {
	/* octal escape sequence */
	int result;
     
	(void) sscanf( yytext + 1, "%o", &result );
     
	if ( result > 0xff ) {
		free(yylval.string);
		return UNKNOWN_CHAR;
	}
	REALLOC_STRING;
	*string_buf_ptr++ = result;
	str_len++;
}
     
<str>\\[0-9]+ {
	/* generate error - bad escape sequence; something
         * like '\48' or '\0777777'
         */
	free(yylval.string);
	return UNKNOWN_CHAR;
}

<str>\\n  {
	REALLOC_STRING;
	*string_buf_ptr++ = '\n';
	str_len++;
}
<str>\\t  {
	REALLOC_STRING;
	*string_buf_ptr++ = '\t';
	str_len++;
}
<str>\\r  {
	REALLOC_STRING;
	*string_buf_ptr++ = '\r';
	str_len++;
}
<str>\\b  {
	REALLOC_STRING;
	*string_buf_ptr++ = '\b';
	str_len++;
}
<str>\\f  {
	REALLOC_STRING;
	*string_buf_ptr++ = '\f';
	str_len++;
}
     
<str>\\(.|\n)  {
	REALLOC_STRING;
	*string_buf_ptr++ = yytext[1];
	str_len++;
}
     
<str>[^\\\n\"]+	{
	char *yptr = yytext;
        while ( *yptr ) {
		REALLOC_STRING;
		*string_buf_ptr++ = *yptr++;
		str_len++;
	}
}

"/*" 	BEGIN(comment);
<comment>[^*\n]*
<comment>[^*\n]*\n      
<comment>"*"+[^*/\n]*
<comment>"*"+[^*/\n]*\n 
<comment>"*"+"/"        BEGIN(INITIAL);

[ \t]+          /* eat up whitespace */	
"\n"	line_no++;

. {
	return UNKNOWN_CHAR;
} 
