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
	if(txtinfo.screenwidth>=80)
		term.width=80;
	if(txtinfo.screenheight>=49)
		term.height=49;
	else
		term.height=24;
	term.x=(txtinfo.screenwidth-term.width)/2;
	term.y=(txtinfo.screenheight-term.height)/2;
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
	for(x=term.x;x<term.x+term.width;x++) {
		for(y=term.y;y<term.y+term.height;y++) {
			winbuf[(y*txtinfo.screenwidth+x)*2]=' ';
			winbuf[(y*txtinfo.screenwidth+x)*2+1]=0;
		}
	}
	puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,winbuf);
	free(winbuf);
	return(0);
}
