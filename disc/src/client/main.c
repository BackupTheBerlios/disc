/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Mon Oct 28 11:54:28 CET 2002
    copyright            : (C) 2002 by Illya Komarov
    email                : komarov@fokus.gmd.de
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include "diameter_api.h"



static void sig_handler(int signo)
{
	printf(" ----------- SIGNAL RECEIVED (%d)(%d) -----------------\n",
			(int)pthread_self(), getpid() );
	if (AAAClose()==AAA_ERR_NOT_INITIALIZED)
		return;
	exit(0);
}



int main(int argc, char *argv[])
{
	if( AAAOpen("aaa_lib.cfg")!=AAA_ERR_SUCCESS ) {
		return -1;
	}

	if (signal(SIGINT,sig_handler)==SIG_ERR ) {
		printf("Cannot install signal handler!\n");
		return -1;
	}

	for(;;)
		sleep( 40 );

	AAAClose();

	return EXIT_SUCCESS;
}
