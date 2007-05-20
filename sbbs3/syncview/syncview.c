#include <stdio.h>
#include <string.h>

#include "dirwrap.h"

#include "ciolib.h"
#include "keys.h"
#include "cterm.h"

#define SCROLL_LINES	1000
#define BUF_SIZE		1024


void viewscroll(void)
{
	int	top;
	int key;
	int i;
	char	*scrollback;
	struct	text_info txtinfo;

    gettextinfo(&txtinfo);
	/* ToDo: Watch this... may be too large for alloca() */
	scrollback=(char *)malloc((txtinfo.screenwidth*2*SCROLL_LINES)+(txtinfo.screenheight*txtinfo.screenwidth*2));
	memcpy(scrollback,cterm.scrollback,txtinfo.screenwidth*2*SCROLL_LINES);
	gettext(1,1,txtinfo.screenwidth,txtinfo.screenheight,scrollback+(cterm.backpos)*cterm.width*2);
	top=cterm.backpos;
	for(i=0;!i;) {
		if(top<1)
			top=1;
		if(top>cterm.backpos)
			top=cterm.backpos;
		puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,scrollback+(txtinfo.screenwidth*2*top));
		key=getch();
		switch(key) {
			case 0xff:
			case 0:
				switch(key|getch()<<8) {
					case CIO_KEY_UP:
						top--;
						break;
					case CIO_KEY_DOWN:
						top++;
						break;
					case CIO_KEY_PPAGE:
						top-=txtinfo.screenheight;
						break;
					case CIO_KEY_NPAGE:
						top+=txtinfo.screenheight;
						break;
				}
				break;
			case 'j':
			case 'J':
				top--;
				break;
			case 'k':
			case 'K':
				top++;
				break;
			case 'h':
			case 'H':
				top-=txtinfo.screenheight;
				break;
			case 'l':
			case 'L':
				top+=txtinfo.screenheight;
				break;
			case '\033':
				i=1;
				break;
		}
	}
	free(scrollback);
	return;
}

void lfexpand(char *buf, int *len)
{
	char	newbuf[BUF_SIZE*2];
	int		newlen=0;
	int		i;

	for(i=0; i<*len; i++) {
		if(buf[i]=='\n') {
			newbuf[newlen++]='\r';
		}
		newbuf[newlen++]=buf[i];
	}
	*len=newlen;
	memcpy(buf, newbuf, newlen);

	return;
}

int main(int argc, char **argv)
{
	struct text_info	ti;
	FILE	*f;
	char	buf[BUF_SIZE*2];	/* Room for lfexpand */
	int		len;
	int		speed;
	char	*scrollbuf;
	char	*infile=NULL;
	char	title[MAX_PATH+1];
	int		expand=0;
	int		i;

	textmode(C80);
	gettextinfo(&ti);
	if((scrollbuf=malloc(SCROLL_LINES*ti.screenwidth*2))==NULL) {
		cprintf("Cannot allocate memory\n\n\rPress any key to exit.");
		getch();
		return(-1);
	}
	
	/* Parse command line */
	for(i=1; i<argc; i++) {
		if(argv[i][0]=='-') {
			if(argv[i][1]=='l' && argv[i][2]==0)
				expand=1;
			else
				goto usage;
		}
		else {
			if(infile==NULL)
				infile=argv[i];
			else
				goto usage;
		}
	}

	cterm_init(ti.screenheight, ti.screenwidth, 0, 0, SCROLL_LINES, scrollbuf);
	if(infile) {
		if((f=fopen(infile,"r"))==NULL) {
			cprintf("Cannot read %s\n\n\rPress any key to exit.",argv[1]);
			getch();
			return(-1);
		}
		sprintf(title,"SyncView: %s",getfname(argv[1]));
	}
	else {
		f=stdin;
		strcpy(title,"SyncView: [stdin]");
	}
	settitle(title);
	while((len=fread(buf, 1, BUF_SIZE, f))!=0) {
		if(expand)
			lfexpand(buf, &len);
		cterm_write(buf, len, NULL, 0, &speed);
	}
	viewscroll();
	return(0);

usage:
	cprintf("Usage: %s [-l] [filename]\r\n\r\n"
			"Displays the ANSI file filename expanding \\n to \\r\\n if -l is specified.\r\n"
			"If no filename is specified, reads input from stdin\r\n"
			"\r\n"
			"Press any key to exit.");
	getch();
	return(-1);
}
