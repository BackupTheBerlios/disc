/*
 * $Id: avp.h,v 1.6 2003/03/26 17:58:38 bogdan Exp $
 *
 * 2003-02-04 created by bogdan
 *
 */


#ifndef _AAA_DIAMETER_AVP_H
#define _AAA_DIAMETER_AVP_H

#include "diameter_types.h"



AAAReturnCode  create_avp( AAA_AVP** ,AAA_AVPCode , AAA_AVPFlag ,AAAVendorId,
											char *, size_t ,unsigned int );

AAA_AVP* clone_avp( AAA_AVP *avp, unsigned int clone_data );

#endif

