/*
 * 2003-02-17 created by illya (komarov@fokus.gmd.de)
 * $Id: dictionary.c,v 1.11 2003/03/26 17:18:45 ilk Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include "dictionary.h"

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
  return 0;
}

AAAReturnCode AAADictionaryEntryFromName( char *avpName,
                                          AAAVendorId vendorId,
														AAADictionaryEntry *entry){
  
   return AAAFindDictionaryEntry(vendorId,avpName,0,0,entry);
}

AAAValue AAAValueFromAVPCode(AAA_AVPCode avpCode,
		                         AAAVendorId vendorId,
	                           char *valueName){
  return 0;
}

const char *AAALookupValueNameUsingValue(AAA_AVPCode avpCode,
			                                   AAAVendorId vendorId,
			                                   AAAValue value){
  return 0;
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
      
   if(vendorId != 0){
      if(vendorNameFromId(vendorId,vendorAttr))
         return AAA_ERR_FAILURE; //wrong vendorId
   }
   //entry=(AAADictionaryEntry*)malloc(sizeof(AAADictionaryEntry));
   if(entry==NULL)
      return AAA_ERR_FAILURE;
   charEntry[0]=vendorAttr;
   charEntry[1]=avpName;
   if(avpCode==0)
      charEntry[2]=NULL;
   else{
      charEntry[2]=(char*)malloc(10);
      sprintf(charEntry[2],"%i",avpCode);
   }
   charEntry[3]=NULL;
   
   if(!findEntry(charEntry,4))
      return AAA_ERR_FAILURE;
   entry->vendorId=vendorId;
   entry->avpName=(char*)malloc(strlen(charEntry[1]+1));
   strcpy(entry->avpName,charEntry[1]);
   entry->avpCode=atoi(charEntry[2]);
	char* dataTypes[]={"Grouped"} ;
   for(i=0;i<sizeof(dataTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],dataTypes[i]))
			entry->avpType=AAA_AVP_DATA_TYPE;
   char* octetStrings[]={"OctetString","UTF8String","DiameterIdentity"} ;  
   for(i=0;i<sizeof(octetStrings)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],octetStrings[i]))
			entry->avpType=AAA_AVP_STRING_TYPE;
	char* addressTypes[]={"IPAddress","DiameterURI","IPFilterRule"} ;
   for(i=0;i<sizeof(addressTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],addressTypes[i]))
			entry->avpType=AAA_AVP_ADDRESS_TYPE;
	char* integer32Types[]={"Interger32","Unsigned32","Enumerated"} ;
   for(i=0;i<sizeof(integer32Types)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],integer32Types[i]))
			entry->avpType=AAA_AVP_INTEGER32_TYPE;
	char* integer64Types[]={"Interger64","Unsigned64"} ;
   for(i=0;i<sizeof(integer64Types)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],integer64Types[i]))
			entry->avpType=AAA_AVP_INTEGER64_TYPE;
	char* timeTypes[]={"Time"} ;
   for(i=0;i<sizeof(timeTypes)/sizeof(char*);i++)
   	if(!strcasecmp(charEntry[4],timeTypes[i]))
			entry->avpType=AAA_AVP_TIME_TYPE;
      return AAA_ERR_SUCCESS;
   

   
}
int findEntry( char** extEntry, int sizeOfEntry){
   file=fopen("dictionary","r");
	if(file==NULL){
      //LOG(L_ERR,"ERROR:no dictionary found!\n");
      return 0;
   }
   int s=0;
   int match=0;
   char *entry[sizeOfEntry];
   while(!match && findWord(extEntry[0])){
      int i;
      int size=MAXWORDLEN;
      char* buf=(char*)malloc(size);
      for(i=1; i < sizeOfEntry; i++){
         size=MAXWORDLEN;
         if((size=readWord(buf,size))){
            if(extEntry[i]==NULL){
               entry[i]=(char*)malloc(size+1);
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
               	entry[i]=(char*)malloc(size+1);
               	strcpy(entry[i],buf);
                	//printf("%s\n",buf);
					}    
			}
      }
		free(buf);
   }
   fclose(file);
   if(match==1){
		int i;
   	for(i=0;i<sizeOfEntry;i++)
			if(extEntry[i]==NULL){
				extEntry[i]=(char*)malloc(strlen(entry[i])+1);
				strcpy(extEntry[i],entry[i]);
			}
	}
   return match;
}
int findWord( char* word){
   char num=0;
   int match = 0;
   while((!match) && (num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' && num!=EOF);
      else{
         if(num == word[0]){
				char buff[strlen(word)+1];
            buff[0]=num;
            int i;
            match = 1; 
            for(i=1; i<strlen(word); i++){
               if((buff[i]=fgetc(file))=='\n' || buff[i]==EOF || buff[i]!= word[i]){
                  match = 0;
                  break;
               }
					match = 1;
            }  
            buff[strlen(word)]=0;
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
   file=fopen("vendors","r");
	if(file==NULL){
      //LOG(L_ERR,"ERROR:no vendors file found!\n");
      return 0;
   }
   char num;
   char* vendorValue=NULL;
   char* vendorName=NULL;
   int vid;
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
