/*
 * $Id: sipauth.c,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>

#include "../../../dprint.h"
#include "../../../aaa_module.h"
#include "../../../mem/shm_mem.h"
#include "../../../diameter_api/diameter_api.h"

#include "sipauth.h"
#include "sipaaa_defs.h"

#include "digest_parser.h"
#include "challenge.h"
#include "db.h"
#include "nonce.h"

#define M_NAME "sipauth"

#define RAND_SECRET_LEN	32
#define MAX_LINE_LEN 255

char* sec_rand = 0;
str secret;

static int mod_init();
static void mod_destroy();
static int mod_msg(AAAMessage *msg, void *context);
static int mod_tout(int event, AAASessionId* sId, void *context);

/* shared-secred parameter */
char* sec_param = 0;

/* computeHA1=1 if HA1 is stored i the database */
int computeHA1 = 1;

/* 
 * qop=QOP_UNSPEC,	qop directive not present
 * qop=QOP_AUTH,	qop=auth
 * qop=QOP_AUTHINT,	qop=auth-int
 */
int qop = QOP_UNSPEC;

/* Nonce lifetime */
int nonce_expire = 300;

char *authDBUserName = 0 ;
char *authDBUserPass = 0;
char *authDBHost = 0;
char *authDB = 0;

static struct module_param params[]={
	{"secret",  STR_TYPE, &sec_param},
	{"qop",		INT_TYPE, &qop},
	{"computeHA1", INT_TYPE, &computeHA1},
	{"nonce_expire", INT_TYPE, &nonce_expire},
	{"db_user_name", STR_TYPE, &authDBUserName},
	{"db_user_pass", STR_TYPE, &authDBUserPass},
	{"db_host", STR_TYPE, &authDBHost},
	{"db_name", STR_TYPE, &authDB},
	{0,0,0} /* null terminated array */
};

struct module_exports exports = {
	"sipauth",
	AAA_SERVER,
	AAA_APP_NASREQ,
	DOES_AUTH | DOES_ACCT | RUN_ON_REPLIES,
	params,
	
	mod_init,
	mod_destroy,
	mod_msg,
	mod_tout
};


/*
 * Secret parameter was not used so we generate a random value here
 */
static inline int generate_random_secret(void)
{
	int i;

	sec_rand = (char*)shm_malloc(RAND_SECRET_LEN);
	if (!sec_rand) 
	{
		LOG(L_ERR, M_NAME": generate_random_secret(): No memory left\n");		
		return -1;
	}

	srandom(time(0));

	for(i = 0; i < RAND_SECRET_LEN; i++) 
		sec_rand[i] = 32 + (int)(95.0 * rand() / (RAND_MAX + 1.0));
		
	secret.s = sec_rand;
	secret.len = RAND_SECRET_LEN;

	return 0;
}


static int mod_init()
{
	DBG(M_NAME" module : initializing...\n");

	/* If the parameter was not used */
	if (sec_param == 0) 
	{
		/* Generate secret using random generator */
		if (generate_random_secret() < 0) 
		{
			LOG(L_ERR, M_NAME": random secret not generated\n");
			return -1;
		}
	} 
	else 
	{
		/* Otherwise use the parameter's value */
		secret.s = sec_param;
		secret.len = strlen(secret.s);
	}

	DBG(M_NAME" module : DB connecting ...\n");
	
	if(init_db_connection() != AAA_ERR_SUCCESS)	
	{
		LOG(L_ERR, M_NAME": Error while connecting to db\n");
		return -1;
	}
	DBG(M_NAME" module : DB connected ...\n");

	return 0;
}

static void mod_destroy()
{
	close_db_connection();
}

