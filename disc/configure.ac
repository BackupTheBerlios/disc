dnl $Id: configure.ac,v 1.1 2003/03/24 18:45:10 andrei Exp $
dnl Process this file with autoconf to produce a configure script
dnl autoconf version: 2.54, automake 1.7.1, libtool 1.4.3

AC_PREREQ(2.54)
AC_REVISION($Id: configure.ac,v 1.1 2003/03/24 18:45:10 andrei Exp $)
AC_INIT(aaa_client, 0.0.1)
AC_CONFIG_SRCDIR([src/aaa_lock.c])
AM_INIT_AUTOMAKE([nostdinc dist-bzip2 dist-shar dist-zip dist-tarZ])
AM_MAINTAINER_MODE


dnl FIXME:
AC_DEFINE(NAME, ["]AC_PACKAGE_NAME["])
# AC_DEFINE(VERSION, AC_PACKAGE_VERSION)
AC_DEFINE(CFG_DIR, ["/etc/aaa_client"])


dnl Command line options
AC_ARG_ENABLE(debug_build,
				AC_HELP_STRING([--enable-debug_build],
							[compile with debugging support (default is NO)]))
if test "$enable_debug_build" = no; then
	enable_debug_build=""
fi

AC_ARG_ENABLE(debug,
				AC_HELP_STRING([--disable-debug],
							[disable verbose debugging info (default is NO)]))
if test -z "$enable_debug"; then
	enable_debug=yes
fi
if test "$enable_debug" = no; then
	AC_DEFINE(NO_DEBUG)
fi

AC_ARG_ENABLE(extra_debug,
				AC_HELP_STRING([--enable-extra_debug],
							[enable special debugging code (default is NO)]))
if test -z "$enable_extra_debug"; then
	enable_extra_debug=no
fi
if test "$enable_extra_debug" = yes; then
	AC_DEFINE(EXTRA_DEBUG)
fi

AC_ARG_ENABLE(log,
				AC_HELP_STRING([--disable-log],
						[disables loging (not a good ideea, default is NO)]) )
if test -z "$enable_log"; then
	enable_log=yes
fi
if test "$enable_log" = no; then
	AC_DEFINE(NO_LOG)
fi

AC_ARG_ENABLE(ipv6, 
				AC_HELP_STRING([--disable-ipv6], 
								[disable ipv6 support (default is NO)]) )
if test -z "$enable_ipv6"; then
	enable_ipv6=no
fi
if test "$enable_ipv6" = yes; then
	AC_DEFINE(USE_IPV6)
fi

AC_ARG_ENABLE(dns_ip_hack, 
				AC_HELP_STRING([--disable-dns_ip_hack],
                        [disable fast ip address conversion (default is NO)]))
if test -z "$enable_dns_ip_hack"; then
	enable_dns_ip_hack=no
fi
if test "$enable_dns_ip_hack" = yes; then
	AC_DEFINE(DNS_IP_HACK)
fi

dnl FIXME: replace the following defines as above

#AC_DEFINE(PKG_MALLOC)
AC_DEFINE(SHM_MEM)
AC_DEFINE(SHM_MMAP)
AC_DEFINE(ADAPTIVE_WAIT)
AC_DEFINE(ADAPTIVE_WAIT_LOOPS, 1024)
#AC_DEFINE(F_MALLOC)
dnl FIXME: add the missing ones

dnl cpu specific stuff

AC_CANONICAL_HOST
case "${host_cpu}" in
	i[[3456]]86)	cpu=i386
						;;
	sparc*)			cpu=sparc
						;;
	arm*)			cpu=arm
						;;
	*) cpu=${host_cpu} ;;
esac

os=`echo $host_os|sed -e 'y/-./__/'`
AC_DEFINE_UNQUOTED([ARCH], ["${cpu}"] )
AC_DEFINE_UNQUOTED([__CPU_${cpu}])
AC_DEFINE_UNQUOTED([OS],   ["${host_os}"])
AC_DEFINE_UNQUOTED([__OS_${os}])

dnl Checks for programs
AC_PROG_CC
AM_PROG_LEX
#AC_DECL_YYTEXT
AC_PROG_YACC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl get/set compiler flags
if test -z "$enable_debug_build" ; then
	CFLAGS=`./ccopts.sh $CC`
fi
CCVER_OUTPUT=`./ccver.sh $CC`
CCVER=`echo $CCVER_OUTPUT| cut -d" " -f2`
CCNAME=`echo $CCVER_OUTPUT| cut -d" " -f1`

dnl compiler specific stuff
case "${CCNAME}" in
	suncc)
			LIBS="$LIBS -lfast"
			;;
	gcc|icc)
			case "${cpu}" in
				i386|sparc|arm)
				dnl FAST_LOCK inline asm doesn't work with all the compilers
							fast_lock=yes
							;;
			esac
			;;
		*)
			AC_MSG_WARN([*** unknown compiler $CCNAME, trying defaults ***])
			;;
esac

if test "$fast_lock" = yes ; then
	AC_DEFINE(FAST_LOCK)
else
	AC_MSG_WARN([*** disabling fast locking for $CCNAME $cpu ***])
fi

AC_DEFINE_UNQUOTED([COMPILER], ["$CC $CCVER"])


dnl Checks for libraries
#AC_SEARCH_LIBS([dlopen], [dl c_r])
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([res_search], [resolv])
dnl solaris libs
AC_SEARCH_LIBS([sendto], [socket xnet])
AC_SEARCH_LIBS([gethostbyname], [nsl xnet])
AC_SEARCH_LIBS([sched_yield], [rt])
dnl gethostbyname2/getipnodebyname!
AC_SEARCH_LIBS([gethostbyname2], [nsl xnet])
AC_SEARCH_LIBS([getipnodebyname], [nsl xnet])
dnl ?libfast on Solaris??


dnl Checks for header files
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([sys/sockio.h])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h ], , AC_MSG_ERROR(["*** not all required *.h files found - please fix ***"]))


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPE([union semun], AC_DEFINE(HAVE_UNION_SEMUN),,
				[#include <sys/sem.h> ])

dnl  Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF

 AC_CHECK_FUNCS([gethostbyaddr gettimeofday inet_ntoa localtime_r memchr memmove memset mkfifo munmap regcomp select socket strcasecmp strchr strdup strerror strncasecmp strtol uname], ,
 AC_MSG_ERROR(["*** not all required functions found - please fix ***"]))

dnl Checks for "special" functions
AC_CHECK_FUNCS([gethostbyname], ,
			AC_MSG_ERROR(["*** gethostbyname not found - please fix ***"]))
if test "$enable_ipv6" = yes ; then
	AC_CHECK_FUNCS([gethostbyname2], [use_gethostbyname2=yes],
									[no_gethostbyname2=yes] )
	if test "$no_gethostbyname2" = yes ; then
		AC_CHECK_FUNCS([getipnodebyname], [use_getipnodebyname=yes],
				AC_MSG_ERROR(["*** neither gethostbyname2 or getipnodebyaddr present - fix or compile without ipv6 support ***"]))
	fi
fi


dnl Checks for structure members
AC_CHECK_MEMBERS([struct sockaddr.sa_len], AC_DEFINE([HAVE_SOCKADDR_SA_LEN]),,
[ 
  #if HAVE_SYS_TYPES_H
  #include <sys/types.h>
  #endif
  #if HAVE_SYS_SOCKET_H
  #include <sys/socket.h>
  #endif
 ]
)


AC_OUTPUT(src/Makefile src/client/Makefile src/diameter_api/Makefile
			src/mem/Makefile src/transport/Makefile)
