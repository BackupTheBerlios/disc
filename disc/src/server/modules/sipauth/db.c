/*
 * $Id: db.c,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <mysql/mysql.h>

#include "../../../dprint.h"
#include "../../../mem/shm_mem.h"

#include "nonce.h"
#include "digest_parser.h"
#include "rfc2617.h" 
#include "db.h"
#include "sipaaa_defs.h"

#define M_NAME "sipauth"

#define avp_no  12
#define col_no  13

static char col[col_no][20]={"SIP_FROM","SIP_TO","SIP_STATUS","SIP_METHOD",
	"I_URI","O_URI", "FROM_URI", "TO_URI", "SIP_CALLID", "USERNAME",
	"FROMTAG", "TOTAG","TIME" };

static int col_len[col_no]={8, 6, 10, 10, 5, 5, 8, 6, 10, 8, 7, 5, 4};

static int avpname[avp_no]={AVP_SIP_FROM, AVP_SIP_TO, AVP_SIP_STATUS,
	AVP_SIP_METHOD, AVP_SIP_IURI, AVP_SIP_OURI, AVP_SIP_FROM_URI,
	AVP_SIP_TO_URI, AVP_SIP_CALLID, AVP_User_Name, AVP_SIP_FROM_TAG,
	AVP_SIP_TO_TAG};

str values[col_no];

extern char* authDBUserName;
extern char* authDBUserPass;
extern char* authDBHost;
extern char* authDB;


static MYSQL *mysql = NULL;


AAAReturnCode init_db_connection( )
{
    static MYSQL init_mysql;

    // initialization of the MySQL connection
    memset(&init_mysql, 0, sizeof(MYSQL));
	mysql_init( &init_mysql );

    mysql = mysql_real_connect( &init_mysql,
                                authDBHost,
                                authDBUserName,
                                authDBUserPass,
                                authDB,
                                0,
                                NULL,
                                0 );

	if( !mysql ) 
	{
        LOG( L_NOTICE, "init_db_connection: Couldn't connect"
						" to database server!\n");
        return AAA_ERR_NOT_INITIALIZED;
    }

    return AAA_ERR_SUCCESS;
}


AAAReturnCode close_db_connection( )
{
    // closing the MySQL connection
    mysql_close( mysql );

    return AAA_ERR_SUCCESS;
}

/*
 *
 */
