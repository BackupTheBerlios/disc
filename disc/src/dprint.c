/*
 * $Id: dprint.c,v 1.1 2003/03/14 16:47:48 bogdan Exp $
 *
 * 2003-02-03 added by bogdan; created by andrei
 *
 */



#include "dprint.h"
 
#include <stdarg.h>
#include <stdio.h>

void dprint(char * format, ...)
{
	va_list ap;

	//fprintf(stderr, "%2d(%d) ", process_no, my_pid());
	va_start(ap, format);
	vfprintf(stderr,format,ap);
	fflush(stderr);
	va_end(ap);
}
