##
## $Id: Makefile.am,v 1.4 2003/04/09 22:10:34 bogdan Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

noinst_LTLIBRARIES = libdiameter_api.la
libdiameter_api_la_SOURCES =	init_conf.c sender.c session.c \
								session.h diameter_api.h \
								init_conf.h 
								
#								dictionary.c  dictionary.h
								
libdiameter_api_la_LIBADD = ../diameter_msg/libdiameter_msg.la

EXTRA_DIST=	#dictionary



