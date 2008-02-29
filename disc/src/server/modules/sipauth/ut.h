/*
 * $Id: ut.h,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#ifndef ut_h
#define ut_h

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>

#include "../../../dprint.h"
#include "../../../str.h"



/* zero-string wrapper */
#define ZSW(_c) ((_c)?(_c):"")

/* returns string beginning and length without insignificant chars */
#define trim_len( _len, _begin, _mystr ) \
	do{ 	static char _c; \
		(_len)=(_mystr).len; \
		while ((_len) && ((_c=(_mystr).s[(_len)-1])==0 || _c=='\r' || \
					_c=='\n' || _c==' ' || _c=='\t' )) \
			(_len)--; \
		(_begin)=(_mystr).s; \
		while ((_len) && ((_c=*(_begin))==' ' || _c=='\t')) { \
			(_len)--;\
			(_begin)++; \
		} \
	}while(0)

#define trim_r( _mystr ) \
	do{	static char _c; \
		while( ((_mystr).len) && ( ((_c=(_mystr).s[(_mystr).len-1]))==0 ||\
									_c=='\r' || _c=='\n' ) \
				) \
			(_mystr).len--; \
	}while(0)


#define  translate_pointer( _new_buf , _org_buf , _p) \
	( (_p)?(_new_buf + (_p-_org_buf)):(0) )

#define via_len(_via) \
	((_via)->bsize-((_via)->name.s-\
		((_via)->hdr.s+(_via)->hdr.len)))


