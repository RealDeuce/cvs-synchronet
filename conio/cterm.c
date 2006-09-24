/* $Id: cterm.c,v 1.89 2006/09/02 08:25:24 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2006 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
 #include <malloc.h>	/* alloca() on Win32 */
#endif

#include <genwrap.h>
#include <xpbeep.h>
#include <link_list.h>
#ifdef __unix__
	#include <xpsem.h>
#endif
#include <threadwrap.h>

#if (defined CIOLIB_IMPORTS)
 #undef CIOLIB_IMPORTS
#endif
#if (defined CIOLIB_EXPORTS)
 #undef CIOLIB_EXPORTS
#endif

#include "ciolib.h"
#include "keys.h"

#include "cterm.h"
#include "allfonts.h"

#define	BUFSIZE	2048

struct cterminal cterm;

/* const int tabs[11]={1,8,16,24,32,40,48,56,64,72,80}; */
const int cterm_tabs[11]={9,17,25,33,41,49,57,65,73,80,80};

const char *octave="C#D#EF#G#A#B";

/* Characters allowed in music strings... if one that is NOT in here is
 * found, CTerm leaves music capture mode and tosses the buffer away */
const char *musicchars="aAbBcCdDeEfFgGlLmMnNoOpPsStT0123456789.-+#<> ";
const uint note_frequency[]={	/* Hz*1000 */
/* Octave 0 (Note 1) */
	 65406
	,69296
	,73416
	,77782
	,82407
	,87307
	,92499
	,97999
	,103820
	,110000
	,116540
	,123470
/* Octave 1 */
	,130810
	,138590
	,146830
	,155560
	,164810
	,174610
	,185000
	,196000
	,207650
	,220000
	,233080
	,246940
/* Octave 2 */
	,261620
	,277180
	,293660
	,311130
	,329630
	,349230
	,369990
	,392000
	,415300
	,440000
	,466160
	,493880
/* Octave 3 */
	,523250
	,554370
	,587330
	,622250
	,659260
	,698460
	,739990
	,783990
	,830610
	,880000
	,932330
	,987770
/* Octave 4 */
	,1046500
	,1108700
	,1174700
	,1244500
	,1318500
	,1396900
	,1480000
	,1568000
	,1661200
	,1760000
	,1864600
	,1975500
/* Octave 5 */
	,2093000
	,2217500
	,2349300
	,2489000
	,2637000
	,2793800
	,2959900
	,3136000
	,3322400
	,3520000
	,3729300
	,3951100
/* Octave 6 */
	,4186000
	,4435000
	,4698600
	,4978000
	,5274000
	,5587000
	,5920000
	,6272000
	,6644000
	,7040000
	,7458600
	,7902200
};

struct note_params {
	int notenum;
	int	notelen;
	int	dotted;
	int	tempo;
	int	noteshape;
	int	foreground;
};

static int playnote_thread_running=FALSE;
static link_list_t	notes;
sem_t	playnote_thread_terminated;
sem_t	note_completed_sem;

void playnote_thread(void *args)
{
	/* Tempo is quarter notes per minute */
	int duration;
	int pauselen;
	struct note_params *note;
	int	device_open=FALSE;

	playnote_thread_running=TRUE;
	while(1) {
		if(device_open) {
			if(!listSemTryWaitBlock(&notes,5000)) {
				xptone_close();
				device_open=FALSE;
				listSemWait(&notes);
			}
		}
		else
			listSemWait(&notes);
		xptone_open();
		device_open=TRUE;
		note=listShiftNode(&notes);
		if(note==NULL)
			break;
		if(note->dotted)
			duration=360000/note->tempo;
		else
			duration=240000/note->tempo;
		duration/=note->notelen;
		switch(note->noteshape) {
			case CTERM_MUSIC_STACATTO:
				pauselen=duration/4;
				break;
			case CTERM_MUSIC_LEGATO:
				pauselen=0;
				break;
			case CTERM_MUSIC_NORMAL:
			default:
				pauselen=duration/8;
				break;
		}
		duration-=pauselen;
		if(note->notenum < 72 && note->notenum >= 0)
			xptone(((double)note_frequency[note->notenum])/1000,duration,WAVE_SHAPE_SINE_SAW_HARM);
		else
			SLEEP(duration);
		SLEEP(pauselen);
		if(note->foreground)
			sem_post(&note_completed_sem);
		free(note);
	}
	playnote_thread_running=FALSE;
	sem_post(&playnote_thread_terminated);
}

