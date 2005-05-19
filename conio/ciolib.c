/* $Id: ciolib.c,v 1.40 2005/05/19 23:54:16 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <stdarg.h>
#include <stdlib.h>	/* malloc */
#include <stdio.h>

#include <threadwrap.h>

#define CIOLIB_NO_MACROS
#include "ciolib.h"

#ifdef _WIN32
 #include "win32cio.h"
#else
 #ifndef NO_X
  #include "x_cio.h"
 #endif
 #include "curs_cio.h"
 #undef getch
#endif

#include "ansi_cio.h"

cioapi_t	cio_api;

static int ungotch;
static struct text_info cio_textinfo;
static int lastmode=3;
int _wscroll=1;
int directvideo=0;
int hold_update=0;
static int initialized=0;

int ciolib_movetext(int sx, int sy, int ex, int ey, int dx, int dy);
char *ciolib_cgets(char *str);
int ciolib_cscanf (char *format , ...);
int ciolib_kbhit(void);
int ciolib_getch(void);
int ciolib_getche(void);
int ciolib_ungetch(int ch);
void ciolib_gettextinfo(struct text_info *info);
int ciolib_wherex(void);
int ciolib_wherey(void);
void ciolib_wscroll(void);
void ciolib_gotoxy(int x, int y);
void ciolib_clreol(void);
void ciolib_clrscr(void);
int ciolib_cputs(char *str);
int	ciolib_cprintf(char *fmat, ...);
void ciolib_textbackground(int colour);
void ciolib_textcolor(int colour);
void ciolib_highvideo(void);
void ciolib_lowvideo(void);
void ciolib_normvideo(void);
int ciolib_puttext(int a,int b,int c,int d,unsigned char *e);
int ciolib_gettext(int a,int b,int c,int d,unsigned char *e);
void ciolib_textattr(int a);
void ciolib_delay(long a);
int ciolib_putch(int a);
void ciolib_setcursortype(int a);
void ciolib_textmode(int mode);
void ciolib_window(int sx, int sy, int ex, int ey);
void ciolib_delline(void);
void ciolib_insline(void);
char *ciolib_getpass(const char *prompt);
void ciolib_copytext(const char *text, size_t buflen);
char *ciolib_getcliptext(void);

#define CIOLIB_INIT()		{ if(!initialized) initciolib(CIOLIB_MODE_AUTO); }

#ifndef _WIN32
 #ifndef NO_X
int try_x_init(int mode)
{
	if(!console_init()) {
		cio_api.mode=CIOLIB_MODE_X;
		cio_api.mouse=1;
		cio_api.puttext=x_puttext;
		cio_api.gettext=x_gettext;
		cio_api.textattr=x_textattr;
		cio_api.kbhit=x_kbhit;
		cio_api.delay=x_delay;
		cio_api.wherey=x_wherey;
		cio_api.wherex=x_wherex;
		cio_api.putch=x_putch;
		cio_api.gotoxy=x_gotoxy;
		cio_api.gettextinfo=x_gettextinfo;
		cio_api.setcursortype=x_setcursortype;
		cio_api.getch=x_getch;
		cio_api.getche=x_getche;
		cio_api.textmode=x_textmode;
		cio_api.showmouse=NULL;
		cio_api.hidemouse=NULL;
		cio_api.settitle=x_settitle;
		cio_api.copytext=x_copytext;
		cio_api.getcliptext=x_getcliptext;
		return(1);
	}
	return(0);
}
 #endif

