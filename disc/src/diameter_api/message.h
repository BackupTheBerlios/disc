/*
 * $Id: message.h,v 1.5 2003/03/11 18:06:29 bogdan Exp $
 *
 * 2003-02-07 created by bogdan
 *
 */

#ifndef _AAA_DIAMETER_MESSAGE_H
#define _AAA_DIAMETER_MESSAGE_H

#include "utils/aaa_lock.h"
#include "diameter_types.h"
#include "peer.h"
#include "session.h"
#include "trans.h"


struct msg_manager {
	unsigned int  hop_by_hop;
	aaa_lock     *hop_by_hop_lock;
	unsigned int  end_to_end;
	aaa_lock     *end_to_end_lock;
};


#define is_req(_msg_) \
	(((_msg_)->flags)&0x80)


#if (__BYTE_ORDER==LITTLE_ENDIAN)
	#define AS_MSG_CODE      0x12010000
	#define AC_MSG_CODE      0x0f010000
	#define CE_MSG_CODE      0x01010000
	#define DW_MSG_CODE      0x18010000
	#define DP_MSG_CODE      0x1a010000
	#define RA_MSG_CODE      0x02010000
	#define ST_MSG_CODE      0x13010000
	#define MASK_MSG_CODE    0xffffff00
#else
	#define AS_MSG_CODE      0x00000112
	#define AC_MSG_CODE      0x0000010f
	#define CE_MSG_CODE      0x00000101
	#define DW_MSG_CODE      0x00000118
	#define DP_MSG_CODE      0x0000011a
	#define RA_MSG_CODE      0x00000102
	#define ST_MSG_CODE      0x00000113
	#define MASK_MSG_CODE    0x00ffffff
#endif


int init_msg_manager();

void destroy_msg_manager();

void print_aaa_message( AAAMessage *msg);

int send_aaa_request( str *buf, struct session *ses, struct peer *dst_peer );

int send_aaa_response( str *buf, struct trans *tr);

AAAMessage* build_rpl_from_req(AAAMessage *req, unsigned int result_code,
														str *err_msg);

int process_msg( unsigned char *, unsigned int, struct peer*);





#endif