void play_music(void)
{
	int		i;
	char	*p;
	char	*out;
	int		offset;
	char	note;
	int		notelen;
	char	numbuf[10];
	int		dotted;
	int		notenum;
	struct	note_params *np;
	int		fore_count;

	p=cterm.musicbuf;
	fore_count=0;
	if(cterm.music==1) {
		switch(toupper(*p)) {
			case 'F':
				cterm.musicfore=TRUE;
				p++;
				break;
			case 'B':
				cterm.musicfore=FALSE;
				p++;
				break;
			case 'N':
				if(!isdigit(*(p+1))) {
					cterm.noteshape=CTERM_MUSIC_NORMAL;
					p++;
				}
				break;
			case 'L':
				cterm.noteshape=CTERM_MUSIC_LEGATO;
				p++;
				break;
			case 'S':
				cterm.noteshape=CTERM_MUSIC_STACATTO;
				p++;
				break;
		}
	}
	for(;*p;p++) {
		notenum=0;
		offset=0;
		switch(toupper(*p)) {
			case 'M':
				p++;
				switch(toupper(*p)) {
					case 'F':
						cterm.musicfore=TRUE;
						break;
					case 'B':
						cterm.musicfore=FALSE;
						break;
					case 'N':
						cterm.noteshape=CTERM_MUSIC_NORMAL;
						break;
					case 'L':
						cterm.noteshape=CTERM_MUSIC_LEGATO;
						break;
					case 'S':
						cterm.noteshape=CTERM_MUSIC_STACATTO;
						break;
					default:
						p--;
				}
				break;
			case 'T':						/* Tempo */
				out=numbuf;
				while(isdigit(*(p+1)))
					*(out++)=*(++p);
				*out=0;
				cterm.tempo=atoi(numbuf);
				if(cterm.tempo>255)
					cterm.tempo=255;
				if(cterm.tempo<32)
					cterm.tempo=32;
				break;
			case 'O':						/* Octave */
				out=numbuf;
				while(isdigit(*(p+1)))
					*(out++)=*(++p);
				*out=0;
				cterm.octave=atoi(numbuf);
				if(cterm.octave>6)
					cterm.octave=6;
				break;
			case 'L':
				out=numbuf;
				while(isdigit(*(p+1)))
					*(out++)=*(++p);
				*out=0;
				cterm.notelen=atoi(numbuf);
				if(cterm.notelen<1)
					cterm.notelen=1;
				if(cterm.notelen>64)
					cterm.notelen=64;
				break;
			case 'N':						/* Note by number */
				if(isdigit(*(p+1))) {
					out=numbuf;
					while(isdigit(*(p+1))) {
						*(out++)=*(p+1);
						p++;
					}
					*out=0;
					notenum=atoi(numbuf);
				}
				if(notenum==0) {
					notenum=-1;
					offset=1;
				}
				/* Fall-through */
			case 'A':						/* Notes in current octave by letter */
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'P':
				note=toupper(*p);
				notelen=cterm.notelen;
				dotted=0;
				i=1;
				while(i) {
					i=0;
					if(*(p+1)=='+' || *(p+1)=='#') {	/* SHARP */
						offset+=1;
						p++;
						i=1;
					}
					if(*(p+1)=='-') {					/* FLAT */
						offset-=1;
						p++;
						i=1;
					}
					if(*(p+1)=='.') {					/* Dotted note (1.5*notelen) */
						dotted=1;
						p++;
						i=1;
					}
					if(isdigit(*(p+1))) {
						out=numbuf;
						while(isdigit(*(p+1))) {
							*(out++)=*(p+1);
							p++;
						}
						*out=0;
						notelen=atoi(numbuf);
						i=1;
					}
				}
				if(note=='P') {
					notenum=-1;
					offset=0;
				}
				if(notenum==0) {
					out=strchr(octave,note);
					if(out==NULL) {
						notenum=-1;
						offset=1;
					}
					else {
						notenum=cterm.octave*12+1;
						notenum+=(out-octave);
					}
				}
				notenum+=offset;
				np=(struct note_params *)malloc(sizeof(struct note_params));
				if(np!=NULL) {
					np->notenum=notenum;
					np->notelen=notelen;
					np->dotted=dotted;
					np->tempo=cterm.tempo;
					np->noteshape=cterm.noteshape;
					np->foreground=cterm.musicfore;
					listPushNode(&notes, np);
					if(cterm.musicfore)
						fore_count++;
				}
				break;
			case '<':							/* Down one octave */
				cterm.octave--;
				if(cterm.octave<0)
					cterm.octave=0;
				break;
			case '>':							/* Up one octave */
				cterm.octave++;
				if(cterm.octave>6)
					cterm.octave=6;
				break;
		}
	}
	cterm.music=0;
	cterm.musicbuf[0]=0;
	while(fore_count) {
		sem_wait(&note_completed_sem);
		fore_count--;
	}
}

void scrolldown(void)
{
	char *buf;
	int i,j;

	buf=(char *)alloca(cterm.width*(cterm.height-1)*2);
	gettext(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y+cterm.height-2,buf);
	puttext(cterm.x,cterm.y+1,cterm.x+cterm.width-1,cterm.y+cterm.height-1,buf);
	j=0;
	for(i=0;i<cterm.width;i++) {
		buf[j++]=' ';
		buf[j++]=cterm.attr;
	}
	puttext(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y,buf);
}

void scrollup(void)
{
	char *buf;
	int i,j;

	cterm.backpos++;
	if(cterm.scrollback!=NULL) {
		if(cterm.backpos>cterm.backlines) {
			memmove(cterm.scrollback,cterm.scrollback+cterm.width*2,cterm.width*2*(cterm.backlines-1));
			cterm.backpos--;
		}
		gettext(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y,cterm.scrollback+(cterm.backpos-1)*cterm.width*2);
	}
	buf=(char *)alloca(cterm.width*(cterm.height-1)*2);
	gettext(cterm.x,cterm.y+1,cterm.x+cterm.width-1,cterm.y+cterm.height-1,buf);
	puttext(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y+cterm.height-2,buf);
	j=0;
	for(i=0;i<cterm.width;i++) {
		buf[j++]=' ';
		buf[j++]=cterm.attr;
	}
	puttext(cterm.x,cterm.y+cterm.height-1,cterm.x+cterm.width-1,cterm.y+cterm.height-1,buf);
}

