/*
 * $Id: dictionary.h,v 1.4 2003/04/28 13:58:18 ilk Exp $
 *
 * 2003-03-03 created by Illya
 *
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
 */


#include "diameter_api.h"

#ifndef _AAA_DIAMETER_DICTIONARY_H
#define _AAA_DIAMETER_DICTIONARY_H
AAAReturnCode AAAFindDictionaryEntry(  AAAVendorId vendorId,
                                       char *avpName,
                                       AAA_AVPCode avpCode,
                                       AAAValue value,
                                       AAADictionaryEntry *entry);
                                       
AAAReturnCode AAAFindValue(  char* vendorName,
										char* avpName,
										char** valueName,
										AAAValue* value);
										                                       
int findEntry( char** entry,int sizeOfEntry);
int findWord( char* word);    
int readWord(char* buf,int size);
int vendorNameFromId(AAAVendorId vendorId, char* vendorAttr);
#endif
