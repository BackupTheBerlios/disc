##
## $Id: Makefile.am,v 1.15 2003/04/08 17:49:09 andrei Exp $
##
## Process this file with automake to produce Makefile.in
##

## Other intersting options: nostdinc
## 
##

@SET_MAKE@

SUBDIRS = diameter_msg diameter_api transport mem client server

INCLUDES = @INCLTDL@

bin_PROGRAMS = aaa_client aaa_server
common =			 aaa_lock.c dprint.c hash_table.c \
					timer.c aaa_module.c aaa_core.c \
					cfg_init.c cfg_parser.c \
					aaa_lock.h dprint.h hash.h lock_alloc.h sem.h \
					timer.h config.h fastlock.h hash_table.h lock_ops.h \
					utils.h counter.h globals.h msg_queue.h msg_queue.c \
					list.h locking.h str.h aaa_module.h aaa_core.h \
					cfg_init.h cfg_parser.h parser_f.h
#
# client
#
aaa_client_SOURCES = $(common)
# hash.c only for tests
				
EXTRA_DIST=		server aaa_client.cfg aaa_lib.cfg

aaa_client_LDADD =  mem/libmem.la \
					transport/libtransport.la client/libclient.la \
					@LIBLTDL@
# @LIBADD_DL@ # not needed for newer libtool version (>=1.4)
aaa_client_LDFLAGS = -export-dynamic -pthread

#
# server
#
aaa_server_SOURCES = $(common)
aaa_server_LDADD =  mem/libmem.la \
					transport/libtransport.la server/libserver.la \
					@LIBLTDL@
# @LIBADD_DL@ # not needed for newer libtool version (>=1.4)
aaa_server_LDFLAGS = -export-dynamic -pthread