void dellines(int lines)
{
	char *buf;
	int i,j,k;
	int linestomove;

	if(lines<1)
		return;
	linestomove=cterm.height-wherey();
	buf=(char *)alloca(cterm.width*(linestomove>lines?linestomove:lines)*2);
	gettext(cterm.x,cterm.y+wherey()-1+lines,cterm.x+cterm.width-1,cterm.y+cterm.height-1,buf);
	puttext(cterm.x,cterm.y+wherey()-1,cterm.x+cterm.width-1,cterm.y+cterm.height-1-lines,buf);
	j=0;
	k=cterm.width*lines;
	for(i=0;i<k;i++) {
		buf[j++]=' ';
		buf[j++]=cterm.attr;
	}
	puttext(cterm.x,cterm.y+cterm.height-lines,cterm.x+cterm.width-1,cterm.y+cterm.height-1,buf);
}

void clear2bol(void)
{
	char *buf;
	int i,j,k;

	k=wherex();
	buf=(char *)alloca(k*2);
	j=0;
	for(i=0;i<k;i++) {
		buf[j++]=' ';
		buf[j++]=cterm.attr;
	}
	puttext(cterm.x,cterm.y+wherey()-1,cterm.x+wherex()-1,cterm.y+wherey()-1,buf);
}

void clear2eol(void)
{
	clreol();
}

void clearscreen(char attr)
{
	if(cterm.scrollback!=NULL) {
		cterm.backpos+=cterm.height;
		if(cterm.backpos>cterm.backlines) {
			memmove(cterm.scrollback,cterm.scrollback+cterm.width*2*(cterm.backpos-cterm.backlines),cterm.width*2*(cterm.backlines-(cterm.backpos-cterm.backlines)));
			cterm.backpos=cterm.backlines;
		}
		gettext(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y+cterm.height-1,cterm.scrollback+(cterm.backpos-cterm.height)*cterm.width*2);
	}
	clrscr();
}

