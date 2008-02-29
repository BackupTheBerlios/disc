/*
 * $Id: challenge.c,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#include "../../../mem/shm_mem.h"
#include "../../../dprint.h"

#include "sipauth.h"
#include "nonce.h"
#include "challenge.h"
#include "digest_parser.h"

#define M_NAME "sipauth"

/*
 * Create Chalenge Response 
 */
char *build_challenge(int _stale, str _realm, int _qop)
{
	char *chal, *p;
	int len=0;

     /* length calculation */
	len=DIGEST_REALM_LEN
		+_realm.len
		+DIGEST_NONCE_LEN
		+NONCE_LEN
		+1 /* '"' */
		+((_qop==QOP_AUTH)? QOP_AUTH_PARAM_LEN:0)
		+((_qop==QOP_AUTHINT)? QOP_AUTHINT_PARAM_LEN:0)
		+((_stale)? STALE_PARAM_LEN : 0)
#ifdef _PRINT_MD5
		+DIGEST_MD5_LEN
#endif
		+CRLF_LEN ;
	
	p = chal = (char*)shm_malloc((len+1)*sizeof(char));
	if (!chal)
	{
		LOG(L_ERR, M_NAME": no more free memory\n");
		return 0;
	}

	memcpy(p, DIGEST_REALM, DIGEST_REALM_LEN);p+=DIGEST_REALM_LEN;
	memcpy(p, _realm.s, _realm.len);p+=_realm.len;
	memcpy(p, DIGEST_NONCE, DIGEST_NONCE_LEN);p+=DIGEST_NONCE_LEN;
	calc_nonce(p, time(0) + nonce_expire, &secret);
	p+=NONCE_LEN;
	*p='"';p++;
	if (_qop==QOP_AUTH) 
	{
		memcpy(p, QOP_AUTH_PARAM, QOP_AUTH_PARAM_LEN);
		p+=QOP_AUTH_PARAM_LEN;
	}
	if (_qop==QOP_AUTHINT) 
	{
		memcpy(p, QOP_AUTHINT_PARAM, QOP_AUTHINT_PARAM_LEN);
		p+=QOP_AUTHINT_PARAM_LEN;
	}

	if (_stale) 
	{
		memcpy(p, STALE_PARAM, STALE_PARAM_LEN);
		p+=STALE_PARAM_LEN;
	}
#ifdef _PRINT_MD5
	memcpy(p, DIGEST_MD5, DIGEST_MD5_LEN ); p+=DIGEST_MD5_LEN;
#endif
	memcpy(p, CRLF, CRLF_LEN ); p+=CRLF_LEN;
	*p=0; /* zero terminator, just in case */

#ifdef DEBUG	
	DBG(M_NAME":build_challenge(): '%s'\n", chal);
#endif

	return chal;
}

