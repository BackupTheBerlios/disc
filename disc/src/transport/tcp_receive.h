/*
 * $Id: tcp_receive.h,v 1.2 2003/04/06 22:19:49 bogdan Exp $
 *
 *  History:
 *  --------
 *  2003-03-07  created by bogdan
 */



#ifndef _TCP_SHELL_RECEIVE_H
#define _TCP_SHELL_RECEIVE_H



#include <sys/types.h>
#include <sys/select.h>


void* do_receive(void *attr);


#endif


