/* $Id: curs_cio.h,v 1.9 2004/07/05 08:22:36 deuce Exp $ */

#ifdef __unix__
#include "ciowrap.h"

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
void curs_initciowrap(long inmode);
void curs_gettextinfo(struct text_info *info);
void curs_setcursortype(int type);
int curs_getch(void);
int curs_getche(void);
void curs_textmode(int mode);
#ifdef __cplusplus
}
#endif

#endif