void do_ansi(char *retbuf, size_t retsize, int *speed)
{
	char	*p;
	char	*p2;
	char	tmp[1024];
	int		i,j,k,l;
	int		row,col;

	switch(cterm.escbuf[0]) {
		case '[':
			/* ANSI stuff */
			p=cterm.escbuf+strlen(cterm.escbuf)-1;
			if(cterm.escbuf[1]>=60 && cterm.escbuf[1] <= 63) {	/* Private extenstions */
				switch(*p) {
					case 'M':
						if(cterm.escbuf[1] == '=') {	/* ANSI Music setup */
							i=atoi(cterm.escbuf+2);
							switch(i) {
								case 1:					/* BANSI (ESC[N) music only) */
									cterm.music_enable=CTERM_MUSIC_BANSI;
									break;
								case 2:					/* ESC[M ANSI music */
									cterm.music_enable=CTERM_MUSIC_ENABLED;
									break;
								default:					/* Disable ANSI Music */
									cterm.music_enable=CTERM_MUSIC_SYNCTERM;
									break;
							}
						}
						break;
					case '{':
						if(cterm.escbuf[1] == '=') {	/* Font loading */
							i=255;
							j=0;
							if(strlen(cterm.escbuf)>2) {
								if((p=strtok(cterm.escbuf+2,";"))!=NULL) {
									i=atoi(p);
									if(!i && cterm.escbuf[2] != '0')
										i=255;
									if((p=strtok(NULL,";"))!=NULL) {
										j=atoi(p);
									}
								}
							}
							if(i>255)
								break;
							cterm.font_start_time=time(NULL);
							cterm.font_read=0;
							cterm.font_slot=i;
							switch(j) {
								case 0:
									cterm.font_size=4096;
									break;
								case 1:
									cterm.font_size=3586;
									break;
								case 2:
									cterm.font_size=2048;
									break;
								default:
									cterm.font_size=0;
									break;
							}
						}
						break;
				}
				break;
			}
			switch(*p) {
				case '@':	/* Insert Char */
					i=wherex();
					j=wherey();
					k=atoi(cterm.escbuf+1);
					if(k<1)
						k=1;
					if(k>cterm.width - j)
						k=cterm.width - j;
					gettext(cterm.x+i-1,cterm.y+j-1,cterm.x+cterm.width-1-k,cterm.y+j-1,tmp);
					for(l=0; l< k; l++)
						putch(' ');
					puttext(cterm.x+i-1+k,cterm.y+j-1,cterm.x+cterm.width-1,cterm.y+j-1,tmp);
					gotoxy(i,j);
					break;
				case 'A':	/* Cursor Up */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					i=wherey()-i;
					if(i<1)
						i=1;
					gotoxy(wherex(),i);
					break;
				case 'B':	/* Cursor Down */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					i=wherey()+i;
					if(i>cterm.height)
						i=cterm.height;
					gotoxy(wherex(),i);
					break;
				case 'C':	/* Cursor Right */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					i=wherex()+i;
					if(i>cterm.width)
						i=cterm.width;
					gotoxy(i,wherey());
					break;
				case 'D':	/* Cursor Left and Font Select */
					if(*(p-1)==' ') {	/* Font Select */
						i=0;
						j=0;
						if(strlen(cterm.escbuf)>2) {
							if((p=strtok(cterm.escbuf+1,";"))!=NULL) {
								i=atoi(p);
								if((p=strtok(NULL,";"))!=NULL) {
									j=atoi(p);
								}
							}
							if(i==0) {	/* Only the primary font is currently supported */
								setfont(j,FALSE);
							}
						}
					}
					else {
						i=atoi(cterm.escbuf+1);
						if(i==0)
							i=1;
						i=wherex()-i;
						if(i<1)
							i=1;
						gotoxy(i,wherey());
					}
					break;
				case 'E':	/* Cursor next line */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					i=wherey()+i;
					if(i>cterm.height)
						i=cterm.height;
					gotoxy(1,i);
					break;
				case 'F':	/* Cursor preceding line */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					i=wherey()-i;
					if(i<1)
						i=1;
					gotoxy(1,i);
					break;
				case 'f':
				case 'H':
					row=1;
					col=1;
					*p=0;
					if(strlen(cterm.escbuf)>1) {
						if((p=strtok(cterm.escbuf+1,";"))!=NULL) {
							row=atoi(p);
							if((p=strtok(NULL,";"))!=NULL) {
								col=atoi(p);
							}
						}
					}
					if(row<1)
						row=1;
					if(col<1)
						col=1;
					if(row>cterm.height)
						row=cterm.height;
					if(col>cterm.width)
						col=cterm.width;
					gotoxy(col,row);
					break;
				case 'J':
					i=atoi(cterm.escbuf+1);
					switch(i) {
						case 0:
							clear2eol();
							p2=(char *)alloca(cterm.width*2);
							j=0;
							for(i=0;i<cterm.width;i++) {
								p2[j++]=' ';
								p2[j++]=cterm.attr;
							}
							for(i=wherey()+1;i<=cterm.height;i++) {
								puttext(cterm.x,cterm.y+i-1,cterm.x+cterm.width-1,cterm.y+i-1,p2);
							}
							break;
						case 1:
							clear2bol();
							p2=(char *)alloca(cterm.width*2);
							j=0;
							for(i=0;i<cterm.width;i++) {
								p2[j++]=' ';
								p2[j++]=cterm.attr;
							}
							for(i=wherey()-1;i>=1;i--) {
								puttext(cterm.x,cterm.y+i-1,cterm.x+cterm.width-1,cterm.y+i-1,p2);
							}
							break;
						case 2:
							clearscreen((char)cterm.attr);
							gotoxy(1,1);
							break;
					}
					break;
				case 'K':
					i=atoi(cterm.escbuf+1);
					switch(i) {
						case 0:
							clear2eol();
							break;
						case 1:
							clear2bol();
							break;
						case 2:
							p2=(char *)alloca(cterm.width*2);
							j=0;
							for(i=0;i<cterm.width;i++) {
								p2[j++]=' ';
								p2[j++]=cterm.attr;
							}
							puttext(cterm.x,cterm.y+wherey()-1,cterm.x+cterm.width-1,cterm.y+wherey()-1,p2);
							break;
					}
					break;
				case 'L':		/* Insert line */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					if(i>cterm.height-wherey())
						i=cterm.height-wherey();
					if(i) {
						p2=(char *)alloca((cterm.height-wherey()-i+1)*cterm.width*2);
						gettext(cterm.x,cterm.y+wherey()-1,cterm.x+cterm.width-1,cterm.y+cterm.height-1-i,p2);
						puttext(cterm.x,cterm.y+wherey()-1+i,cterm.x+cterm.width-1,cterm.y+cterm.height-1,p2);
					}
					else {
						p2=(char *)alloca(cterm.width*2);
					}
					j=0;
					for(k=0;k<cterm.width;k++) {
						p2[j++]=' ';
						p2[j++]=cterm.attr;
					}
					for(j=0;j<i;j++) {
						puttext(cterm.x,cterm.y+wherey()-1+j,cterm.x+cterm.width-1,cterm.y+wherey()-1+j,p2);
					}
					break;
				case 'M':	/* ANSI music and also supposed to be delete line! */
					if(cterm.music_enable==CTERM_MUSIC_ENABLED) {
						cterm.music=1;
					}
					else {
						i=atoi(cterm.escbuf+1);
						if(i<1)
							i=1;
						dellines(i);
					}
					break;
				case 'N':
					/* BananANSI style... does NOT start with MF or MB */
					/* This still conflicts (ANSI erase field) */
					if(cterm.music_enable >= CTERM_MUSIC_BANSI)
						cterm.music=2;
					break;
				case 'P':	/* Delete char */
					i=atoi(cterm.escbuf+1);
					if(i==0)
						i=1;
					if(i>cterm.width-wherex()+1)
						i=cterm.width-wherex()+1;
					p2=(char *)alloca((cterm.width-wherex()+1)*2);
					gettext(cterm.x+wherex(),cterm.y+wherey()-1,cterm.x+cterm.width-1,cterm.y+wherey()-1,p2);
					j=(cterm.width-wherex())*2;
					for(k=0;k<0;k++) {
						p2[j++]=' ';
						p2[j++]=cterm.attr;
					}
					puttext(cterm.x+wherex()-1,cterm.y+wherey()-1,cterm.x+cterm.width-1,cterm.y+wherey()-1,p2);
					break;
				case 'S':
					i=atoi(cterm.escbuf+1);
					if(i==0 && cterm.escbuf[1] != '0')
						i=1;
					for(j=0; j<i; j++)
						scrollup();
					break;
				case 'T':
					i=atoi(cterm.escbuf+1);
					if(i==0 && cterm.escbuf[1] != '0')
						i=1;
					for(j=0; j<i; j++)
						scrolldown();
					break;
#if 0
				case 'U':
					clearscreen(7);
					gotoxy(1,1);
					break;
#endif
				case 'X':
					i=atoi(cterm.escbuf+1);
					if(i<1)
						i=1;
					if(i>cterm.width-wherex())
						i=cterm.width-wherex();
					p2=alloca(i*2);
					j=0;
					for(k=0;k<i;k++) {
						p2[j++]=' ';
						p2[j++]=cterm.attr;
					}
					puttext(cterm.x+wherex()-1,cterm.y+wherey()-1,cterm.x+wherex()-1+i-1,cterm.y+wherey()-1,p2);
					break;
				case 'Z':
					i=atoi(cterm.escbuf+1);
					if(i==0 && cterm.escbuf[0] != '0')
						i=1;
					for(j=10;j>=0;j--) {
						if(cterm_tabs[j]<wherex()) {
							k=j-i+1;
							if(k<0)
								k=0;
							gotoxy(cterm_tabs[k],wherey());
							break;
						}
					}
					break;
				case 'b':	/* ToDo?  Banana ANSI */
					break;
				case 'c':	/* Device Attributes */
					i=atoi(cterm.escbuf+1);
					if(!i) {
						if(retbuf!=NULL) {
							if(strlen(retbuf)+strlen(cterm.DA) < retsize)
								strcat(retbuf,cterm.DA);
						}
					}
					break;
				case 'g':	/* ToDo?  VT100 Tabs */
					break;
				case 'h':	/* ToDo?  Scrolling regeion, word-wrap, doorway mode */
					break;
				case 'i':	/* ToDo?  Printing */
					break;
				case 'l':	/* ToDo?  Scrolling regeion, word-wrap, doorway mode */
					break;
				case 'm':
					*(p--)=0;
					p2=cterm.escbuf+1;
					if(p2>p) {
						cterm.attr=7;
						break;
					}
					while((p=strtok(p2,";"))!=NULL) {
						p2=NULL;
						switch(atoi(p)) {
							case 0:
								cterm.attr=7;
								break;
							case 1:
								cterm.attr|=8;
								break;
							case 2:
								cterm.attr&=247;
								break;
							case 4:	/* Underscore */
								break;
							case 5:
							case 6:
								cterm.attr|=128;
								break;
							case 7:
							case 27:
								i=cterm.attr&7;
								j=cterm.attr&112;
								cterm.attr &= 136;
								cterm.attr |= j>>4;
								cterm.attr |= i<<4;
								break;
							case 8:
								j=cterm.attr&112;
								cterm.attr&=112;
								cterm.attr |= j>>4;
								break;
							case 22:
								cterm.attr &= 0xf7;
								break;
							case 25:
								cterm.attr &= 0x7f;
								break;
							case 30:
								cterm.attr&=248;
								break;
							case 31:
								cterm.attr&=248;
								cterm.attr|=4;
								break;
							case 32:
								cterm.attr&=248;
								cterm.attr|=2;
								break;
							case 33:
								cterm.attr&=248;
								cterm.attr|=6;
								break;
							case 34:
								cterm.attr&=248;
								cterm.attr|=1;
								break;
							case 35:
								cterm.attr&=248;
								cterm.attr|=5;
								break;
							case 36:
								cterm.attr&=248;
								cterm.attr|=3;
								break;
							case 37:
							case 39:
								cterm.attr&=248;
								cterm.attr|=7;
								break;
							case 40:
								cterm.attr&=143;
								break;
							case 41:
								cterm.attr&=143;
								cterm.attr|=4<<4;
								break;
							case 42:
								cterm.attr&=143;
								cterm.attr|=2<<4;
								break;
							case 43:
								cterm.attr&=143;
								cterm.attr|=6<<4;
								break;
							case 44:
								cterm.attr&=143;
								cterm.attr|=1<<4;
								break;
							case 45:
								cterm.attr&=143;
								cterm.attr|=5<<4;
								break;
							case 46:
								cterm.attr&=143;
								cterm.attr|=3<<4;
								break;
							case 47:
							case 49:
								cterm.attr&=143;
								cterm.attr|=7<<4;
								break;
						}
					}
					textattr(cterm.attr);
					break;
				case 'n':
					i=atoi(cterm.escbuf+1);
					switch(i) {
						case 5:
							if(retbuf!=NULL) {
								strcpy(tmp,"\x1b[0n");
								if(strlen(retbuf)+strlen(tmp) < retsize)
									strcat(retbuf,tmp);
							}
							break;
						case 6:
							if(retbuf!=NULL) {
								sprintf(tmp,"%c[%d;%dR",27,wherey(),wherex());
								if(strlen(retbuf)+strlen(tmp) < retsize)
									strcat(retbuf,tmp);
							}
							break;
						case 255:
							if(retbuf!=NULL) {
								sprintf(tmp,"%c[%d;%dR",27,cterm.height,cterm.width);
								if(strlen(retbuf)+strlen(tmp) < retsize)
									strcat(retbuf,tmp);
							}
							break;
					}
					break;
				case 'p': /* ToDo?  ANSI keyboard reassignment */
					break;
				case 'q': /* ToDo?  VT100 keyboard lights */
					break;
				case 'r': /* ToDo?  Scrolling reigon */
					/* Set communication speed (if has a * before it) */
					if(*(p-1) == '*' && speed != NULL) {
						/*
						 * Ps1 			Comm Line 		Ps2 		Communication Speed
						 * none, 0, 1 	Host Transmit 	none, 0 	Use default speed.
						 * 2		 	Host Receive 	1 			300
						 * 3 			Printer 		2 			600
						 * 4		 	Modem Hi 		3 			1200
						 * 5		 	Modem Lo 		4 			2400
						 * 								5 			4800
						 * 								6 			9600
						 * 								7 			19200
						 * 								8 			38400
						 * 								9 			57600
						 * 								10 			76800
						 * 								11 			115200
						 */
						int newspeed=0;

						*(--p)=0;
						if(cterm.escbuf[1]) {
							p=strtok(cterm.escbuf+1,";");
							if(p!=NULL) {
								if(p!=cterm.escbuf+1 || atoi(p)<2) {
									if(p==cterm.escbuf+1)
										p=strtok(NULL,";");
									if(p!=NULL) {
										switch(atoi(p)) {
											case 0:
												newspeed=0;
												break;
											case 1:
												newspeed=300;
												break;
											case 2:
												newspeed=600;
												break;
											case 3:
												newspeed=1200;
												break;
											case 4:
												newspeed=2400;
												break;
											case 5:
												newspeed=4800;
												break;
											case 6:
												newspeed=9600;
												break;
											case 7:
												newspeed=19200;
												break;
											case 8:
												newspeed=38400;
												break;
											case 9:
												newspeed=57600;
												break;
											case 10:
												newspeed=76800;
												break;
											case 11:
												newspeed=115200;
												break;
											default:
												newspeed=-1;
												break;
										}
									}
								}
								else
									newspeed = -1;
							}
						}
						if(newspeed >= 0)
							*speed = newspeed;
					}
					break;
				case 's':
					cterm.save_xpos=wherex();
					cterm.save_ypos=wherey();
					break;
				case 'u':
					if(cterm.save_ypos>0 && cterm.save_ypos<=cterm.height
							&& cterm.save_xpos>0 && cterm.save_xpos<=cterm.width) {
						gotoxy(cterm.save_xpos,cterm.save_ypos);
					}
					break;
				case 'y':	/* ToDo?  VT100 Tests */
					break;
				case 'z':	/* ToDo?  Reset */
					break;
				case '|':	/* SyncTERM ANSI Music */
					break;
			}
			break;
#if 0
		case 'D':
			scrollup();
			break;
		case 'M':
			scrolldown();
			break;
#endif
		case 'c':
			/* ToDo: Reset Terminal */
			break;
	}
	cterm.escbuf[0]=0;
	cterm.sequence=0;
}