int try_curses_init(int mode)
{
	if(curs_initciolib(mode)) {
		cio_api.mode=CIOLIB_MODE_CURSES_IBM;
		cio_api.puttext=curs_puttext;
		cio_api.gettext=curs_gettext;
		cio_api.textattr=curs_textattr;
		cio_api.kbhit=curs_kbhit;
		cio_api.delay=curs_delay;
		cio_api.wherey=curs_wherey;
		cio_api.wherex=curs_wherex;
		cio_api.putch=curs_putch;
		cio_api.gotoxy=curs_gotoxy;
		cio_api.gettextinfo=curs_gettextinfo;
		cio_api.setcursortype=curs_setcursortype;
		cio_api.getch=curs_getch;
		cio_api.getche=curs_getche;
		cio_api.textmode=curs_textmode;
		cio_api.showmouse=curs_showmouse;
		cio_api.hidemouse=curs_hidemouse;
		cio_api.settitle=NULL;
		cio_api.copytext=NULL;
		cio_api.getcliptext=NULL;
		return(1);
	}
	return(0);
}
#endif

int try_ansi_init(int mode)
{
	if(ansi_initciolib(mode)) {
		cio_api.mode=CIOLIB_MODE_ANSI;
		cio_api.mouse=0;
		cio_api.puttext=ansi_puttext;
		cio_api.gettext=ansi_gettext;
		cio_api.textattr=ansi_textattr;
		cio_api.kbhit=ansi_kbhit;
		cio_api.delay=ansi_delay;
		cio_api.wherey=ansi_wherey;
		cio_api.wherex=ansi_wherex;
		cio_api.putch=ansi_putch;
		cio_api.gotoxy=ansi_gotoxy;
		cio_api.gettextinfo=ansi_gettextinfo;
		cio_api.setcursortype=ansi_setcursortype;
		cio_api.getch=ansi_getch;
		cio_api.getche=ansi_getche;
		cio_api.textmode=ansi_textmode;
		cio_api.showmouse=NULL;
		cio_api.hidemouse=NULL;
		cio_api.settitle=NULL;
		cio_api.copytext=NULL;
		cio_api.getcliptext=NULL;
		return(1);
	}
	return(0);
}

#ifdef _WIN32
#if defined(__BORLANDC__)
        #pragma argsused
#endif
int try_conio_init(int mode)
{
	/* This should test for something or other */
	if(win32_initciolib(mode)) {
		cio_api.mode=CIOLIB_MODE_CONIO;
		cio_api.mouse=1;
		cio_api.puttext=win32_puttext;
		cio_api.gettext=win32_gettext;
		cio_api.textattr=win32_textattr;
		cio_api.kbhit=win32_kbhit;
		cio_api.wherey=win32_wherey;
		cio_api.wherex=win32_wherex;
		cio_api.putch=win32_putch;
		cio_api.gotoxy=win32_gotoxy;
		cio_api.gettextinfo=win32_gettextinfo;
		cio_api.setcursortype=win32_setcursortype;
		cio_api.getch=win32_getch;
		cio_api.getche=win32_getche;
		cio_api.textmode=win32_textmode;
		cio_api.showmouse=win32_showmouse;
		cio_api.hidemouse=win32_hidemouse;
		cio_api.settitle=win32_settitle;
		cio_api.copytext=win32_copytext;
		cio_api.getcliptext=win32_getcliptext;
		return(1);
	}
	return(0);
}
#endif

int initciolib(int mode)
{
	switch(mode) {
		case CIOLIB_MODE_AUTO:
#ifdef _WIN32
			if(!try_conio_init(mode))
#else
#ifndef NO_X
			if(!try_x_init(mode))
#endif
				if(!try_curses_init(mode))
#endif
					try_ansi_init(mode);
			break;
#ifdef _WIN32
		case CIOLIB_MODE_CONIO:
			try_conio_init(mode);
			break;
#else
		case CIOLIB_MODE_CURSES:
		case CIOLIB_MODE_CURSES_IBM:
			try_curses_init(mode);
			break;

		case CIOLIB_MODE_X:
#ifndef NO_X
			try_x_init(mode);
#endif
			break;
#endif
		case CIOLIB_MODE_ANSI:
			try_ansi_init(mode);
			break;
	}
	if(cio_api.mode==CIOLIB_MODE_AUTO) {
		fprintf(stderr,"CIOLIB initialization failed!\n");
		return(-1);
	}

	initialized=1;
	ciolib_gettextinfo(&cio_textinfo);
	cio_textinfo.winleft=1;
	cio_textinfo.wintop=1;
	cio_textinfo.winright=cio_textinfo.screenwidth;
	cio_textinfo.winbottom=cio_textinfo.screenheight;
	cio_textinfo.normattr=7;
	_beginthread(ciolib_mouse_thread,0,NULL);
	return(0);
}

