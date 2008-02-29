/*
 * $Id: digest_parser.c,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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
 *
 */
#include <string.h>
#include "../../../dprint.h"

#include "digest_parser.h"
#include "param_parser.h"
#include "ut.h"
#include "trim.h"


#define DIGEST_SCHEME "digest"
#define DIG_LEN 6

#define QOP_AUTH_STR "auth"
#define QOP_AUTH_STR_LEN 4

#define QOP_AUTHINT_STR "auth-int"
#define QOP_AUTHINT_STR_LEN 8

#define ALG_MD5_STR "MD5"
#define ALG_MD5_STR_LEN 3

#define ALG_MD5SESS_STR "MD5-sess"
#define ALG_MD5SESS_STR_LEN 8


/*
 * Parse quoted string in a parameter body
 * return the string without quotes in r
 * parameter and update s to point behind the
 * closing quote
 */
static inline int parse_quoted(str* s, str* r)
{
	char* end_quote;

	/* The string must have at least surrounding quotes */
	if (s->len < 2) 
		return -1;

	/* Skip opening quote */
	s->s++;
	s->len--;

	/* Find closing quote */
	end_quote = q_memchr(s->s, '\"', s->len);

	/* Not found, return error */
	if (!end_quote) 
		return -2;

	/* Let r point to the string without surrounding quotes */
	r->s = s->s;
	r->len = end_quote - s->s;

	/* Update s parameter to point behind the closing quote */
	s->len -= (end_quote - s->s + 1);
	s->s = end_quote + 1;

	/* Everything went OK */
	return 0;
}


/*
 * Parse unquoted token in a parameter body
 * let r point to the token and update s
 * to point right behind the token
 */
static inline int parse_token(str* s, str* r)
{
	int i;

	/* Save the begining of the token in _r->s */
	r->s = s->s;

	/* Iterate throught the token body */
	for(i = 0; i < s->len; i++) 
	{
	     /* All these characters mark end of the token */
		switch(s->s[i]) 
		{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case ',':
			     	/* So if you find any of them stop iterating */
					goto out;
		}
	}
	
 out:
	/* Empty token is error */
	if (i == 0) 
        return -2;


	/* Save length of the token */
    r->len = i;

	/* Update s parameter so it points right behind the end of the token */
	s->s = s->s + i;
	s->len -= i;

	/* Everything went OK */
	return 0;
}

/*
 * Parse a digest parameter
 */
static inline int parse_digest_param(str* s, dig_cred_t* c)
{
	dig_par_t t;
	str* ptr;
	str dummy;

	/* Get type of the parameter */
	if (parse_param_name(s, &t) < 0) 
		return -1;

	s->s++;  /* skip = */
	s->len--;

	/* Find the begining of body */
	trim_leading(s);

	if (s->len == 0)
		return -2;

	/* Decide in which attribute the body content will be stored */
	switch(t)
	{
		case PAR_USERNAME:  
						ptr = &c->username.whole;  
						break;
		case PAR_REALM:     
						ptr = &c->realm;           
						break;
		case PAR_NONCE:     
						ptr = &c->nonce;           
						break;
		case PAR_URI:       
						ptr = &c->uri;             
						break;
		case PAR_RESPONSE:  
						ptr = &c->response;        
						break;
		case PAR_CNONCE:    
						ptr = &c->cnonce;          
						break;
		case PAR_OPAQUE:    
						ptr = &c->opaque;          
						break;
		case PAR_QOP:       
						ptr = &c->qop.qop_str;     
						break;
		case PAR_NC:        
						ptr = &c->nc;              
						break;
		case PAR_ALGORITHM: 
						ptr = &c->alg.alg_str;     
						break;
		case PAR_OTHER:     
						ptr = &dummy;               
						break;
		default:            
						ptr = &dummy;               
						break;
	}

	/* If the first character is qoute, it is a quoted string, 
	 * otherwise it is a token */
	if (s->s[0] == '\"')
	{
		if (parse_quoted(s, ptr) < 0) 
			return -3;
	}	
	else 
		if (parse_token(s, ptr) < 0) 
			return -4;
	
	return 0;
}


/*
 * Parse qop parameter body
 */
