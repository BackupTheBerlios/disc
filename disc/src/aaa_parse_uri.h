/*
 * $Id: aaa_parse_uri.h,v 1.1 2003/04/08 18:59:52 andrei Exp $
 */
/*
 * History:
 * --------
 *  2003-04-08  created by andrei
 */

#ifndef aaa_parse_uri_h
#define aaa_parse_uri_h


struct aaa_uri {
	str host;      /* Host name */
	str port;      /* Port number */
	str params;    /* Parameters, all of them */
	str transport; /* shortcut to transport */
	str protocol;  /* shortcut to protocol */
	unsigned short port_no;
};


int aaa_parse_uri(char* buf, int len, struct aaa_uri* uri);

#endif