/* char to hex conversion table */
static char fourbits2char[16] = { '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };


/* converts a str to an u. short, returns the u. short and sets *err on
 * error and if err!=null
  */
static inline unsigned short str2s(char* s, unsigned int len,
									int *err)
{
	unsigned short ret;
	int i;
	unsigned char *limit;
	unsigned char *init;
	unsigned char* str;

	/*init*/
	str=(unsigned char*)s;
	ret=i=0;
	limit=str+len;
	init=str;

	for(;str<limit ;str++){
		if ( (*str <= '9' ) && (*str >= '0') ){
				ret=ret*10+*str-'0';
				i++;
				if (i>5) goto error_digits;
		}else{
				//error unknown char
				goto error_char;
		}
	}
	if (err) *err=0;
	return ret;

error_digits:
	DBG("str2s: ERROR: too many letters in [%.*s]\n", (int)len, init);
	if (err) *err=1;
	return 0;
error_char:
	DBG("str2s: ERROR: unexpected char %c in %.*s\n", *str, (int)len, init);
	if (err) *err=1;
	return 0;
}

#define INT2STR_MAX_LEN 11 /* 10 digits + 0 */

/* returns a pointer to a static buffer containing l in asciiz & sets len */
static inline char* int2str(unsigned int l, int* len)
{
	static char r[INT2STR_MAX_LEN];
	int i;
	
	i=9;
	r[10]=0; /* null terminate */
	do{
		r[i]=l%10+'0';
		i--;
		l/=10;
	}while(l && (i>=0));
	if (l && (i<0)){
		LOG(L_CRIT, "BUG: int2str: overflow\n");
	}
	if (len) *len=9-i;
	return &r[i+1];
}



/* faster memchr version */
static inline char* q_memchr(char* p, int c, unsigned int size)
{
	char* end;

	end=p+size;
	for(;p<end;p++){
		if (*p==(unsigned char)c) return p;
	}
	return 0;
}
	
/* double output length assumed ; does NOT zero-terminate */
inline static int string2hex( 
	/* input */ unsigned char *str, int len,
	/* output */ char *hex )
{
	int orig_len;

	if (len==0) {
		*hex='0';
		return 1;
	}

	orig_len=len;
	while ( len ) {

		*hex=fourbits2char[(*str) >> 4];
		hex++;
		*hex=fourbits2char[(*str) & 0xf];
		hex++;
		len--;
		str++;

	}
	return orig_len-len;
}
/*
 * Convert an integer to its hex representation,
 * destination array must be at least 8 bytes long,
 * this string is NOT zero terminated
 */
static inline void integer2hex(char* _d, int _s)
{
	int i;
	unsigned char j;
	char* s;

	_s = htonl(_s);
	s = (char*)&_s;
    
	for (i = 0; i < 4; i++) {
		
		j = (s[i] >> 4) & 0xf;
		if (j <= 9) {
			_d[i * 2] = (j + '0');
		} else { 
			_d[i * 2] = (j + 'a' - 10);
		}

		j = s[i] & 0xf;
		if (j <= 9) {
			_d[i * 2 + 1] = (j + '0');
		} else {
		       _d[i * 2 + 1] = (j + 'a' - 10);
		}
	}
}


/*
 * Convert hex string to integer
 */
static inline int hex2integer(char* _s)
{
	unsigned int i, res = 0;

	for(i = 0; i < 8; i++) {
		res *= 16;
		if ((_s[i] >= '0') && (_s[i] <= '9')) {
			res += _s[i] - '0';
		} else if ((_s[i] >= 'a') && (_s[i] <= 'f')) {
			res += _s[i] - 'a' + 10;
		} else if ((_s[i] >= 'A') && (_s[i] <= 'F')) {
			res += _s[i] - 'A' + 10;
		} else return 0;
	}

	return res;
}

inline static int hex2int(char hex_digit)
{
	if (hex_digit>='0' && hex_digit<='9')
		return hex_digit-'0';
	if (hex_digit>='a' && hex_digit<='f')
		return hex_digit-'a'+10;
	if (hex_digit>='A' && hex_digit<='F')
		return hex_digit-'A'+10;
	/* no valid hex digit ... */
	LOG(L_ERR, "ERROR: hex2int: '%c' is no hex char\n", hex_digit );
	return -1;
}

/* Un-escape URI user  -- it takes a pointer to original user
   str, as well as the new, unescaped one, which MUST have
   an allocated buffer linked to the 'str' structure ;
   (the buffer can be allocated with the same length as
   the original string -- the output string is always
   shorter (if escaped characters occur) or same-long
   as the original one).

   only printeable characters are permitted

	<0 is returned on an uneascaping error, length of the
	unescaped string otherwise
*/
inline static int un_escape(str *user, str *new_user ) 
{
 	int i, j, value;
	int hi, lo;

	if( new_user==0 || new_user->s==0) {
		LOG(L_CRIT, "BUG: un_escape: called with invalid param\n");
		return -1;
	}

	new_user->len = 0;
	j = 0;

	for (i = 0; i < user->len; i++) {
		if (user->s[i] == '%') {
			if (i + 2 >= user->len) {
				LOG(L_ERR, "ERROR: un_escape: escape sequence too short in"
					" '%.*s' @ %d\n",
					user->len, user->s, i );
				goto error;
			}
			hi=hex2int(user->s[i + 1]);
			if (hi<0) {
				LOG(L_ERR, "ERROR: un_escape: non-hex high digit in an escape sequence in"
					" '%.*s' @ %d\n",
					user->len, user->s, i+1 );
				goto error;
			}
			lo=hex2int(user->s[i + 2]);
			if (lo<0) {
				LOG(L_ERR, "ERROR: non-hex low digit in an escape sequence in "
					"'%.*s' @ %d\n",
					user->len, user->s, i+2 );
				goto error;
			}
			value=(hi<<4)+lo;
			if (value < 32 || value > 126) {
				LOG(L_ERR, "ERROR: non-ASCII escaped character in '%.*s' @ %d\n",
					user->len, user->s, i );
				goto error;
			}
			new_user->s[j] = value;
			i+=2; /* consume the two hex digits, for cycle will move to the next char */
		} else {
			new_user->s[j] = user->s[i];
		}
        j++; /* good -- we translated another character */
	}
	new_user->len = j;
	return j;

error:
	new_user->len = j;
	return -1;
} 

#endif
