/* $Id: curs_cio.h,v 1.2 2004/07/17 01:44:19 deuce Exp $ */

#ifdef __unix__
#include "conio.h"
#undef getch
#undef ungetch
#include "curs_fix.h"

#ifdef __cplusplus
extern "C" {
#endif
short curses_color(short color);
int curs_puttext(int sx, int sy, int ex, int ey, unsigned char *fill);
int curs_gettext(int sx, int sy, int ex, int ey, unsigned char *fill);
void curs_textattr(unsigned char attr);
int curs_kbhit(void);
void curs_delay(long msec);
int curs_wherey(void);
int curs_wherex(void);
int _putch(unsigned char ch, BOOL refresh_now);
int curs_putch(unsigned char ch);
void curs_gotoxy(int x, int y);
int curs_initciowrap(long inmode);
void curs_gettextinfo(struct text_info *info);
void curs_setcursortype(int type);
int curs_getch(void);
int curs_getche(void);
void curs_textmode(int mode);
#ifdef __cplusplus
}
#endif

#define UIFC_IBM	(1<<4)  /* Force use of IBM charset     */

#endif
