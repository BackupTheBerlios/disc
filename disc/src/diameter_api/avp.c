/*
 * $Id: avp.c,v 1.3 2003/03/11 18:06:29 bogdan Exp $
 *
 * 2002-10-04 created  by illya (komarov@fokus.gmd.de)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/dprint.h"
#include "diameter_types.h"



/*
 * each AVP type has some default set/reset flags and a proper data type.
 * All this default values (for flags and data-type) are correct/set by this
 * function.
 */
inline void set_avp_fields( AAA_AVPCode code, AAA_AVP *avp)
{
	switch (code) {
		case   1: /*AVP_User_Name*/
		case  25: /*AVP_Class*/
		case 263: /*AVP_Session_Id*/
		case 283: /*AVP_Destination_Realm*/
		case 293: /*AVP Destination Host*/
		case 264: /*AVP_Origin_Host*/
		case 296: /*AVP Origin_Realm*/
			avp->flags = 0x40|(0x20&avp->flags);
			avp->type = AAA_AVP_STRING_TYPE;
			break;
		case  27: /*AVP_Session_Timeout*/
		case 258: /*AVP_Auth_Aplication_Id*/
		case 262: /*AVP_Redirect_Max_Cache_Time*/
		case 265: /*AVP_Supported_Vendor_Id*/
		case 266: /*AVP_Vendor_Id*/
		case 268: /*AVP_Result_Code*/
		case 270: /*AVP_Session_Binding*/
		case 276: /*AVP_Auth_Grace_Period*/
		case 278: /*AVP_Origin_State_Id*/
		case 291: /*AVP_Authorization_Lifetime*/
			avp->flags = 0x40|(0x20&avp->flags);
			avp->type = AAA_AVP_INTEGER32_TYPE;
			break;
		case 33: /*AVP_Proxy_State*/
			avp->flags = 0x40;
			avp->type = AAA_AVP_STRING_TYPE;
			break;
		case 257: /*AVP_Host_IP_Address*/
			avp->flags = 0x40|(0x20&avp->flags);
			avp->type = AAA_AVP_ADDRESS_TYPE;
			break;
		case 269: /*AVP_Product_Name*/
			avp->flags = 0x00;
			avp->type = AAA_AVP_STRING_TYPE;
			break;
		case 281: /*AVP_Error_Message*/
			avp->flags = (0x20&avp->flags);
			avp->type = AAA_AVP_STRING_TYPE;
			break;
		default:
			avp->type = AAA_AVP_DATA_TYPE;
	};
}



/* internal function - builds an avp ; if free_it is 1, the data will be freed
 * along with the avp; if 0, it will not be freed
 */
AAAReturnCode  create_avp(
	AAA_AVP **avp,
	AAA_AVPCode code,
	AAA_AVPFlag flags,
	AAAVendorId vendorId,
	char *data,
	size_t length,
	unsigned int free_it)
{
	/* allocated a new AVP struct */
	(*avp) = 0;
	(*avp) = (AAA_AVP*)malloc(sizeof(AAA_AVP));
	if (!(*avp))
		goto error;
	memset( *avp, 0, sizeof(AAA_AVP) );

	/* set some fields */
	(*avp)->free_it = free_it;
	(*avp)->packetType = AAA_DIAMETER;
	(*avp)->code=code;
	(*avp)->flags=flags;
	(*avp)->vendorId=vendorId;
	set_avp_fields( code, *avp);

	/* data */
	(*avp)->data.s = data;
	(*avp)->data.len = length;

	return AAA_ERR_SUCCESS;
error:
	LOG(L_ERR,"ERROR:create_avp: no more free memory!\n");
	if (*avp) free(*avp);
	if (data && free_it) free(data);
	return AAA_ERR_NOMEM;
}



/* This function creates an AVP and returns a pointer to it;
 * For data, a own copy will be made.
 */
