##
## $Id: Makefile.am,v 1.4 2003/04/01 11:35:00 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

SUBDIRS = . modules

noinst_LTLIBRARIES = libclient.la
libclient_la_SOURCES =	main.c worker.h worker.c \
						config.h
libclient_la_LIBADD = ../transport/libtransport.la \
					  ../diameter_api/libdiameter_api.la



