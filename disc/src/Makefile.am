##
## $Id: Makefile.am,v 1.7 2003/04/01 12:03:16 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

## Other intersting options: nostdinc
## 
##

@SET_MAKE@

SUBDIRS = diameter_api transport mem client

INCLUDES = @INCLTDL@

bin_PROGRAMS = aaa_client
aaa_client_SOURCES = aaa_lock.c dprint.c hash_table.c sh_mutex.c \
					timer.c aaa_module.c \
					aaa_lock.h dprint.h hash.h lock_alloc.h sem.h \
					timer.h config.h fastlock.h hash_table.h lock_ops.h \
					sh_mutex.h utils.h counter.h globals.h list.h \
					msg_queue.h msg_queue.c \
					locking.h str.h aaa_module.h
# hash.c only for tests
				
EXTRA_DIST=		server

aaa_client_LDADD =  mem/libmem.la \
					transport/libtransport.la client/libclient.la \
					@LIBLTDL@

# @LIBADD_DL@ # not needed for newer libtool version (>=1.4)