int ciolib_kbhit(void)
{
	CIOLIB_INIT();
	if(ungotch)
		return(1);
	if(mouse_pending())
		return(1);
	return(cio_api.kbhit());
}

int ciolib_getch(void)
{
	int ch;

	CIOLIB_INIT();

	if(ungotch) {
		ch=ungotch;
		ungotch=0;
		return(ch);
	}
	return(cio_api.getch());
}

int ciolib_getche(void)
{
	int ch;

	CIOLIB_INIT();

	if(ungotch) {
		ch=ungotch;
		ungotch=0;
		ciolib_putch(ch);
		return(ch);
	}
	return(cio_api.getche());
}

int ciolib_ungetch(int ch)
{
	CIOLIB_INIT();
	
	if(ungotch)
		return(EOF);
	ungotch=ch;
	return(ch);
}

int ciolib_movetext(int sx, int sy, int ex, int ey, int dx, int dy)
{
	int width;
	int height;
	unsigned char *buf;

	CIOLIB_INIT();
	
	width=ex-sx;
	height=ey-sy;
	buf=(unsigned char *)malloc((width+1)*(height+1)*2);
	if(buf==NULL)
		return(0);
	if(!ciolib_gettext(sx,sy,ex,ey,buf)) {
		free(buf);
		return(0);
	}
	if(!ciolib_puttext(dx,dy,dx+width,dy+height,buf)) {
		free(buf);
		return(0);
	}
	free(buf);
	return(1);
}

char *ciolib_cgets(char *str)
{
	int	maxlen;
	int len=0;
	int ch;

	CIOLIB_INIT();
	
	maxlen=*(unsigned char *)str;
	while((ch=ciolib_getche())!='\n' && ch !='\r') {
		switch(ch) {
			case 0:	/* Skip extended keys */
				ciolib_getche();
				break;
			case '\r':	/* Skip \r (ToDo: Should this be treated as a \n? */
				break;
			case '\b':
				if(len==0) {
					ciolib_putch(7);
					break;
				}
				ciolib_putch('\b');
				len--;
				break;
			default:
				str[(len++)+2]=ch;
				if(len==maxlen) {
					str[len+2]=0;
					*((unsigned char *)(str+1))=(unsigned char)len;
					return(&str[2]);
				}
				break;
		}
	}
	str[len+2]=0;
	*((unsigned char *)(str+1))=(unsigned char)len;
	return(&str[2]);
}

#ifdef _MSC_VER	/* Use lame vsscanf() implementation */
/* This is a way to do _vsscanf without using fancy stack tricks or using the
 * "_input" method provided by Microsoft, which is no longer exported as of .NET.
 * The function has a limit of 25 arguments (or less if you run out of stack space),
 *  but how many arguments do you need?
 */
/* From "krabsheva" - http://www.codeguru.com/Cpp/Cpp/string/comments.php/c5631/?thread=1051 */
int vsscanf( const char *buffer, const char *format, va_list arg_ptr )
{
	int i, ret;
	void *arg_arr[25];

	// Do exception handling in case we go too far //
	__try
	{
		for ( i = 0; i < 25; i++ )
			arg_arr[i] = va_arg( arg_ptr, void * );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
	}

	/* This is lame, but the extra arguments won't be used by sscanf */
	ret = sscanf( buffer, format, arg_arr[0], arg_arr[1], arg_arr[2], arg_arr[3],
		arg_arr[4], arg_arr[5], arg_arr[6], arg_arr[7], arg_arr[8], arg_arr[9],
		arg_arr[10], arg_arr[11], arg_arr[12], arg_arr[13], arg_arr[14],
		arg_arr[15], arg_arr[16], arg_arr[17], arg_arr[18], arg_arr[19],
		arg_arr[20], arg_arr[21], arg_arr[22], arg_arr[23], arg_arr[24] );

	return ret;
}
#endif

