/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Mon Oct 28 11:54:28 CET 2002
    copyright            : (C) 2002 by Illya Komarov
    email                : komarov@fokus.gmd.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "diameter_api.h"

int main(int argc, char *argv[])
{
	//AAASessionId *SID;
	//AAA_AVP *avp;
	//char buf[500];
	//FILE* fp = fopen("../../../diameter_dump","r");
	//char source[102400];
	//AAAMessage* req;
	//AAAMessage* res;
	//int i=0;

	if( AAAOpen("cucu.txt")!=AAA_ERR_SUCCESS ) {
		return -1;
	}
	//AAAStartSession( &SID, 0, 0, 0);
	//printf("Session ID=<%.*s> [%p]\n",SID->len,SID->s,SID->s);
	//req = AAANewMessage( 34, 0, SID, 0, 0);
	//AAASendMessage( req );

	for(;;)
		sleep( 5 );

	//while(feof(fp)==0){
	//	source[i] = fgetc(fp);
	//	i++;
	//}
	//req = AAATranslateMessage(source+0x6a/*+608+82*/,i);
	//res = AAANewMessage( 34, 0, 0, 0, req );

	AAAClose();
  return EXIT_SUCCESS;
}
