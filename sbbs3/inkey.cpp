/* inkey.cpp */

/* Synchronet single key input function (no wait) */

/* $Id: inkey.cpp,v 1.28 2006/09/07 22:59:26 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2006 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include "sbbs.h"

#define LAST_STAT_LINE 16

#define nosound()	

/****************************************************************************/
/* Returns character if a key has been hit remotely and responds			*/
/* Called from functions getkey, msgabort and main_sec						*/
/****************************************************************************/
char sbbs_t::inkey(long mode, unsigned long timeout)
{
	uchar	ch=0;

	if(keybuftop!=keybufbot) {   
		ch=keybuf[keybufbot++];   
		if(keybufbot==KEY_BUFSIZE)   
			keybufbot=0;   
	} else 
		ch=incom(timeout);

	if(ch==0) {
		// moved here from getkey() on AUG-29-2001
		if(sys_status&SS_SYSPAGE) 
			sbbs_beep(sbbs_random(800),100);
#if 0
		if(!(mode&K_GETSTR) || mode&K_LOWPRIO || cfg.node_misc&NM_LOWPRIO)
			YIELD();
#endif
		return(0);
	}

	if(cfg.node_misc&NM_7BITONLY
		&& (!(sys_status&SS_USERON) || useron.misc&NO_EXASCII))
		ch&=0x7f; 

	timeout=time(NULL);

	/* Is this a control key */
	if(ch<' ') {
		if(cfg.ctrlkey_passthru&(1<<ch))	/*  flagged as passthru? */
			return(ch);						/* do not handle here */
		return(handle_ctrlkey(ch,mode));
	}

	if(mode&K_UPPER)
		ch=toupper(ch);

	return(ch);
}

