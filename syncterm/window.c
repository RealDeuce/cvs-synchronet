#include <gen_defs.h>
#include <ciolib.h>
#include "uifcinit.h"
#include "term.h"

int drawwin(void)
{
	struct	text_info txtinfo;
	char	*winbuf;
	char	*p;
	char	str[32];
	int		x,y,c;

	strcpy(str,"SyncTERM ");
    gettextinfo(&txtinfo);
	term.width=80;
	term.height=txtinfo.screenheight;
	if(!term.nostatus)
		term.height--;
	if(term.height<24) {
		term.height=24;
		term.nostatus=1;
	}
	term.x=(txtinfo.screenwidth-term.width)/2+2;
	term.y=(txtinfo.screenheight-term.height)/2+2;
	if((winbuf=(char *)malloc(txtinfo.screenheight*txtinfo.screenwidth*2))==NULL) {
		uifcmsg("Cannot allocate memory for terminal buffer",	"`Memory error`\n\n"
																"Either your system is dangerously low on resources or your\n"
																"window is farking huge!");
		return(-1);
	}
	p=str;

	c=0;
	for(y=0;y<txtinfo.screenheight;y++) {
		p=str;
		for(x=0;x<y;x++) {
			p++;
			if(!*p)
				p=str;
		}
		for(x=0;x<txtinfo.screenwidth;x++) {
			winbuf[c++]=*(p++);
			if(!*p)
				p=str;
			winbuf[c++]=YELLOW|(BLUE<<4);
		}
	}
	puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,winbuf);
	free(winbuf);
	return(0);
}
