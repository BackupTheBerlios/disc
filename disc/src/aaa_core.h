/*
 * $Id: aaa_core.h,v 1.1 2003/04/08 13:29:28 bogdan Exp $
 *
 * 2003-04-08 created by bogdan
 */


#ifndef _AAA_DIAMETER_AAA_CORE_H
#define _AAA_DIAMETER_AAA_CORE_H


int init_aaa_core(char *cfg_file);

int start_aaa_core();

void destroy_aaa_core();

#endif
