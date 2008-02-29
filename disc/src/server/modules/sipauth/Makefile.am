##
## $Id: Makefile.am,v 1.1 2008/02/29 16:09:57 anomarme Exp $
##
## Process this file with automake to produce Makefile.in
##

@SET_MAKE@

moduledir= $(libdir)/@PACKAGE@/@server_module_dir@
module_LTLIBRARIES = sipauth.la
sipauth_la_SOURCES = sipauth.c digest_parser.c param_parser.c \
			challenge.c nonce.c md5.c db.c rfc2617.c
sipauth_la_LDFLAGS = -module -avoid-version
sipauth_la_LIBADD  = -L/usr/lib -lmysqlclient

INCLUDES = -I/usr/include/mysql -I/usr/include/local -I/usr/include/local/mysql \
	    -I/usr/include