AAAReturnCode  AAACreateAVP(
	AAA_AVP **avp,
	AAA_AVPCode code,
	AAA_AVPFlag flags,
	AAAVendorId vendorId,
	char *data,
	size_t length )
{
	char *p=0;

	/* first check the params */
	if(avp==0 || data==0 || length==0) {
		LOG(L_ERR,"ERROR:AAACreateAndAddAVPToList: NULL value received for"
			" param avp/data/length !!\n");
		return AAA_ERR_PARAMETER;
	}

	/* make a duplicate for data */
	p = (void*)malloc(length);
	if(!p) {
		LOG(L_ERR,"ERROR:AAACreateAVP: no more free memory!\n");
		goto error;
	}
	memcpy( p, data, length);

	return create_avp( avp, code, flags, vendorId, p, length, 1);
error:
	return AAA_ERR_NOMEM;
}



/** This function creates an AVP and adds it to an AVP list */
AAAReturnCode AAACreateAndAddAVPToList(
	AAA_AVP_LIST **avpList,
	AAA_AVPCode code,
	AAA_AVPFlag flags,
	AAAVendorId vendorId,
	char *data,
	size_t length )
{
	AAA_AVP *avp;

	/* some checks */
	if (!avpList || !avpList) {
		LOG(L_ERR,"ERROR:AAACreateAndAddAVPToList: param avpList cannot"
			" be null!!\n");
		return AAA_ERR_PARAMETER;
	}

	/* build the AVP */
	if (AAACreateAVP(&avp,code,flags,vendorId,data,length)!=AAA_ERR_SUCCESS)
		return AAA_ERR_NOMEM;

	/* link the AVP into the list */
	if (!(*avpList)->tail)
		(*avpList)->head = avp;
	else
		(*avpList)->tail->next = avp;
	avp->prev = (*avpList)->tail;
	(*avpList)->tail = avp;

	return AAA_ERR_SUCCESS;
}



/* Insert the AVP avp into this avpList after position */
AAAReturnCode  AAAAddAVPToList(
	AAA_AVP_LIST **avpList,
	AAA_AVP *avp,
	AAA_AVP *position)
{
	AAA_AVP *avp_t;

	if ( !avpList || !avp || (!*avpList && position) ) {
		LOG(L_ERR,"ERROR:AAAAddAVPToList: param avpList or avp passed null"
			" or *avpList=NULL and position!=NULL !!\n");
		return AAA_ERR_PARAMETER;
	}

	/* do we have to create a new list? */
	if (*avpList==0) {
		*avpList = (AAA_AVP_LIST*)malloc(sizeof(AAA_AVP_LIST));
		if (*avpList==0) {
			LOG(L_ERR,"ERROR: AAACreateAVP: no more free memory!\n");
			return AAA_ERR_NOMEM;
		}
		(*avpList)->tail = (*avpList)->head = 0;
	}

	if (!position) {
		/* insert at the begining */
		avp->next = (*avpList)->head;
		avp->prev = 0;
		(*avpList)->head = avp;
		if (avp->next)
			avp->next->prev = avp;
		else
			(*avpList)->tail = avp;
	} else {
		/* look after avp from position */
		for(avp_t=(*avpList)->head;avp_t&&avp_t!=position;avp_t=avp_t->next);
		if (!avp_t) {
			LOG(L_ERR,"ERROR: AAACreateAVP: the \"position\" avp is not in"
				"\"avpList\" list!!\n");
			return AAA_ERR_PARAMETER;
		}
		/* insert after position */
		avp->next = position->next;
		position->next = avp;
		if (avp->next)
			avp->next->prev = avp;
		else
			(*avpList)->tail = avp;
		avp->prev = position;
	}

	return AAA_ERR_SUCCESS;

}



/* This function finds an AVP with matching code and vendor id */
AAA_AVP  *AAAFindMatchingAVP(
	AAA_AVP_LIST *avpList,
	AAA_AVP *startAvp,
	AAA_AVPCode avpCode,
	AAAVendorId vendorId,
	AAASearchType searchType)
{
	AAA_AVP *avp_t;

	/* param checking */
	if (!avpList) {
		LOG(L_ERR,"ERROR:FindMatchingAVP: param avpList passed null !!\n");
		goto error;
	}
	/* search the startAVP avp */
	for(avp_t=avpList->head;avp_t&&avp_t!=startAvp;avp_t=avp_t->next);
	if (!avp_t) {
		LOG(L_ERR,"ERROR: AAAFindMatchingAVP: the \"position\" avp is not in"
			"\"avpList\" list!!\n");
		goto error;
	}

	/* where should I start searching from ? */
	if (!startAvp)
		avp_t=(searchType==AAA_FORWARD_SEARCH)?(avpList->head):(avpList->tail);
	else
		avp_t=startAvp;

	/* start searching */
	while(avp_t) {
		if (avp_t->code==avpCode && avp_t->vendorId==vendorId)
			return avp_t;
		avp_t = (searchType==AAA_FORWARD_SEARCH)?(avp_t->next):(avp_t->prev);
	}

error:
	return 0;
}



