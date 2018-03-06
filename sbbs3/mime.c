/* Synchronet MIME functions, originally written/submitted by Marc Lanctot */

/* $Id: mime.c,v 1.10 2018/03/04 07:11:56 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

/* See RFCs 1521,2045,2046,2047,2048,2049,1590 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "sbbs.h"
#include "mime.h"
#include "mailsrvr.h"
#include "base64.h"

#define SIZEOF_MIMEBOUNDARY     36

char* mimegetboundary()
{
    int		i, num;
    char*	boundaryString = (char*)malloc(SIZEOF_MIMEBOUNDARY + 1);

    srand((unsigned int)time(NULL));
    if (boundaryString == NULL) 
        return NULL;
    for (i=0;i<SIZEOF_MIMEBOUNDARY;i++) {
        num=(rand()%62);    
        if(num<10)
            num+=48;  
        else if(num>=10 && num<36)
            num+=55;
        else
            num+=61;
        boundaryString[i]=(char)num;
    }
    boundaryString[i]='\0';
    return boundaryString;
}

void mimeheaders(SOCKET socket, int sess, char* boundary)
{
    sockprintf(socket,sess,"MIME-Version: 1.0");
    sockprintf(socket,sess,"Content-Type: multipart/mixed;");
    sockprintf(socket,sess," boundary=\"%s\"",boundary);
}

void mimeblurb(SOCKET socket, int sess, char* boundary)
{
    sockprintf(socket,sess,"This is a multi-part message in MIME format.");
    sockprintf(socket,sess,"");
}

void mimetextpartheader(SOCKET socket, int sess, char* boundary, const char* charset)
{
	if(charset == NULL || *charset == 0)
		charset = "iso-8859-1";
    sockprintf(socket,sess,"--%s",boundary);
    sockprintf(socket,sess,"Content-Type: text/plain;");
    sockprintf(socket,sess," charset=\"%s\"", charset);
    sockprintf(socket,sess,"Content-Transfer-Encoding: 7bit");
}

BOOL base64out(SOCKET socket, int sess, char* pathfile)
{
    FILE *  fp;
    char    in[57];
    char    out[77];
    int     bytesread;

    if((fp=fopen(pathfile,"rb"))==NULL) 
        return(FALSE);
    while(1) {
        bytesread=fread(in,1,sizeof(in),fp);
		if((b64_encode(out,sizeof(out),in,bytesread)==-1)
				|| !sockprintf(socket,sess,out))  {
			fclose(fp);
			return(FALSE);
		}
        if(bytesread!=sizeof(in) || feof(fp))
            break;
    }
	fclose(fp);
    sockprintf(socket,sess,"");
	return(TRUE);
}

BOOL mimeattach(SOCKET socket, int sess, char* boundary, char* pathfile)
{
    char* fname = getfname(pathfile);

    sockprintf(socket,sess,"--%s",boundary);
    sockprintf(socket,sess,"Content-Type: application/octet-stream;");
    sockprintf(socket,sess," name=\"%s\"",fname);
    sockprintf(socket,sess,"Content-Transfer-Encoding: base64");
    sockprintf(socket,sess,"Content-Disposition: attachment;");
    sockprintf(socket,sess," filename=\"%s\"",fname);
    sockprintf(socket,sess,"");
    if(!base64out(socket,sess,pathfile))
		return(FALSE);
    sockprintf(socket,sess,"");
	return(TRUE);
}

void endmime(SOCKET socket, int sess, char* boundary)
{
	/* last boundary */
    sockprintf(socket,sess,"--%s--",boundary);
    sockprintf(socket,sess,"");
}
