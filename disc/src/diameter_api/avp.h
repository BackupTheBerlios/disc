/*
 * $Id: avp.h,v 1.2 2003/03/11 20:52:13 bogdan Exp $
 *
 * 2003-02-04 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_AVP_H
#define _AAA_DIAMETER_AVP_H

#include "diameter_types.h"


#define AVP_HDR_SIZE  (AVP_CODE_SIZE+AVP_FLAGS_SIZE+AVP_LENGTH_SIZE)

#define for_all_AVPS_do_switch( _buf_ , _foo_ , _ptr_ ) \
	for( (_ptr_) =  (_buf_)->s + AAA_MSG_HDR_SIZE, (_foo_)=(_ptr_) ;\
	(_ptr_) < (_buf_)->s+(_buf_)->len ;\
	(_ptr_) = (_foo_)+(ntohl( ((unsigned int *)(_foo_))[1] )&0x00ffffff) ) \
		switch( ntohl( ((unsigned int *)(_ptr_))[0] ) )

#define set_AVP_mask( _mask_, _pos_) \
	do{ (_mask_)|=1<<(_pos_); }while(0)


#if 0

typedef enum {
	AVP_User_Name                     =    1,
	AVP_Class                         =   25,
	AVP_Session_Timeout               =   27,
	AVP_Proxy_State                   =   33,
	AVP_Host_IP_Address               =  257,
	AVP_Auth_Aplication_Id            =  258,
	AVP_Vendor_Specific_Application_Id=  260,
	AVP_Redirect_Max_Cache_Time       =  262,
	AVP_Session_Id                    =  263,
	AVP_Origin_Host                   =  264,
	AVP_Supported_Vendor_Id           =  265,
	AVP_Vendor_Id                     =  266,
	AVP_Result_Code                   =  268,
	AVP_Product_Name                  =  269,
	AVP_Session_Binding               =  270,
	AVP_Disconnect_Cause              =  273,
	AVP_Auth_Request_Type             =  274,
	AVP_Auth_Grace_Period             =  276,
	AVP_Auth_Session_State            =  277,
	AVP_Origin_State_Id               =  278,
	AVP_Proxy_Host                    =  280,
	AVP_Error_Message                 =  281,
	AVP_Record_Route                  =  282,
	AVP_Destination_Realm             =  283,
	AVP_Proxy_Info                    =  284,
	AVP_Re_Auth_Request_Type          =  285,
	AVP_Authorization_Lifetime        =  291,
	AVP_Redirect_Host                 =  292,
	AVP_Destination_Host              =  293,
	AVP_Termination_Cause             =  295,
	AVP_Origin_Realm                  =  296,
}AAA_AVPCode;
#endif


AAAReturnCode  create_avp( AAA_AVP** ,AAA_AVPCode , AAA_AVPFlag ,AAAVendorId,
											char *, size_t ,unsigned int );

AAA_AVP* clone_avp( AAA_AVP *avp, unsigned int clone_data );

#endif

