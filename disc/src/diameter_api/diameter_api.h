/*
 * $Id: diameter_api.h,v 1.3 2003/03/19 16:55:49 ilk Exp $
 *
 * 2002-10-04 created by illya (komarov@fokus.gmd.de)
 *
 */


#ifndef _AAA_DIAMETER_API_H
#define _AAA_DIAMETER_API_H

#include "diameter_types.h"



AAAReturnCode AAAOpen(
	char *configFileName);


AAAReturnCode AAAClose();


const char *AAAGetDefaultConfigFileName();


/*AAACallbackHandle *AAARegisterCommandCallback(
	AAACommandCode       commandCode,
	AAAVendorId          vendorId,
	char                 *commandName,
	AAAExtensionId       extensionId,
	AAACallback          callback,
	AAACallbackLocation  position);


AAACallbackHandle AAARegisterNoncommandCallback(
	AAACallback callback,
	AAACallbackLocation position);


AAAReturnCode AAADeregisterCommandCallback(
	AAACallbackHandle *handle);


AAAReturnCode AAADeregisterNoncommandCallback(
	AAACallbackHandle *handle);


AAAReturnCode AAARegisterExtension(
	AAAExtensionId extensionId);*/


AAAReturnCode AAAStartSession(
		AAASessionId **sessionId,
		AAAApplicationId appHandle,
		char *userName,
		AAACallback abortCallback);

/*  AAAReturnCode AAARegisterPeerSession(AAASessionId **sessionId,
				                               AAAApplicationId *appHandle,   				
                               				 AAAMessage *message,
				                               char *userName,
					                             char *hostName);
  AAAReturnCode AAAEndSession(AAASessionId *sessionId);
  AAAReturnCode AAAAbortSession(AAASessionId *sessionId);
  AAAServer *AAALookupServer(IP_ADDR ipAddr);
  AAAReturnCode AAASetSessionMessageTimeout(AAASessionId *id,
                                            time_t timeout);
  AAAResultCode  AAAGetDomainInterconnectType(AAAMessage *message,
																							char *domainName,
				                                      char *type);
*/
AAAReturnCode AAADictionaryEntryFromAVPCode(AAA_AVPCode avpCode,
				                                AAAVendorId vendorId,
   				                              AAADictionaryEntry *entry);
AAAValue AAAValueFromName(char *avpName,
		                        char *vendorName,
		                        char *valueName);
AAAReturnCode AAADictionaryEntryFromName(char *avpName,
													     AAAVendorId vendorId,
														 AAADictionaryEntry *entry);
AAAValue AAAValueFromAVPCode(AAA_AVPCode avpCode,
			                         AAAVendorId vendorId,
		                           char *valueName);
const char *AAALookupValueNameUsingValue(AAA_AVPCode avpCode,
				                                   AAAVendorId vendorId,
				                                   AAAValue value);
//Insert the AVP avp into this avpList after position
boolean_t AAAGetCommandCode(char *commandName,
		                          AAACommandCode *commandCode,
		                          AAAVendorId *vendorId);

AAAMessage *AAANewMessage(
		AAACommandCode commandCode,
		AAAVendorId vendorId,
		AAASessionId *sessionId,
		AAAExtensionId extensionId,
		AAAMessage *request);

AAAReturnCode AAAFreeMessage(
		AAAMessage **message);

AAAReturnCode AAARespondToMessage(
		AAAMessage* message,
		AAACommandCode commandCode,
		AAAVendorId vendorId,
		AAAResultCode resultCode);

AAAReturnCode AAAAddProxyState(
		AAAMessage *message);

AAAReturnCode AAACreateAVP(
		AAA_AVP **avp,
		AAA_AVPCode code,
		AAA_AVPFlag flags,
		AAAVendorId vendorId,
		char *data,
		size_t length);

AAAReturnCode AAACreateAndAddAVPToList(
		AAA_AVP_LIST **avpList,
		AAA_AVPCode code,
		AAA_AVPFlag flags,
		AAAVendorId vendorId,
		char *data,
		size_t length);

AAAReturnCode AAAAddAVPToList(
		AAA_AVP_LIST **avpList,
		AAA_AVP *avp,
		AAA_AVP *position);

AAA_AVP *AAAFindMatchingAVP(
		AAA_AVP_LIST *avpList,
		AAA_AVP *startAvp,
		AAA_AVPCode avpCode,
		AAAVendorId vendorId,
		AAASearchType searchType);

AAAReturnCode AAAJoinAVPLists(
		AAA_AVP_LIST *dest,
		AAA_AVP_LIST *source,
		AAA_AVP      *position);

AAAReturnCode AAARemoveAVPFromList(
		AAA_AVP_LIST *avpList,
		AAA_AVP *avp);

AAAReturnCode AAAFreeAVP(
		AAA_AVP **avp);

AAA_AVP* AAAGetFirstAVP(
		AAA_AVP_LIST *avpList);

AAA_AVP* AAAGetLastAVP(
		AAA_AVP_LIST *avpList);

AAA_AVP* AAAGetNextAVP(
		AAA_AVP *avp);

AAA_AVP* AAAGetPrevAVP(
		AAA_AVP *avp);

char *AAAConvertAVPToString(
		AAA_AVP *avp,
		char *dest,
		size_t destLen);

AAAResultCode AAASetMessageResultCode(
		AAAMessage *message,
		AAAResultCode resultCode);

AAAReturnCode  AAASetServer(
		AAAMessage *message,
		IP_ADDR host);

AAAReturnCode AAASendMessage(
		AAAMessage *message);

AAAReturnCode AAASendAcctRequest(
		AAASessionId *aaaSessionId,
		AAAExtensionId extensionId,
		AAA_AVP_LIST *acctAvpList,
		AAAAcctMessageType msgType);

AAASecurityStatus AAAGetPeerSecurityStatus(
		IP_ADDR remoteHost);

AAAMessage* AAATranslateMessage(
		unsigned char* source,
		size_t sourceLen);

#endif