int ciolib_cscanf (char *format , ...)
{
	char str[255];
    va_list argptr;
	int ret;

	CIOLIB_INIT();
	
	str[0]=-1;
	va_start(argptr,format);
	ret=vsscanf(ciolib_cgets(str),format,argptr);
	va_end(argptr);
	return(ret);
}

char *ciolib_getpass(const char *prompt)
{
	static char pass[9];
	int len=0;
	int ch;

	CIOLIB_INIT();
	
	ciolib_cputs((char *)prompt);
	while((ch=ciolib_getch())!='\n') {
		switch(ch) {
			case 0:	/* Skip extended keys */
				ciolib_getch();
				break;
			case '\r':	/* Skip \r (ToDo: Should this be treeated as a \n? */
				break;
			case '\b':
				if(len==0) {
					ciolib_putch(7);
					break;
				}
				len--;
				break;
			default:
				if(len==8)
					ciolib_putch(7);
				else
					pass[len++]=ch;
				break;
		}
	}
	pass[len]=0;
	return(pass);
}

void ciolib_gettextinfo(struct text_info *info)
{
	if(!initialized)
		initciolib(CIOLIB_MODE_AUTO);
	else {
		cio_api.gettextinfo(&cio_textinfo);
	}
	if(info!=&cio_textinfo) {
		info->winleft=cio_textinfo.winleft;        /* left window coordinate */
		info->wintop=cio_textinfo.wintop;         /* top window coordinate */
		info->winright=cio_textinfo.winright;       /* right window coordinate */
		info->winbottom=cio_textinfo.winbottom;      /* bottom window coordinate */
		info->attribute=cio_textinfo.attribute;      /* text attribute */
		info->normattr=cio_textinfo.normattr;       /* normal attribute */
		info->currmode=cio_textinfo.currmode;       /* current video mode:
                               			 BW40, BW80, C40, C80, or C4350 */
		info->screenheight=cio_textinfo.screenheight;   /* text screen's height */
		info->screenwidth=cio_textinfo.screenwidth;    /* text screen's width */
		info->curx=cio_textinfo.curx-cio_textinfo.winleft+1;           /* x-coordinate in current window */
		info->cury=cio_textinfo.cury-cio_textinfo.wintop+1;           /* y-coordinate in current window */
	}
}

void ciolib_wscroll(void)
{
	int os;
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);
	if(!_wscroll)
		return;
	ciolib_movetext(ti.winleft,ti.wintop+1,ti.winright,ti.winbottom,ti.winleft,ti.wintop);
	ciolib_gotoxy(1,ti.winbottom-ti.winleft+1);
	os=_wscroll;
	_wscroll=0;
	/* ciolib_cprintf("%*s",ti.winright-ti.winleft+1,""); */
	ciolib_clreol();
	_wscroll=os;
	ciolib_gotoxy(ti.curx,ti.cury);
}

int ciolib_wherex(void)
{
	int x;

	CIOLIB_INIT();
	
	x=cio_api.wherex();
	x=x-cio_textinfo.winleft+1;
	return(x);
}

int ciolib_wherey(void)
{
	int y;

	CIOLIB_INIT();
	
	y=cio_api.wherey();
	y=y-cio_textinfo.wintop+1;
	return(y);
}

