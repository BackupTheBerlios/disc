/*
 * $Id: nonce.c,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../../../dprint.h"

#include "md5global.h"
#include "md5.h"
#include "ut.h"
#include "nonce.h"


/* Nonce value is build from time in seconds 
 * since 1.1 1970 and secret phrase */
void calc_nonce(char* _nonce, int _expires, str* _secret)
{
	MD5_CTX ctx;
	unsigned char bin[16];

	MD5Init(&ctx);
	
	integer2hex(_nonce, _expires);
	MD5Update(&ctx, _nonce, 8);

	MD5Update(&ctx, _secret->s, _secret->len);
	MD5Final(bin, &ctx);
	string2hex(bin, 16, _nonce + 8);
	_nonce[8 + 32] = '\0';
}


/* Get expiry time from nonce string */
time_t get_nonce_expires(str* _n)
{
	return (time_t)hex2integer(_n->s);
}


/* Check, if the nonce received from client is correct */
int check_nonce(str* _nonce, str* _secret)
{
	int expires;
	char non[NONCE_LEN + 1];

	if (_nonce->s == 0) 
		return -1;  /* Invalid nonce */

	if (NONCE_LEN != _nonce->len) 
		return -1; /* Lengths must be equal */

	expires = get_nonce_expires(_nonce);
	calc_nonce(non, expires, _secret);

	if (!memcmp(non, _nonce->s, _nonce->len)) 
		return 0; /* nonce OK */

	return -1;
}


/* Check if a nonce is stale */
int is_nonce_stale(str* _n) 
{
	int now, expires;
	
	if (!_n->s) return 0;
	
	now = time(0);
	expires = get_nonce_expires(_n);
	
	if (expires < now) 
		return 1;
	
	return 0;
}

