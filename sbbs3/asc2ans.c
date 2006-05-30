/* asc2ans.c */

/* Converts Synchronet Ctrl-A codes into ANSI escape sequences */

/* $Id: asc2ans.c,v 1.4 2006/05/09 23:03:09 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2003 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <stdio.h>
#include <ctype.h>	/* toupper */
#include <string.h>	/* strcmp */

#define ANSI fprintf(out,"\x1b[")

int main(int argc, char **argv)
{
	char	revision[16];
	int		ch;
	FILE*	in;
	FILE*	out;

	sscanf("$Revision: 1.4 $", "%*s %s", revision);

	if(argc<2) {
		fprintf(stderr,"\nasc2ans %s\n",revision);
		fprintf(stderr,"\nusage: %s infile.asc [outfile.ans]\n",argv[0]);
		return(0); 
	}

	if(strcmp(argv[1],"-")) {
		if((in=fopen(argv[1],"rb"))==NULL) {
			perror(argv[1]);
			return(1);
		}
	}
	else
		in=stdin;

	if(argc > 2 && (strcmp(argv[2],"-"))) {
		if((out=fopen(argv[2],"wb"))==NULL) {
			perror(argv[2]);
			return(1);
		}
	}
	else
		out=stdout;

	while((ch=fgetc(in))!=EOF) {
		if(ch==1) { /* ctrl-a */
			ch=fgetc(in);
			if(ch==EOF)
				break;
			if(ch>=0x7f) {					/* move cursor right x columns */
				ANSI;
				fprintf(out,"%uC",ch-0x7f);
				continue; 
			}
			switch(toupper(ch)) {
				case 'A':
					fputc('\1',out);
					break;
				case '<':
					fputc('\b',out);
					break;
				case '>':
					ANSI;
					fputc('K',out);
					break;
				case '[':
					fputc('\r',out);
					break;
				case ']':
					fputc('\n',out);
					break;
				case 'L':
					ANSI;
					fprintf(out,"2J");
					break;
				case '-':
				case '_':
				case 'N':
					ANSI;
					fprintf(out,"0m");
					break;
				case 'H':
					ANSI;
					fprintf(out,"1m");
					break;
				case 'I':
					ANSI;
					fprintf(out,"5m");
					break;
				case 'K':
					ANSI;
					fprintf(out,"30m");
					break;
				case 'R':
					ANSI;
					fprintf(out,"31m");
					break;
				case 'G':
					ANSI;
					fprintf(out,"32m");
					break;
				case 'Y':
					ANSI;
					fprintf(out,"33m");
					break;
				case 'B':
					ANSI;
					fprintf(out,"34m");
					break;
				case 'M':
					ANSI;
					fprintf(out,"35m");
					break;
				case 'C':
					ANSI;
					fprintf(out,"36m");
					break;
				case 'W':
					ANSI;
					fprintf(out,"37m");
					break;
				case '0':
					ANSI;
					fprintf(out,"40m");
					break;
				case '1':
					ANSI;
					fprintf(out,"41m");
					break;
				case '2':
					ANSI;
					fprintf(out,"42m");
					break;
				case '3':
					ANSI;
					fprintf(out,"43m");
					break;
				case '4':
					ANSI;
					fprintf(out,"44m");
					break;
				case '5':
					ANSI;
					fprintf(out,"45m");
					break;
				case '6':
					ANSI;
					fprintf(out,"46m");
					break;
				case '7':
					ANSI;
					fprintf(out,"47m");
					break;
				default:
					fprintf(out,"\1%c",ch);
					break; 
			} 
		}
		else
			fputc(ch,out); 
	}

	return(0);
}	