void ciolib_gotoxy(int x, int y)
{
	int nx;
	int ny;
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);
	if(		x < 1
			|| x > ti.winright-ti.winleft+1
			|| y < 1
			|| y > ti.winbottom-ti.wintop+1)
		return;
	nx=x+ti.winleft-1;
	ny=y+ti.wintop-1;
	cio_api.gotoxy(nx,ny);
}

void ciolib_textmode(mode)
{
	CIOLIB_INIT();
	
	if(mode==-1) {
		ciolib_gettextinfo(&cio_textinfo);
		cio_api.textmode(lastmode);
		lastmode=cio_textinfo.currmode;
	}
	else {
		ciolib_gettextinfo(&cio_textinfo);
		lastmode=cio_textinfo.currmode;
		cio_api.textmode(mode);
	}
	ciolib_gettextinfo(&cio_textinfo);
	cio_textinfo.winleft=1;
	cio_textinfo.wintop=1;
	cio_textinfo.winright=cio_textinfo.screenwidth;
	cio_textinfo.winbottom=cio_textinfo.screenheight;
}

void ciolib_window(int sx, int sy, int ex, int ey)
{
	CIOLIB_INIT();
	
	ciolib_gettextinfo(&cio_textinfo);
	if(		   sx < 1
			|| sy < 1
			|| ex < 1
			|| ey < 1
			|| sx > cio_textinfo.screenwidth
			|| sy > cio_textinfo.screenheight
			|| sx > ex
			|| sy > ey
			|| ex > cio_textinfo.screenwidth
			|| ey > cio_textinfo.screenheight)
		return;
	cio_textinfo.winleft=sx;
	cio_textinfo.wintop=sy;
	cio_textinfo.winright=ex;
	cio_textinfo.winbottom=ey;
	ciolib_gotoxy(1,1);
}

void ciolib_clreol(void)
{
	unsigned char *buf;
	int i;
	int width,height;
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);

	width=ti.winright-ti.curx+1;
	height=1;
	buf=(unsigned char *)malloc(width*height*2);
	for(i=0;i<width*height*2;) {
		buf[i++]=' ';
		buf[i++]=ti.attribute;
	}
	ciolib_puttext(ti.curx+ti.winleft-1,ti.cury+ti.wintop-1,ti.winright,ti.cury+ti.wintop-1,buf);
	free(buf);
}

void ciolib_clrscr(void)
{
	unsigned char *buf;
	int i;
	int width,height;
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);

	width=ti.winright-ti.winleft+1;
	height=ti.winbottom-ti.wintop+1;
	buf=(unsigned char *)malloc(width*height*2);
	for(i=0;i<width*height*2;) {
		buf[i++]=' ';
		buf[i++]=ti.attribute;
	}
	ciolib_puttext(ti.winleft,ti.wintop,ti.winright,ti.winbottom,buf);
	free(buf);
}

void ciolib_delline(void)
{
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);

	ciolib_movetext(ti.winleft,ti.cury+1,ti.winright,ti.winbottom,ti.winleft,ti.cury);
	ciolib_gotoxy(1,ti.winbottom-ti.wintop+1);
	ciolib_clreol();
	ciolib_gotoxy(ti.curx,ti.cury);
}

void ciolib_insline(void)
{
	struct text_info ti;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&ti);

	ciolib_movetext(ti.winleft,ti.cury,ti.winright,ti.winbottom,ti.winleft,ti.cury+1);
	ciolib_gotoxy(1,ti.cury);
	ciolib_clreol();
	ciolib_gotoxy(ti.curx,ti.cury);
}