void cterm_init(int height, int width, int xpos, int ypos, int backlines, unsigned char *scrollback)
{
	char	*revision="$Revision: 1.89 $";
	char *in;
	char	*out;

	memset(&cterm, 0, sizeof(cterm));
	cterm.x=xpos;
	cterm.y=ypos;
	cterm.height=height;
	cterm.width=width;
	cterm.attr=7;
	cterm.save_xpos=0;
	cterm.save_ypos=0;
	cterm.escbuf[0]=0;
	cterm.sequence=0;
	cterm.music_enable=CTERM_MUSIC_BANSI;
	cterm.music=0;
	cterm.tempo=120;
	cterm.octave=4;
	cterm.notelen=4;
	cterm.noteshape=CTERM_MUSIC_NORMAL;
	cterm.musicfore=TRUE;
	cterm.backpos=0;
	cterm.backlines=backlines;
	cterm.scrollback=scrollback;
	cterm.log=CTERM_LOG_NONE;
	cterm.logfile=NULL;
	if(cterm.scrollback!=NULL)
		memset(cterm.scrollback,0,cterm.width*2*cterm.backlines);
	textattr(cterm.attr);
	_setcursortype(_NORMALCURSOR);
	window(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y+cterm.height-1);
	clrscr();
	gotoxy(1,1);
	strcpy(cterm.DA,"\x1b[=67;84;101;114;109;");
	out=strchr(cterm.DA, 0);
	if(out != NULL) {
		for(in=revision; *in; in++) {
			if(isdigit(*in))
				*(out++)=*in;
			if(*in=='.')
				*(out++)=';';
		}
		*out=0;
	}
	strcat(cterm.DA,"c");
	/* Did someone call _init() without calling _end()? */
	if(playnote_thread_running) {
		if(sem_trywait(&playnote_thread_terminated)==-1) {
			listSemPost(&notes);
			sem_wait(&playnote_thread_terminated);
		}
		sem_destroy(&playnote_thread_terminated);
		sem_destroy(&note_completed_sem);
		listFree(&notes);
	}
	/* Fire up note playing thread */
	if(!playnote_thread_running) {
		listInit(&notes, LINK_LIST_SEMAPHORE|LINK_LIST_MUTEX);
		sem_init(&note_completed_sem,0,0);
		sem_init(&playnote_thread_terminated,0,0);
		_beginthread(playnote_thread, 0, NULL);
	}
}

