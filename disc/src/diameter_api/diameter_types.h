/*
 * $Id: diameter_types.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
 *
 * 2002-09-25 created by illya (komarov@fokus.gmd.de)
 */


#ifndef _AAA_DIAMETER_TYPES_H
#define _AAA_DIAMETER_TYPES_H


//#include <inttypes.h>
#include "utils/ip_addr.h"
#include "utils/str.h"

#define AAA_NO_VENDOR_ID   0



typedef struct ip_addr  IP_ADDR;
typedef unsigned int    AAACommandCode;
typedef unsigned int    AAAVendorId;
typedef unsigned int    AAAExtensionId;
typedef unsigned int    AAA_AVPCode;
typedef unsigned int    AAAValue;
typedef void            AAAServer;
typedef str             AAASessionId;
typedef unsigned int    AAAMsgIdentifier;
typedef void*           AAACallback;
typedef void*           AAAApplicationId;
typedef uint8_t         AAAMsgFlag;



/* Status codes returned by functions in the AAA API */
typedef enum {
	AAA_ERR_NOT_FOUND = -2,         /* handle or id not found */
	AAA_ERR_FAILURE   = -1,         /* unspecified failure during an AAA op. */
	AAA_ERR_SUCCESS   =  0,         /* AAA operation succeeded */
	AAA_ERR_NOMEM,                  /* op. caused memory to be exhausted */
	AAA_ERR_PROTO,                  /*  AAA protocol error */
	AAA_ERR_SECURITY,
	AAA_ERR_PARAMETER,
	AAA_ERR_CONFIG,
	AAA_ERR_UNKNOWN_CMD,
	AAA_ERR_MISSING_AVP,
	AAA_ERR_ALREADY_INIT,
	AAA_ERR_TIMED_OUT,
	AAA_ERR_CANNOT_SEND_MSG,
	AAA_ERR_ALREADY_REGISTERED,
	AAA_ERR_CANNOT_REGISTER,
	AAA_ERR_NOT_INITIALIZED,
	AAA_ERR_NETWORK_ERROR,
} AAAReturnCode;


//    The following are codes used to indicate where a callback should be
//   installed in callback chain for processing:

      typedef enum {
           AAA_APP_INSTALL_FIRST,
           AAA_APP_INSTALL_ANYWHERE,
           AAA_APP_INSTALL_LAST
      } AAACallbackLocation;

/* The following are AVP data type codes. They correspond directly to
 * the AVP data types outline in the Diameter specification [1]: */
typedef enum {
	AAA_AVP_DATA_TYPE,
	AAA_AVP_STRING_TYPE,
	AAA_AVP_ADDRESS_TYPE,
	AAA_AVP_INTEGER32_TYPE,
	AAA_AVP_INTEGER64_TYPE,
	AAA_AVP_TIME_TYPE,
} AAA_AVPDataType;


/* The following are used for AVP header flags and for flags in the AVP
 *  wrapper struct and AVP dictionary definitions. */
typedef enum {
	AAA_AVP_FLAG_NONE               = 0x00,
	AAA_AVP_FLAG_MANDATORY          = 0x40,
	AAA_AVP_FLAG_RESERVED           = 0x1F,
	AAA_AVP_FLAG_VENDOR_SPECIFIC    = 0x80,
	AAA_AVP_FLAG_END_TO_END_ENCRYPT = 0x20,
	//AAA_AVP_FLAG_UNKNOWN            = 0x10000,
	//AAA_AVP_FLAG_ENCRYPT            = 0x40000,
} AAA_AVPFlag;


//   The following domain interconnection types are returned by
//   AAAGetDomainInternconnectType(). They indicate the type of domain
//   interconnection:
     typedef enum {
          AAA_DOMAIN_LOCAL,
          AAA_DOMAIN_PROXY,
          AAA_DOMAIN_BROKER,
          AAA_DOMAIN_FORWARD
      } AAADomainInterconnect;

// The following are the result codes returned from remote servers as
// part of messages
      typedef   enum {
          AAA_SUCCESS = 0,
          AAA_FAILURE,
          AAA_POOR_REQUEST,
          AAA_INVALID_AUTH,
          AAA_UNKNOWN_SESSION_ID,
          AAA_USER_UNKNOWN,
          AAA_COMMAND_UNSUPPORTED,
          AAA_TIMEOUT,
          AAA_AVP_UNSUPPORTED,
          AAA_REDIRECT_INDICATION,
          AAA_REALM_NOT_SERVED,
          AAA_UNSUPPORTED_TRANSFORM,
          AAA_AUTHENTICATION_REJECTED,
          AAA_AUTHORIZATION_REJECTED,
          AAA_INVALID_AVP_VALUE,
          AAA_MISSING_AVP,
          AAA_UNABLE_TO_DELIVER
      } AAAResultCode;



/*   The following type allows the client to specify which direction to
 *   search for an AVP in the AVP list: */
typedef enum {
	AAA_FORWARD_SEARCH = 0,
	AAA_BACKWARD_SEARCH
} AAASearchType;



      typedef enum {
           AAA_ACCT_EVENT = 1,
           AAA_ACCT_START = 2,
           AAA_ACCT_INTERIM = 3,
           AAA_ACCT_STOP = 4
      } AAAAcctMessageType;

//   The following defines the possible security characteristics for a
//   host.

      typedef enum {
           AAA_SEC_NOT_DEFINED = -2,
           AAA_SEC_NOT_CONNECTED = -1,
           AAA_SEC_NO_SECURITY = 0,
           AAA_SEC_CMS_SECURITY = 1,
           AAA_SEC_CMS_PROXIED = 2
      } AAASecurityStatus;
// The following structure is returned by the dictionary entry lookup
// functions
      typedef struct dictionaryEntry {
           AAA_AVPCode    avpCode;
           char*          avpName;
           AAA_AVPDataType     avpType;
           AAAVendorId    vendorId;
           AAA_AVPFlag    flags;
      } AAADictionaryEntry;


/* The following structure contains a message AVP in parsed format */
typedef struct avp {
	struct avp *next;
	struct avp *prev;
	enum {
		AAA_RADIUS,
		AAA_DIAMETER
	} packetType;
	AAA_AVPCode code;
	AAA_AVPFlag flags;
	AAA_AVPDataType type;
	AAAVendorId vendorId;
	str data;
	uint32_t free_it;
} AAA_AVP;


/* The following structure is used for representing lists of AVPs on the
 * message: */
typedef struct _avp_list_t {
	AAA_AVP *head;
	AAA_AVP *tail;
} AAA_AVP_LIST;


/* The following structure contains the full AAA message: */
typedef struct _message_t {
	AAAMsgFlag          flags;
	AAACommandCode      commandCode;
	AAAVendorId         vendorId;
	//AAAResultCode       resultCode;
	//IP_ADDR             originator;
	//IP_ADDR             sender;
	AAA_AVP_LIST        *avpList;
	AAA_AVP             *proxyAVP;
	AAAMsgIdentifier    endtoendID;
	AAAMsgIdentifier    hopbyhopID;
	//time_t              secondsTillExpire;
	//time_t              startTime;
	void                *appHandle;
	/* added */
	str                 orig_buf;
} AAAMessage;


//typedef AAAReturnCode (*func)(AAAMessage *message) AAACallback;


#endif
