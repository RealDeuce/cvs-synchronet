/* scansubs.cpp */

/* Synchronet message database scanning routines */

/* $Id: scansubs.cpp,v 1.1 2000/10/10 11:24:48 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/****************************************************************************/
/* Used to scan single or multiple sub-boards. 'mode' is the scan type.     */
/****************************************************************************/
void sbbs_t::scansubs(long mode)
{
	char ch,str[256];
	uint i=0,found=0;

	mnemonics(text[SubGroupOrAll]);
	ch=(char)getkeys("SGA\r",0);
	if(sys_status&SS_ABORT || ch==CR)
		return;

	if(ch!='A' && mode&(SCAN_FIND|SCAN_TOYOU)) {
		if(yesno(text[DisplayTitlesOnlyQ])) i=1;
		if(mode&SCAN_FIND) {
			bputs(text[SearchStringPrompt]);
			if(!getstr(str,40,K_LINE|K_UPPER))
				return;
			if(i) { 			/* if titles only */
				if(ch=='S')
					found=searchsub(usrsub[curgrp][cursub[curgrp]],str);
				else if(ch=='G')
					for(i=0;i<usrsubs[curgrp] && !msgabort();i++)
						found=searchsub(usrsub[curgrp][i],str);
				sprintf(tmp,"Searched messages for '%s'",str);
				logline(nulstr,tmp);
				if(!found)
					CRLF;
				return; } }
		else if(mode&SCAN_TOYOU && i) {
			if(ch=='S')
				found=searchsub_toyou(usrsub[curgrp][cursub[curgrp]]);
			else if(ch=='G')
				for(i=0;i<usrsubs[curgrp] && !msgabort();i++)
					found=searchsub_toyou(usrsub[curgrp][i]);
			if(!found)
				CRLF;
			return; } }

	if(ch=='S') {
		if(useron.misc&(RIP|WIP) && !(useron.misc&EXPERT)) {
			menu("MSGSCAN"); }
		i=scanposts(usrsub[curgrp][cursub[curgrp]],mode,str);
		bputs(text[MessageScan]);
		if(i) bputs(text[MessageScanAborted]);
		else bputs(text[MessageScanComplete]);
		return; }
	if(ch=='G') {
		if(useron.misc&(RIP|WIP) && !(useron.misc&EXPERT)) {
			menu("MSGSCAN"); }
		for(i=0;i<usrsubs[curgrp] && !msgabort();i++)
			if(((mode&SCAN_NEW &&
				(sub_cfg[usrsub[curgrp][i]]&SUB_CFG_NSCAN
					|| cfg.sub[usrsub[curgrp][i]]->misc&SUB_FORCED))
				|| (mode&SCAN_TOYOU && sub_cfg[usrsub[curgrp][i]]&SUB_CFG_SSCAN)
				|| mode&SCAN_FIND)
				&& scanposts(usrsub[curgrp][i],mode,str)) break;
		bputs(text[MessageScan]);
		if(i==usrsubs[curgrp]) bputs(text[MessageScanComplete]);
			else bputs(text[MessageScanAborted]);
		return; }

	scanallsubs(mode);
}

/****************************************************************************/
/* Performs a new message scan all all sub-boards							*/
/****************************************************************************/
void sbbs_t::scanallsubs(long mode)
{
	char str[256];
	uint i,j,found=0;

	if(/* action==NODE_MAIN && */ mode&(SCAN_FIND|SCAN_TOYOU)) {
		i=yesno(text[DisplayTitlesOnlyQ]);
		if(mode&SCAN_FIND) {
			bputs(text[SearchStringPrompt]);
			if(!getstr(str,40,K_LINE|K_UPPER))
				return;
			if(i) { 			/* if titles only */
				for(i=0;i<usrgrps;i++) {
					for(j=0;j<usrsubs[i] && !msgabort();j++)
						found=searchsub(usrsub[i][j],str);
					if(j<usrsubs[i])
						break; }
				if(!found)
					CRLF;
				sprintf(tmp,"Searched messages for '%s'",str);
				logline(nulstr,tmp);
				return; } }
		else if(mode&SCAN_TOYOU && i) {
			for(i=0;i<usrgrps;i++) {
				for(j=0;j<usrsubs[i] && !msgabort();j++)
					found=searchsub_toyou(usrsub[i][j]);
				if(j<usrsubs[i])
					break; }
			if(!found)
				CRLF;
			return; } }

	if(useron.misc&(RIP|WIP) && !(useron.misc&EXPERT)) {
		menu("MSGSCAN"); }
	for(i=0;i<usrgrps;i++) {
		for(j=0;j<usrsubs[i] && !msgabort();j++)
			if(((mode&SCAN_NEW && sub_cfg[usrsub[i][j]]&SUB_CFG_NSCAN)
				|| cfg.sub[usrsub[i][j]]->misc&SUB_FORCED
				|| mode&SCAN_FIND
				|| (mode&SCAN_TOYOU && sub_cfg[usrsub[i][j]]&SUB_CFG_SSCAN))
				&& scanposts(usrsub[i][j],mode,str)) break;
		if(j<usrsubs[i])
			break; }
	bputs(text[MessageScan]);
	if(i<usrgrps) {
		bputs(text[MessageScanAborted]);
		return; }
	bputs(text[MessageScanComplete]);
	if(mode&SCAN_NEW && !(mode&(SCAN_BACK|SCAN_TOYOU))
		&& useron.misc&ANFSCAN && !(useron.rest&FLAG('T'))) {
		xfer_cmds++;
		scanalldirs(FL_ULTIME); }
}