int do_auth( AAAMessage *msg, int calc_ha1, str* secret)
{
	MYSQL_RES *result;	
	MYSQL_ROW row;
	unsigned long *lengths;
	int num_fields=0;
	char ha1[256];
	HASHHEX resp, hent;

	AAA_AVP *avp;
	dig_cred_t cred;
	char *query;
	str passwd_h;
	int query_len=0, pos=0;
	char select[]="SELECT passwd_h FROM authentication WHERE username=\"";
	int select_len, selectc_len;
	char selectc[]="\" AND domain=\"";
	
	select_len  = strlen(select);
	selectc_len = strlen(selectc);

	avp = AAAFindMatchingAVP(msg, NULL, AVP_Response, vendorID,
					AAA_FORWARD_SEARCH);
	if(!avp)
		return NOT_AUTHORIZED; /* no response avp */	
	
	init_dig_cred(&cred);
	parse_digest_cred(&(avp->data), &cred);

	/* client nonce must be correct and not expired */
	if( check_nonce(&(cred.nonce), secret) )
		return NOT_AUTHORIZED;/* nonce invalid */

	if( is_nonce_stale(&(cred.nonce)) )
		return NONCE_STALE;/* nonce stale */

	query_len=select_len+cred.username.user.len+selectc_len+cred.realm.len+5;

	query = (char*)shm_malloc(query_len*sizeof(char));
	if(query==0)
		return NOT_AUTHORIZED;

	pos=0;
	memcpy(query+pos, select, select_len);
	pos += select_len;
	memcpy(query+pos,cred.username.user.s, cred.username.user.len );
	pos += cred.username.user.len;
	memcpy(query+pos, selectc, selectc_len );
	pos += selectc_len;
	memcpy(query+pos,cred.realm.s, cred.realm.len );
	pos += cred.realm.len;
	memcpy(query+pos, "\"\0", 2);

	if(!mysql) 
	{
		LOG( L_ERR, M_NAME": no db connection");
		return MYSQL_ERROR;
	}

	if( mysql_query( mysql, query ) ) 
	{
 		LOG(L_ERR, M_NAME": mysql_query error\n");
		return MYSQL_ERROR;
	}
	result = mysql_store_result(mysql);
	if(!result)
	{
		LOG(L_ERR, M_NAME": mysql_query error\n");
		return MYSQL_ERROR; /* the user does not exist */
	}		
	
	num_fields = mysql_num_fields(result);
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);
		
	if(!lengths) /* the user is not registerd for this domain */
		return REJECTED;
	
	if(calc_ha1)
	{
		passwd_h.s = (char*)shm_malloc(lengths[0]*sizeof(char));
		passwd_h.len = lengths[0];
		memcpy(passwd_h.s, row[0], lengths[0]);
		calc_HA1(HA_MD5, &cred.username.user, &cred.realm,
					&passwd_h, 0, 0, ha1); 
	}
	else
	{
		memcpy(ha1, row[0], lengths[0]);
		memcpy(ha1+lengths[0], "\0", 1);
	}
	
	if(cred.response.len != HASHHEXLEN)
	{
		LOG(L_ERR, M_NAME": response length must is not %d!\n", HASHHEXLEN);
		return NOT_AUTHORIZED;
	}

	avp = AAAFindMatchingAVP(msg, NULL, AVP_Method, vendorID,
					AAA_FORWARD_SEARCH);
	if(!avp)
	{
		LOG(L_ERR, M_NAME": Method AVP is missing\n");
		return NOT_AUTHORIZED;
	}

	calc_response(ha1, &(cred.nonce), &(cred.nc), &(cred.cnonce), 
		      &(cred.qop.qop_str), cred.qop.qop_parsed == QOP_AUTHINT,
		      &(avp->data), &(cred.uri), hent, resp);

	if (!memcmp(resp, cred.response.s, HASHHEXLEN)) 
		return AUTHORIZED; /* authorized */
	
	return NOT_AUTHORIZED; /* not authorized */
}

/*
 *
 */
int check_group( AAAMessage *msg )
{
	MYSQL_RES *result;	
	MYSQL_ROW row;
	unsigned long *lengths;
	int num_fields=0;

	AAA_AVP *avp_u, *avp_g;
	char *query;
	int query_len=0, pos=0;
	char select[]="SELECT * FROM sipgroups WHERE user=\"";
	int select_len, selectc_len;
	char selectc[]="\" AND user_group=\"";
	
	select_len  = strlen(select);
	selectc_len = strlen(selectc);

	avp_u = AAAFindMatchingAVP(msg, NULL, AVP_User_Name, vendorID,
					AAA_FORWARD_SEARCH);
	if(!avp_u)
		return NOT_USER_IN; /* no user name avp */	
	
	avp_g = AAAFindMatchingAVP(msg, NULL, AVP_User_Group, vendorID,
					AAA_FORWARD_SEARCH);
	if(!avp_u)
		return NOT_USER_IN; /* no user group avp */	
	
	query_len=select_len+avp_u->data.len+selectc_len+avp_g->data.len+5;

	query = (char*)shm_malloc(query_len*sizeof(char));
	if(query == 0)
		return NOT_USER_IN;

	pos=0;
	memcpy(query+pos, select, select_len);
	pos += select_len;
	memcpy(query+pos,avp_u->data.s, avp_u->data.len );
	pos += avp_u->data.len;
	memcpy(query+pos, selectc, selectc_len );
	pos += selectc_len;
	memcpy(query+pos,avp_g->data.s, avp_g->data.len );
	pos += avp_g->data.len;
	memcpy(query+pos, "\"\0", 2);

	if(!mysql) 
	{
		LOG( L_ERR, M_NAME": no db connection");
		return MYSQL_ERROR;
	}

	if( mysql_query( mysql, query ) ) 
	{
 		LOG(L_ERR, ": mysql_query error\n");
		return MYSQL_ERROR;
	}
	result = mysql_store_result(mysql);
	if(! result)
	{
		LOG(L_ERR, ": mysql_query error\n");
		return MYSQL_ERROR; /* the user does not exist */
	}		
	
	num_fields = mysql_num_fields(result);
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);
		
	if(!lengths) /* the user is not registerd for this domain */
		return NOT_USER_IN;

	DBG("User in\n");
	return USER_IN;	
}

