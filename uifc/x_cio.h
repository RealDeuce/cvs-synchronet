/* $Id: x_cio.h,v 1.9 2004/07/05 05:22:29 deuce Exp $ */

#ifdef __unix__
#include "ciowrap.h"

#ifdef __cplusplus
extern "C" {
#endif
int x_puttext(int sx, int sy, int ex, int ey, unsigned char *fill);
int x_gettext(int sx, int sy, int ex, int ey, unsigned char *fill);
void x_textattr(unsigned char attr);
int x_kbhit(void);
void x_delay(long msec);
int x_wherey(void);
int x_wherex(void);
int x_putch(unsigned char ch);
int x_cprintf(char *fmat, ...);
int x_cputs(unsigned char *str);
void x_gotoxy(int x, int y);
void x_initciowrap(long inmode);
void x_gettextinfo(struct text_info *info);
void x_setcursortype(int type);
void x_textbackground(int colour);
void x_textcolor(int colour);
int x_getch(void);
int x_getche(void);
int x_beep(void);
void x_highvideo(void);
void x_lowvideo(void);
void x_normvideo(void);
void x_textmode(int mode);
#ifdef __cplusplus
}
#endif

#endif
