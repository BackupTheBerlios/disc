##
## $Id: Makefile.am,v 1.2 2003/04/08 22:30:18 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

SUBDIRS = . 

noinst_LTLIBRARIES = libserver.la
libserver_la_SOURCES =	main.c worker.c \
						work.h

libserver_la_LIBADD = ../transport/libtransport.la \
					  ../diameter_api/libdiameter_api.la

libserver_la_LDFLAGS = 