void ctputs(char *buf)
{
	char *outp;
	char *p;
	int		oldscroll;
	int		cx;
	int		cy;
	int		i;

	outp=buf;
	oldscroll=_wscroll;
	_wscroll=0;
	cx=wherex();
	cy=wherey();
	if(cterm.log==CTERM_LOG_ASCII && cterm.logfile != NULL)
		fputs(buf, cterm.logfile);
	for(p=buf;*p;p++) {
		switch(*p) {
			case '\r':
				cx=1;
				break;
			case '\n':
				*p=0;
				cputs(outp);
				outp=p+1;
				if(cy==cterm.height)
					scrollup();
				else
					cy++;
				gotoxy(cx,cy);
				break;
			case '\b':
				*p=0;
				cputs(outp);
				outp=p+1;
				if(cx>0)
					cx--;
				gotoxy(cx,cy);
				break;
			case 7:		/* Bell */
				break;
			case '\t':
				for(i=0;i<10;i++) {
					if(cterm_tabs[i]>cx) {
						while(cx<cterm_tabs[i]) {
							cx++;
						}
						break;
					}
				}
				if(i==10) {
					cx=1;
					if(cy==cterm.height) {
						*p=0;
						cputs(outp);
						outp=p+1;
						scrollup();
						gotoxy(cx,cy);
					}
					else
						cy++;
				}
				break;
			default:
				if(cy==cterm.height
						&& cx==cterm.width) {
					char ch;
					ch=*p;
					*p=0;
					cputs(outp);
					*p=ch;
					outp=p;
					scrollup();
					cx=1;
					gotoxy(cx,cy);
				}
				else {
					if(cx==cterm.width) {
						cx=1;
						cy++;
					}
					else {
						cx++;
					}
				}
				break;
		}
	}
	cputs(outp);
	_wscroll=oldscroll;
}

