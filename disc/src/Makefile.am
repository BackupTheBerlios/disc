##
## $Id: Makefile.am,v 1.1 2003/03/24 18:45:10 andrei Exp $
##
## Process this file with automake to produce Makefile.in
##

## Other intersting options: nostdinc
## 
##

@SET_MAKE@

SUBDIRS = diameter_api transport mem client

bin_PROGRAMS = aaa_client
aaa_client_SOURCES = aaa_lock.c dprint.c hash_table.c sh_mutex.c \
					timer.c \
					aaa_lock.h dprint.h hash.h lock_alloc.h sem.h \
					timer.h config.h fastlock.h hash_table.h lock_ops.h \
					sh_mutex.h utils.h counter.h globals.h list.h \
					locking.h str.h
# hash.c only for tests
				
EXTRA_DIST=		server

aaa_client_LDADD = diameter_api/libdiameter_api.la mem/libmem.la \
					transport/libtransport.la client/libclient.la