int ciolib_cprintf(char *fmat, ...)
{
    va_list argptr;
	int		ret;
#ifdef _MSC_VER		/* Can't figure out a way to allocate a "big enough" buffer for Win32. */
	char	str[16384];
#else
	char	*str;
#endif

	CIOLIB_INIT();

    va_start(argptr,fmat);
#ifdef _MSC_VER
	ret=_vsnprintf(str,sizeof(str)-1,fmat,argptr);
#else
    ret=vsnprintf(NULL,0,fmat,argptr);
	if(ret<0)
		return(EOF);
	str=(char *)malloc(ret+1);
	if(str==NULL)
		return(EOF);
	ret=vsprintf(str,fmat,argptr);
#endif
    va_end(argptr);
	if(ret>=0)
		ciolib_cputs(str);
	else
		ret=EOF;
#ifndef _MSC_VER
	free(str);
#endif
    return(ret);
}

int ciolib_cputs(char *str)
{
	int		pos;
	int		ret=0;
	int		olddmc;

	CIOLIB_INIT();

	olddmc=hold_update;
	hold_update=1;
	for(pos=0;str[pos];pos++)
	{
		ret=str[pos];
		if(str[pos]=='\n')
			ciolib_putch('\r');
		ciolib_putch(str[pos]);
	}
	hold_update=olddmc;
	ciolib_gotoxy(ciolib_wherex(),ciolib_wherey());
	return(ret);
}

void ciolib_textbackground(int colour)
{
	unsigned char attr;
	unsigned char col;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&cio_textinfo);
	attr=cio_textinfo.attribute;
	attr&=143;
	col=(colour & 0x07);
	attr|=(col<<4);
	ciolib_textattr(attr);
}

void ciolib_textcolor(int colour)
{
	unsigned char attr;
	unsigned char col;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&cio_textinfo);
	attr=cio_textinfo.attribute;
	attr&=240;
	col=colour&0x0f;
	attr|=col;
	ciolib_textattr(attr);
}

void ciolib_highvideo(void)
{
	int attr;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&cio_textinfo);
	attr=cio_textinfo.attribute;
	attr |= 8;
	ciolib_textattr(attr);
}

void ciolib_lowvideo(void)
{
	int attr;

	CIOLIB_INIT();
	
	ciolib_gettextinfo(&cio_textinfo);
	attr=cio_textinfo.attribute;
	attr &= 0xf7;
	ciolib_textattr(attr);
}

void ciolib_normvideo(void)
{
	CIOLIB_INIT();
	
	ciolib_textattr(0x07);
}

int ciolib_puttext(int a,int b,int c,int d,unsigned char *e)
{
	CIOLIB_INIT();
	
	return(cio_api.puttext(a,b,c,d,e));
}

int ciolib_gettext(int a,int b,int c,int d,unsigned char *e)
{
	CIOLIB_INIT();
	
	return(cio_api.gettext(a,b,c,d,e));
}

void ciolib_textattr(int a)
{
	CIOLIB_INIT();
	
	cio_api.textattr(a);
}

void ciolib_delay(long a)
{
	CIOLIB_INIT();
	
	cio_api.delay(a);
}

int ciolib_putch(int a)
{
	unsigned char a1=a;
	CIOLIB_INIT();

	return(cio_api.putch(a1));
}

void ciolib_setcursortype(int a)
{
	CIOLIB_INIT();
	
	cio_api.setcursortype(a);
}

int ciolib_showmouse(void) {
	CIOLIB_INIT();

	if(cio_api.showmouse!=NULL)
		return(cio_api.showmouse());
	return(-1);
}

int ciolib_hidemouse(void) {
	CIOLIB_INIT();

	if(cio_api.hidemouse!=NULL)
		return(cio_api.hidemouse());
	return(-1);
}

void ciolib_settitle(const char *title) {
	CIOLIB_INIT();

	if(cio_api.settitle!=NULL)
		cio_api.settitle(title);
}

void ciolib_copytext(const char *text, size_t buflen)
{
	CIOLIB_INIT();

	if(cio_api.copytext!=NULL)
		cio_api.copytext(text,buflen);
}

char *ciolib_getcliptext(void)
{
	CIOLIB_INIT();

	if(cio_api.getcliptext!=NULL)
		return(cio_api.getcliptext());
	else
		return(NULL);
}