char sbbs_t::handle_ctrlkey(char ch, long mode)
{
	char	str[512];
	char 	tmp[512];
	uint	i,j;

	if(ch==CTRL_C) {  /* Ctrl-C Abort */
		sys_status|=SS_ABORT;
		if(mode&K_SPIN) /* back space once if on spinning cursor */
			backspace();
		return(0); 
	}
	if(ch==CTRL_Z && !(mode&K_MSG)
		&& action!=NODE_PCHT) {	 /* Ctrl-Z toggle raw input mode */
		if(hotkey_inside>1)	/* only allow so much recursion */
			return(0);
		hotkey_inside++;
		if(mode&K_SPIN)
			bputs("\b ");
		SAVELINE;
		attr(LIGHTGRAY);
		CRLF;
		bputs(text[RawMsgInputModeIsNow]);
		if(console&CON_RAW_IN)
			bputs(text[OFF]);
		else
			bputs(text[ON]);
		console^=CON_RAW_IN;
		CRLF;
		CRLF;
		RESTORELINE;
		lncntr=0;
		hotkey_inside--;
		if(action!=NODE_MAIN && action!=NODE_XFER)
			return(CTRL_Z);
		return(0); 
	}

	if(console&CON_RAW_IN)	 /* ignore ctrl-key commands if in raw mode */
		return(ch);

#if 0	/* experimental removal to fix Tracker1's pause module problem with down-arrow */
	if(ch==LF)				/* ignore LF's if not in raw mode */
		return(0);
#endif

	/* Global hot key event */
	for(i=0;i<cfg.total_hotkeys;i++)
		if(cfg.hotkey[i]->key==ch)
			break;
	if(i<cfg.total_hotkeys) {
		if(hotkey_inside>1)	/* only allow so much recursion */
			return(0);
		hotkey_inside++;
		if(mode&K_SPIN)
			bputs("\b ");
		if(!(sys_status&SS_SPLITP)) {
			SAVELINE;
			attr(LIGHTGRAY);
			CRLF; 
		}
		external(cmdstr(cfg.hotkey[i]->cmd,nulstr,nulstr,NULL),0);
		if(!(sys_status&SS_SPLITP)) {
			CRLF;
			RESTORELINE; 
		}
		lncntr=0;
		hotkey_inside--;
		return(0);
	}

	switch(ch) {
		case CTRL_O:	/* Ctrl-O toggles pause temporarily */
			useron.misc^=UPAUSE;
			return(0); 
		case CTRL_P:	/* Ctrl-P Private node-node comm */
			if(!(sys_status&SS_USERON))
				return(0);			 /* keep from being recursive */
			if(hotkey_inside>1)	/* only allow so much recursion */
				return(0);
			hotkey_inside++;
			if(mode&K_SPIN)
				bputs("\b ");
			if(!(sys_status&SS_SPLITP)) {
				SAVELINE;
				attr(LIGHTGRAY);
				CRLF; 
			}
			nodesync(); 	/* read any waiting messages */
			nodemsg();		/* send a message */
			SYNC;
			if(!(sys_status&SS_SPLITP)) {
				CRLF;
				RESTORELINE; 
			}
			lncntr=0;
			hotkey_inside--;
			return(0); 

		case CTRL_U:	/* Ctrl-U Users online */
			/* needs recursion checking */
			if(!(sys_status&SS_USERON))
				return(0);
			if(hotkey_inside>1)	/* only allow so much recursion */
				return(0);
			hotkey_inside++;
			if(mode&K_SPIN)
				bputs("\b ");
			if(!(sys_status&SS_SPLITP)) {
				SAVELINE;
				attr(LIGHTGRAY);
				CRLF; 
			}
			whos_online(true); 	/* list users */
			ASYNC;
			if(!(sys_status&SS_SPLITP)) {
				CRLF;
				RESTORELINE; 
			}
			lncntr=0;
			hotkey_inside--;
			return(0); 
		case CTRL_T: /* Ctrl-T Time information */
			if(sys_status&SS_SPLITP)
				return(ch);
			if(!(sys_status&SS_USERON))
				return(0);
			if(hotkey_inside>1)	/* only allow so much recursion */
				return(0);
			hotkey_inside++;
			if(mode&K_SPIN)
				bputs("\b ");
			SAVELINE;
			attr(LIGHTGRAY);
			now=time(NULL);
			bprintf(text[TiLogon],timestr(&logontime));
			bprintf(text[TiNow],timestr(&now));
			bprintf(text[TiTimeon]
				,sectostr(now-logontime,tmp));
			bprintf(text[TiTimeLeft]
				,sectostr(timeleft,tmp));
			SYNC;
			RESTORELINE;
			lncntr=0;
			hotkey_inside--;
			return(0); 
		case CTRL_K:  /*  Ctrl-k Control key menu */
			if(sys_status&SS_SPLITP)
				return(ch);
			if(!(sys_status&SS_USERON))
				return(0);
			if(hotkey_inside>1)	/* only allow so much recursion */
				return(0);
			hotkey_inside++;
			if(mode&K_SPIN)
				bputs("\b ");
			SAVELINE;
			attr(LIGHTGRAY);
			lncntr=0;
			bputs(text[ControlKeyMenu]);
			ASYNC;
			RESTORELINE;
			lncntr=0;
			hotkey_inside--;
			return(0); 
		case ESC:
			i=incom(mode&K_GETSTR ? 3000:1000);
			if(i==NOINP)		// timed-out waiting for '['
				return(ESC);
			ch=i;
			if(ch!='[') {
				ungetkey(ch);
				return(ESC); 
			}
			i=j=0;
			autoterm|=ANSI; 			/* <ESC>[x means they have ANSI */
			if(sys_status&SS_USERON && useron.misc&AUTOTERM && !(useron.misc&ANSI)
				&& useron.number) {
				useron.misc|=ANSI;
				putuserrec(&cfg,useron.number,U_MISC,8,ultoa(useron.misc,str,16)); 
			}
			while(i<10 && j<30) {		/* up to 3 seconds */
				ch=incom(100);
				if(ch==(NOINP&0xff)) {
					j++;
					continue;
				}
				if(ch!=';' && !isdigit(ch) && ch!='R') {    /* other ANSI */
					switch(ch) {
						case 'A':
							return(0x1e);	/* ctrl-^ (up arrow) */
						case 'B':
							return(LF); 	/* ctrl-j (dn arrow) */
						case 'C':
							return(CTRL_F);	/* ctrl-f (rt arrow) */
						case 'D':
							return(0x1d);	/* ctrl-] (lf arrow) */
						case 'H':	/* ANSI:  home cursor */
							return(CTRL_B);	/* ctrl-b (beg line) */
						case 'F':	/* Xterm: cursor preceding line */
						case 'K':	/* ANSI:  clear-to-end-of-line */
							return(CTRL_E);	/* ctrl-e (end line) */
					}
					ungetkey('[');
					for(j=0;j<i;j++)
						ungetkey(str[j]);
					ungetkey(ch);
					return(ESC); 
				}
				if(ch=='R') {       /* cursor position report */
					if(i && !(useron.rows)) {	/* auto-detect rows */
						str[i]=0;
						rows=atoi(str);
						lprintf(LOG_DEBUG,"Node %d ANSI cursor position report: %u rows"
							,cfg.node_num, rows);
						if(rows<10 || rows>99) rows=24; 
					}
					return(0); 
				}
				str[i++]=ch; 
			}

			ungetkey('[');
			for(j=0;j<i;j++)
				ungetkey(str[j]);
			return(ESC); 
	}
	return(ch);
}
