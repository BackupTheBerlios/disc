/*
 * $Id: resolve.h,v 1.2 2003/04/02 15:05:13 bogdan Exp $
 *
 * resolver related functions
 */



#ifndef __resolve_h
#define __resolve_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/nameser.h>

#include "ip_addr.h"
#include "../dprint.h"


#define MAX_QUERY_SIZE 8192
#define ANS_SIZE       8192
#define DNS_HDR_SIZE     12
#define MAX_DNS_NAME 256
#define MAX_DNS_STRING 255


#define rev_resolvehost(ip)\
					gethostbyaddr((char*)(ip)->u.addr, (ip)->len, (ip)->af);



#define HEX2I(c) \
	(	(((c)>='0') && ((c)<='9'))? (c)-'0' :  \
		(((c)>='A') && ((c)<='F'))? ((c)-'A')+10 : \
		(((c)>='a') && ((c)<='f'))? ((c)-'a')+10 : -1 )



#if 0
/* converts a str to an ipv4 address, returns the address or 0 on error
   Warning: the result is a pointer to a statically allocated structure */
static inline struct ip_addr* str2ip(str* st)
{
	int i;
	unsigned char *limit;
	static struct ip_addr ip;
	unsigned char* s;

	s = st->s;

	/*init*/
	ip.u.addr32[0]=0;
	i=0;
	limit=st->s + st->len;

	for(;s<limit ;s++){
		if (*s=='.'){
				i++;
				if (i>3) goto error_dots;
		}else if ( (*s <= '9' ) && (*s >= '0') ){
				ip.u.addr[i]=ip.u.addr[i]*10+*s-'0';
		}else{
				//error unknown char
				goto error_char;
		}
	}
	ip.af=AF_INET;
	ip.len=4;
	
	return &ip;

error_dots:
	DBG("str2ip: ERROR: too many dots in [%.*s]\n", st->len, st->s);
	return 0;
 error_char:
	DBG("str2ip: WARNING: unexpected char %c in [%.*s]\n", *s, st->len, st->s);
	return 0;
}



/* returns an ip_addr struct.; on error returns 0
 * the ip_addr struct is static, so subsequent calls will destroy its content*/
static inline struct ip_addr* str2ip6(str* st)
{
	int i, idx1, rest;
	int no_colons;
	int double_colon;
	int hex;
	static struct ip_addr ip;
	unsigned short* addr_start;
	unsigned short addr_end[8];
	unsigned short* addr;
	unsigned char* limit;
	unsigned char* s;
	
	/* init */
	s=st->s;
	i=idx1=rest=0;
	double_colon=0;
	no_colons=0;
	ip.af=AF_INET6;
	ip.len=16;
	addr_start=ip.u.addr16;
	addr=addr_start;
	limit=st->s+st->len;
	memset(addr_start, 0 , 8*sizeof(unsigned short));
	memset(addr_end, 0 , 8*sizeof(unsigned short));
	for (; s<limit; s++){
		if (*s==':'){
			no_colons++;
			if (no_colons>7) goto error_too_many_colons;
			if (double_colon){
				idx1=i;
				i=0;
				if (addr==addr_end) goto error_colons;
				addr=addr_end;
			}else{
				double_colon=1;
				addr[i]=htons(addr[i]);
				i++;
			}
		}else if ((hex=HEX2I(*s))>=0){
				addr[i]=addr[i]*16+hex;
				double_colon=0;
		}else{
			/* error, unknown char */
			goto error_char;
		}
	}
	if (no_colons<2) goto error_too_few_colons;
	if (!double_colon){ /* not ending in ':' */
		addr[i]=htons(addr[i]);
		i++; 
	}
	/* if address contained '::' fix it */
	if (addr==addr_end){
		rest=8-i-idx1;
		memcpy(addr_start+idx1+rest, addr_end, i*sizeof(unsigned short));
	}
/*
	DBG("str2ip6: idx1=%d, rest=%d, no_colons=%d, hex=%x\n",
			idx1, rest, no_colons, hex);
	DBG("str2ip6: address %x:%x:%x:%x:%x:%x:%x:%x\n", 
			addr_start[0], addr_start[1], addr_start[2],
			addr_start[3], addr_start[4], addr_start[5],
			addr_start[6], addr_start[7] );
*/
	return &ip;

error_too_many_colons:
	DBG("str2ip6: ERROR: too many colons in [%.*s]\n", st->len, st->s);
	return 0;

error_too_few_colons:
	DBG("str2ip6: ERROR: too few colons in [%.*s]\n", st->len, st->s);
	return 0;

error_colons:
	DBG("str2ip6: ERROR: too many double colons in [%.*s]\n", st->len, st->s);
	return 0;

error_char:
	DBG("str2ip6: WARNING: unexpected char %c in  [%.*s]\n", *s, st->len,
			st->s);
	return 0;
}
#endif
#define HAVE_GETHOSTBYNAME2

/* gethostbyname wrappers
 * use this, someday they will use a local cache */

static inline struct hostent* resolvehost(const char* name)
{
	static struct hostent* he=0;
#ifdef HAVE_GETIPNODEBYNAME 
	int err;
	static struct hostent* he2=0;
#endif
#ifdef DNS_IP_HACK
	struct ip_addr* ip;
	str s;

	s.s = (char*)name;
	s.len = strlen(name);

	/* check if it's an ip address */
	if ( ((ip=str2ip(&s))!=0)
#ifdef	USE_IPV6
		  || ((ip=str2ip6(&s))!=0)
#endif
		){
		/* we are lucky, this is an ip address */
		return ip_addr2he(&s, ip);
	}
	
#endif
	/* ipv4 */
	he=gethostbyname(name);
#ifdef USE_IPV6
	if(he==0){
		/*try ipv6*/
	#ifdef HAVE_GETHOSTBYNAME2
		he=gethostbyname2(name, AF_INET6);
	#elif defined HAVE_GETIPNODEBYNAME
		/* on solaris 8 getipnodebyname has a memory leak,
		 * after some time calls to it will fail with err=3
		 * solution: patch your solaris 8 installation */
		if (he2) freehostent(he2);
		he=he2=getipnodebyname(name, AF_INET6, 0, &err);
	#else
		#error neither gethostbyname2 or getipnodebyname present
	#endif
	}
#endif
	return he;
}



#endif
