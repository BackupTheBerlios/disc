/*
 * $Id: message.h,v 1.1 2003/03/07 10:34:24 bogdan Exp $
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


#define AAA_MSG_HDR_SIZE  \
	(VER_SIZE + MESSAGE_LENGTH_SIZE + FLAGS_SIZE + COMMAND_CODE_SIZE +\
	VENDOR_ID_SIZE + HOP_BY_HOP_IDENTIFIER_SIZE + END_TO_END_IDENTIFIER_SIZE)

#define is_req(_msg_) \
	(((_msg_)->flags)&0x80)


int init_msg_manager();
void destroy_msg_manager();

void print_aaa_message( AAAMessage *msg);


int send_aaa_message( AAAMessage* , struct trans* , struct session* ,
														struct peer*);

AAAMessage* build_rpl_from_req(AAAMessage *req, unsigned int result_code,
														str *err_msg);

int process_msg( unsigned char *, unsigned int, struct peer*);

#endif