char *cterm_write(unsigned char *buf, int buflen, char *retbuf, size_t retsize, int *speed)
{
	unsigned char ch[2];
	unsigned char prn[BUFSIZE];
	int j,k;
	struct text_info	ti;
	int	olddmc;
	int oldptnm;

	oldptnm=puttext_can_move;
	puttext_can_move=1;
	olddmc=hold_update;
	hold_update=1;
	if(retbuf!=NULL)
		retbuf[0]=0;
	gettextinfo(&ti);
	window(cterm.x,cterm.y,cterm.x+cterm.width-1,cterm.y+cterm.height-1);
	gotoxy(cterm.xpos,cterm.ypos);
	textattr(cterm.attr);
	ch[1]=0;
	switch(buflen) {
		case 0:
			break;
		default:
			if(cterm.log==CTERM_LOG_RAW && cterm.logfile != NULL)
				fwrite(buf, buflen, 1, cterm.logfile);
			prn[0]=0;
			for(j=0;j<buflen;j++) {
				ch[0]=buf[j];
				if(cterm.font_size) {
					cterm.fontbuf[cterm.font_read++]=ch[0];
					if(cterm.font_read == cterm.font_size) {
						char *buf;

						if((buf=(char *)malloc(cterm.font_size))!=NULL) {
							memcpy(buf,cterm.fontbuf,cterm.font_size);
							if(cterm.font_slot >= CONIO_FIRST_FREE_FONT) {
								switch(cterm.font_size) {
									case 4096:
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].eight_by_sixteen);
										conio_fontdata[cterm.font_slot].eight_by_sixteen=buf;
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].desc);
										conio_fontdata[cterm.font_slot].desc=strdup("Remote Defined Font");
										break;
									case 3586:
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].eight_by_fourteen);
										conio_fontdata[cterm.font_slot].eight_by_fourteen=buf;
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].desc);
										conio_fontdata[cterm.font_slot].desc=strdup("Remote Defined Font");
										break;
									case 2048:
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].eight_by_eight);
										conio_fontdata[cterm.font_slot].eight_by_eight=buf;
										FREE_AND_NULL(conio_fontdata[cterm.font_slot].desc);
										conio_fontdata[cterm.font_slot].desc=strdup("Remote Defined Font");
										break;
								}
							}
							else
								FREE_AND_NULL(buf);
						}
						cterm.font_size=0;
					}
				}
				else if(cterm.sequence) {
					k=strlen(cterm.escbuf);
					strcat(cterm.escbuf,ch);
					if(k) {
						if(cterm.escbuf[0] != '[') {	/* Not a CSI code. */
							/* ANSI control characters */
							if(ch[0] >= 32 && ch[0] <= 47) {
								/* Legal intermediate character */
							}
							else if(ch[0] >= 48 && ch[0] <= 126) {
								/* Terminating character */
								do_ansi(retbuf, retsize, speed);
							}
							else {
								/* Broken sequence detected */
								strcat(prn,"\033");
								strcat(prn,cterm.escbuf);
								cterm.escbuf[0]=0;
								cterm.sequence=0;
							}
						}
						else {
							/* We know that it was a CSI at this point */
							/* Here's where we get funky! */
							/* the last character defines the set of legal next characters */
							if(ch[0] >= 48 && ch[0] <= 63) {
								/* Parameter character.  Only legal after '[' and other param chars */
								if(cterm.escbuf[k]!='[' 
										&& (cterm.escbuf[k] < 48 || cterm.escbuf[k] > 63)) {
									/* Broken sequence detected */
									strcat(prn,"\033");
									strcat(prn,cterm.escbuf);
									cterm.escbuf[0]=0;
									cterm.sequence=0;
								}
							}
							else if(ch[0] >= 32 && ch[0] <= 47) {
								/* Intermediate character.  Legal after '[', param, or intermetiate chars */
								if(cterm.escbuf[k]!='[' 
										&& (cterm.escbuf[k] < 48 || cterm.escbuf[k] > 63) 
										&& (cterm.escbuf[k] < 32 || cterm.escbuf[k] > 47)) {
									/* Broken sequence detected */
									strcat(prn,"\033");
									strcat(prn,cterm.escbuf);
									cterm.escbuf[0]=0;
									cterm.sequence=0;
								}
							}
							else if(ch[0] >= 64 && ch[0] <= 126) {
								/* Terminating character.  Always legal at this point. */
								do_ansi(retbuf, retsize, speed);
							}
							else {
								/* Broken sequence detected */
								strcat(prn,"\033");
								strcat(prn,cterm.escbuf);
								cterm.escbuf[0]=0;
								cterm.sequence=0;
							}
						}
					}
					else {
						/* First char after the ESC */
						if(ch[0] >= 32 && ch[0] <= 47) {
							/* Legal intermediate character */
							/* No CSI then */
						}
						else if(ch[0]=='[') {
							/* CSI received */
						}
						else if(ch[0] >= 48 && ch[0] <= 126) {
							/* Terminating character */
							do_ansi(retbuf, retsize, speed);
						}
						else {
							/* Broken sequence detected */
							strcat(prn,"\033");
							strcat(prn,cterm.escbuf);
							cterm.escbuf[0]=0;
							cterm.sequence=0;
						}
					}
					if(ch[0]=='\033') {	/* Broken sequence followed by a legal one! */
						if(prn[0])	/* Don't display the ESC */
							prn[strlen(prn)-1]=0;
						ctputs(prn);
						prn[0]=0;
						cterm.sequence=1;
					}
				}
				else if (cterm.music) {
					if(ch[0]==14)
						play_music();
					else {
						if(strchr(musicchars,ch[0])!=NULL)
							strcat(cterm.musicbuf,ch);
						else {
							/* Kill non-music strings */
							cterm.music=0;
							cterm.musicbuf[0]=0;
						}
					}
				}
				else {
					switch(buf[j]) {
						case 0:
							break;
						case 7:			/* Beep */
							ctputs(prn);
							prn[0]=0;
							if(cterm.log==CTERM_LOG_ASCII && cterm.logfile != NULL)
								fputs("\t", cterm.logfile);
							#ifdef __unix__
								putch(7);
							#else
								MessageBeep(MB_OK);
							#endif
							break;
						case 12:		/* ^L - Clear screen */
							ctputs(prn);
							prn[0]=0;
							if(cterm.log==CTERM_LOG_ASCII && cterm.logfile != NULL)
								fputs("\t", cterm.logfile);
							clearscreen((char)cterm.attr);
							gotoxy(1,1);
							break;
						case 27:		/* ESC */
							ctputs(prn);
							prn[0]=0;
							cterm.sequence=1;
							break;
						case '\t':
							ctputs(prn);
							prn[0]=0;
							if(cterm.log==CTERM_LOG_ASCII && cterm.logfile != NULL)
								fputs("\t", cterm.logfile);
							for(k=0;k<11;k++) {
								if(cterm_tabs[k]>wherex()) {
									gotoxy(cterm_tabs[k],wherey());
									break;
								}
							}
							break;
						default:
							strcat(prn,ch);
					}
				}
			}
			ctputs(prn);
			prn[0]=0;
			break;
	}
	cterm.xpos=wherex();
	cterm.ypos=wherey();
