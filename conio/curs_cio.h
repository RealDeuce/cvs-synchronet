/* $Id: curs_cio.h,v 1.8 2004/09/10 23:24:49 deuce Exp $ */

#ifdef __unix__
#include "ciolib.h"
#undef getch
#undef ungetch
#undef getmouse
#include "curs_fix.h"

#ifdef __cplusplus
extern "C" {
#endif
short curses_color(short color);
int curs_puttext(int sx, int sy, int ex, int ey, void *fill);
int curs_gettext(int sx, int sy, int ex, int ey, void *fill);
void curs_textattr(int attr);
int curs_kbhit(void);
void curs_delay(long msec);
int curs_wherey(void);
int curs_wherex(void);
int _putch(unsigned char ch, BOOL refresh_now);
int curs_putch(int ch);
void curs_gotoxy(int x, int y);
int curs_initciolib(long inmode);
void curs_gettextinfo(struct text_info *info);
void curs_setcursortype(int type);
int curs_getch(void);
int curs_getche(void);
void curs_textmode(int mode);
int curs_showmouse(void);
int curs_hidemouse(void);
#ifdef __cplusplus
}
#endif

#endif
