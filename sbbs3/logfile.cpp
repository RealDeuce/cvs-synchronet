/* logfile.cpp */

/* Synchronet log file routines */

/* $Id: logfile.cpp,v 1.1 2000/10/10 11:23:57 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include "sbbs.h"

void sbbs_t::logentry(char *code, char *entry)
{
	char str[512];

	now=time(NULL);
	sprintf(str,"Node %2d  %s\r\n   %s",cfg.node_num,timestr(&now),entry);
	logline(code,str);
}

/****************************************************************************/
/* Writes 'str' verbatim into node.log 										*/
/****************************************************************************/
void sbbs_t::log(char *str)
{
	if(logfile_fp==NULL || online==ON_LOCAL) return;
	if(logcol>=78 || (78-logcol)<strlen(str)) {
		fprintf(logfile_fp,"\r\n");
		logcol=1; }
	if(logcol==1) {
		fprintf(logfile_fp,"   ");
		logcol=4; }
	fprintf(logfile_fp,str);
	if(str[strlen(str)-1]==LF)
		logcol=1;
	else
		logcol+=strlen(str);
}

/****************************************************************************/
/* Writes 'str' on it's own line in node.log								*/
/****************************************************************************/
void sbbs_t::logline(char *code, char *str)
{
	lprintf("Node %d %s", cfg.node_num, str);
	if(logfile_fp==NULL || (online==ON_LOCAL && strcmp(code,"!!"))) return;
	if(logcol!=1)
		fprintf(logfile_fp,"\r\n");
	fprintf(logfile_fp,"%-2.2s %s\r\n",code,str);
	logcol=1;
}

/****************************************************************************/
/* Writes a comma then 'ch' to log, tracking column.						*/
/****************************************************************************/
void sbbs_t::logch(char ch, char comma)
{

	if(logfile_fp==NULL || (online==ON_LOCAL)) return;
	if((uchar)ch<SP)	/* Don't log control chars */
		return;
	if(logcol==1) {
		logcol=4;
		fprintf(logfile_fp,"   "); 
	}
	else if(logcol>=78) {
		logcol=4;
		fprintf(logfile_fp,"\r\n   "); 
	}
	if(comma && logcol!=4) {
		fprintf(logfile_fp,",");
		logcol++; 
	}
	if(ch&0x80) {
		ch&=0x7f;
		if(ch<SP)
			return;
		fprintf(logfile_fp,"/"); 
	}
	fprintf(logfile_fp,"%c",ch);
	logcol++;
}