#if 0
	window(ti.winleft,ti.wintop,ti.winright,ti.winbottom);
	gotoxy(ti.curx,ti.cury);
	textattr(ti.attribute);
#endif

	hold_update=olddmc;
	puttext_can_move=oldptnm;
	gotoxy(wherex(),wherey());
	return(retbuf);
}

int cterm_openlog(char *logfile, int logtype)
{
	cterm.logfile=fopen(logfile, "a");
	if(cterm.logfile==NULL)
		return(0);
	cterm.log=logtype;
	return(1);
}

void cterm_closelog()
{
	if(cterm.logfile != NULL)
		fclose(cterm.logfile);
	cterm.logfile=NULL;
	cterm.log=CTERM_LOG_NONE;
}

void cterm_end(void)
{
	int i;

	cterm_closelog();
	for(i=CONIO_FIRST_FREE_FONT; i < 256; i++) {
		FREE_AND_NULL(conio_fontdata[i].eight_by_sixteen);
		FREE_AND_NULL(conio_fontdata[i].eight_by_fourteen);
		FREE_AND_NULL(conio_fontdata[i].eight_by_eight);
		FREE_AND_NULL(conio_fontdata[i].desc);
	}
	if(playnote_thread_running) {
		if(sem_trywait(&playnote_thread_terminated)==-1) {
			listSemPost(&notes);
			sem_wait(&playnote_thread_terminated);
		}
		sem_destroy(&playnote_thread_terminated);
		sem_destroy(&note_completed_sem);
		listFree(&notes);
	}
}
