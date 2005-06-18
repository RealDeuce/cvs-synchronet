/* $Id: uifcinit.c,v 1.14 2005/06/18 23:26:20 deuce Exp $ */

#include <gen_defs.h>
#include <stdio.h>

#include <ciolib.h>
#include <uifc.h>

#include "uifcinit.h"

extern char* syncterm_version;

uifcapi_t uifc; /* User Interface (UIFC) Library API */
static int uifc_initialized=0;

#define UIFC_INIT	(1<<0)
#define WITH_SCRN	(1<<1)
#define WITH_BOT	(1<<2)

static void (*bottomfunc)(int);

int	init_uifc(BOOL scrn, BOOL bottom) {
	int	i;
	struct	text_info txtinfo;

    gettextinfo(&txtinfo);
	if(!uifc_initialized) {
		uifc.scrn_len=txtinfo.screenheight;
		if((i=uifcini32(&uifc))!=0) {
			fprintf(stderr,"uifc library init returned error %d\n",i);
			return(-1);
		}
		bottomfunc=uifc.bottomline;
		uifc_initialized=UIFC_INIT;
	}

	if(scrn) {
		if(uifc.scrn(syncterm_version)) {
			printf(" USCRN (len=%d) failed!\n",uifc.scrn_len+1);
			uifc_initialized=0;
			uifc.bail();
		}
		uifc_initialized |= (WITH_SCRN|WITH_BOT);
	}
	else {
		uifc.timedisplay=NULL;
		if(!bottom) {
			uifc.bottomline=NULL;
			uifc_initialized &= ~WITH_BOT;
		}
		else {
			uifc.bottomline=bottomfunc;
			uifc_initialized |= WITH_BOT;
			gotoxy(1, txtinfo.screenheight);
			textattr(uifc.bclr|(uifc.cclr<<4));
			clreol();
		}
	}

	return(0);
}

void uifcbail(void)
{
	if(uifc_initialized) {
		uifc.bail();
	}
	uifc_initialized=0;
}

void uifcmsg(char *msg, char *helpbuf)
{
	int i;
	char	*buf;
	struct	text_info txtinfo;

    gettextinfo(&txtinfo);
	i=uifc_initialized;
	if(!i) {
		buf=(char *)malloc(txtinfo.screenheight*txtinfo.screenwidth*2);
		gettext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);
	}
	init_uifc(FALSE, FALSE);
	if(uifc_initialized) {
		uifc.helpbuf=helpbuf;
		uifc.msg(msg);
	}
	else
		fprintf(stderr,"%s\n",msg);
	if(!i) {
		uifcbail();
		puttext(1,1,txtinfo.screenwidth,txtinfo.screenheight,buf);
		free(buf);
	}
}
