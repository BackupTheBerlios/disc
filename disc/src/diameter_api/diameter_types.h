/*
 * $Id: diameter_types.h,v 1.5 2003/03/14 14:11:14 bogdan Exp $
 *
 * 2002-09-25 created by illya (komarov@fokus.gmd.de)
 */


#ifndef _AAA_DIAMETER_TYPES_H
#define _AAA_DIAMETER_TYPES_H


#include "utils/ip_addr.h"
#include "utils/str.h"

#define AAA_NO_VENDOR_ID   0

#define VER_SIZE                   1
#define MESSAGE_LENGTH_SIZE        3
#define FLAGS_SIZE                 1
#define COMMAND_CODE_SIZE          3
#define APPLICATION_ID_SIZE        4
#define HOP_BY_HOP_IDENTIFIER_SIZE 4
#define END_TO_END_IDENTIFIER_SIZE 4
#define AVP_CODE_SIZE      4
#define AVP_FLAGS_SIZE     1
#define AVP_LENGTH_SIZE    3
#define AVP_VENDOR_ID_SIZE 4
#define AAA_MSG_HDR_SIZE  \
	(VER_SIZE + MESSAGE_LENGTH_SIZE + FLAGS_SIZE + COMMAND_CODE_SIZE +\
	APPLICATION_ID_SIZE+HOP_BY_HOP_IDENTIFIER_SIZE+END_TO_END_IDENTIFIER_SIZE)




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

/* The following are the result codes returned from remote servers as
 * part of messages */
typedef enum {
	AAA_MUTI_ROUND_AUTH           = 1001,
	AAA_SUCCESS                   = 2001,
	AAA_COMMAND_UNSUPPORTED       = 3001,
	AAA_UNABLE_TO_DELIVER         = 3002,
	AAA_REALM_NOT_SERVED          = 3003,
	AAA_TOO_BUSY                  = 3004,
	AAA_LOOP_DETECTED             = 3005,
	AAA_REDIRECT_INDICATION       = 3006,
	AAA_APPLICATION_UNSUPPORTED   = 3007,
	AAA_INVALID_HDR_BITS          = 3008,
	AAA_INVALID_AVP_BITS          = 3009,
	AAA_UNKNOWN_PEER              = 3010,
	AAA_AUTHENTICATION_REJECTED   = 4001,
	AAA_OUT_OF_SPACE              = 4002,
	AAA_ELECTION_LOST             = 4003,
	AAA_AVP_UNSUPPORTED           = 5001,
	AAA_UNKNOWN_SESSION_ID        = 5002,
	AAA_AUTHORIZATION_REJECTED    = 5003,
	AAA_INVALID_AVP_VALUE         = 5004,
	AAA_MISSING_AVP               = 5005,
	AAA_RESOURCES_EXCEEDED        = 5006,
	AAA_CONTRADICTING_AVPS        = 5007,
	AAA_AVP_NOT_ALLOWED           = 5008,
	AAA_AVP_OCCURS_TOO_MANY_TIMES = 5009,
	AAA_NO_COMMON_APPLICATION     = 5010,
	AAA_UNSUPPORTED_VERSION       = 5011,
	AAA_UNABLE_TO_COMPLY          = 5012,
	AAA_INVALID_BIT_IN_HEADER     = 5013,
	AAA_INVALIS_AVP_LENGTH        = 5014,
	AAA_INVALID_MESSGE_LENGTH     = 5015,
	AAA_INVALID_AVP_BIT_COMBO     = 5016,
	AAA_NO_COMMON_SECURITY        = 5017,
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
	void                *trans;
	str                 buf;
} AAAMessage;


//typedef AAAReturnCode (*func)(AAAMessage *message) AAACallback;


#endif
