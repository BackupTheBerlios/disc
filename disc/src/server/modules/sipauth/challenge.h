/*
 * $Id: challenge.h,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#ifndef CHALENGE_H
#define CHALENGE_H

#include "../../../str.h"

/* Length of nonce string in bytes */
#define NONCE_LEN (8+32)

#define CRLF "\r\n"
#define CRLF_LEN 2

#define QOP_AUTH_PARAM				", qop=\"auth\""
#define QOP_AUTH_PARAM_LEN			(sizeof(QOP_AUTH_PARAM)-1)
#define QOP_AUTHINT_PARAM			", qop=\"auth-int\""
#define QOP_AUTHINT_PARAM_LEN		(sizeof(QOP_AUTHINT_PARAM)-1)
#define STALE_PARAM					", stale=true"
#define STALE_PARAM_LEN				(sizeof(STALE_PARAM)-1)
#define DIGEST_REALM				"Digest realm=\""
#define DIGEST_REALM_LEN			(sizeof(DIGEST_REALM)-1)
#define DIGEST_NONCE				"\", nonce=\""
#define DIGEST_NONCE_LEN			(sizeof(DIGEST_NONCE)-1)
#define DIGEST_MD5					", algorithm=MD5"
#define DIGEST_MD5_LEN				(sizeof(DIGEST_MD5)-1)

char *build_challenge(int _stale, str _realm, int _qop);

#endif /* CHALENGE_H */
