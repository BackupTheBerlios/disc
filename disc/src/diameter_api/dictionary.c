/*
 * 2003-02-17 created by illya (komarov@fokus.gmd.de)
 * $Id: dictionary.c,v 1.17 2003/04/28 14:29:14 ilk Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dictionary.h"
#include "../mem/shm_mem.h"

#define MAXWORDLEN 1024;
FILE* file;

//#if 0


AAAReturnCode AAADictionaryEntryFromAVPCode(AAA_AVPCode avpCode,
						                                AAAVendorId vendorId,
							                              AAADictionaryEntry *entry){
                                                
   return AAAFindDictionaryEntry(vendorId,NULL,avpCode,0,entry);                                                
}

AAAValue AAAValueFromName(char *avpName,
	                        char *vendorName,
	                        char *valueName){
									  
   AAAValue value=0;
   if(AAAFindValue( vendorName,avpName,&valueName,&value)== AAA_ERR_SUCCESS)
		return value;
	else
		return AAA_ERR_NOT_FOUND;
}

AAAReturnCode AAADictionaryEntryFromName( char *avpName,
                                          AAAVendorId vendorId,
														AAADictionaryEntry *entry){
  
   return AAAFindDictionaryEntry(vendorId,avpName,0,0,entry);
}

AAAValue AAAValueFromAVPCode(AAA_AVPCode avpCode,
		                         AAAVendorId vendorId,
	                           char *valueName){
   AAAValue value=0;
   AAADictionaryEntry *entry;
   AAAReturnCode ret;
   entry=(AAADictionaryEntry*)shm_malloc(sizeof(AAADictionaryEntry));
   if (AAADictionaryEntryFromAVPCode(avpCode,vendorId,entry)==AAA_ERR_SUCCESS){
		ret=AAAFindValue( NULL,entry->avpName,&valueName,&value);
      shm_free(entry);
		if(ret== AAA_ERR_SUCCESS)
			return value;
		else
			return AAA_ERR_NOT_FOUND;
	}
	else{
		shm_free(entry);
		return AAA_ERR_NOT_FOUND;
	}

}

const char *AAALookupValueNameUsingValue(AAA_AVPCode avpCode,
			                                   AAAVendorId vendorId,
			                                   AAAValue value){
   char* valueName=NULL;
   AAADictionaryEntry *entry;
   AAAReturnCode ret;
   entry=(AAADictionaryEntry*)shm_malloc(sizeof(AAADictionaryEntry));
   if (AAADictionaryEntryFromAVPCode(avpCode,vendorId,entry)==AAA_ERR_SUCCESS){
		ret=AAAFindValue( NULL,entry->avpName,&valueName,&value);
		shm_free(entry);
		if(ret==AAA_ERR_SUCCESS)
			return valueName;
		else
			return NULL;
	}
	else{
		shm_free(entry);
		return NULL;
	}
}

boolean_t AAAGetCommandCode(char *commandName,
	                          AAACommandCode *commandCode,
	                          AAAVendorId *vendorId){
   return 0;
}

AAAReturnCode AAAFindDictionaryEntry(  AAAVendorId vendorId,
                                       char *avpName,
                                       AAA_AVPCode avpCode,
                                       AAAValue value,
                                       AAADictionaryEntry *entry){
   char vendorName[]="ATTRIBUTE";
   char* vendorAttr=vendorName;
   char* charEntry[4];
   int i;
	char* dataTypes[]={"Grouped"} ;
	char* octetStrings[]={"OctetString","UTF8String","DiameterIdentity"} ;
	char* addressTypes[]={"IPAddress","DiameterURI","IPFilterRule"} ;
	char* integer32Types[]={"Interger32","Unsigned32","Enumerated"} ;
	char* integer64Types[]={"Interger32","Unsigned32"} ;
	char* timeTypes[]={"Time"} ;
   if(vendorId != 0){
      if(vendorNameFromId(vendorId,vendorAttr))
         return AAA_ERR_FAILURE; //wrong vendorId
   }
   //entry=(AAADictionaryEntry*)shm_malloc(sizeof(AAADictionaryEntry));
   if(entry==NULL)
      return AAA_ERR_FAILURE;
   charEntry[0]=vendorAttr;
   charEntry[1]=avpName;
   if(avpCode==0)
      charEntry[2]=NULL;
   else{
      charEntry[2]=(char*)shm_malloc(10);
      sprintf(charEntry[2],"%i",avpCode);
   }
   charEntry[3]=NULL;

   if(!findEntry(charEntry,4))
      return AAA_ERR_FAILURE;
   entry->vendorId=vendorId;
   entry->avpName=(char*)shm_malloc(strlen(charEntry[1]+1));
   strcpy(entry->avpName,charEntry[1]);
   entry->avpCode=atoi(charEntry[2]);
   shm_free(charEntry[2]);
   for(i=0;i<sizeof(dataTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],dataTypes[i]))
			entry->avpType=AAA_AVP_DATA_TYPE;
   for(i=0;i<sizeof(octetStrings)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],octetStrings[i]))
			entry->avpType=AAA_AVP_STRING_TYPE;
   for(i=0;i<sizeof(addressTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],addressTypes[i]))
			entry->avpType=AAA_AVP_ADDRESS_TYPE;
   for(i=0;i<sizeof(integer32Types)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],integer32Types[i]))
			entry->avpType=AAA_AVP_INTEGER32_TYPE;
   for(i=0;i<sizeof(integer64Types)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],integer64Types[i]))
			entry->avpType=AAA_AVP_INTEGER64_TYPE;
   for(i=0;i<sizeof(timeTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[3],timeTypes[i]))
			entry->avpType=AAA_AVP_TIME_TYPE;
      return AAA_ERR_SUCCESS;



}
AAAReturnCode AAAFindValue(  char* vendorName,
										char* avpName,
                              char** valueName,
                              AAAValue* value){
   char* charEntry[4];
   char vendorVal[]="VALUE";
   charEntry[0]=vendorVal;
   charEntry[1]=avpName;
   charEntry[2]=*valueName;
   if(*value==0)
   	charEntry[3]=NULL;
   else{
		charEntry[3]=(char*)shm_malloc(10);
      sprintf(charEntry[3],"%i",*value);
   }
	if(!findEntry(charEntry,4))
      return AAA_ERR_NOT_FOUND;
   *valueName=(char*)shm_malloc(strlen(charEntry[2])+1);
   strcpy(*valueName,charEntry[2]);
	*value=atoi(charEntry[3]);
	shm_free(charEntry[3]);
   return AAA_ERR_SUCCESS;
}

int findEntry( char** extEntry, int sizeOfEntry){
	int match=0;
   char *entry[sizeOfEntry];
   int i;
   int size;
   char* buf;
   file=fopen("dictionary","r");
	if(file==NULL){
      //LOG(L_ERR,"ERROR:no dictionary found!\n");
      return 0;
   }
   while(!match && findWord(extEntry[0])){
      size=MAXWORDLEN;
      buf=(char*)shm_malloc(size);
      for(i=1; i < sizeOfEntry; i++){
         size=MAXWORDLEN;
         if((size=readWord(buf,size))){
            if(extEntry[i]==NULL){
               entry[i]=(char*)shm_malloc(size+1);
               strcpy(entry[i],buf);
               match=1;
               //printf("%s\n",buf);
            }
            else
            	if(strcmp(extEntry[i],buf)){
                  match=0;
                  break;
               }
               else{
               	entry[i]=(char*)shm_malloc(size+1);
               	strcpy(entry[i],buf);
                	//printf("%s\n",buf);
					}
			}
      }
		shm_free(buf);
   }
   fclose(file);
   if(match==1){
		int i;
   	for(i=0;i<sizeOfEntry;i++)
			if(extEntry[i]==NULL){
				extEntry[i]=(char*)shm_malloc(strlen(entry[i])+1);
				strcpy(extEntry[i],entry[i]);
				shm_free(entry[i]);
			}
	}
   return match;
}
int findWord( char* word){
   char num=0;
   int match = 0;
   int i;
   while((!match) && (num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' && num!=EOF);
      else{
         if(num == word[0]){
            match = 1; 
            for(i=1; i<strlen(word); i++){
               if((num=fgetc(file))=='\n' || num==EOF || num!= word[i]){
                  match = 0;
                  break;
               }
					match = 1;
            }  
			} 
		}
	}
   return match;
}

int readWord(char* buf,int size){
   char num=0;
   int i = 0;
   while ((num=fgetc(file))==' ' || num=='\t' )
      if(num =='\n' || num==EOF)
         return 0;
	buf[i++]=num;
   while ((num=fgetc(file))!=' ' && num!='\t' && num!='\n' && num!=EOF && i<size ){
      buf[i++]=num;
   }
   buf[i]=0;
   return strlen(buf);

}

int vendorNameFromId(AAAVendorId vendorId, char* vendorAttr){
	char num;
   char* vendorValue=NULL;
   char* vendorName=NULL;
   int vid;
   file=fopen("vendors","r");
	if(file==NULL){
      //LOG(L_ERR,"ERROR:no vendors file found!\n");
      return 0;
   }
   while((num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' || num!=EOF);
      else{
         ungetc(num,file);
         if(fscanf(file,"%s %s %i %s\n",vendorAttr,vendorValue,&vid,vendorName)==4){
            if(vid==vendorId){
               return 1;
            }
         }
         else
            while ((num=fgetc(file))!='\n' || num!=EOF);
      }
   }
   return 0;
}
