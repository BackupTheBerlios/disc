/*
 * $Id: cfg_parser.c,v 1.2 2003/03/14 18:29:11 bogdan Exp $
 *
 * configuration parser
 *
 * Config file format:
 *
 *  id = value    # comment
 */
/*
 * History:
 * --------
 *  2003-03-13  created by andrei
 */


#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "cfg_parser.h"
#include "../dprint.h"
#include "parser_f.h"
#include "../mem/shm_mem.h"





/* params: null terminated text line => fills cl
 * returns 0, or on error -1. */
int cfg_parse_line(char* line, struct cfg_line* cl)
{
	/* format:
		line = expr | comment
		comment = SP* '#'.*
		expr = SP* id SP* '=' SP* value comment?
	*/
		
	char* tmp;
	char* end;
	char* t;
	
	end=line+strlen(line);
	tmp=eat_space_end(line, end);
	if ((tmp==end)||(is_empty_end(tmp, end))) {
		cl->type=CFG_EMPTY;
		goto skip;
	}
	if (*tmp=='#'){
		cl->type=CFG_COMMENT;
		goto skip;
	}
	cl->id=tmp;
	tmp=eat_token2_end(cl->id,end, '=');
	if (tmp==end) goto error;
	t=tmp;
	tmp=eat_space_end(tmp,end);
	if (tmp==end) goto error;
	if (*tmp!='=') goto error;
	*t=0;
	tmp++;
	tmp=eat_space_end(tmp,end);
	if (tmp==end) goto error;
	cl->value=tmp;
	tmp=eat_token_end(cl->value, end);
	if (tmp<end) {
		*tmp=0;
		if (tmp+1<end){
			if (!is_empty_end(tmp+1,end)){
				/* check if comment */
				tmp=eat_space_end(tmp+1, end);
				if (*tmp!='#'){
					/* extra chars at the end of line */
					goto error;
				}
			}
		}
	}
	
	cl->type=CFG_DEF;
skip:
	return 0;
error:
	cl->type=CFG_ERROR;
	return -1;
}



/* parses the cfg, returns 0 on success, line no otherwise */
int cfg_parse_stream(FILE* stream)
{
	int line;
	struct cfg_line cl;
	char buf[MAX_LINE_SIZE];
	int ret;

	line=1;
	while(!feof(stream)){
		if (fgets(buf, MAX_LINE_SIZE, stream)){
			cfg_parse_line(buf, &cl);
			switch (cl.type){
				case CFG_DEF:
					if ((ret=cfg_run_def(&cl))!=0){
						LOG(L_CRIT, "ERROR: on line %d\n", line);
						LOG(L_CRIT, " ----: cfg_run_def returned %d\n", ret);
						goto error;
					}
					break;
				case CFG_COMMENT:
				case CFG_SKIP:
					break;
				case CFG_ERROR:
					LOG(L_CRIT, "ERROR: bad config line (%d):%s\n", line, buf);
					goto error;
					break;
			}
			line++;
		}else{
			if (ferror(stream)){
				LOG(L_CRIT,
						"ERROR: reading configuration: %s\n",
						strerror(errno));
				goto error;
			}
			break;
		}
	}
	return 0;

error:
	return line;
}



int cfg_getint(char* p, int* i)
{
	char* end;

	*i=strtol(p,&end ,10);
	if (*end) return -3;
	else return 0;
}


int cfg_getstr(char* p, str* r)
{
	int quotes;
	char* s;
	int len;
	
	quotes=0;
	s=0;
	if (*p=='"') { p++; quotes++; };
	for (; *p; p++){
		if (*p=='"') quotes++;
		if (s==0) s=p;
	}
	if (quotes%2) return -3; /* bad quote number */
	if (quotes){
		if (*(p-1)!='"') return -3; /* not terminated by quotes */
		len=p-1-s;
		*(p-1)=0;
	}else{
		len=p-s;
	}
	r->s=(char*)shm_malloc(len+1);
	if (r->s==0) return -4; /* mem. alloc. error */
	memcpy(r->s, s, len);
	r->s[len]=0; /* null terminate, just in case */
	r->len=len;
	return 0;
}



int cfg_run_def(struct cfg_line *cl)
{
	struct cfg_def* def;
		
	
	for(def=cfg_ids; def && def->name; def++)
		if (strcasecmp(cl->id, def->name)==0){
			switch(def->type){
				case INT_VAL:
					return cfg_getint(cl->value, def->value);
					break;
				case STR_VAL:
					return cfg_getstr(cl->value, def->value);
					break;
				default:
					LOG(L_CRIT, "BUG: cfg_run_def: unknown type %d\n",
							def->type);
					return -2;
			}
		}
	/* not found */
	LOG(L_CRIT, "ERROR: unknown id <%s>\n", cl->id);
	return -1;
}


