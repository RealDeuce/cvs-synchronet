/* Synchronet message/menu display routine */
// vi: tabstop=4
 
/* $Id: putmsg.cpp,v 1.40 2018/10/22 04:18:05 rswindell Exp $ */

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

#include "sbbs.h"
#include "wordwrap.h"

/****************************************************************************/
/* Outputs a NULL terminated string with @-code parsing,                    */
/* checking for message aborts, pauses, ANSI escape and ^A sequences.		*/
/* Changes local text attributes if necessary.                              */
/* Returns the last char of the buffer access.. 0 if not aborted.           */
/* If P_SAVEATR bit is set in mode, the attributes set by the message       */
/* will be the current attributes after the message is displayed, otherwise */
/* the attributes prior to displaying the message are always restored.      */
/* Stops parsing/displaying upon CTRL-Z (only in P_CPM_EOF mode).           */
/****************************************************************************/
char sbbs_t::putmsg(const char *buf, long mode)
{
	char	tmpatr,tmp2[256],tmp3[128];
	char	ret;
	char*	str=(char*)buf;
	uchar	exatr=0;
	int 	orgcon=console,i;
	ulong	l=0,sys_status_sav=sys_status;
	int		defered_pause=FALSE;
	uint	lines_printed = 0;

	attr_sp=0;	/* clear any saved attributes */
	tmpatr=curatr;	/* was lclatr(-1) */
	if(!(mode&P_SAVEATR))
		attr(LIGHTGRAY);
	if(mode&P_NOPAUSE)
		sys_status|=SS_PAUSEOFF;
	if(mode&P_HTML)
		putcom("\x02\x02");
	if(!(mode&P_NOATCODES) && memcmp(str, "@WORDWRAP@", 10) == 0) {
		mode |= P_WORDWRAP;
		l += 10;
	}
	if(mode&P_WORDWRAP) {
		char *wrapped;
		if((wrapped=::wordwrap((char*)str+l, cols-1, 79, /* handle_quotes: */TRUE)) == NULL)
			errormsg(WHERE,ERR_ALLOC,"wordwrap buffer",0);
		else {
			truncsp_lines(wrapped);
			str=wrapped;
			l=0;
		}
	}

	while(str[l] && (mode&P_NOABORT || !msgabort()) && online) {
		if((mode&P_TRUNCATE) && column >= (cols - 1)) {
			switch(str[l]) {
				case '\r':
				case '\n':
				case FF:
				case CTRL_A:
					break;
				default:
					l++;
					continue;
			}
		}
		if(str[l]==CTRL_A && str[l+1]!=0) {
			if(str[l+1]=='"' && !(sys_status&SS_NEST_PF)) {  /* Quote a file */
				l+=2;
				i=0;
				while(i<(int)sizeof(tmp2)-1 && isprint((unsigned char)str[l]) && str[l]!='\\' && str[l]!='/')
					tmp2[i++]=str[l++];
				if(i > 0) {
					tmp2[i]=0;
					sys_status|=SS_NEST_PF; 	/* keep it only one message deep! */
					SAFEPRINTF2(tmp3,"%s%s",cfg.text_dir,tmp2);
					printfile(tmp3,0);
					sys_status&=~SS_NEST_PF; 
				}
			}
			else {
				ctrl_a(str[l+1]);
				if((sys_status&SS_ABORT) && !lines_printed)	/* Aborted at (auto) pause prompt (e.g. due to CLS)? */
					sys_status &= ~SS_ABORT;				/* Clear the abort flag (keep displaying the msg/file) */
				l+=2;
			} 
		}
		else if((str[l]=='`' || str[l]=='�') && str[l+1]=='[') {   
			outchar(ESC); /* Convert `[ and �[ to ESC[ */
			l++; 
		}
		else if(cfg.sys_misc&SM_PCBOARD && str[l]=='@' && str[l+1]=='X'
			&& isxdigit((unsigned char)str[l+2]) && isxdigit((unsigned char)str[l+3])) {
			sprintf(tmp2,"%.2s",str+l+2);
			attr(ahtoul(tmp2));
			exatr=1;
			l+=4; 
		}
		else if(cfg.sys_misc&SM_WILDCAT && str[l]=='@' && str[l+3]=='@'
			&& isxdigit((unsigned char)str[l+1]) && isxdigit((unsigned char)str[l+2])) {
			sprintf(tmp2,"%.2s",str+l+1);
			attr(ahtoul(tmp2));
			// exatr=1;
			l+=4; 
		}
		else if(cfg.sys_misc&SM_RENEGADE && str[l]=='|' && isdigit((unsigned char)str[l+1])
			&& isdigit((unsigned char)str[l+2]) && !(useron.misc&(RIP|WIP))) {
			sprintf(tmp2,"%.2s",str+l+1);
			i=atoi(tmp2);
			if(i>=16) { 				/* setting background */
				i-=16;
				i<<=4;
				i|=(curatr&0x0f);		/* leave foreground alone */
			} 	
			else
				i|=(curatr&0xf0);		/* leave background alone */
			attr(i);
			exatr=1;
			l+=3;	/* Skip |xx */
		}	
		else if(cfg.sys_misc&SM_CELERITY && str[l]=='|' && isalpha((unsigned char)str[l+1])
			&& !(useron.misc&(RIP|WIP))) {
			switch(str[l+1]) {
				case 'k':
					attr((curatr&0xf0)|BLACK);
					break;
				case 'b':
					attr((curatr&0xf0)|BLUE);
					break;
				case 'g':
					attr((curatr&0xf0)|GREEN);
					break;
				case 'c':
					attr((curatr&0xf0)|CYAN);
					break;
				case 'r':
					attr((curatr&0xf0)|RED);
					break;
				case 'm':
					attr((curatr&0xf0)|MAGENTA);
					break;
				case 'y':
					attr((curatr&0xf0)|YELLOW);
					break;
				case 'w':
					attr((curatr&0xf0)|LIGHTGRAY);
					break;
				case 'd':
					attr((curatr&0xf0)|BLACK|HIGH);
					break;
				case 'B':
					attr((curatr&0xf0)|BLUE|HIGH);
					break;
				case 'G':
					attr((curatr&0xf0)|GREEN|HIGH);
					break;
				case 'C':
					attr((curatr&0xf0)|CYAN|HIGH);
					break;
				case 'R':
					attr((curatr&0xf0)|RED|HIGH);
					break;
				case 'M':
					attr((curatr&0xf0)|MAGENTA|HIGH);
					break;
				case 'Y':   /* Yellow */
					attr((curatr&0xf0)|YELLOW|HIGH);
					break;
				case 'W':
					attr((curatr&0xf0)|LIGHTGRAY|HIGH);
					break;
				case 'S':   /* swap foreground and background - TODO: This sets foreground to BLACK! */
					attr((curatr&0x07)<<4);
					break; 
			}
			exatr=1;
			l+=2;	/* Skip |x */
		}  /* Skip second digit if it exists */
		else if(cfg.sys_misc&SM_WWIV && str[l]==CTRL_C && isdigit((unsigned char)str[l+1])) {
			exatr=1;
			switch(str[l+1]) {
				default:
					attr(LIGHTGRAY);
					break;
				case '1':
					attr(CYAN|HIGH);
					break;
				case '2':
					attr(BROWN|HIGH);
					break;
				case '3':
					attr(MAGENTA);
					break;
				case '4':
					attr(LIGHTGRAY|HIGH|BG_BLUE);
					break;
				case '5':
					attr(GREEN);
					break;
				case '6':
					attr(RED|HIGH|BLINK);
					break;
				case '7':
					attr(BLUE|HIGH);
					break;
				case '8':
					attr(BLUE);
					break;
				case '9':
					attr(CYAN);
					break; 
			}
			l+=2; 
		}
		else {
			if(str[l]=='\n') {
				if(exatr) 	/* clear at newline for extra attr codes */
					attr(LIGHTGRAY);
				if(l==0 || str[l-1]!='\r')	/* expand sole LF to CR/LF */
					outchar('\r');
				lines_printed++;
			}

			/* ansi escape sequence */
			if(outchar_esc) {
				if(str[l]=='A' || str[l]=='B' || str[l]=='H' || str[l]=='J'
					|| str[l]=='f' || str[l]=='u')    /* ANSI anim */
					lncntr=0;			/* so defeat pause */
				if(str[l]=='"') {
					l++;				/* don't pass on keyboard reassignment */
					continue; 
				} 
			}
			if(str[l]=='!' && str[l+1]=='|' && useron.misc&(RIP|WIP)) /* RIP */
				lncntr=0;				/* so defeat pause */
			if(str[l]==ESC && str[l+1]=='$')    /* WIP command */
				lncntr=0;
			if(str[l]=='@' && !(mode&P_NOATCODES)) {
				if(memcmp(str+l, "@EOF@", 5) == 0)
					break;
				if(memcmp(str+l, "@CLEAR@", 7) == 0) {
					CLS;
					l += 7;
					while(str[l] != 0 && (str[l] == '\r' || str[l] == '\n'))
						l++;
					continue;
				}
				if(memcmp(str+l, "@CENTER@", 8) == 0) {
					l += 8;
					i=0;
					while(i<(int)sizeof(tmp2)-1 && str[l] != 0 && str[l] != '\r')
						tmp2[i++] = str[l++];
					tmp2[i] = 0;
					truncsp(tmp2);
					center(tmp2);
					if(str[l] == '\r')
						l++;
					if(str[l] == '\n')
						l++;
					continue;
				}
				if(memcmp(str+l, "@SYSONLY@", 9) == 0) {
					if(!SYSOP)
						console^=CON_ECHO_OFF;
					l += 9;
					continue;
				}
				/* In HTML mode, defer PAUSE and MORE to end and suppress message */
				if(mode&P_HTML) {
					if(!memcmp(str+l,"@MORE@",6)) {
						defered_pause=TRUE;
						l+=6;
						continue;
					}
					if(!memcmp(str+l,"@PAUSE@",7)) {
						defered_pause=TRUE;
						l+=7;
						continue;
					}
				}
				i=show_atcode((char *)str+l);	/* returns 0 if not valid @ code */
				l+=i;					/* i is length of code string */
				if((sys_status&SS_ABORT) && !lines_printed)	/* Aborted at (auto) pause prompt (e.g. due to CLS)? */
					sys_status &= ~SS_ABORT;				/* Clear the abort flag (keep displaying the msg/file) */
				if(i)					/* if valid string, go to top */
					continue; 
			}
			if(mode&P_CPM_EOF && str[l]==CTRL_Z)
				break;
			outchar(str[l]);
#if 0
				if(!(mode&P_HTML) && !exatr && !outchar_esc && lncntr && lbuflen && cols && ++col==cols)
					lncntr++;
				else
					col=0;
#endif
			l++; 
		} 
	}
	if(mode&P_HTML)
		putcom("\x02");
	if(!(mode&P_SAVEATR)) {
		console=orgcon;
		attr(tmpatr); 
	}

	attr_sp=0;	/* clear any saved attributes */

	/* Handle defered pauses */
	if(defered_pause) {
		if(mode&P_HTML)
			getkey(0);
		else
			pause();
	}

	ret=str[l];
	if(str!=buf)	/* malloc'd copy of buffer */
		free(str);

	/* Restore original settings of Forced Pause On/Off */
	sys_status&=~(SS_PAUSEOFF|SS_PAUSEON);
	sys_status|=(sys_status_sav&(SS_PAUSEOFF|SS_PAUSEON));
	return(ret);
}