static int mod_msg(AAAMessage *msg, void *context)
{
	AAAMessage *ans;
	AAA_AVP *avp, *last;
	char *challenge = 0;
	int ret;

	if ( ! is_req(msg) ) 
		return 1;

	switch ( msg->commandCode )
	{
		case AA_REQUEST:
			ans=AAANewMessage( AA_ANSWER, AAA_APP_NASREQ, msg->sId, msg);
			if (ans==0)
				return -1;
			break;

		case ACCOUNTING_REQUEST :
	 		ans=AAANewMessage(ACCOUNTING_ANSWER, AAA_APP_NASREQ, msg->sId, msg);
			if (ans==0)
				return -1;
			break;

		default:
			DBG(M_NAME": Unknown message type\n");
			return 1; /* drop the message */
	}

	avp = AAAFindMatchingAVP(msg, NULL, AVP_Service_Type,  vendorID,
					AAA_FORWARD_SEARCH);
	if(!avp)
		return 1; /* message not well formed, drop it */
	

	switch(avp->data.s[0])
	{
		case SIP_AUTH_SERVICE:
			ret=do_auth(msg, computeHA1, &secret);
			switch(ret)
			{
				case AUTHORIZED:
					if( AAASetMessageResultCode(ans, AAA_SUCCESS)
													!=AAA_ERR_SUCCESS)
						goto error;		
				break;

				case NOT_AUTHORIZED:
				case NONCE_STALE:
					if( AAASetMessageResultCode(ans,AAA_AUTHENTICATION_REJECTED)
											!= AAA_ERR_SUCCESS)
						goto error;		
							
					/* generate a chalenge */
					challenge = build_challenge(ret==NONCE_STALE, 
												msg->dest_realm->data, qop);
					if(!challenge)
					{
						LOG(L_ERR, M_NAME": challenge not created\n");
						goto error;
					}
			
					if( (avp=AAACreateAVP(AVP_Chalenge, 0, 0, challenge,
									strlen(challenge), AVP_FREE_DATA)) == 0)
					{
						LOG(L_ERR,M_NAME": AVP_Challenge not created\n");
						goto error;
					}
		
					last = AAAGetLastAVP(&(ans->avpList));
					if( AAAAddAVPToMessage(ans, avp, last)!= AAA_ERR_SUCCESS)
					{
						LOG(L_ERR, M_NAME": AVP_Challenge not added \n");
						AAAFreeAVP(&avp);
						goto error;
					}
					break;
				
				case REJECTED:
					if( AAASetMessageResultCode(ans, AAA_AUTHORIZATION_REJECTED)
												 != AAA_ERR_SUCCESS)
						goto error;		
					break;

				case MYSQL_ERROR:
					LOG(L_ERR, M_NAME":  Error to mysql\n");
					AAASetMessageResultCode(ans, AAA_AUTHORIZATION_REJECTED);
					return -1;			

				default:
					break;
			}
			break;
		case SIP_GROUP_SERVICE: /* GROUP CKECK */
			if(check_group(msg)==USER_IN)
			{
				if( AAASetMessageResultCode( ans, AAA_SUCCESS) 
												!= AAA_ERR_SUCCESS)
					goto error;		
			}
			else
			{
				if( AAASetMessageResultCode( ans, AAA_AUTHENTICATION_REJECTED)
										 		!= AAA_ERR_SUCCESS)
					goto error;		
			}
			break;
		
		case SIP_ACC_SERVICE:
			if(do_acc(msg)==AAA_ERR_SUCCESS) //all went OK
			{
				if( AAASetMessageResultCode( ans, AAA_SUCCESS) 
												!= AAA_ERR_SUCCESS)
					goto error;
			}
			else
			{
				if( AAASetMessageResultCode( ans, AAA_AUTHENTICATION_REJECTED)
										 		!= AAA_ERR_SUCCESS)
					goto error;
			}
			break;
		default:
			return 1;
	}	
	

#ifdef DEBUG
	AAAPrintMessage(ans);
#endif
	AAASendMessage(ans);
	AAAFreeMessage(&ans);
	return 1;
	
error:
	LOG(L_ERR, M_NAME": error occured\n");
	AAAFreeMessage(&ans);
	return -1;
}

static int mod_tout(int event, AAASessionId* sId, void *context)
{
	AAAEndSession( sId );
	return 1;
}


