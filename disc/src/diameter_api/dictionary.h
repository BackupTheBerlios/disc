/*
 * $Id: dictionary.h,v 1.1 2003/03/19 17:18:51 ilk Exp $
 *
 * 2003-03-03 created by Illya
 *
 */

#include "diameter_api.h"

#ifndef _AAA_DIAMETER_DICTIONARY_H
#define _AAA_DIAMETER_DICTIONARY_H
AAAReturnCode AAAFindDictionaryEntry(  AAAVendorId vendorId,
                                       char *avpName,
                                       AAA_AVPCode avpCode,
                                       AAAValue value,
                                       AAADictionaryEntry *entry);
int findEntry( char** entry);
int findWord( char* word);
int readWord(char* buf,int size);
int vendorNameFromId(AAAVendorId vendorId, char* vendorAttr);
#endif
