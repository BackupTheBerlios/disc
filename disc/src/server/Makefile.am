##
## $Id: Makefile.am,v 1.1 2003/04/08 17:22:01 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

SUBDIRS = . 

noinst_LTLIBRARIES = libserver.la
libserver_la_SOURCES =	main.c

libserver_la_LIBADD = ../transport/libtransport.la \
					  ../diameter_api/libdiameter_api.la

libserver_la_LDFLAGS = 

