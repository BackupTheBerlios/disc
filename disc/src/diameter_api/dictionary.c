/*
 * 2003-02-17 created by illya (komarov@fokus.gmd.de)
 * $Id: dictionary.c,v 1.7 2003/03/19 16:55:49 ilk Exp $
 */
#include <stdio.h>
#include "diameter_api.h"

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
}

AAAReturnCode AAADictionaryEntryFromName( char *avpName,
                                          AAAVendorId vendorId,
														AAADictionaryEntry *entry){
  
   return AAAFindDictionaryEntry(vendorId,avpName,0,0,entry);
}

AAAValue AAAValueFromAVPCode(AAA_AVPCode avpCode,
		                         AAAVendorId vendorId,
	                           char *valueName){
}

const char *AAALookupValueNameUsingValue(AAA_AVPCode avpCode,
			                                   AAAVendorId vendorId,
			                                   AAAValue value){
}

boolean_t AAAGetCommandCode(char *commandName,
	                          AAACommandCode *commandCode,
	                          AAAVendorId *vendorId){
}

AAAReturnCode AAAFindDictionaryEntry(  AAAVendorId vendorId,
                                       char *avpName,
                                       AAA_AVPCode avpCode,
                                       AAAValue value,
                                       AAADictionaryEntry *entry){
   char vendorName[]="ATTRIBUTE";
   char* vendorAttr=vendorName;
   char* charEntry[4];
      
   if(vendorId != 0){
      if(vendorNameFromId(vendorId,vendorAttr))
         return AAA_ERR_FAILURE; //wrong vendorId
   }
   entry=(AAADictionaryEntry*)malloc(sizeof(AAADictionaryEntry));
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
   if(!findEntry(charEntry))
      return AAA_ERR_FAILURE;
   entry->vendorId=vendorId;
   strcpy(entry->avpName,charEntry[1]);   
   entry->avpCode=atoi(charEntry[2]);
   //entry->avpType=
      return AAA_ERR_SUCCESS;
   

   
}
int findEntry( char** entry){
   FILE* file;
	file=fopen("dictionary","r");
	if(file==NULL){
      LOG(L_ERR,"ERROR:no dictionary found!\n");
      return 0;
   }
   int match=0;
   while(!match && findWord(entry[0])){
      int i;
      int size=MAXWORDLEN;
      char* buf;
      for(i=1; i<sizeof(entry); i++){
         if((size=readWord(buf,size))){
            if(entry[i]==NULL){
               entry[i]=(char*)malloc(size);
               strcpy(entry[i],buf);
               match=1;
            }
            else
               if(strcmp(entry[i],buf)){
                  match=0;
                  break;
               }
         }
      }
   }
   return match;
}
int findWord( char* word){
   char num;
   int match = 0;
   while(!match ||(num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' || num!=EOF);
      else{
         if(num == word[0]){
            char buf[strlen(word)];
            buf[0]=num;
            int i;
            match = 1;
            for(i=1; i<strlen(word); i++){
               if((buf[i]=fgetc(file))=='\n' || buf[i]==EOF){
                  match = 0;
                  break;
               }
               match = 1;
            }
            buf[strlen(word)]=0;
         }
     }
   }
   return match;
}

int readWord(char* buf,int size){
   int realSize;
   char num;
   int i = 0;
   buf=(char*)malloc(size);
   while ((num=fgetc(file))==' ' || num=='\t' )
      if(num =='\n' || num==EOF)
         return 0;
   while ((num=fgetc(file))!=' ' || num!='\t' || num!='\n' || num!=EOF ){
      buf[i++]=num;
   }
   return strlen(buf);

}

int vendorNameFromId(AAAVendorId vendorId, char* vendorAttr){
   FILE* file;
	file=fopen("vendors","r");
	if(file==NULL){
      LOG(L_ERR,"ERROR:no vendors file found!\n");
      return 0;
   }
   char num;
   char* vendorValue;
   char* vendorName;
   int vid;
   while((num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' || num!=EOF);
      else{
         ungetc(num,file);
         if(fscanf(file,"%s %s %i %s\n",vendorAttr,vendorValue,vid,vendorName)==4){
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
