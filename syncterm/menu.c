#include <uifc.h>
#include <ciolib.h>
#include <keys.h>

#include "term.h"
#include "uifcinit.h"

void viewscroll(void)
{
#if 0
	int	top;
	int key;
	int i;
	char	*buf;
	struct	text_info txtinfo;

	uifcbail();
    gettextinfo(&txtinfo);
	buf=(char *)malloc(txtinfo.screenheight*txtinfo.screenwidth*2);
	gettext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);
	drawwin();
	top=term.backpos-term.height;
	for(i=0;!i;) {
		if(top<1)
			top=1;
		if(top>term.backpos-term.height)
			top=term.backpos-term.height;
		puttext(term.x+1,term.y+1,term.x+term.width,term.y+term.height,term.scrollback+(term.width*2*top));
		key=getch();
		switch(key) {
			case 0:
				switch(getch()<<8) {
					case CIO_KEY_UP:
						top--;
						break;
					case CIO_KEY_DOWN:
						top++;
						break;
					case CIO_KEY_PPAGE:
						top-=term.height;
						break;
					case CIO_KEY_NPAGE:
						top+=term.height;
						break;
					case CIO_KEY_F(1):
						init_uifc();
						uifc.helpbuf=	"`Scrollback Buffer`\n\n"
										"~ J ~ or ~ Up Arrow ~   Scrolls up one line\n"
										"~ K ~ or ~ Down Arrow ~ Scrolls down one line\n"
										"~ H ~ or ~ Page Up ~    Scrolls up one screen\n"
										"~ L ~ or ~ Page Down ~  Scrolls down one screen\n";
						uifc.showhelp();
						uifcbail();
						drawwin();
						break;
				}
				break;
			case 'j':
			case 'J':
			case 'k':
			case 'K':
			case 'h':
			case 'H':
			case 'l':
			case 'L':
			case ESC:
				i=1;
				break;
		}
	}
	puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);
	free(buf);
	return;
#endif
}

int syncmenu(void)
{
	char	*opts[3]={
#ifdef __unix__
						 "Scrollback (ALT-S)"
#else
						 "Scrollback"
#endif
						,"Disconnect (CTRL-Q)"
						,""};
	int		opt=0;
	int		i;
	struct	text_info txtinfo;
	char	*buf;
	int		ret;

    gettextinfo(&txtinfo);
	buf=(char *)malloc(txtinfo.screenheight*txtinfo.screenwidth*2);
	gettext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);

	for(ret=0;!ret;) {
		init_uifc();
		i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&opt,NULL,"SyncTERM Online Menu",opts);
		switch(i) {
			case -1:	/* Cancel */
				ret=1;
				break;
			case 0:		/* Scrollback */
				viewscroll();
				break;
			case 1:		/* Disconnect */
				ret=-1;
				break;
		}
	}

	uifcbail();
	puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);
	free(buf);
	return(ret);
}
