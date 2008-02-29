/*
 * $Id: sipaaa_defs.h,v 1.1 2008/02/29 16:02:38 anomarme Exp $
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

#ifndef SIPAAA_DEFS_H
#define SIPAAA_DEFS_H

#define SIP_AUTH_SERVICE 	'6'
#define SIP_GROUP_SERVICE	'8'
#define SIP_ACC_SERVICE		'9'

#define AA_REQUEST			265
#define AA_ANSWER			265
#define ACCOUNTING_REQUEST	271
#define ACCOUNTING_ANSWER	271


#define AAA_SIP_OK			"0"
#define AAA_SIP_CHALENGE	"1"
#define AAA_SIP_REJECTED	"2"
#define AAA_SIP_SRVERR		"3"
#define AAA_SIP_LEN			 1

#define APP_ID_1			"1"
#define APP_ID_1_LEN			 1

#define vendorID	0

#define MAXMSG		512

enum{

	AVP_Resource					=  400,
	AVP_Response					=  401,	
	AVP_Chalenge					=  402,
	AVP_Method						=  403,
	AVP_Service_Type				=  404,
	AVP_User_Group					=  405,
	AVP_SIP_MSGID					=  406	
};



#endif