/*  The following function joins together two AVP lists */
AAAReturnCode  AAAJoinAVPLists(
	AAA_AVP_LIST *dest,
	AAA_AVP_LIST *source,
	AAA_AVP      *position)
{
	AAA_AVP *avp_t;

	/* param check */
	if ( !dest || !source ) {
		LOG(L_ERR,"ERROR:AAAAddAVPToList: param AVP_LIST \"dest\" or \"source\" "
			"passed null !!\n");
		return AAA_ERR_PARAMETER;
	}

	/* search the "position" avp */
	for(avp_t=dest->head;avp_t&&avp_t!=position;avp_t=avp_t->next);
	if (!avp_t) {
		LOG(L_ERR,"ERROR: AAACreateAVP: the \"position\" avp is not in "
			"\"dest\" avp_list!!\n");
		return AAA_ERR_PARAMETER;
	}

	/* join */
	if (source->head) {
		if (position) {
			source->tail->next = position->next;
			position->next = source->head;
		} else {
			source->tail->next = dest->head;
			dest->head = source->head;
		}
		source->head->prev = position;
		if (source->tail->next)
			source->tail->next->prev = source->tail;
	}
	return AAA_ERR_SUCCESS;
}



/* This function removes an AVP from a list */
AAAReturnCode  AAARemoveAVPFromList(
	AAA_AVP_LIST *avpList,
	AAA_AVP *avp)
{
	AAA_AVP *avp_t;

	/* param check */
	if ( !avpList || !avp ) {
		LOG(L_ERR,"ERROR:AAAAddAVPToList: param AVP_LIST \"avpList\" or AVP "
			"\"avp\" passed null !!\n");
		return AAA_ERR_PARAMETER;
	}

	/* search the "avp" avp */
	for(avp_t=avpList->head;avp_t&&avp_t!=avp;avp_t=avp_t->next);
	if (!avp_t) {
		LOG(L_ERR,"ERROR: AAACreateAVP: the \"avp\" avp is not in "
			"\"avpList\" avp list!!\n");
		return AAA_ERR_PARAMETER;
	}

	/* remove the avp from list */
	if (avpList->head==avp)
		avpList->head = avp->next;
	else
		avp->prev->next = avp->next;
	if (avp->next)
		avp->next->prev = avp->prev;
	else
		avpList->tail = avp->prev;
	avp->next = avp->prev = 0;

	return AAA_ERR_SUCCESS;
}



/* The function frees an AVP */
AAAReturnCode  AAAFreeAVP(AAA_AVP **avp)
{
	/* some checks */
	if (!avp || !(*avp)) {
		LOG(L_ERR,"ERROR:AAAFreeAVP: param avp cannot be null!!\n");
		return AAA_ERR_PARAMETER;
	}

	/* free all the mem */
	if ( (*avp)->free_it && (*avp)->data.s )
		free((*avp)->data.s);
	free( *avp );
	avp = 0;

	return AAA_ERR_SUCCESS;
}



/* This function returns a pointer to the first AVP in the list */
AAA_AVP*  AAAGetFirstAVP(AAA_AVP_LIST *avpList){
	return avpList->head;
}



/* This function returns a pointer to the last AVP in the list */
AAA_AVP*  AAAGetLastAVP(AAA_AVP_LIST *avpList)
{
	return avpList->tail;
}




/* This function returns a pointer to the next AVP in the list */
AAA_AVP*  AAAGetNextAVP(AAA_AVP *avp)
{
	return avp->next;
}



/* This function returns a pointer to the previous AVP in the list */
AAA_AVP*  AAAGetPrevAVP(AAA_AVP *avp)
{
	return avp->prev;
}



/* This function converts the data in the AVP to a format suitable for
 * log or display functions. */
