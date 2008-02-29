/*
 * $Id: db.h,v 1.1 2008/02/29 16:09:57 anomarme Exp $
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

#ifndef DB_H
#define DB_H

#include "../../../diameter_api/diameter_api.h"

#define AUTHORIZED 				1
#define NOT_AUTHORIZED 			2
#define REJECTED 				3
#define MYSQL_ERROR				4
#define NONCE_STALE				5
#define NOT_USER_IN				6
#define USER_IN					7
    
int init_db_connection();
int close_db_connection();
int do_acc( AAAMessage *message );
AAAReturnCode do_auth( AAAMessage *message, int ha, str *secret );
int check_group( AAAMessage *msg);


#endif