static inline void parse_qop(struct qp* q)
{
	str s;

	s.s = q->qop_str.s;
	s.len = q->qop_str.len;

	trim(&s);

	if ((s.len == QOP_AUTH_STR_LEN) &&
	    !strncasecmp(s.s, QOP_AUTH_STR, QOP_AUTH_STR_LEN)) {
		q->qop_parsed = QOP_AUTH;
	} else if ((s.len == QOP_AUTHINT_STR_LEN) &&
		   !strncasecmp(s.s, QOP_AUTHINT_STR, QOP_AUTHINT_STR_LEN)) {
		q->qop_parsed = QOP_AUTHINT;
	} else {
		q->qop_parsed = QOP_OTHER;
	}
}


/*
 * Parse algorithm parameter body
 */
static inline void parse_algorithm(struct algorithm* a)
{
	str s;

	s.s = a->alg_str.s;
	s.len = a->alg_str.len;

	trim(&s);

	if ((s.len == ALG_MD5_STR_LEN) &&
	    !strncasecmp(s.s, ALG_MD5_STR, ALG_MD5_STR_LEN)) {
		a->alg_parsed = ALG_MD5;
	} else if ((s.len == ALG_MD5SESS_STR_LEN) &&
		   !strncasecmp(s.s, ALG_MD5SESS_STR, ALG_MD5SESS_STR_LEN)) {
		a->alg_parsed = ALG_MD5SESS;
	} else {
		a->alg_parsed = ALG_OTHER;
	}	
}


/*
 * Parse username for user and domain parts
 */
static inline void parse_username(struct username* u)
{
	char* d;

	u->user = u->whole;
	if (u->whole.len <= 2) 
			return;

	d = q_memchr(u->whole.s, '@', u->whole.len);

	if (d) 
	{
		u->domain.s = d + 1;
		u->domain.len = u->whole.len - (d - u->whole.s) - 1;
		u->user.len = d - u->user.s;
	}
}


/*
 * Parse Digest credentials parameter, one by one
 */
static inline int parse_digest_params(str* _s, dig_cred_t* _c)
{
	char* comma=NULL;

	do{
		/* Parse the first parameter */
		if (parse_digest_param(_s, _c) < 0) 
		{
			return -1;
		}
		
		/* Try to find the next parameter */
		comma = q_memchr(_s->s, ',', _s->len);
		if (comma) 
		{
		  /* Yes, there is another, remove any leading whitespaces and 
		   * let _s point to the next parameter name
		   */
		
			_s->len -= comma - _s->s + 1;
			_s->s = comma + 1;
			trim_leading(_s);
		}
	} while(comma); /* Repeat while there are next parameters */

	/* Parse QOP body if the parameter was present */
	if (_c->qop.qop_str.s != 0) 
		parse_qop(&_c->qop);

	/* Parse algorithm body if the parameter was present */
	if (_c->alg.alg_str.s != 0) 
		parse_algorithm(&_c->alg);
	

	if (_c->username.whole.s != 0) 
		parse_username(&_c->username);

	return 0;
}


/*
 * We support Digest authentication only
 *
 * Returns:
 *  1- if everything is OK
 * -1 - Unknown scheme
 * -2 - Error while parsing
 */
int parse_digest_cred(str* _s, dig_cred_t* _c)
{
	str tmp;

	/* Make a temporary copy, we are going to modify it */
	tmp.s = _s->s;
	tmp.len = _s->len;
	/* Remove any leading spaces, tabs, \r and \n */
	trim_leading(&tmp);
	/* Check the string length */
	if (tmp.len < (DIG_LEN + 1)) 
		return -1; /* Too short, unknown scheme */

	/* Now test, if it is digest scheme, since it is the only 
	 * scheme we are able to parse here
	 */
	if (!strncasecmp(tmp.s, DIGEST_SCHEME, DIG_LEN) &&
	    ((tmp.s[DIG_LEN] == ' ') ||     /* Test for one of LWS chars */
	     (tmp.s[DIG_LEN] == '\r') || 
	     (tmp.s[DIG_LEN] == 'n') || 
	     (tmp.s[DIG_LEN] == '\t') ||
	     (tmp.s[DIG_LEN] == ','))) 
	{
		/* Scheme is Digest */
		tmp.s += DIG_LEN + 1;
		tmp.len -= DIG_LEN + 1;
		
		/* Again, skip all whitespaces */
		trim_leading(&tmp);

		/* And parse digest parameters */
		if (parse_digest_params(&tmp, _c) < 0) 
			return -2; /* We must not return -1 in this function ! */
		else 
			return 1;
	} 
	else
		return -1; /* Unknown scheme */
}


/*
 * Initialize a digest credentials structure
 */
void init_dig_cred(dig_cred_t* c)
{
	memset(c, 0, sizeof(dig_cred_t));
}