char*  AAAConvertAVPToString(AAA_AVP *avp, char *dest, size_t destLen)
{
	int l;
	int i;

	if (!avp || !dest || !destLen) {
		LOG(L_ERR,"ERROR:AAAConvertAVPToString: param AVP, DEST or DESTLEN "
			"passed as null!!!\n");
		return 0;
	}
	l = snprintf(dest,destLen,"AVP(%p < %p >%p):packetType=%u;code=%u,"
		"flags=%x;\nDataType=%u;VendorID=%u;DataLen=%u;\n",
		avp->prev,avp,avp->next,avp->packetType,avp->code,avp->flags,
		avp->type,avp->vendorId,avp->data.len);
	switch(avp->type) {
		case AAA_AVP_STRING_TYPE:
			l+=snprintf(dest+l,destLen-l,"String: <%.*s>",avp->data.len,
				avp->data.s);
			break;
		case AAA_AVP_INTEGER32_TYPE:
			l+=snprintf(dest+l,destLen-l,"Int32: <%u>(%x)",
				htonl(*((unsigned int*)avp->data.s)),
				htonl(*((unsigned int*)avp->data.s)));
			break;
		case AAA_AVP_ADDRESS_TYPE:
			i = 1;
			switch (avp->data.len) {
				case 4: i=i*0;
				case 6: i=i*2;
					l+=snprintf(dest+l,destLen-l,"Address IPv4: <%d.%d.%d.%d>",
						(unsigned char)avp->data.s[i+0],
						(unsigned char)avp->data.s[i+1],
						(unsigned char)avp->data.s[i+2],
						(unsigned char)avp->data.s[i+3]);
					break;
				case 16: i=i*0;
				case 18: i=i*2;
					l+=snprintf(dest+l,destLen-l,
						"Address IPv6: <%x.%x.%x.%x.%x.%x.%x.%x>",
						((avp->data.s[i+0]<<8)+avp->data.s[i+1]),
						((avp->data.s[i+2]<<8)+avp->data.s[i+3]),
						((avp->data.s[i+4]<<8)+avp->data.s[i+5]),
						((avp->data.s[i+6]<<8)+avp->data.s[i+7]),
						((avp->data.s[i+8]<<8)+avp->data.s[i+9]),
						((avp->data.s[i+10]<<8)+avp->data.s[i+11]),
						((avp->data.s[i+12]<<8)+avp->data.s[i+13]),
						((avp->data.s[i+14]<<8)+avp->data.s[i+15]));
					break;
			break;
			}
			break;
		//case AAA_AVP_INTEGER64_TYPE:
		case AAA_AVP_TIME_TYPE:
		default:
			LOG(L_WARN,"WARNING:AAAConvertAVPToString: don't know how to print"
				" this data type [%d] -> tryng hexa\n",avp->type);
		case AAA_AVP_DATA_TYPE:
			for (i=0;i<avp->data.len&&l<destLen-1;i++)
			l+=snprintf(dest+l,destLen-l-1,"%x",
				((unsigned char*)avp->data.s)[i]);
	}
	return dest;
}



AAA_AVP* clone_avp( AAA_AVP *avp , unsigned int clone_data)
{
	AAA_AVP *n_avp;

	if (!avp || !(avp->data.s) || !(avp->data.len) )
		goto error;

	/* clone the avp structure */
	n_avp = (AAA_AVP*)malloc( sizeof(AAA_AVP) );
	if (!n_avp) {
		LOG(L_ERR,"ERROR:clone_avp: cannot get free memory!!\n");
		goto error;
	}
	memcpy( n_avp, avp, sizeof(AAA_AVP));
	n_avp->next = n_avp->prev = 0;

	if (clone_data) {
		/* clone the avp data */
		n_avp->data.s = (char*)malloc( avp->data.len );
		if (!(n_avp->data.s)) {
			LOG(L_ERR,"ERROR:clone_avp: cannot get free memory!!\n");
			free( n_avp );
			goto error;
		}
		memcpy( n_avp->data.s, avp->data.s, avp->data.len);
		avp->free_it = 1;
	} else {
		/* link the clone's data to the original's data */
		n_avp->data.s = avp->data.s;
		n_avp->data.len = avp->data.len;
		avp->free_it = 0;
	}

	/* don't clone the data -> link it */

	return n_avp;
error:
	return 0;
}


