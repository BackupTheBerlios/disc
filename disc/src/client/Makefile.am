##
## $Id: Makefile.am,v 1.6 2003/04/06 22:09:16 andrei Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

SUBDIRS = . modules

noinst_LTLIBRARIES = libclient.la
libclient_la_SOURCES =	main.c worker.h worker.c \
					    dest.h dest.c \
						config.h
libclient_la_LIBADD = ../transport/libtransport.la \
					  ../diameter_api/libdiameter_api.la

libclient_la_LDFLAGS = 

