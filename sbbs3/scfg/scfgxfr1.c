/* scfgxfr1.c */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2002 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include "scfg.h"

void xfer_opts()
{
	static int xfr_dflt;
	char str[81],done;
	int i,j,k,l,dflt,bar;
	static fextr_t savfextr;
	static fview_t savfview;
	static ftest_t savftest;
	static fcomp_t savfcomp;
	static prot_t savprot;
	static dlevent_t savdlevent;
	static char savaltpath[LEN_DIR+1];

while(1) {
	i=0;
	sprintf(opt[i++],"%-33.33s%uk","Min Bytes Free Disk Space"
		,cfg.min_dspace);
	sprintf(opt[i++],"%-33.33s%u","Max Files in Batch UL Queue"
		,cfg.max_batup);
	sprintf(opt[i++],"%-33.33s%u","Max Files in Batch DL Queue"
		,cfg.max_batdn);
	sprintf(opt[i++],"%-33.33s%u","Max Users in User Transfers"
		,cfg.max_userxfer);
	sprintf(opt[i++],"%-33.33s%u%%","Default Credit on Upload"
		,cfg.cdt_up_pct);
	sprintf(opt[i++],"%-33.33s%u%%","Default Credit on Download"
		,cfg.cdt_dn_pct);
	if(cfg.leech_pct)
		sprintf(str,"%u%% after %u seconds"
			,cfg.leech_pct,cfg.leech_sec);
	else
		strcpy(str,"Disabled");
	sprintf(opt[i++],"%-33.33s%s","Leech Protocol Detection",str);
	strcpy(opt[i++],"Viewable Files...");
	strcpy(opt[i++],"Testable Files...");
	strcpy(opt[i++],"Download Events...");
	strcpy(opt[i++],"Extractable Files...");
	strcpy(opt[i++],"Compressable Files...");
	strcpy(opt[i++],"Transfer Protocols...");
	strcpy(opt[i++],"Alternate File Paths...");
	opt[i][0]=0;
	SETHELP(WHERE);
/*
File Transfer Configuration:

This menu has options and sub-menus that pertain specifically to the
file transfer section of the BBS.
*/
	switch(ulist(WIN_ORG|WIN_ACT|WIN_CHE,0,0,72,&xfr_dflt,0
		,"File Transfer Configuration",opt)) {
		case -1:
			i=save_changes(WIN_MID);
			if(i==-1)
				break;
			if(!i) {
				write_file_cfg(&cfg,backup_level);
                rerun_nodes();
            }
            return;
		case 0:
			SETHELP(WHERE);
/*
Minimum Kilobytes Free Disk Space to Allow Uploads:

This is the minimum free space in a file directory to allow user
uploads.
*/
			uinput(WIN_MID,0,0
				,"Minimum Kilobytes Free Disk Space to Allow Uploads"
				,itoa(cfg.min_dspace,tmp,10),5,K_EDIT|K_NUMBER);
			cfg.min_dspace=atoi(tmp);
			break;
		case 1:
			SETHELP(WHERE);
/*
Maximum Files in Batch Upload Queue:

This is the maximum number of files that can be placed in the batch
upload queue.
*/
			uinput(WIN_MID,0,0,"Maximum Files in Batch Upload Queue"
				,itoa(cfg.max_batup,tmp,10),5,K_EDIT|K_NUMBER);
			cfg.max_batup=atoi(tmp);
            break;
		case 2:
			SETHELP(WHERE);
/*
Maximum Files in Batch Download Queue:

This is the maximum number of files that can be placed in the batch
download queue.
*/
			uinput(WIN_MID,0,0,"Maximum Files in Batch Download Queue"
				,itoa(cfg.max_batdn,tmp,10),5,K_EDIT|K_NUMBER);
			cfg.max_batdn=atoi(tmp);
            break;
		case 3:
			SETHELP(WHERE);
/*
Maximum Destination Users in User to User Transfer:

This is the maximum number of users allowed in the destination user list
of a user to user upload.
*/
			uinput(WIN_MID,0,0
				,"Maximum Destination Users in User to User Transfers"
				,itoa(cfg.max_userxfer,tmp,10),5,K_EDIT|K_NUMBER);
			cfg.max_userxfer=atoi(tmp);
			break;
		case 4:
SETHELP(WHERE);
/*
Default Percentage of Credits to Credit Uploader on Upload:

This is the default setting that will be used when new file
directories are created.
*/
			uinput(WIN_MID,0,0
				,"Default Percentage of Credits to Credit Uploader on Upload"
				,itoa(cfg.cdt_up_pct,tmp,10),4,K_EDIT|K_NUMBER);
			cfg.cdt_up_pct=atoi(tmp);
			break;
		case 5:
SETHELP(WHERE);
/*
Default Percentage of Credits to Credit Uploader on Download:

This is the default setting that will be used when new file
directories are created.
*/
			uinput(WIN_MID,0,0
				,"Default Percentage of Credits to Credit Uploader on Download"
				,itoa(cfg.cdt_dn_pct,tmp,10),4,K_EDIT|K_NUMBER);
			cfg.cdt_dn_pct=atoi(tmp);
			break;
		case 6:
			SETHELP(WHERE);
/*
Leech Protocol Detection Percentage:

This value is the sensitivity of the leech protocol detection feature of
Synchronet. If the transfer is apparently unsuccessful, but the transfer
time was at least this percentage of the estimated transfer time (based
on the estimated CPS of the connection result code), then a leech
protocol error is issued and the user's leech download counter is
incremented. Setting this value to 0 disables leech protocol detection.
*/
			savnum=0;
			uinput(WIN_MID|WIN_SAV,0,0
				,"Leech Protocol Detection Percentage (0=Disabled)"
				,itoa(cfg.leech_pct,tmp,10),3,K_EDIT|K_NUMBER);
			cfg.leech_pct=atoi(tmp);
			if(!cfg.leech_pct)
				break;
			SETHELP(WHERE);
/*
Leech Protocol Minimum Time (in Seconds):

This option allows you to adjust the sensitivity of the leech protocol
detection feature. This value is the minimum length of transfer time
(in seconds) that must elapse before an aborted tranfser will be
considered a possible leech attempt.
*/
			uinput(WIN_MID,0,0
				,"Leech Protocol Minimum Time (in Seconds)"
				,itoa(cfg.leech_sec,tmp,10),3,K_EDIT|K_NUMBER);
			cfg.leech_sec=atoi(tmp);
			break;
		case 7: 	/* Viewable file types */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.total_fviews && i<MAX_OPTS;i++)
					sprintf(opt[i],"%-3.3s  %-40s",cfg.fview[i]->ext,cfg.fview[i]->cmd);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if(cfg.total_fviews<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.total_fviews)
					i|=WIN_DEL|WIN_GET;
				if(savfview.cmd[0])
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
Viewable File Types:

This is a list of file types that have content information that can be
viewed through the execution of an external program. Here are a couple of
command line examples for a few file types.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Viewable File Types",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.fview[i]);
					cfg.total_fviews--;
					while(i<cfg.total_fviews) {
						cfg.fview[i]=cfg.fview[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.fview=(fview_t **)REALLOC(cfg.fview
						,sizeof(fview_t *)*(cfg.total_fviews+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_fviews+1);
						cfg.total_fviews=0;
						bail(1);
						continue; }
					if(!cfg.total_fviews) {
						if((cfg.fview[0]=(fview_t *)MALLOC(
							sizeof(fview_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fview_t));
							continue; }
						memset(cfg.fview[0],0,sizeof(fview_t));
						strcpy(cfg.fview[0]->ext,"ZIP");
						strcpy(cfg.fview[0]->cmd,"%!pkunzip -v %f"); }
					else {
						for(j=cfg.total_fviews;j>i;j--)
							cfg.fview[j]=cfg.fview[j-1];
						if((cfg.fview[i]=(fview_t *)MALLOC(
							sizeof(fview_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fview_t));
							continue; }
						*cfg.fview[i]=*cfg.fview[i+1]; }
					cfg.total_fviews++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					savfview=*cfg.fview[i];
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					*cfg.fview[i]=savfview;
					changes=1;
					continue; }
				k=0;
				done=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-22.22s%s","File Extension"
						,cfg.fview[i]->ext);
					sprintf(opt[j++],"%-22.22s%-40s","Command Line"
						,cfg.fview[i]->cmd);
					sprintf(opt[j++],"%-22.22s%s","Access Requirements"
						,cfg.fview[i]->arstr);
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"Viewable File Type",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Viewable File Type Extension"
								,cfg.fview[i]->ext,3,K_EDIT);
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Command Line"
								,cfg.fview[i]->cmd,50,K_EDIT);
							break;
						case 2:
							savnum=2;
							sprintf(str,"Viewable File Type %s"
								,cfg.fview[i]->ext);
							getar(str,cfg.fview[i]->arstr);
                            break; } } }
            break;
		case 8:    /* Testable file types */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.total_ftests && i<MAX_OPTS;i++)
					sprintf(opt[i],"%-3.3s  %-40s",cfg.ftest[i]->ext,cfg.ftest[i]->cmd);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if(cfg.total_ftests<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.total_ftests)
					i|=WIN_DEL|WIN_GET;
				if(savftest.cmd[0])
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
Testable File Types:

This is a list of file types that will have a command line executed to
test the file integrity upon their upload. The file types are specified
by extension and if one file extension is listed more than once, each
command line will be executed. The command lines must return a DOS error
code of 0 (no error) in order for the file to pass the test. This method
of file testing upon upload is also known as an upload event. This test
or event, can do more than just test the file, it can perform any
function that the sysop wishes. Such as adding comments to an archived
file, or extracting an archive and performing a virus scan. While the
external program is executing, a text string is displayed to the user.
This working string can be set for each file type and command line
listed.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Testable File Types",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.ftest[i]);
					cfg.total_ftests--;
					while(i<cfg.total_ftests) {
						cfg.ftest[i]=cfg.ftest[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.ftest=(ftest_t **)REALLOC(cfg.ftest
						,sizeof(ftest_t *)*(cfg.total_ftests+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_ftests+1);
						cfg.total_ftests=0;
						bail(1);
						continue; }
					if(!cfg.total_ftests) {
						if((cfg.ftest[0]=(ftest_t *)MALLOC(
							sizeof(ftest_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(ftest_t));
							continue; }
						memset(cfg.ftest[0],0,sizeof(ftest_t));
						strcpy(cfg.ftest[0]->ext,"ZIP");
						strcpy(cfg.ftest[0]->cmd,"%!pkunzip -t %f");
						strcpy(cfg.ftest[0]->workstr,"Testing ZIP Integrity..."); }
					else {

						for(j=cfg.total_ftests;j>i;j--)
							cfg.ftest[j]=cfg.ftest[j-1];
						if((cfg.ftest[i]=(ftest_t *)MALLOC(
							sizeof(ftest_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(ftest_t));
							continue; }
						*cfg.ftest[i]=*cfg.ftest[i+1]; }
					cfg.total_ftests++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					savftest=*cfg.ftest[i];
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					*cfg.ftest[i]=savftest;
					changes=1;
					continue; }
				done=0;
				k=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-22.22s%s","File Extension"
						,cfg.ftest[i]->ext);
					sprintf(opt[j++],"%-22.22s%-40s","Command Line"
						,cfg.ftest[i]->cmd);
					sprintf(opt[j++],"%-22.22s%s","Working String"
						,cfg.ftest[i]->workstr);
					sprintf(opt[j++],"%-22.22s%s","Access Requirements"
						,cfg.ftest[i]->arstr);
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"Testable File Type",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Testable File Type Extension"
								,cfg.ftest[i]->ext,3,K_EDIT);
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Command Line"
								,cfg.ftest[i]->cmd,50,K_EDIT);
							break;
						case 2:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Working String"
								,cfg.ftest[i]->workstr,40,K_EDIT|K_MSG);
							break;
						case 3:
							savnum=2;
							sprintf(str,"Testable File Type %s",cfg.ftest[i]->ext);
							getar(str,cfg.ftest[i]->arstr);
							break; } } }
			break;
		case 9:    /* Download Events */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.total_dlevents && i<MAX_OPTS;i++)
					sprintf(opt[i],"%-3.3s  %-40s",cfg.dlevent[i]->ext,cfg.dlevent[i]->cmd);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if(cfg.total_dlevents<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.total_dlevents)
					i|=WIN_DEL|WIN_GET;
				if(savdlevent.cmd[0])
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
Download Events:

This is a list of file types that will have a command line executed to
perform an event upon their download (e.g. trigger a download event).
The file types are specified by extension and if one file extension
is listed more than once, each command line will be executed. The
command lines must return a DOS error code of 0 (no error) in order
for the file to pass the test. This test or event, can do more than
just test the file, it can perform any function that the sysop wishes.
Such as adding comments to an archived file, or extracting an archive
and performing a virus scan. While the external program is executing,
a text string is displayed to the user. This working string can be set
for each file type and command line listed.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Download Events",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.dlevent[i]);
					cfg.total_dlevents--;
					while(i<cfg.total_dlevents) {
						cfg.dlevent[i]=cfg.dlevent[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.dlevent=(dlevent_t **)REALLOC(cfg.dlevent
						,sizeof(dlevent_t *)*(cfg.total_dlevents+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_dlevents+1);
						cfg.total_dlevents=0;
						bail(1);
						continue; }
					if(!cfg.total_dlevents) {
						if((cfg.dlevent[0]=(dlevent_t *)MALLOC(
							sizeof(dlevent_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(dlevent_t));
							continue; }
						memset(cfg.dlevent[0],0,sizeof(dlevent_t));
						strcpy(cfg.dlevent[0]->ext,"ZIP");
						strcpy(cfg.dlevent[0]->cmd,"%!pkzip -z %f "
							"< ..\\text\\zipmsg.txt");
						strcpy(cfg.dlevent[0]->workstr,"Adding ZIP Comment..."); }
					else {

						for(j=cfg.total_dlevents;j>i;j--)
							cfg.dlevent[j]=cfg.dlevent[j-1];
						if((cfg.dlevent[i]=(dlevent_t *)MALLOC(
							sizeof(dlevent_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(dlevent_t));
							continue; }
						*cfg.dlevent[i]=*cfg.dlevent[i+1]; }
					cfg.total_dlevents++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					savdlevent=*cfg.dlevent[i];
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					*cfg.dlevent[i]=savdlevent;
					changes=1;
					continue; }
				done=0;
				k=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-22.22s%s","File Extension"
						,cfg.dlevent[i]->ext);
					sprintf(opt[j++],"%-22.22s%-40s","Command Line"
						,cfg.dlevent[i]->cmd);
					sprintf(opt[j++],"%-22.22s%s","Working String"
						,cfg.dlevent[i]->workstr);
					sprintf(opt[j++],"%-22.22s%s","Access Requirements"
						,cfg.dlevent[i]->arstr);
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"Download Event",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Download Event Extension"
								,cfg.dlevent[i]->ext,3,K_EDIT);
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Command Line"
								,cfg.dlevent[i]->cmd,50,K_EDIT);
							break;
						case 2:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Working String"
								,cfg.dlevent[i]->workstr,40,K_EDIT|K_MSG);
							break;
						case 3:
							savnum=2;
							sprintf(str,"Download Event %s",cfg.dlevent[i]->ext);
							getar(str,cfg.dlevent[i]->arstr);
							break; } } }
            break;
		case 10:	 /* Extractable file types */
			dflt=bar=0;
            while(1) {
				for(i=0;i<cfg.total_fextrs && i<MAX_OPTS;i++)
                    sprintf(opt[i],"%-3.3s  %-40s"
                        ,cfg.fextr[i]->ext,cfg.fextr[i]->cmd);
				opt[i][0]=0;
                i=WIN_ACT|WIN_SAV;  /* save cause size can change */
				if(cfg.total_fextrs<MAX_OPTS)
                    i|=WIN_INS;
                if(cfg.total_fextrs)
                    i|=WIN_DEL|WIN_GET;
				if(savfextr.cmd[0])
                    i|=WIN_PUT;
                savnum=0;
                SETHELP(WHERE);
/*
Extractable File Types:

This is a list of archive file types that can be extracted to the temp
directory by an external program. The file types are specified by their
extension. For each file type you must specify the command line used to
extract the file(s).
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Extractable File Types",opt);
                if(i==-1)
                    break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
                    FREE(cfg.fextr[i]);
                    cfg.total_fextrs--;
                    while(i<cfg.total_fextrs) {
                        cfg.fextr[i]=cfg.fextr[i+1];
                        i++; }
                    changes=1;
                    continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.fextr=(fextr_t **)REALLOC(cfg.fextr
						,sizeof(fextr_t *)*(cfg.total_fextrs+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_fextrs+1);
						cfg.total_fextrs=0;
						bail(1);
						continue; }
                    if(!cfg.total_fextrs) {
                        if((cfg.fextr[0]=(fextr_t *)MALLOC(
                            sizeof(fextr_t)))==NULL) {
                            errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fextr_t));
                            continue; }
						memset(cfg.fextr[0],0,sizeof(fextr_t));
                        strcpy(cfg.fextr[0]->ext,"ZIP");
                        strcpy(cfg.fextr[0]->cmd,"%!pkunzip -o %f %g %s"); }
                    else {

                        for(j=cfg.total_fextrs;j>i;j--)
                            cfg.fextr[j]=cfg.fextr[j-1];
                        if((cfg.fextr[i]=(fextr_t *)MALLOC(
                            sizeof(fextr_t)))==NULL) {
                            errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fextr_t));
                            continue; }
                        *cfg.fextr[i]=*cfg.fextr[i+1]; }
                    cfg.total_fextrs++;
                    changes=1;
                    continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
                    savfextr=*cfg.fextr[i];
                    continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
                    *cfg.fextr[i]=savfextr;
                    changes=1;
                    continue; }
				k=0;
				done=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-22.22s%s","File Extension"
						,cfg.fextr[i]->ext);
					sprintf(opt[j++],"%-22.22s%-40s","Command Line"
						,cfg.fextr[i]->cmd);
					sprintf(opt[j++],"%-22.22s%s","Access Requirements"
						,cfg.fextr[i]->arstr);
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"Extractable File Type",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Extractable File Type Extension"
								,cfg.fextr[i]->ext,3,K_EDIT);
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Command Line"
								,cfg.fextr[i]->cmd,50,K_EDIT);
							break;
						case 2:
							savnum=2;
							sprintf(str,"Extractable File Type %s"
								,cfg.fextr[i]->ext);
							getar(str,cfg.fextr[i]->arstr);
                            break; } } }
            break;
		case 11:	 /* Compressable file types */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.total_fcomps && i<MAX_OPTS;i++)
					sprintf(opt[i],"%-3.3s  %-40s",cfg.fcomp[i]->ext,cfg.fcomp[i]->cmd);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if(cfg.total_fcomps<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.total_fcomps)
					i|=WIN_DEL|WIN_GET;
				if(savfcomp.cmd[0])
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
Compressable File Types:

This is a list of compression methods available for different file types.
These will be used for items such as creating QWK packets, temporary
files from the transfer section, and more.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Compressable File Types",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.fcomp[i]);
					cfg.total_fcomps--;
					while(i<cfg.total_fcomps) {
						cfg.fcomp[i]=cfg.fcomp[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.fcomp=(fcomp_t **)REALLOC(cfg.fcomp
						,sizeof(fcomp_t *)*(cfg.total_fcomps+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_fcomps+1);
						cfg.total_fcomps=0;
						bail(1);
						continue; }
					if(!cfg.total_fcomps) {
						if((cfg.fcomp[0]=(fcomp_t *)MALLOC(
							sizeof(fcomp_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fcomp_t));
							continue; }
						memset(cfg.fcomp[0],0,sizeof(fcomp_t));
						strcpy(cfg.fcomp[0]->ext,"ZIP");
						strcpy(cfg.fcomp[0]->cmd,"%!pkzip %f %s"); }
					else {
						for(j=cfg.total_fcomps;j>i;j--)
							cfg.fcomp[j]=cfg.fcomp[j-1];
						if((cfg.fcomp[i]=(fcomp_t *)MALLOC(
							sizeof(fcomp_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(fcomp_t));
							continue; }
						*cfg.fcomp[i]=*cfg.fcomp[i+1]; }
					cfg.total_fcomps++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					savfcomp=*cfg.fcomp[i];
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					*cfg.fcomp[i]=savfcomp;
					changes=1;
					continue; }
				k=0;
				done=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-22.22s%s","File Extension"
						,cfg.fcomp[i]->ext);
					sprintf(opt[j++],"%-22.22s%-40s","Command Line"
						,cfg.fcomp[i]->cmd);
					sprintf(opt[j++],"%-22.22s%s","Access Requirements"
						,cfg.fcomp[i]->arstr);
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"Compressable File Type",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Compressable File Type Extension"
								,cfg.fcomp[i]->ext,3,K_EDIT);
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Command Line"
								,cfg.fcomp[i]->cmd,50,K_EDIT);
							break;
						case 2:
							savnum=2;
							sprintf(str,"Compressable File Type %s"
								,cfg.fcomp[i]->ext);
							getar(str,cfg.fcomp[i]->arstr);
                            break; } } }
            break;
		case 12:	/* Transfer protocols */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.total_prots && i<MAX_OPTS;i++)
					sprintf(opt[i],"%c  %-40s"
						,cfg.prot[i]->mnemonic,cfg.prot[i]->ulcmd);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if(cfg.total_prots<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.total_prots)
					i|=WIN_DEL|WIN_GET;
				if(savprot.mnemonic)
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
File Transfer Protocols:

This is a list of file transfer protocols that can be used to transfer
files either to or from a remote user. For each protocol, you can
specify the mnemonic (hot-key) to use to specify that protocol, the
command line to use for uploads, downloads, batch uploads, batch
downloads, bidirectional file transfers, and the support of DSZLOG. If
the protocol doesn't support a certain method of transfer, or you don't
wish it to be available for a certain method of transfer, leave the
command line for that method blank. Be advised, that if you add or
remove any transfer protocols, you will need to edit the protocol menus
(ULPROT, DLPROT, BATUPROT, BATDPROT, and BIPROT) in the TEXT\MENU
directory accordingly.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"File Transfer Protocols",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.prot[i]);
					cfg.total_prots--;
					while(i<cfg.total_prots) {
						cfg.prot[i]=cfg.prot[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.prot=(prot_t **)REALLOC(cfg.prot
						,sizeof(prot_t *)*(cfg.total_prots+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_prots+1);
						cfg.total_prots=0;
						bail(1);
						continue; }
					if(!cfg.total_prots) {
						if((cfg.prot[0]=(prot_t *)MALLOC(
							sizeof(prot_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(prot_t));
							continue; }
						memset(cfg.prot[0],0,sizeof(prot_t));
						cfg.prot[0]->mnemonic='Y';
						cfg.prot[0]->misc=PROT_DSZLOG;
						strcpy(cfg.prot[0]->ulcmd
							,"%!dsz port %p estimate 0 %e rb %f");
						strcpy(cfg.prot[0]->dlcmd
							,"%!dsz port %p estimate 0 %e sb %f");
						strcpy(cfg.prot[0]->batulcmd
							,"%!dsz port %p estimate 0 %e rb %f");
						strcpy(cfg.prot[0]->batdlcmd
							,"%!dsz port %p estimate 0 %e sb @%f"); }
					else {

						for(j=cfg.total_prots;j>i;j--)
							cfg.prot[j]=cfg.prot[j-1];
						if((cfg.prot[i]=(prot_t *)MALLOC(
							sizeof(prot_t)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(prot_t));
							continue; }
						*cfg.prot[i]=*cfg.prot[i+1]; }
					cfg.total_prots++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					savprot=*cfg.prot[i];
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					*cfg.prot[i]=savprot;
					changes=1;
					continue; }
				done=0;
				k=0;
				while(!done) {
					j=0;
					sprintf(opt[j++],"%-25.25s%c","Mnemonic (Command Key)"
						,cfg.prot[i]->mnemonic);
					sprintf(opt[j++],"%-25.25s%-40s","Protocol Name"
						,cfg.prot[i]->name);
					sprintf(opt[j++],"%-25.25s%-40s","Access Requirements"
						,cfg.prot[i]->arstr);
					sprintf(opt[j++],"%-25.25s%-40s","Upload Command Line"
						,cfg.prot[i]->ulcmd);
					sprintf(opt[j++],"%-25.25s%-40s","Download Command Line"
						,cfg.prot[i]->dlcmd);
					sprintf(opt[j++],"%-25.25s%-40s","Batch UL Command Line"
						,cfg.prot[i]->batulcmd);
					sprintf(opt[j++],"%-25.25s%-40s","Batch DL Command Line"
						,cfg.prot[i]->batdlcmd);
					sprintf(opt[j++],"%-25.25s%-40s","Bidir Command Line"
						,cfg.prot[i]->bicmd);
					sprintf(opt[j++],"%-25.25s%s","Uses DSZLOG"
						,cfg.prot[i]->misc&PROT_DSZLOG ? "Yes":"No");
					opt[j][0]=0;
					savnum=1;
					switch(ulist(WIN_RHT|WIN_BOT|WIN_SAV|WIN_ACT,0,0,0,&k,0
						,"File Transfer Protocol",opt)) {
						case -1:
							done=1;
							break;
						case 0:
							str[0]=cfg.prot[i]->mnemonic;
							str[1]=0;
							uinput(WIN_MID|WIN_SAV,0,0
								,"Mnemonic (Command Key)"
								,str,1,K_UPPER|K_EDIT);
							if(str[0])
								cfg.prot[i]->mnemonic=str[0];
							break;
						case 1:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Protocol Name"
								,cfg.prot[i]->name,25,K_EDIT);
                            break;
						case 2:
							savnum=2;
							sprintf(str,"Protocol %s",cfg.prot[i]->name);
							getar(str,cfg.prot[i]->arstr);
							break;
						case 3:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Upload Command"
								,cfg.prot[i]->ulcmd,50,K_EDIT);
							break;
						case 4:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Download Command"
								,cfg.prot[i]->dlcmd,50,K_EDIT);
                            break;
						case 5:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Batch UL Command"
								,cfg.prot[i]->batulcmd,50,K_EDIT);
                            break;
						case 6:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Batch DL Command"
								,cfg.prot[i]->batdlcmd,50,K_EDIT);
                            break;
						case 7:
							uinput(WIN_MID|WIN_SAV,0,0
								,"Bi-dir Command"
								,cfg.prot[i]->bicmd,50,K_EDIT);
                            break;
						case 8:
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							l=0;
							savnum=2;
							l=ulist(WIN_MID|WIN_SAV,0,0,0,&l,0
								,"Uses DSZLOG",opt);
							if(!l && !(cfg.prot[i]->misc&PROT_DSZLOG)) {
								cfg.prot[i]->misc|=PROT_DSZLOG;
								changes=1; }
							else if(l==1 && cfg.prot[i]->misc&PROT_DSZLOG) {
								cfg.prot[i]->misc&=~PROT_DSZLOG;
								changes=1; }
							break; } } }
			break;
		case 13:	/* Alternate file paths */
			dflt=bar=0;
			while(1) {
				for(i=0;i<cfg.altpaths;i++)
					sprintf(opt[i],"%3d: %-40s",i+1,cfg.altpath[i]);
				opt[i][0]=0;
				i=WIN_ACT|WIN_SAV;	/* save cause size can change */
				if((int)cfg.altpaths<MAX_OPTS)
					i|=WIN_INS;
				if(cfg.altpaths)
					i|=WIN_DEL|WIN_GET;
				if(savaltpath[0])
					i|=WIN_PUT;
				savnum=0;
				SETHELP(WHERE);
/*
Alternate File Paths:

This option allows the sysop to add and configure alternate file paths
for files stored on drives and directories other than the configured
storage path for a file directory. This command is useful for those who
have file directories where they wish to have files listed from
multiple CD-ROMs or hard disks.
*/
				i=ulist(i,0,0,50,&dflt,&bar,"Alternate File Paths",opt);
				if(i==-1)
					break;
				if((i&MSK_ON)==MSK_DEL) {
					i&=MSK_OFF;
					FREE(cfg.altpath[i]);
					cfg.altpaths--;
					while(i<cfg.altpaths) {
						cfg.altpath[i]=cfg.altpath[i+1];
						i++; }
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_INS) {
					i&=MSK_OFF;
					if((cfg.altpath=(char **)REALLOC(cfg.altpath
						,sizeof(char *)*(cfg.altpaths+1)))==NULL) {
						errormsg(WHERE,ERR_ALLOC,nulstr,cfg.altpaths+1);
						cfg.altpaths=0;
						bail(1);
                        continue; }
					if(!cfg.altpaths) {
						if((cfg.altpath[0]=(char *)MALLOC(LEN_DIR+1))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,LEN_DIR+1);
							continue; }
						memset(cfg.altpath[0],0,LEN_DIR+1); }
					else {
						for(j=cfg.altpaths;j>i;j--)
							cfg.altpath[j]=cfg.altpath[j-1];
						if((cfg.altpath[i]=(char *)MALLOC(LEN_DIR+1))==NULL) {
							errormsg(WHERE,ERR_ALLOC,nulstr,LEN_DIR+1);
							continue; }
						memcpy(cfg.altpath[i],cfg.altpath[i+1],LEN_DIR+1); }
					cfg.altpaths++;
					changes=1;
					continue; }
				if((i&MSK_ON)==MSK_GET) {
					i&=MSK_OFF;
					memcpy(savaltpath,cfg.altpath[i],LEN_DIR+1);
					continue; }
				if((i&MSK_ON)==MSK_PUT) {
					i&=MSK_OFF;
					memcpy(cfg.altpath[i],savaltpath,LEN_DIR+1);
					changes=1;
					continue; }
				sprintf(str,"Path %d",i+1);
				uinput(WIN_MID|WIN_SAV,0,0,str,cfg.altpath[i],50,K_EDIT); }
			break; } }
}

