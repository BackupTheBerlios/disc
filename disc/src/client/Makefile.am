##
## $Id: Makefile.am,v 1.3 2003/03/28 20:27:24 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

SUBDIRS = . modules

noinst_LTLIBRARIES = libclient.la
libclient_la_SOURCES =	main.c \
						config.h
libclient_la_LIBADD = ../transport/libtransport.la \
					  ../diameter_api/libdiameter_api.la



