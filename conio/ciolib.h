/* $Id: ciolib.h,v 1.1 2004/07/26 22:06:01 rswindell Exp $ */

#ifndef _CIOLIB_H_
#define _CIOLIB_H_

enum {
	 CIOLIB_AUTO_MODE
	,CIOLIB_CURSES_MODE
	,CIOLIB_CURSES_IBM_MODE
	,CIOLIB_ANSI_MODE
	,CIOLIB_X_MODE
	,CIOLIB_CONIO_MODE
};

#ifndef __unix__
#include <conio.h>
#else

#ifndef BOOL
#define BOOL    int
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#endif

enum {
	 BLACK
	,BLUE
	,GREEN
	,CYAN
	,RED
	,MAGENTA
	,BROWN	
	,LIGHTGRAY	
	,DARKGRAY
	,LIGHTBLUE	
	,LIGHTGREEN	
	,LIGHTCYAN
	,LIGHTRED
	,LIGHTMAGENTA
	,YELLOW
	,WHITE
};
#define BLINK 128

#define LASTMODE	-1
#define BW40		0
#define C40			1
#define BW80		2
#define	C80			3
#define MONO		7
#define C4350		64

#define COLOR_MODE	C80

enum
{
	_NOCURSOR,
	_SOLIDCURSOR,
	_NORMALCURSOR
};

struct text_info {
	unsigned char winleft;        /* left window coordinate */
	unsigned char wintop;         /* top window coordinate */
	unsigned char winright;       /* right window coordinate */
	unsigned char winbottom;      /* bottom window coordinate */
	unsigned char attribute;      /* text attribute */
	unsigned char normattr;       /* normal attribute */
	unsigned char currmode;       /* current video mode:
                                	 BW40, BW80, C40, C80, or C4350 */
	unsigned char screenheight;   /* text screen's height */
	unsigned char screenwidth;    /* text screen's width */
	unsigned char curx;           /* x-coordinate in current window */
	unsigned char cury;           /* y-coordinate in current window */
};

#endif

typedef struct {
	int		mode;
	void	(*clreol)		(void);
	int		(*puttext)		(int,int,int,int,unsigned char *);
	int		(*gettext)		(int,int,int,int,unsigned char *);
	void	(*textattr)		(unsigned char);
	int		(*kbhit)		(void);
	void	(*delay)		(long);
	int		(*wherex)		(void);
	int		(*wherey)		(void);
	int		(*putch)		(unsigned char);
	void	(*gotoxy)		(int,int);
	void	(*clrscr)		(void);
	void	(*gettextinfo)	(struct text_info *);
	void	(*setcursortype)(int);
	int		(*getch)		(void);
	int		(*getche)		(void);
	int		(*beep)			(void);
	void	(*highvideo)	(void);
	void	(*lowvideo)		(void);
	void	(*normvideo)	(void);
	void	(*textmode)		(int);
	int		(*ungetch)		(int);
	int		(*movetext)		(int,int,int,int,int,int);
	char	*(*cgets)		(char *);
	int		(*cscanf)		(char *,...);
	char	*(*getpass)		(const char *);
	void	(*wscroll)		(void);
	void	(*window)		(int,int,int,int);
	void	(*delline)		(void);
	void	(*insline)		(void);
	int		(*cprintf)		(char *,...);
	int		(*cputs)		(char *);
	void	(*textbackground)	(int);
	void	(*textcolor)	(int);
} cioapi_t;

extern cioapi_t cio_api;
extern int _wscroll;
extern int directvideo;

#define _conio_kbhit()		kbhit()

#ifdef __cplusplus
extern "C" {
#endif
int initciolib(int mode);
#ifdef __cplusplus
}
#endif

#ifndef CIOLIB_NO_MACROS
	#define cscanf					ciolib_cscanf
	#define cprintf					ciolib_cprintf

	#define movetext(a,b,c,d,e,f)	ciolib_movetext(a,b,c,d,e,f)
	#define cgets(a)				ciolib_cgets(a)
	#define kbhit()					ciolib_kbhit()
	#define getch()					ciolib_getch()
	#define getchr()				ciolib_getche()
	#define ungetch(a)				ciolib_ungetch(a)
	#define gettextinfo(a)			ciolib_gettextinfo(a)
	#define wherex()				ciolib_wherex()
	#define wherey()				ciolib_wherey()
	#define	wscroll()				ciolib_wscroll()
	#define gotoxy(a,b)				ciolib_gotoxy(a,b)
	#define clreol()				ciolib_clreol()
	#define clrscr()				ciolib_clrscr()
	#define cputs(a)				ciolib_cputs(a)
	#define textbackground(a)		ciolib_textbackground(a)
	#define textcolor(a)			ciolib_textcolor(a)
	#define highvideo()				ciolib_highvideo()
	#define lowvideo()				ciolib_lowvideo()
	#define normvideo()				ciolib_normvideo()
	#define puttext(a,b,c,d,e)		ciolib_puttext(a,b,c,d,e)
	#define gettext(a,b,c,d,e)		ciolib_gettext(a,b,c,d,e)
	#define textattr(a)				ciolib_textattr(a)
	#define delay(a)				ciolib_delay(a)
	#define putch(a)				ciolib_putch(a)
	#define _setcursortype(a)		ciolib_setcursortype(a)
	#define textmode(a)				ciolib_textmode(a)
	#define window(a,b,c,d)			ciolib_window(a,b,c,d)
	#define delline()				ciolib_delline()
	#define insline					ciolib_insline()
	#define getpass(a)				ciolib_getpass(a);
#endif

#endif	/* Do not add anything after this line */