void sbbs_t::new_scan_ptr_cfg()
{
	uint	i,j;
	long	s;
	ulong	l;
	time_t	t;

	while(online) {
		bputs(text[CfgGrpLstHdr]);
		for(i=0;i<usrgrps && !msgabort();i++) {
			checkline();
			if(i<9) outchar(SP);
			if(i<99) outchar(SP);
			bprintf(text[CfgGrpLstFmt],i+1,cfg.grp[usrgrp[i]]->lname); }
		SYNC;
		mnemonics(text[WhichOrAll]);
		s=getkeys("AQ",i);
		if(!s || s==-1 || s=='Q')
			break;
		if(s=='A') {
			mnemonics("\r\nEnter number of messages from end, ~Date, ~Quit, or"
				" [Last Message]: ");
			s=getkeys("DLQ",9999);
			if(s==-1 || s=='Q')
				continue;
			if(s=='D') {
				t=time(NULL);
				if(inputnstime(&t) && !(sys_status&SS_ABORT)) {
					bputs(text[LoadingMsgPtrs]);
					for(i=0;i<usrgrps && online;i++)
						for(j=0;j<usrsubs[i] && online;j++) {
							checkline();
							sub_ptr[usrsub[i][j]]=getmsgnum(usrsub[i][j],t); } }
				continue; }
			if(s=='L')
				s=0;
			if(s)
				s&=~0x80000000L;
			bputs(text[LoadingMsgPtrs]);
			for(i=0;i<usrgrps;i++)
				for(j=0;j<usrsubs[i] && online;j++) {
					checkline();
					getlastmsg(usrsub[i][j],&l,0);
					if(s>(long)l)
						sub_ptr[usrsub[i][j]]=0;
					else
						sub_ptr[usrsub[i][j]]=l-s; }
			continue; }
		i=(s&~0x80000000L)-1;
		while(online) {
			l=0;
			bprintf(text[CfgSubLstHdr],cfg.grp[usrgrp[i]]->lname);
			for(j=0;j<usrsubs[i] && !msgabort();j++) {
				checkline();
				if(j<9) outchar(SP);
				if(j<99) outchar(SP);
				t=getmsgtime(usrsub[i][j],sub_ptr[usrsub[i][j]]);
				if(t>(long)l)
					l=t;
				bprintf(text[SubPtrLstFmt],j+1,cfg.sub[usrsub[i][j]]->lname
					,timestr(&t),nulstr); }
			SYNC;
			mnemonics(text[WhichOrAll]);
			s=getkeys("AQ",j);
			if(sys_status&SS_ABORT) {
				lncntr=0;
				return; }
			if(s==-1 || !s || s=='Q')
				break;
			if(s=='A') {    /* The entire group */
				mnemonics("\r\nEnter number of messages from end, ~Date, ~Quit, or"
					" [Last Message]: ");
				s=getkeys("DLQ",9999);
				if(s==-1 || s=='Q')
					continue;
				if(s=='D') {
					t=l;
					if(inputnstime(&t) && !(sys_status&SS_ABORT)) {
						bputs(text[LoadingMsgPtrs]);
						for(j=0;j<usrsubs[i] && online;j++) {
							checkline();
							sub_ptr[usrsub[i][j]]=getmsgnum(usrsub[i][j],t); } }
					continue; }
				if(s=='L')
					s=0;
				if(s)
					s&=~0x80000000L;
				bputs(text[LoadingMsgPtrs]);
				for(j=0;j<usrsubs[i] && online;j++) {
					checkline();
					getlastmsg(usrsub[i][j],&l,0);
					if(s>(long)l)
						sub_ptr[usrsub[i][j]]=0;
					else
						sub_ptr[usrsub[i][j]]=l-s; }
				continue; }
			else {
				j=(s&~0x80000000L)-1;
				mnemonics("\r\nEnter number of messages from end, ~Date, ~Quit, or"
					" [Last Message]: ");
				s=getkeys("DLQ",9999);
				if(s==-1 || s=='Q')
					continue;
				if(s=='D') {
					t=getmsgtime(usrsub[i][j],sub_ptr[usrsub[i][j]]);
					if(inputnstime(&t) && !(sys_status&SS_ABORT)) {
						bputs(text[LoadingMsgPtrs]);
						sub_ptr[usrsub[i][j]]=getmsgnum(usrsub[i][j],t); }
					continue; }
				if(s=='L')
					s=0;
				if(s)
					s&=~0x80000000L;
				getlastmsg(usrsub[i][j],&l,0);
				if(s>(long)l)
					sub_ptr[usrsub[i][j]]=0;
				else
					sub_ptr[usrsub[i][j]]=l-s; }
				} }
}

