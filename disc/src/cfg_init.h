/*
 * $Id
 */
/* History:
 * --------
 *  2003-04-07  created by andrei
 */


#ifndef cfg_init_h
#define cfg_init_h

#include "str.h"
#include "aaa_parse_uri.h"


struct cfg_peer_list{
	struct aaa_uri uri;
	str full_uri;
	str alias;
	struct cfg_peer_list *next;
};

extern struct cfg_peer_list* cfg_peer_lst;


int read_config_file(char*);

#endif
