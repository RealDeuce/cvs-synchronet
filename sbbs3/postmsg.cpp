/* postmsg.cpp */

/* Synchronet user create/post public message routine */

/* $Id: postmsg.cpp,v 1.33 2003/05/09 07:00:10 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2003 Rob Swindell - http://www.synchro.net/copyright.html		*
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

extern "C" const char* beta_version;

static char* program_id()
{
	static char str[128];
	char compiler[64];

	DESCRIBE_COMPILER(compiler);
	sprintf(str,"%.10s %s%c-%s%s%s %s %s"
		,VERSION_NOTICE,VERSION,REVISION,PLATFORM_DESC
		,beta_version
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,__DATE__,compiler);
	return(str);
}

/****************************************************************************/
/* Posts a message on subboard number sub, with 'top' as top of message.    */
/* Returns 1 if posted, 0 if not.                                           */
/****************************************************************************/
bool sbbs_t::postmsg(uint subnum, smbmsg_t *remsg, long wm_mode)
{
	char	str[256],touser[256],title[LEN_TITLE+1],buf[SDT_BLOCK_LEN]
			,top[256];
	char	msg_id[256];
	char*	pid;
	ushort	xlat,msgattr;
	int 	i,j,x,file,storage;
	ulong	l,length,offset,crc=0xffffffff;
	FILE*	instream;
	smbmsg_t msg,tmpmsg;

	if(remsg) {
		sprintf(title,"%.*s",LEN_TITLE,remsg->subj);
#if 0	/* We *do* support internet posts to specific people July-11-2002 */
		if(cfg.sub[subnum]->misc&SUB_INET)	// All Internet posts to "All" 05/20/97
			touser[0]=0;
		else 
#endif
		if(remsg->hdr.attr&MSG_ANONYMOUS)
			strcpy(touser,text[Anonymous]);
		else
			strcpy(touser,remsg->from);
		msgattr=(ushort)(remsg->hdr.attr&MSG_PRIVATE);
		sprintf(top,text[RegardingByToOn],title,touser,remsg->to
			,timestr((time_t *)&remsg->hdr.when_written.time)
			,zonestr(remsg->hdr.when_written.zone)); }
	else {
		title[0]=0;
		touser[0]=0;
		top[0]=0;
		msgattr=0; }

	/* Security checks */
	if(!chk_ar(cfg.sub[subnum]->post_ar,&useron)) {
		bputs(text[CantPostOnSub]);
		return(false); 
	}
	if(useron.rest&FLAG('P')) {
		bputs(text[R_Post]);
		return(false); 
	}
	if((cfg.sub[subnum]->misc&(SUB_QNET|SUB_FIDO|SUB_PNET|SUB_INET))
		&& (useron.rest&FLAG('N'))) {
		bputs(text[CantPostOnSub]);
		return(false); 
	}
	if(useron.ptoday>=cfg.level_postsperday[useron.level]) {
		bputs(text[TooManyPostsToday]);
		return(false); 
	}

	bprintf(text[Posting],cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname);
	action=NODE_PMSG;
	nodesync();

	if(!(msgattr&MSG_PRIVATE) && (cfg.sub[subnum]->misc&SUB_PONLY
		|| (cfg.sub[subnum]->misc&SUB_PRIV && !noyes(text[PrivatePostQ]))))
		msgattr|=MSG_PRIVATE;

	if(sys_status&SS_ABORT)
		return(false);

	if(
#if 0	/* we *do* support internet posts to specific people July-11-2002 */
		!(cfg.sub[subnum]->misc&SUB_INET) &&	// Prompt for TO: user
#endif
		(cfg.sub[subnum]->misc&SUB_TOUSER || msgattr&MSG_PRIVATE || touser[0])) {
		if(!touser[0] && !(msgattr&MSG_PRIVATE))
			strcpy(touser,"All");
		bputs(text[PostTo]);
		i=LEN_ALIAS;
		if(cfg.sub[subnum]->misc&SUB_QNET)
			i=25;
		if(cfg.sub[subnum]->misc&SUB_FIDO)
			i=35;
		if(cfg.sub[subnum]->misc&(SUB_PNET|SUB_INET))
			i=60;
		getstr(touser,i,K_UPRLWR|K_LINE|K_EDIT|K_AUTODEL);
		if(stricmp(touser,"ALL")
		&& !(cfg.sub[subnum]->misc&(SUB_PNET|SUB_FIDO|SUB_QNET|SUB_INET|SUB_ANON))) {
			if(cfg.sub[subnum]->misc&SUB_NAME) {
				if(!userdatdupe(useron.number,U_NAME,LEN_NAME,touser,0)) {
					bputs(text[UnknownUser]);
					return(false); } }
			else {
				if((i=finduser(touser))==0)
					return(false);
				username(&cfg,i,touser); } }
		if(sys_status&SS_ABORT)
			return(false); }

	if(!touser[0])
		strcpy(touser,"All");       // Default to ALL

	if(!stricmp(touser,"SYSOP") && !SYSOP)  // Change SYSOP to user #1
		username(&cfg,1,touser);

	if(msgattr&MSG_PRIVATE && !stricmp(touser,"ALL")) {
		bputs(text[NoToUser]);
		return(false); }
	if(msgattr&MSG_PRIVATE)
		wm_mode|=WM_PRIVATE;

	if(cfg.sub[subnum]->misc&SUB_AONLY
		|| (cfg.sub[subnum]->misc&SUB_ANON && useron.exempt&FLAG('A')
			&& !noyes(text[AnonymousQ])))
		msgattr|=MSG_ANONYMOUS;

	if(cfg.sub[subnum]->mod_ar[0] && chk_ar(cfg.sub[subnum]->mod_ar,&useron))
		msgattr|=MSG_MODERATED;

	if(cfg.sub[subnum]->misc&SUB_SYSPERM && sub_op(subnum))
		msgattr|=MSG_PERMANENT;

	if(msgattr&MSG_PRIVATE)
		bputs(text[PostingPrivately]);

	if(msgattr&MSG_ANONYMOUS)
		bputs(text[PostingAnonymously]);

	if(cfg.sub[subnum]->misc&SUB_NAME)
		bputs(text[UsingRealName]);

	sprintf(str,"%sINPUT.MSG",cfg.node_dir);
	if(!writemsg(str,top,title,wm_mode,subnum,touser)) {
		bputs(text[Aborted]);
		return(false); }

	bputs(text[WritingIndx]);

	if((i=smb_stack(&smb,SMB_STACK_PUSH))!=0) {
		errormsg(WHERE,ERR_OPEN,cfg.sub[subnum]->code,i);
		return(false); }

	sprintf(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb))!=0) {
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(false); }

	if(filelength(fileno(smb.shd_fp))<1) {	 /* Create it if it doesn't exist */
		smb.status.max_crcs=cfg.sub[subnum]->maxcrcs;
		smb.status.max_msgs=cfg.sub[subnum]->maxmsgs;
		smb.status.max_age=cfg.sub[subnum]->maxage;
		smb.status.attr=cfg.sub[subnum]->misc&SUB_HYPER ? SMB_HYPERALLOC : 0;
		if((i=smb_create(&smb))!=0) {
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			errormsg(WHERE,ERR_CREATE,smb.file,i,smb.last_error);
			return(false); } }

	if((i=smb_locksmbhdr(&smb))!=0) {
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
		return(false); }

	if((i=smb_getstatus(&smb))!=0) {
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_READ,smb.file,i,smb.last_error);
		return(false); }

	length=flength(str)+2;	 /* +2 for translation string */

	if(length&0xfff00000UL) {
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_LEN,str,length);
		return(false); }

	if(smb.status.attr&SMB_HYPERALLOC) {
		offset=smb_hallocdat(&smb);
		storage=SMB_HYPERALLOC; }
	else {
		if((i=smb_open_da(&smb))!=0) {
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
			return(false); }
		if(cfg.sub[subnum]->misc&SUB_FAST) {
			offset=smb_fallocdat(&smb,length,1);
			storage=SMB_FASTALLOC; }
		else {
			offset=smb_allocdat(&smb,length,1);
			storage=SMB_SELFPACK; }
		smb_close_da(&smb); }

	if((file=open(str,O_RDONLY|O_BINARY))==-1
		|| (instream=fdopen(file,"rb"))==NULL) {
		smb_freemsgdat(&smb,offset,length,1);
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);
		errormsg(WHERE,ERR_OPEN,str,O_RDONLY|O_BINARY);
		return(false); }

	setvbuf(instream,NULL,_IOFBF,2*1024);
	fseek(smb.sdt_fp,offset,SEEK_SET);
	xlat=XLAT_NONE;
	fwrite(&xlat,2,1,smb.sdt_fp);
	x=SDT_BLOCK_LEN-2;				/* Don't read/write more than 255 */
	while(!feof(instream)) {
		memset(buf,0,x);
		j=fread(buf,1,x,instream);
		if(j<1)
			break;
		if(j>1 && (j!=x || feof(instream)) && buf[j-1]==LF && buf[j-2]==CR)
			buf[j-1]=buf[j-2]=0;	/* Convert to NULL */
		if(cfg.sub[subnum]->maxcrcs) {
			for(i=0;i<j;i++)
				crc=ucrc32(buf[i],crc); }
		fwrite(buf,j,1,smb.sdt_fp);
		x=SDT_BLOCK_LEN; }
	fflush(smb.sdt_fp);
	fclose(instream);
	crc=~crc;

	memset(&msg,0,sizeof(smbmsg_t));
	msg.hdr.version=smb_ver();
	msg.hdr.attr=msgattr;
	msg.hdr.when_written.time=msg.hdr.when_imported.time=time(NULL);
	msg.hdr.when_written.zone=msg.hdr.when_imported.zone=cfg.sys_timezone;

	/* using the idx records here is not technically necessary, just for convenience */
	msg.idx.attr=msg.hdr.attr;
	msg.idx.time=msg.hdr.when_imported.time;
	msg.idx.number=smb.status.last_msg+1; /* this *should* be the new message number */

	pid=program_id();
	smb_hfield(&msg,FIDOPID,strlen(pid),pid);

	/* Generate default (RFC822) message-id (always) */
	SAFECOPY(msg_id,get_msgid(&cfg,subnum,&msg));
	smb_hfield(&msg,RFC822MSGID,strlen(msg_id),msg_id);

	/* Generate FTN (FTS-9) MSGID */
	if(cfg.sub[subnum]->misc&SUB_FIDO) {
		SAFECOPY(msg_id,ftn_msgid(cfg.sub[subnum],&msg));
		smb_hfield(&msg,FIDOMSGID,strlen(msg_id),msg_id);
	}
	if(remsg) {
		if(remsg->ftn_msgid!=NULL)
			smb_hfield(&msg,FIDOREPLYID,strlen(remsg->ftn_msgid),remsg->ftn_msgid);
		if(remsg->id!=NULL)
			smb_hfield(&msg,RFC822REPLYID,strlen(remsg->id),remsg->id);
		msg.hdr.thread_orig=remsg->hdr.number;
		if(!remsg->hdr.thread_first) {
			remsg->hdr.thread_first=smb.status.last_msg+1;
			if((i=smb_lockmsghdr(&smb,remsg))!=0)
				errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
			else {
				i=smb_putmsghdr(&smb,remsg);
				smb_unlockmsghdr(&smb,remsg);
				if(i)
					errormsg(WHERE,ERR_WRITE,smb.file,i); } }
		else {
			l=remsg->hdr.thread_first;
			while(1) {
				tmpmsg.idx.offset=0;
				if(!loadmsg(&tmpmsg,l))
					break;
				if(tmpmsg.hdr.thread_next && tmpmsg.hdr.thread_next!=l) {
					l=tmpmsg.hdr.thread_next;
					smb_unlockmsghdr(&smb,&tmpmsg);
					smb_freemsgmem(&tmpmsg);
					continue; }
				tmpmsg.hdr.thread_next=smb.status.last_msg+1;
				if((i=smb_putmsghdr(&smb,&tmpmsg))!=0)
					errormsg(WHERE,ERR_WRITE,smb.file,i);
				smb_unlockmsghdr(&smb,&tmpmsg);
				smb_freemsgmem(&tmpmsg);
				break; } } }


	if(cfg.sub[subnum]->maxcrcs) {
		i=smb_addcrc(&smb,crc);
		if(i) {
			smb_freemsgdat(&smb,offset,length,1);
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			attr(cfg.color[clr_err]);
			bputs("Duplicate message!\r\n");
			return(false); 
		} 
	}

	msg.hdr.offset=offset;

	smb_hfield(&msg,RECIPIENT,strlen(touser),touser);
	strlwr(touser);
	msg.idx.to=crc16(touser,0);

	strcpy(str,cfg.sub[subnum]->misc&SUB_NAME ? useron.name : useron.alias);
	smb_hfield(&msg,SENDER,strlen(str),str);
	strlwr(str);
	msg.idx.from=crc16(str,0);

	sprintf(str,"%u",useron.number);
	smb_hfield(&msg,SENDEREXT,strlen(str),str);

	smb_hfield(&msg,SUBJECT,strlen(title),title);
	msg.idx.subj=subject_crc(title);

	smb_dfield(&msg,TEXT_BODY,length);

	i=smb_addmsghdr(&smb,&msg,storage);	// calls smb_unlocksmbhdr() 
	smb_close(&smb);
	smb_stack(&smb,SMB_STACK_POP);

	smb_freemsgmem(&msg);
	if(i) {
		smb_freemsgdat(&smb,offset,length,1);
		errormsg(WHERE,ERR_WRITE,smb.file,i);
		return(false); }

	useron.ptoday++;
	useron.posts++;
	logon_posts++;
	putuserrec(&cfg,useron.number,U_POSTS,5,ultoa(useron.posts,str,10));
	putuserrec(&cfg,useron.number,U_PTODAY,5,ultoa(useron.ptoday,str,10));
	bprintf(text[Posted],cfg.grp[cfg.sub[subnum]->grp]->sname
		,cfg.sub[subnum]->lname);
	sprintf(str,"%s posted on %s %s"
		,useron.alias,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname);
	logline("P+",str);

	signal_sub_sem(&cfg,subnum);

	return(true);
}