/*
 *
 */
int do_acc( AAAMessage *msg )
{

	AAA_AVP *avp;
	
	char *query;
	int pos=0, post=0;
	int i, len=0, cnt_col=0, cnt_values=0;

	struct tm *tm;
	time_t timep;
	char query1[18] = "INSERT INTO acc (";
	int offset1 = 17;
	char query2[10] = " VALUES (";
	int offset2 = 9;
	char *tmp;
	
	for(i=0; i<avp_no; i++)
	{		
		avp = AAAFindMatchingAVP(msg, NULL, avpname[i], vendorID,
					AAA_FORWARD_SEARCH);
		if(avp)
		{
			values[i].len = avp->data.len;
			if( (values[i].s = (char*)shm_malloc(avp->data.len*sizeof(char))) == 0)
				return AAA_ERR_FAILURE;

			memcpy(values[i].s, avp->data.s, avp->data.len);
			cnt_values = cnt_values + values[i].len + 3;
			cnt_col = cnt_col + strlen(col[i])+1;
		}
		else
		{
			values[i].len = 0;
			values[i].s = NULL;
		}
	}
	
	timep = time(NULL);
	tm = gmtime(&timep);
	if( (values[i].s = (char*)shm_malloc(20*sizeof(char))) == 0)
		return AAA_ERR_FAILURE;

	strftime(values[i].s, 20, "%Y-%m-%d %H:%M:%S", tm);
	values[i].len = strlen(values[i].s);
	cnt_col = cnt_col + strlen(col[i])+1;
	cnt_values = cnt_values + values[i].len + 3;

	len = offset1 + cnt_col+ offset2 + cnt_values + 1;

	if( (query = (char*)shm_malloc(len*sizeof(char))) == 0)
		return AAA_ERR_FAILURE;

	memcpy(query+pos, query1, offset1);
	pos += offset1; 
	
	if( (tmp = (char*)shm_malloc(cnt_values*sizeof(char))) == 0)
	{
		shm_free(query);
		return AAA_ERR_FAILURE;
	}
	post = 0;
	for(i=0; i<col_no; i++)
		if(values[i].s)
		{
			memcpy(query+pos, col[i], col_len[i]);
			pos += col_len[i];
			memcpy(query+pos, ",", 1);
			pos ++;

			memcpy(tmp+post, "\"", 1); post ++;
			memcpy(tmp+post, values[i].s, values[i].len);
			post += values[i].len;
			memcpy(tmp+post, "\",", 2); post += 2;

			shm_free(values[i].s);
			values[i].s = NULL;
			values[i].len = 0;
		}
	*(query+pos-1) = ')';
	*(tmp+post-1)  = ')';
	
	memcpy(query+pos, query2, offset2);
	pos += offset2;
    memcpy(query+pos, tmp, post);		
	shm_free(tmp);
	pos += post;
	*(query+pos) = 0;

	if( mysql_query( mysql, query ) ) 
	{
 		LOG(L_ERR, M_NAME": mysql_query error '%.*s'\n", pos, query);
		shm_free(query);
		return AAA_ERR_FAILURE;
	}
	
	shm_free(query);
	return AAA_ERR_SUCCESS;		

}



