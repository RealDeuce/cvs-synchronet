/* pack_qwk.cpp */

/* Synchronet pack QWK packet routine */

/* $Id$ */

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
#include "qwk.h"
#include "post.h"

/****************************************************************************/
/* Creates QWK packet, returning 1 if successful, 0 if not. 				*/
/****************************************************************************/
bool sbbs_t::pack_qwk(char *packet, ulong *msgcnt, bool prepack)
{
	char	str[256],tmp2[256],ch,*p;
	char 	tmp[512];
	char*	fname;
	int 	file,mode;
	uint	i,j,k,conf;
	long	l,size,msgndx,posts,ex;
	ulong	mailmsgs=0;
	ulong	totalcdt,totaltime,lastmsg
			,files,submsgs,msgs,netfiles=0,preqwk=0;
	float	f;	/* Sparky is responsible */
	time_t	start;
	node_t	node;
	mail_t	*mail;
	post_t	HUGE16 *post;
	glob_t	g;
	FILE	*stream,*qwk,*personal,*ndx;
	DIR*	dir;
	DIRENT*	dirent;
	struct	tm* tm;
	smbmsg_t msg;


	ex=EX_OUTL|EX_OUTR;
	if(prepack)
		ex|=EX_OFFLINE;

	delfiles(cfg.temp_dir,ALLFILES);
	sprintf(str,"%sfile/%04u.qwk",cfg.data_dir,useron.number);
	if(fexist(str)) {
		for(k=0;k<cfg.total_fextrs;k++)
			if(!stricmp(cfg.fextr[k]->ext,useron.tmpext)
				&& chk_ar(cfg.fextr[k]->ar,&useron))
				break;
		if(k>=cfg.total_fextrs)
			k=0;
		i=external(cmdstr(cfg.fextr[k]->cmd,str,ALLFILES,NULL),ex);
		if(!i)
			preqwk=1; }

	if(useron.rest&FLAG('Q') && useron.qwk&QWK_RETCTLA)
		useron.qwk|=(QWK_NOINDEX|QWK_NOCTRL|QWK_VIA|QWK_TZ);

	if(useron.qwk&QWK_EXPCTLA)
		mode=A_EXPAND;
	else if(useron.qwk&QWK_RETCTLA)
		mode=A_LEAVE;
	else mode=0;
	if(useron.qwk&QWK_TZ)
		mode|=TZ;
	if(useron.qwk&QWK_VIA)
		mode|=VIA;
	(*msgcnt)=0L;
	if(/* !prepack && */ !(useron.qwk&QWK_NOCTRL)) {
		/***************************/
		/* Create CONTROL.DAT file */
		/***************************/
		sprintf(str,"%sCONTROL.DAT",cfg.temp_dir);
		if((stream=fnopen(&file,str,O_WRONLY|O_CREAT|O_TRUNC))==NULL) {
			errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_TRUNC);
			return(false); }

		now=time(NULL);
		tm=gmtime(&now);
		if(tm==NULL)
			return(false);

		fprintf(stream,"%s\r\n%s\r\n%s\r\n%s, Sysop\r\n0000,%s\r\n"
			"%02u-%02u-%u,%02u:%02u:%02u\r\n"
			,cfg.sys_name
			,cfg.sys_location
			,cfg.node_phone
			,cfg.sys_op
			,cfg.sys_id
			,tm->tm_mon+1,tm->tm_mday,tm->tm_year+1900
			,tm->tm_hour,tm->tm_min,tm->tm_sec);
		k=0;
		for(i=0;i<usrgrps;i++)
			for(j=0;j<usrsubs[i];j++)
				k++;	/* k is how many subs */
		fprintf(stream,"%s\r\n\r\n0\r\n0\r\n%u\r\n",useron.alias,k);
		fprintf(stream,"0\r\nE-mail\r\n");   /* first conference is e-mail */
		for(i=0;i<usrgrps;i++)
			for(j=0;j<usrsubs[i];j++)
				fprintf(stream,"%u\r\n%s\r\n"
					,cfg.sub[usrsub[i][j]]->qwkconf ? cfg.sub[usrsub[i][j]]->qwkconf
					: ((i+1)*1000)+j+1,cfg.sub[usrsub[i][j]]->qwkname);
		fprintf(stream,"HELLO\r\nBBSNEWS\r\nGOODBYE\r\n");
		fclose(stream);
		/***********************/
		/* Create DOOR.ID File */
		/***********************/
		sprintf(str,"%sDOOR.ID",cfg.temp_dir);
		if((stream=fnopen(&file,str,O_WRONLY|O_CREAT|O_TRUNC))==NULL) {
			errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_TRUNC);
			return(false); }
		p="CONTROLTYPE = ";
		fprintf(stream,"DOOR = %s\r\nVERSION = %s\r\n"
			"SYSTEM = %s v%s\r\n"
			"CONTROLNAME = SBBS\r\n"
			"%sADD\r\n"
			"%sDROP\r\n"
			"%sYOURS\r\n"
			"%sRESET\r\n"
			"%sRESETALL\r\n"
			"%sFILES\r\n"
			"%sATTACH\r\n"
			"%sOWN\r\n"
			"%smail\r\n"
			"%sDELMAIL\r\n"
			"%sCTRL-A\r\n"
			"%sFREQ\r\n"
			"%sNDX\r\n"
			"%sTZ\r\n"
			"%sVIA\r\n"
			"%sCONTROL\r\n"
			"MIXEDCASE = YES\r\n"
			,"Synchronet"
			,VERSION
			,"Synchronet"
			,VERSION
			,p,p,p,p
			,p,p,p,p
			,p,p,p,p
			,p,p,p,p
			);
		fclose(stream);
		if(useron.rest&FLAG('Q')) {
			/***********************/
			/* Create NETFLAGS.DAT */
			/***********************/
			sprintf(str,"%sNETFLAGS.DAT",cfg.temp_dir);
			if((stream=fnopen(&file,str,O_WRONLY|O_CREAT))==NULL) {
				errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT);
				return(false); }
			ch=1;						/* Net enabled */
			if(usrgrps)
				for(i=0;i<(usrgrps*1000)+usrsubs[usrgrps-1];i++)
					fputc(ch,stream);
			fclose(stream); }
		} /* !prepack */

	/****************************************************/
	/* Create MESSAGES.DAT, write header and leave open */
	/****************************************************/
	sprintf(str,"%sMESSAGES.DAT",cfg.temp_dir);
	if((qwk=fnopen(&file,str,O_CREAT|O_WRONLY|O_TRUNC))==NULL) {
		errormsg(WHERE,ERR_OPEN,str,O_CREAT|O_WRONLY|O_TRUNC);
		return(false); }
	l=filelength(file);
	if(l<1) {
		fprintf(qwk,"%-128s",QWK_HEADER);
		msgndx=1; }
	else
		msgndx=l/128L;
	fseek(qwk,0,SEEK_END);
	sprintf(str,"%sNEWFILES.DAT",cfg.temp_dir);
	remove(str);
	if(!(useron.rest&FLAG('T')) && useron.qwk&QWK_FILES)
		files=create_filelist("NEWFILES.DAT",FL_ULTIME);
	else
		files=0;

	start=time(NULL);

	if(useron.rest&FLAG('Q'))
		useron.qwk|=(QWK_EMAIL|QWK_ALLMAIL|QWK_DELMAIL);

	if(!(useron.qwk&QWK_NOINDEX)) {
		sprintf(str,"%sPERSONAL.NDX",cfg.temp_dir);
		if((personal=fnopen(&file,str,O_CREAT|O_WRONLY|O_APPEND))==NULL) {
			fclose(qwk);
			errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_APPEND);
			return(false); }
		fseek(personal,0L,SEEK_END); }
	else
		personal=NULL;

	if(useron.qwk&(QWK_EMAIL|QWK_ALLMAIL) /* && !prepack */) {
		sprintf(smb.file,"%smail",cfg.data_dir);
		smb.retry_time=cfg.smb_retry_time;
		if((i=smb_open(&smb))!=0) {
			fclose(qwk);
			if(personal)
				fclose(personal);
			errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
			return(false); }

		/***********************/
		/* Pack E-mail, if any */
		/***********************/
		qwkmail_time=time(NULL);
		mail=loadmail(&smb,&mailmsgs,useron.number,0,useron.qwk&QWK_ALLMAIL ? 0
			: LM_UNREAD);
		if(mailmsgs && !(sys_status&SS_ABORT)) {
			bputs(text[QWKPackingEmail]);
			if(!(useron.qwk&QWK_NOINDEX)) {
				sprintf(str,"%s000.NDX",cfg.temp_dir);
				if((ndx=fnopen(&file,str,O_CREAT|O_WRONLY|O_APPEND))==NULL) {
					fclose(qwk);
					if(personal)
						fclose(personal);
					smb_close(&smb);
					errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_APPEND);
					FREE(mail);
					return(false); }
				fseek(ndx,0L,SEEK_END); }
			else
				ndx=NULL;

			if(useron.rest&FLAG('Q'))
				mode|=TO_QNET;
			else
				mode&=~TO_QNET;

			for(l=0;(ulong)l<mailmsgs;l++) {
				bprintf("\b\b\b\b\b\b\b\b\b\b\b\b%4lu of %-4lu"
					,l+1,mailmsgs);

				msg.idx.offset=mail[l].offset;
				if(!loadmsg(&msg,mail[l].number))
					continue;

				if(msg.hdr.auxattr&MSG_FILEATTACH && useron.qwk&QWK_ATTACH) {
					sprintf(str,"%sfile/%04u.in/%s"
						,cfg.data_dir,useron.number,msg.subj);
					sprintf(tmp,"%s%s",cfg.temp_dir,msg.subj);
					if(fexist(str) && !fexist(tmp))
						mv(str,tmp,1); }

				size=msgtoqwk(&msg,qwk,mode,INVALID_SUB,0);
				smb_unlockmsghdr(&smb,&msg);
				smb_freemsgmem(&msg);
				if(ndx) {
					msgndx++;
					f=ltomsbin(msgndx); 	/* Record number */
					ch=0;					/* Sub number, not used */
					if(personal) {
						fwrite(&f,4,1,personal);
						fwrite(&ch,1,1,personal); }
					fwrite(&f,4,1,ndx);
					fwrite(&ch,1,1,ndx);
					msgndx+=size/128L; } }
			bprintf(text[QWKPackedEmail],mailmsgs);
			if(ndx)
				fclose(ndx); }
		smb_close(&smb);					/* Close the e-mail */
		if(mailmsgs)
			FREE(mail);
		}

	/*********************/
	/* Pack new messages */
	/*********************/
	for(i=0;i<usrgrps;i++) {
		for(j=0;j<usrsubs[i] && !msgabort();j++)
			if(sub_cfg[usrsub[i][j]]&SUB_CFG_NSCAN
				|| (!(useron.rest&FLAG('Q'))
				&& cfg.sub[usrsub[i][j]]->misc&SUB_FORCED)) {
				if(!chk_ar(cfg.sub[usrsub[i][j]]->read_ar,&useron))
					continue;
				lncntr=0;						/* defeat pause */
				if(useron.rest&FLAG('Q') && !(cfg.sub[usrsub[i][j]]->misc&SUB_QNET))
					continue;	/* QWK Net Node and not QWK networked, so skip */

				msgs=getlastmsg(usrsub[i][j],&lastmsg,0);
				if(!msgs || lastmsg<=sub_ptr[usrsub[i][j]]) { /* no msgs */
					if(sub_ptr[usrsub[i][j]]>lastmsg)	{ /* corrupted ptr */
						sub_ptr[usrsub[i][j]]=lastmsg; /* so fix automatically */
						sub_last[usrsub[i][j]]=lastmsg; }
					bprintf(text[NScanStatusFmt]
						,cfg.grp[cfg.sub[usrsub[i][j]]->grp]->sname
						,cfg.sub[usrsub[i][j]]->lname,0L,msgs);
					continue; }

				sprintf(smb.file,"%s%s"
					,cfg.sub[usrsub[i][j]]->data_dir,cfg.sub[usrsub[i][j]]->code);
				smb.retry_time=cfg.smb_retry_time;
				if((k=smb_open(&smb))!=0) {
					errormsg(WHERE,ERR_OPEN,smb.file,k,smb.last_error);
					continue; }

				k=0;
				if(useron.qwk&QWK_BYSELF)
					k|=LP_BYSELF;
				if(!(sub_cfg[usrsub[i][j]]&SUB_CFG_YSCAN))
					k|=LP_OTHERS;
				post=loadposts(&posts,usrsub[i][j],sub_ptr[usrsub[i][j]],k);

				bprintf(text[NScanStatusFmt]
					,cfg.grp[cfg.sub[usrsub[i][j]]->grp]->sname
					,cfg.sub[usrsub[i][j]]->lname,posts,msgs);
				if(!posts)	{ /* no new messages */
					smb_close(&smb);
					continue; }
				bputs(text[QWKPackingSubboard]);	
				submsgs=0;
				conf=cfg.sub[usrsub[i][j]]->qwkconf;
				if(!conf)
					conf=((i+1)*1000)+j+1;

				if(!(useron.qwk&QWK_NOINDEX)) {
					sprintf(str,"%s%u.NDX",cfg.temp_dir,conf);
					if((ndx=fnopen(&file,str,O_CREAT|O_WRONLY|O_APPEND))==NULL) {
						fclose(qwk);
						if(personal)
							fclose(personal);
						smb_close(&smb);
						errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_APPEND);
						LFREE(post);
						return(false); }
					fseek(ndx,0L,SEEK_END); }
				else
					ndx=NULL;

				for(l=0;l<posts && !msgabort();l++) {
					bprintf("\b\b\b\b\b%-5lu",l+1);

					sub_ptr[usrsub[i][j]]=post[l].number;	/* set ptr */
					sub_last[usrsub[i][j]]=post[l].number; /* set last read */

					msg.idx.offset=post[l].offset;
					if(!loadmsg(&msg,post[l].number))
						continue;

					if(useron.rest&FLAG('Q')) {
						if(msg.from_net.type && msg.from_net.type!=NET_QWK &&
							!(cfg.sub[usrsub[i][j]]->misc&SUB_GATE)) { /* From other */
							smb_freemsgmem(&msg);			 /* net, don't gate */
							smb_unlockmsghdr(&smb,&msg);
							continue; }
						mode|=(TO_QNET|TAGLINE);
						if(msg.from_net.type==NET_QWK) {
							mode&=~TAGLINE;
							if(route_circ((char *)msg.from_net.addr,useron.alias)
								|| !strnicmp(msg.subj,"NE:",3)) {
								smb_freemsgmem(&msg);
								smb_unlockmsghdr(&smb,&msg);
								continue; } } }
					else
						mode&=~(TAGLINE|TO_QNET);

					size=msgtoqwk(&msg,qwk,mode,usrsub[i][j],conf);
					smb_unlockmsghdr(&smb,&msg);

					if(ndx) {
						msgndx++;
						f=ltomsbin(msgndx); 	/* Record number */
						ch=0;					/* Sub number, not used */
						if(personal
							&& (!stricmp(msg.to,useron.alias)
								|| !stricmp(msg.to,useron.name))) {
							fwrite(&f,4,1,personal);
							fwrite(&ch,1,1,personal); }
						fwrite(&f,4,1,ndx);
						fwrite(&ch,1,1,ndx);
						msgndx+=size/128L; }

					smb_freemsgmem(&msg);
					(*msgcnt)++;
					submsgs++;
					if(cfg.max_qwkmsgs
						&& !(useron.rest&FLAG('Q')) && (*msgcnt)>=cfg.max_qwkmsgs) {
						bputs(text[QWKmsgLimitReached]);
						break; } }
				if(!(sys_status&SS_ABORT))
					bprintf(text[QWKPackedSubboard],submsgs,(*msgcnt));
				if(ndx) {
					fclose(ndx);
					sprintf(str,"%s%u.NDX",cfg.temp_dir,conf);
					if(!flength(str))
						remove(str); }
				smb_close(&smb);
				LFREE(post);
				if(l<posts)
					break; }
		if(j<usrsubs[i]) /* if sub aborted, abort all */
			break; }

	if((*msgcnt)+mailmsgs && time(NULL)-start) {
		bprintf("\r\n\r\n\1n\1hPacked %lu messages in %lu seconds "
			"(%lu messages/second)."
			,(*msgcnt)+mailmsgs,time(NULL)-start
			,((*msgcnt)+mailmsgs)/(time(NULL)-start));
		lprintf("Packed %lu messages in %lu seconds (%lu msgs/sec)"
			,(*msgcnt)+mailmsgs,time(NULL)-start
			,((*msgcnt)+mailmsgs)/(time(NULL)-start));
	}

	fclose(qwk);			/* close MESSAGE.DAT */
	if(personal) {
		fclose(personal);		 /* close PERSONAL.NDX */
		sprintf(str,"%sPERSONAL.NDX",cfg.temp_dir);
		if(!flength(str))
			remove(str); }
	CRLF;

	if(!prepack && (sys_status&SS_ABORT || !online))
		return(false);

	if(/*!prepack && */ useron.rest&FLAG('Q')) { /* If QWK Net node, check for files */
		sprintf(str,"%sqnet/%s.out/",cfg.data_dir,useron.alias);
		dir=opendir(str);
		while((dirent=readdir(dir))!=NULL) {    /* Move files into temp dir */
			sprintf(str,"%sqnet/%s.out/%s",cfg.data_dir,useron.alias,dirent->d_name);
			if(isdir(str))
				continue;
			sprintf(tmp2,"%s%s",cfg.temp_dir,dirent->d_name);
			lncntr=0;	/* Default pause */
			lprintf("Including %s in packet",str);
			bprintf(text[RetrievingFile],str);
			if(!mv(str,tmp2,1))
				netfiles++;
		}
		closedir(dir);
		if(netfiles)
			CRLF; }

	if(batdn_total) {
		for(i=0,totalcdt=0;i<batdn_total;i++)
				totalcdt+=batdn_cdt[i];
		if(!(useron.exempt&FLAG('D'))
			&& totalcdt>useron.cdt+useron.freecdt) {
			bprintf(text[YouOnlyHaveNCredits]
				,ultoac(useron.cdt+useron.freecdt,tmp)); }
		else {
			for(i=0,totaltime=0;i<batdn_total;i++) {
				if(!(cfg.dir[batdn_dir[i]]->misc&DIR_TFREE) && cur_cps)
					totaltime+=batdn_size[i]/(ulong)cur_cps; }
			if(!(useron.exempt&FLAG('T')) && !SYSOP && totaltime>timeleft)
				bputs(text[NotEnoughTimeToDl]);
			else {
				for(i=0;i<batdn_total;i++) {
					lncntr=0;
					unpadfname(batdn_name[i],tmp);
					sprintf(tmp2,"%s%s",cfg.temp_dir,tmp);
					if(!fexist(tmp2)) {
						seqwait(cfg.dir[batdn_dir[i]]->seqdev);
						bprintf(text[RetrievingFile],tmp);
						sprintf(str,"%s%s"
							,batdn_alt[i]>0 && batdn_alt[i]<=cfg.altpaths
							? cfg.altpath[batdn_alt[i]-1]
							: cfg.dir[batdn_dir[i]]->path
							,tmp);
						mv(str,tmp2,1); /* copy the file to temp dir */
						getnodedat(cfg.node_num,&thisnode,1);
						thisnode.aux=0xfe;
						putnodedat(cfg.node_num,&thisnode);
						CRLF; } } } } }

	if(!(*msgcnt) && !mailmsgs && !files && !netfiles && !batdn_total
		&& (prepack || !preqwk)) {
		bputs(text[QWKNoNewMessages]);
		return(false); }

	if(!prepack && !(useron.rest&FLAG('Q'))) {      /* Don't include in network */
		/***********************/					/* packets */
		/* Copy QWK Text files */
		/***********************/
		sprintf(str,"%sQWK/HELLO",cfg.text_dir);
		if(fexist(str)) {
			sprintf(tmp2,"%sHELLO",cfg.temp_dir);
			mv(str,tmp2,1); }
		sprintf(str,"%sQWK/BBSNEWS",cfg.text_dir);
		if(fexist(str)) {
			sprintf(tmp2,"%sBBSNEWS",cfg.temp_dir);
			mv(str,tmp2,1); }
		sprintf(str,"%sQWK/GOODBYE",cfg.text_dir);
		if(fexist(str)) {
			sprintf(tmp2,"%sGOODBYE",cfg.temp_dir);
			mv(str,tmp2,1); }
		sprintf(str,"%sQWK/BLT-*",cfg.text_dir);
		glob(str,0,NULL,&g);
		for(i=0;i<g.gl_pathc;i++) { 			/* Copy BLT-*.* files */
			fname=getfname(g.gl_pathv[i]);
			padfname(fname,str);
			if(isdigit(str[4]) && isdigit(str[9])) {
				sprintf(str,"%sQWK/%s",cfg.text_dir,fname);
				sprintf(tmp2,"%s%s",cfg.temp_dir,fname);
				mv(str,tmp2,1); 
			}
		}
		globfree(&g);
	}

	if(prepack) {
		for(i=1;i<=cfg.sys_nodes;i++) {
			getnodedat(i,&node,0);
			if((node.status==NODE_INUSE || node.status==NODE_QUIET
				|| node.status==NODE_LOGON) && node.useron==useron.number)
				break; }
		if(i<=cfg.sys_nodes)	/* Don't pre-pack with user online */
			return(false); }

	/*******************/
	/* Compress Packet */
	/*******************/
	sprintf(tmp2,"%s%s",cfg.temp_dir,ALLFILES);
	i=external(cmdstr(temp_cmd(),packet,tmp2,NULL),ex);
	if(!fexist(packet)) {
		bputs(text[QWKCompressionFailed]);
		if(i)
			errormsg(WHERE,ERR_EXEC,cmdstr(temp_cmd(),packet,tmp2,NULL),i);
		else
			errorlog("Couldn't compress QWK packet");
		return(false); }

	if(prepack) 		/* Early return if pre-packing */
		return(true);

	l=flength(packet);
	sprintf(str,"%s.qwk",cfg.sys_id);
	bprintf(text[FiFilename],str);
	bprintf(text[FiFileSize],ultoac(l,tmp));
	if(l>0L && cur_cps)
		i=l/(ulong)cur_cps;
	else
		i=0;
	bprintf(text[FiTransferTime],sectostr(i,tmp));
	CRLF;
	if(!(useron.exempt&FLAG('T')) && i>timeleft) {
		bputs(text[NotEnoughTimeToDl]);
		return(false); 
	}

	if(useron.rest&FLAG('Q')) {
		sprintf(str,"%s.qwk",cfg.sys_id);
		dir=opendir(cfg.temp_dir);
		while((dirent=readdir(dir))!=NULL) {
			if(!stricmp(str,dirent->d_name))	/* QWK packet */
				continue;
			sprintf(tmp,"%s%s",cfg.temp_dir,dirent->d_name);
			if(!isdir(tmp))
				remove(tmp); 
		}
		closedir(dir);
	}

	return(true);
}