extern "C" void DLLCALL signal_sub_sem(scfg_t* cfg, uint subnum)
{
	char str[MAX_PATH+1];
	int file;

	if(subnum==INVALID_SUB || subnum>=cfg->total_subs)	/* e-mail? */
		return;

	/* signal semaphore files */
	if(cfg->sub[subnum]->misc&SUB_FIDO && cfg->echomail_sem[0])		
		if((file=nopen(cmdstr(cfg,NULL,cfg->echomail_sem,nulstr,nulstr,str)
			,O_WRONLY|O_CREAT|O_TRUNC))!=-1)
			close(file);
	if(cfg->sub[subnum]->post_sem[0]) 
		if((file=nopen(cmdstr(cfg,NULL,cfg->sub[subnum]->post_sem,nulstr,nulstr,str)
			,O_WRONLY|O_CREAT|O_TRUNC))!=-1)
			close(file);
}

extern "C" int DLLCALL savemsg(scfg_t* cfg, smb_t* smb, smbmsg_t* msg, char* msgbuf)
{
	char	pad=0;
	char*	pid;
	char*	reply_id;
	char	msg_id[256];
	char*	lzhbuf=NULL;
	ushort	xlat;
	int		xlatlen;
	int 	i;
	int		storage;
	long	lzhlen;
	BOOL	lzh=FALSE;
	ulong	l;
	ulong	length;
	ulong	offset;
	ulong	crc=0xffffffff;
	smbmsg_t remsg;
	smbmsg_t firstmsg;

	if(!SMB_IS_OPEN(smb)) {
		if(smb->subnum==INVALID_SUB)
			sprintf(smb->file,"%smail",cfg->data_dir);
		else
			sprintf(smb->file,"%s%s",cfg->sub[smb->subnum]->data_dir,cfg->sub[smb->subnum]->code);
		smb->retry_time=cfg->smb_retry_time;
		if((i=smb_open(smb))!=0)
			return(i);
	}

	if(filelength(fileno(smb->shd_fp))<1) {	 /* Create it if it doesn't exist */
		if(smb->subnum==INVALID_SUB) {
			smb->status.max_crcs=cfg->mail_maxcrcs;
			smb->status.max_age=cfg->mail_maxage;
			smb->status.max_msgs=MAX_SYSMAIL;
			smb->status.attr=SMB_EMAIL;
		} else {
			smb->status.max_crcs=cfg->sub[smb->subnum]->maxcrcs;
			smb->status.max_msgs=cfg->sub[smb->subnum]->maxmsgs;
			smb->status.max_age=cfg->sub[smb->subnum]->maxage;
			smb->status.attr=cfg->sub[smb->subnum]->misc&SUB_HYPER ? SMB_HYPERALLOC : 0;
		}
		if((i=smb_create(smb))!=0) 
			return(i); 
	}
	if((i=smb_locksmbhdr(smb))!=0) 
		return(i);

	if((i=smb_getstatus(smb))!=0) {
		smb_unlocksmbhdr(smb);
		return(i);
	}
	length=strlen(msgbuf);

	/* Calculate CRC-32 of message text (before encoding, if any) */
	if(smb->status.max_crcs) {
		for(i=0;msgbuf[i];i++)
			crc=ucrc32(msgbuf[i],crc); 
		crc=~crc;
	}

	/* LZH compress? */
	if(smb->subnum!=INVALID_SUB && cfg->sub[smb->subnum]->misc&SUB_LZH 
		&& length+sizeof(xlat)>=SDT_BLOCK_LEN
		&& (lzhbuf=(char *)LMALLOC(length*2))!=NULL) {
		lzhlen=lzh_encode((uchar *)msgbuf,length,(uchar *)lzhbuf);
		if(lzhlen>1
			&& smb_datblocks(lzhlen+(sizeof(xlat)*2)) 
				< smb_datblocks(length+sizeof(xlat))) {
			length=lzhlen+sizeof(xlat); 	/* Compressable */
			lzh=TRUE;
			msgbuf=lzhbuf; 
		}
	}

	length+=sizeof(xlat);	/* xlat string terminator */

	if(length&0x80000000) {
		smb_unlocksmbhdr(smb);
		sprintf(smb->last_error,"message length: 0x%lX",length);
		FREE_AND_NULL(lzhbuf);
		return(-1);
	}

	/* Allocate Data Blocks */
	if(smb->status.attr&SMB_HYPERALLOC) {
		offset=smb_hallocdat(smb);
		storage=SMB_HYPERALLOC; 
	} else {
		if((i=smb_open_da(smb))!=0) {
			smb_unlocksmbhdr(smb);
			FREE_AND_NULL(lzhbuf);
			return(i);
		}
		if((smb->subnum==INVALID_SUB && cfg->sys_misc&SM_FASTMAIL)
			|| (smb->subnum!=INVALID_SUB && cfg->sub[smb->subnum]->misc&SUB_FAST)) {
			offset=smb_fallocdat(smb,length,1);
			storage=SMB_FASTALLOC; 
		} else {
			offset=smb_allocdat(smb,length,1);
			storage=SMB_SELFPACK; 
		}
		smb_close_da(smb); 
	}

	smb_fseek(smb->sdt_fp,offset,SEEK_SET);
	xlatlen=0;
	if(lzh) {
		xlat=XLAT_LZH;
		smb_fwrite(&xlat,sizeof(xlat),smb->sdt_fp);
		xlatlen+=sizeof(xlat);
	}
	xlat=XLAT_NONE;
	smb_fwrite(&xlat,sizeof(xlat),smb->sdt_fp);
	xlatlen+=sizeof(xlat);

	smb_fwrite(msgbuf,length-xlatlen,smb->sdt_fp);
	for(l=length;l%SDT_BLOCK_LEN;l++)
		smb_fwrite(&pad,1,smb->sdt_fp);

	fflush(smb->sdt_fp);

	FREE_AND_NULL(lzhbuf);

	/* Add CRC to CRC history (and check for duplicates) */
	if(smb->status.max_crcs) {
		i=smb_addcrc(smb,crc);
		if(i) {
			smb_freemsgdat(smb,offset,length,1);
			smb_unlocksmbhdr(smb);
			return(i);
		} 
	}

	if(msg==NULL)	/* don't create message header */
		return(smb_unlocksmbhdr(smb));

	msg->hdr.version=smb_ver();
	msg->hdr.when_imported.time=time(NULL);
	msg->hdr.when_imported.zone=cfg->sys_timezone;
	msg->hdr.offset=offset;
	msg->idx.time=msg->hdr.when_imported.time;
	msg->idx.number=smb->status.last_msg+1; /* this *should* be the new message number */

	if(smb->subnum!=INVALID_SUB) {	/* enforce anonymous/private posts only */
		if(cfg->sub[smb->subnum]->misc&SUB_PONLY)
			msg->hdr.attr|=MSG_PRIVATE;
		if(cfg->sub[smb->subnum]->misc&SUB_AONLY)
			msg->hdr.attr|=MSG_ANONYMOUS;
	}
	smb_dfield(msg,TEXT_BODY,length);

	pid=program_id();
	smb_hfield(msg,FIDOPID,strlen(pid),pid);

	/* Generate default (RFC822) message-id  */
	if(smb_get_hfield(msg,RFC822MSGID,NULL)==NULL) {
		SAFECOPY(msg_id,get_msgid(cfg,smb->subnum,msg));
		smb_hfield(msg,RFC822MSGID,strlen(msg_id),msg_id);
	}

	/* Generate FTN MSGID */
	if(smb->subnum!=INVALID_SUB && cfg->sub[smb->subnum]->misc&SUB_FIDO
		&& smb_get_hfield(msg,FIDOMSGID,NULL)==NULL) {
		SAFECOPY(msg_id,ftn_msgid(cfg->sub[smb->subnum],msg));
		smb_hfield(msg,FIDOMSGID,strlen(msg_id),msg_id);
	}

	if(msg->to==NULL)	/* no recipient, don't add header */
		return(smb_unlocksmbhdr(smb));

	/* Look-up thread_orig if Reply-ID was specified */
	if(msg->hdr.thread_orig==0 && msg->reply_id!=NULL) {
		if(get_msg_by_id(cfg, smb, msg->reply_id, &remsg)==TRUE) {
			msg->hdr.thread_orig=remsg.hdr.number;	/* needed for thread linkage */
			smb_freemsgmem(&remsg);
		}
	}

	/* Auto-thread linkage */
	if(msg->hdr.thread_orig) {
		memset(&remsg,0,sizeof(remsg));
		remsg.hdr.number=msg->hdr.thread_orig;
		if((i=smb_getmsgidx(smb, &remsg))!=SMB_SUCCESS)	/* invalid thread origin */
			return(i);

		if((i=smb_lockmsghdr(smb,&remsg))!=SMB_SUCCESS)
			return(i);

		if((i=smb_getmsghdr(smb, &remsg))!=SMB_SUCCESS) {
			smb_unlockmsghdr(smb,&remsg); 
			return(i);
		}

		/* Add RFC-822 Reply-ID (generate if necessary) */
		if(msg->reply_id==NULL) {
			reply_id=get_msgid(cfg,smb->subnum,&remsg);
			smb_hfield(msg,RFC822REPLYID,strlen(reply_id),reply_id);
		}

		/* Add FidoNet Reply if original message has FidoNet MSGID */
		if(msg->ftn_reply==NULL && remsg.ftn_msgid!=NULL)
			smb_hfield(msg,FIDOREPLYID,strlen(remsg.ftn_msgid),remsg.ftn_msgid);

		if(!remsg.hdr.thread_first) {	/* This msg is first reply */
			remsg.hdr.thread_first=msg->idx.number;
			i=smb_putmsghdr(smb,&remsg);
			smb_unlockmsghdr(smb,&remsg);
			if(i!=SMB_SUCCESS) {
				smb_freemsgmem(&remsg);
				return(i); 
			}
		} else {	/* Search for last reply and extend chain */
			smb_unlockmsghdr(smb,&remsg);
			memset(&firstmsg,0,sizeof(firstmsg));
			l=remsg.hdr.thread_first;	/* start with first reply */
			while(1) {
				firstmsg.idx.offset=0;
				firstmsg.hdr.number=l;
				if(smb_getmsgidx(smb, &firstmsg)!=SMB_SUCCESS) /* invalid thread origin */
					break;
				if(smb_lockmsghdr(smb,&remsg)!=SMB_SUCCESS)
					break;
				if(smb_getmsghdr(smb, &remsg)!=SMB_SUCCESS) {
					smb_unlockmsghdr(smb,&remsg); 
					break;
				}
				if(firstmsg.hdr.thread_next && firstmsg.hdr.thread_next!=l) {
					l=firstmsg.hdr.thread_next;
					smb_unlockmsghdr(smb,&firstmsg);
					smb_freemsgmem(&firstmsg);
					continue; 
				}
				firstmsg.hdr.thread_next=msg->idx.number;
				smb_putmsghdr(smb,&firstmsg);
				smb_unlockmsghdr(smb,&firstmsg);
				smb_freemsgmem(&firstmsg);
				break; 
			}
		}
		smb_freemsgmem(&remsg);
	}

	if((i=smb_addmsghdr(smb,msg,storage))!=0) // calls smb_unlocksmbhdr() 
		smb_freemsgdat(smb,offset,length,1);

	signal_sub_sem(cfg,smb->subnum);

	return(i);
}

