/*
 * $Id: diameter_api.h,v 1.9 2003/04/11 17:48:02 bogdan Exp $
 *
 * 2002-10-04 created by illya (komarov@fokus.gmd.de)
 *
 */


#ifndef _AAA_DIAMETER_API_H
#define _AAA_DIAMETER_API_H

#include "../diameter_msg/diameter_msg.h"




/****************************** TYPES ***************************************/


/* types of timeout - passed when a Tout handler is called
 */
#define ANSWER_TIMEOUT_EVENT  1
#define SESSION_TIMEOUT_EVENT 2


/*  */
typedef enum {
	_B_FALSE,
	_B_TRUE
}boolean_t;


/* possible identities for AAA parties */
enum AAA_STATUS {
	AAA_UNDEFINED = 0,
	AAA_CLIENT = 1,
	AAA_SERVER = 2,
	AAA_SERVER_STATELESS = 2,
	AAA_SERVER_STATEFULL = 3,
};


/* The following defines the possible security characteristics for a host
 */
typedef enum {
	AAA_SEC_NOT_DEFINED = -2,
	AAA_SEC_NOT_CONNECTED = -1,
	AAA_SEC_NO_SECURITY = 0,
	AAA_SEC_CMS_SECURITY = 1,
	AAA_SEC_CMS_PROXIED = 2
} AAASecurityStatus;


/* The following structure is returned by the dictionary entry lookup
 * functions */
typedef struct dictionaryEntry {
	AAA_AVPCode    avpCode;
	char*          avpName;
	AAA_AVPDataType     avpType;
	AAAVendorId    vendorId;
	AAA_AVPFlag    flags;
} AAADictionaryEntry;



/*********************************** FUNCTIONS *******************************/


#define get_my_appref() \
	((AAAApplicationRef)&exports)


AAAReturnCode AAAOpen();


AAAReturnCode AAAClose();


AAAReturnCode AAAStartSession(
		AAASessionId **sessionId,
		AAAApplicationRef appReference,
		void *context);


AAAReturnCode AAAEndSession(
		AAASessionId *sessionId );


AAAReturnCode AAASessionTimerStart(
		AAASessionId *sessionId ,
		unsigned int timeout);


AAAReturnCode AAASessionTimerStop(
		AAASessionId *sessionId );

/*
AAAReturnCode AAAAbortSession(
		AAASessionId *sessionId);
*/

AAAReturnCode AAADictionaryEntryFromAVPCode(
		AAA_AVPCode avpCode,
		AAAVendorId vendorId,
		AAADictionaryEntry *entry);


AAAValue AAAValueFromName(
		char *avpName,
		char *vendorName,
		char *valueName);


AAAReturnCode AAADictionaryEntryFromName(
		char *avpName,
		AAAVendorId vendorId,
		AAADictionaryEntry *entry);


AAAValue AAAValueFromAVPCode(
		AAA_AVPCode avpCode,
		AAAVendorId vendorId,
		char *valueName);


const char *AAALookupValueNameUsingValue(
		AAA_AVPCode avpCode,
		AAAVendorId vendorId,
		AAAValue value);


boolean_t AAAGetCommandCode(
		char *commandName,
		AAACommandCode *commandCode,
		AAAVendorId *vendorId);


AAAReturnCode AAASendMessage(
		AAAMessage *message);

/*
AAAReturnCode AAASendAcctRequest(
		AAASessionId *aaaSessionId,
		AAAExtensionId extensionId,
		AAA_AVP_LIST *acctAvpList,
		AAAAcctMessageType msgType);
*/

#endif