void sbbs_t::new_scan_cfg(ulong misc)
{
	long	s;
	ulong	i,j;
	ulong	t;

	while(online) {
		bputs(text[CfgGrpLstHdr]);
		for(i=0;i<usrgrps && !msgabort();i++) {
			checkline();
			if(i<9) outchar(SP);
			if(i<99) outchar(SP);
			bprintf(text[CfgGrpLstFmt],i+1,cfg.grp[usrgrp[i]]->lname); }
		SYNC;
		if(misc&SUB_CFG_NSCAN)
			mnemonics(text[NScanCfgWhichGrp]);
		else
			mnemonics(text[SScanCfgWhichGrp]);
		s=getnum(i);
		if(s<1)
			break;
		i=s-1;
		while(online) {
			if(misc&SUB_CFG_NSCAN)
				misc&=~SUB_CFG_YSCAN;
			bprintf(text[CfgSubLstHdr],cfg.grp[usrgrp[i]]->lname);
			for(j=0;j<usrsubs[i] && !msgabort();j++) {
				checkline();
				if(j<9) outchar(SP);
				if(j<99) outchar(SP);
				bprintf(text[CfgSubLstFmt],j+1
					,cfg.sub[usrsub[i][j]]->lname
					,sub_cfg[usrsub[i][j]]&misc ?
						(misc&SUB_CFG_NSCAN && sub_cfg[usrsub[i][j]]&SUB_CFG_YSCAN) ?
						"To You Only" : text[On] : text[Off]);
					}
			SYNC;
			if(misc&SUB_CFG_NSCAN)
				mnemonics(text[NScanCfgWhichSub]);
			else
				mnemonics(text[SScanCfgWhichSub]);
			s=getkeys("AQ",j);
			if(sys_status&SS_ABORT) {
				lncntr=0;
				return; }
			if(!s || s==-1 || s=='Q')
				break;
			if(s=='A') {
				t=sub_cfg[usrsub[i][0]]&misc;
				if(misc&SUB_CFG_NSCAN && !t && !(useron.misc&FLAG('Q')))
					if(!noyes("Messages to you only"))
						misc|=SUB_CFG_YSCAN;
				for(j=0;j<usrsubs[i] && online;j++) {
					checkline();
					if(t) sub_cfg[usrsub[i][j]]&=~misc;
					else  {
						if(misc&SUB_CFG_NSCAN)
							sub_cfg[usrsub[i][j]]&=~SUB_CFG_YSCAN;
						sub_cfg[usrsub[i][j]]|=misc; } }
				continue; }
			j=(s&~0x80000000L)-1;
			if(misc&SUB_CFG_NSCAN && !(sub_cfg[usrsub[i][j]]&misc)) {
				if(!(useron.misc&FLAG('Q')) && !noyes("Messages to you only"))
					sub_cfg[usrsub[i][j]]|=SUB_CFG_YSCAN;
				else
					sub_cfg[usrsub[i][j]]&=~SUB_CFG_YSCAN; }
			sub_cfg[usrsub[i][j]]^=misc; } }
}

