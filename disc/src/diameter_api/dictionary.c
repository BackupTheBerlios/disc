/*
 * 2003-02-17 created by illya (komarov@fokus.gmd.de)
 *
 */
#include <stdio.h>
#include "diameter_types.h"

#define MAXWORDLEN 1024;
#if 0
FILE* file;

AAAReturnCode AAADictionaryEntryFromAVPCode(AAA_AVPCode avpCode,
						                                AAAVendorId vendorId,
							                              AAADictionaryEntry *entry){
}

AAAValue AAAValueFromName(char *avpName,
	                        char *vendorName,
	                        char *valueName){
}

AAAReturnCode AAADictionaryEntryFromName(char *avpName,
													               AAAVendorId vendorId,
																				 AAADictionaryEntry *entry){
}

AAAValue AAAValueFromAVPCode(AAA_AVPCode avpCode,
		                         AAAVendorId vendorId,
	                           char *valueName){
}

const char *AAALookupValueNameUsingValue(AAA_AVPCode avpCode,
			                                   AAAVendorId vendorId,
			                                   AAAValue value){
}

boolean_t AAAGetCommandCode(char *commandNamInsert the AVP avp into this avpList after positione,
	                          AAACommandCode *commandCode,
	                          AAAVendorId *vendorId){
}

AAAReturnCode AAAFindDictionaryEntry(  AAAVendorId vendorId,
                                       char *avpName
                                       AAA_AVPCode avpCode,
                                       AAAValue value){

   if(vendorId != NULL){
      AAAFindDictionaryEntryWord(vendorName+"ATTRIBUTE",avpName,avpCode); 
   }  
   else
      AAAFindDictionaryEntryWord("ATTRIBUTE",avpName,avpCode);
   
}
bool FindEntry( char* entry){
   FILE* file;
	entry=malloc(sizeof(AAADictionaryEntry));
   if(entry==NULL)
      return AAA_ERR_NOMEM;
	file=fopen("dictionary","r");
	if(file==NULL){
      LOG(L_ERR,"ERROR:no dictionary found!\n");
      return AAA_ERR_FAILURE;
   }
   bool match=false;
   while(match!=true && findWord(firstWord)){
      int i;
      int size=MAXWORDLEN;
      char* buf;
      for(i=1; i<sizeof(entry); i++){
         if((size=readWord(buf,size))){
            if(entry[i]==NULL){
               entry[i]=malloc(size);
               strcpy(entry,buf);
               match=true;   
            }
            else
               if(strcmp(entry[i],buf)){
                  match=false;
                  break;
               }
         }
      }
   }
   return match;
}
bool findWord( char* word){
   char num;
   bool match = false;
   while(match!=true ||(num=fgetc(file))!=EOF){
      if(num == '#')
         while ((num=fgetc(file))!='\n' || num!=EOF);
      else{
         if(num == word[0]){
            char buf[strlen(word)];
            buf[0]=num;
            int i;
            match = true;
            for(i=1; i<strlen(word); i++){
               if((buf[i]=fgetc(file))=='\n' || buf[i]==EOF){
                  match = false;
                  break;
               }
               match = true;
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
   buf=(char*)malloc(size);
   while ((num=fgetc(file))==' ' || num=='\t' )
      if(num =='\n' || num==EOF)
         return 0;
   while ((num=fgetc(file))!=' ' || num!='\t' || num!='\n' || num!=EOF ){
      buf(i++)=num;     
   }
   return strlen(buf);
      
}
#endif
