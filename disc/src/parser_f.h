/* 
 * $Id: parser_f.h,v 1.2 2003/04/14 15:52:08 andrei Exp $
 *
 * Copyright (C) 2001-2003 Fhg Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History
 * --------
 * 2003-02-28  scratchpad compatibility abandoned (jiri)
 * 2003-03-13  imported into aaa_server from ser (andrei)
 *             changed eat_token2_end to stop also on whitespace (andrei)
 * 2003-04-07  changed eat_token* to stop on '#' (comment) (andrei)
 */


#ifndef parser_f_h
#define parser_f_h


char* eat_line(char* buffer, unsigned int len);

/* turn the most frequently called functions into inline functions */

inline static char* eat_space_end(char* p, char* pend)
{
	for(;(p<pend)&&(*p==' ' || *p=='\t') ;p++);
	return p;
}
#define SP(_c) ((_c)=='\t' || (_c)==' ')
inline static char* eat_lws_end(char* p, char* pend)
{
	while(p<pend) {
		if (SP(*p)) p++;
		/* btw--I really dislike line folding; -jiri */
		else if (*p=='\n' && p+1<pend && SP(*(p+1))) p+=2;
		else if (*p=='\r' && p+2<pend && *(p+1)=='\n' 
					&& SP(*(p+2))) p+=3;
		else break; /* no whitespace encountered */
	}
	return p;
}



inline static char* eat_token_end(char* p, char* pend)
{
	int quotes;
	quotes=0;
	for (;(p<pend)&&((quotes%2)||((*p!=' ')&&(*p!='\t')&&(*p!='\n')&&(*p!='\r')
						&&(*p!='#'))); p++){
		if (*p=='"') quotes++;
	}
	return p;
}



inline static char* eat_token2_end(char* p, char* pend, char delim)
{
	int quotes;
	quotes=0;
	for (;(p<pend)&&((quotes%2)||((*p!=(delim))&&(*p!=' ')&&(*p!='\t')&&
					(*p!='\n') &&(*p!='\r')&&(*p!='#'))); p++){
		if (*p=='"') quotes++;
	}
	return p;
}



inline static int is_empty_end(char* p, char* pend )
{
	p=eat_space_end(p, pend );
	return ((p<(pend )) && (*p=='\r' || *p=='\n'));
}


#endif /* parser_f_h */
