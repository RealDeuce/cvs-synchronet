/* qwktomsg.cpp */

/* Synchronet QWK to SMB message conversion routine */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/****************************************************************************/
/* Converts a QWK message packet into a message.							*/
/****************************************************************************/
bool sbbs_t::qwktomsg(FILE *qwk_fp, char *hdrblk, char fromhub, uint subnum
	, uint touser)
{
	char*	body;
	char*	tail;
	char*	header;
	char	str[256],col=0,lastch=0,*p,*lzhbuf,qwkbuf[QWK_BLOCK_LEN+1];
	uint 	i,j,k,lzh=0,storage,skip=0;
	ushort	xlat;
	long	l,bodylen,taillen,length;
	bool	header_cont=false;
	ulong	crc,block,blocks;
	smbmsg_t	msg;
	smbmsg_t	remsg;
	struct	tm tm;

	memset(&msg,0,sizeof(smbmsg_t));		/* Initialize message header */
	msg.hdr.version=smb_ver();

	blocks=atol(hdrblk+116);
	if(blocks<2) {
		errormsg(WHERE,ERR_CHK,"QWK packet header blocks",blocks);
		return(false);
	}

	if(subnum!=INVALID_SUB
		&& (hdrblk[0]=='*' || hdrblk[0]=='+' || cfg.sub[subnum]->misc&SUB_PONLY))
		msg.idx.attr|=MSG_PRIVATE;
	if(subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_AONLY)
		msg.idx.attr|=MSG_ANONYMOUS;
	if(subnum==INVALID_SUB && cfg.sys_misc&SM_DELREADM)
		msg.idx.attr|=MSG_KILLREAD;
	if((fromhub || useron.rest&FLAG('Q')) &&
		(hdrblk[0]=='*' || hdrblk[0]=='-' || hdrblk[0]=='`'))
		msg.idx.attr|=MSG_READ;

	if(subnum!=INVALID_SUB && !fromhub && cfg.sub[subnum]->mod_ar[0]
		&& chk_ar(cfg.sub[subnum]->mod_ar,&useron))
		msg.idx.attr|=MSG_MODERATED;
	if(subnum!=INVALID_SUB && !fromhub && cfg.sub[subnum]->misc&SUB_SYSPERM
		&& sub_op(subnum))
		msg.idx.attr|=MSG_PERMANENT;

	msg.hdr.attr=msg.idx.attr;

	memset(&tm,0,sizeof(tm));
	tm.tm_mon = ((hdrblk[8]&0xf)*10)+(hdrblk[9]&0xf);
	if(tm.tm_mon>0) tm.tm_mon--;	/* zero based */
	tm.tm_mday=((hdrblk[11]&0xf)*10)+(hdrblk[12]&0xf);
	tm.tm_year=((hdrblk[14]&0xf)*10)+(hdrblk[15]&0xf);
	if(tm.tm_year<Y2K_2DIGIT_WINDOW)
		tm.tm_year+=100;
	tm.tm_hour=((hdrblk[16]&0xf)*10)+(hdrblk[17]&0xf);
	tm.tm_min=((hdrblk[19]&0xf)*10)+(hdrblk[20]&0xf);
	tm.tm_sec=0;

	msg.hdr.when_written.time=mktime(&tm);
	if(!(useron.rest&FLAG('Q')) && !fromhub)
		msg.hdr.when_written.zone=sys_timezone(&cfg);
	msg.hdr.when_imported.time=time(NULL);
	msg.hdr.when_imported.zone=sys_timezone(&cfg);

	hdrblk[116]=0;	// don't include number of blocks in "re: msg number"
	if(!(useron.rest&FLAG('Q')) && !fromhub)
		msg.hdr.thread_back=atol((char *)hdrblk+108);

	if((uint)subnum==INVALID_SUB) { 		/* E-mail */
		msg.idx.to=touser;

		username(&cfg,touser,str);
		smb_hfield_str(&msg,RECIPIENT,str);
		sprintf(str,"%u",touser);
		smb_hfield_str(&msg,RECIPIENTEXT,str); 
	} else {
		sprintf(str,"%25.25s",(char *)hdrblk+21);     /* To user */
		truncsp(str);
		smb_hfield_str(&msg,RECIPIENT,str);
		strlwr(str);
		msg.idx.to=crc16(str,0); 
	}

	sprintf(str,"%25.25s",hdrblk+71);   /* Subject */
	truncsp(str);
	smb_hfield_str(&msg,SUBJECT,str);
	msg.idx.subj=subject_crc(str);

	/********************************/
	/* Convert the QWK message text */
	/********************************/

	if((header=(char *)calloc((blocks-1L)*QWK_BLOCK_LEN*2L,sizeof(char)))==NULL) {
		smb_freemsgmem(&msg);
		errormsg(WHERE,ERR_ALLOC,"QWK msg header",(blocks-1L)*QWK_BLOCK_LEN*2L);
		return(false); 
	}

	bodylen=0;
	if((body=(char *)LMALLOC((blocks-1L)*QWK_BLOCK_LEN*2L))==NULL) {
		LFREE(header);
		smb_freemsgmem(&msg);
		errormsg(WHERE,ERR_ALLOC,"QWK msg body",(blocks-1L)*QWK_BLOCK_LEN*2L);
		return(false); 
	}

	taillen=0;
	if((tail=(char *)LMALLOC((blocks-1L)*QWK_BLOCK_LEN*2L))==NULL) {
		LFREE(header);
		LFREE(body);
		smb_freemsgmem(&msg);
		errormsg(WHERE,ERR_ALLOC,"QWK msg tail",(blocks-1L)*QWK_BLOCK_LEN*2L);
		return(false); 
	}

	memset(qwkbuf,0,sizeof(qwkbuf));

	for(block=1;block<blocks;block++) {
		if(!fread(qwkbuf,1,QWK_BLOCK_LEN,qwk_fp))
			break;
		for(k=0;k<QWK_BLOCK_LEN;k++) {
			if(qwkbuf[k]==0)
				continue;
			if(bodylen==0 && (qwkbuf[k]=='@' || header_cont)) {
				if((p=strchr(qwkbuf+k, QWK_NEWLINE))!=NULL)
					*p=0;
				strcat(header, qwkbuf+k);
				strcat(header, "\n");
				if(p==NULL) {
					header_cont=true;
					break;
				}
				k+=strlen(qwkbuf+k);
				header_cont=false;
				continue;
			}
			if(!taillen && qwkbuf[k]==' ' && col==3 && bodylen>=3
				&& body[bodylen-3]=='-' && body[bodylen-2]=='-'
				&& body[bodylen-1]=='-') {
				bodylen-=3;
				strcpy(tail,"--- ");
				taillen=4;
				col++;
				continue; 
			}
			if(qwkbuf[k]==QWK_NEWLINE) {		/* expand QWK_NEWLINE to crlf */
				if(!taillen && col==3 && bodylen>=3 && body[bodylen-3]=='-'
					&& body[bodylen-2]=='-' && body[bodylen-1]=='-') {
					bodylen-=3;
					strcpy(tail,"---");
					taillen=3; 
				}
				col=0;
				if(taillen) {
					tail[taillen++]=CR;
					tail[taillen++]=LF; 
				}
				else {
					body[bodylen++]=CR;
					body[bodylen++]=LF; 
				}
				continue; 
			}
			/* beep restrict */
			if(!fromhub && qwkbuf[k]==BEL && useron.rest&FLAG('B'))   
				continue;
			/* ANSI restriction */
			if(!fromhub && (qwkbuf[k]==1 || qwkbuf[k]==ESC)
				&& useron.rest&FLAG('A'))
				continue;
			if(qwkbuf[k]!=1 && lastch!=1)
				col++;
			if(lastch==CTRL_A && !validattr(qwkbuf[k])) {
				if(taillen) taillen--;
				else		bodylen--;
				lastch=0;
				continue; 
			}
			lastch=qwkbuf[k];
			if(taillen)
				tail[taillen++]=qwkbuf[k];
			else
				body[bodylen++]=qwkbuf[k]; 
		} 
	}

	while(bodylen && body[bodylen-1]==' ') bodylen--; /* remove trailing spaces */
	if(bodylen>=2 && body[bodylen-2]==CR && body[bodylen-1]==LF)
		bodylen-=2;

	while(taillen && tail[taillen-1]<=' ') taillen--; /* remove trailing garbage */

	skip=0;
	if(useron.rest&FLAG('Q') || fromhub) {      /* QWK Net */
		if(!strnicmp(header,"@VIA:",5)) {
			if(!fromhub)
				set_qwk_flag(QWK_VIA);
			p=strchr(header, '\n');
			if(p) {
				*p=0;
				skip=strlen(header)+1; 
			}
			truncsp(header);
			p=header+5; 					/* Skip "@VIA:" */
			while(*p && *p<=' ') p++;		/* Skip any spaces */
			if(route_circ(p,cfg.sys_id)) {
				free(header);
				free(body);
				free(tail);
				smb_freemsgmem(&msg);
				bprintf("\r\nCircular message path: %s\r\n",p);
				sprintf(str,"Circular message path: %s from %s"
					,p,fromhub ? cfg.qhub[fromhub-1]->id:useron.alias);
				errorlog(str);
				return(false); 
			}
			sprintf(str,"%s/%s"
				,fromhub ? cfg.qhub[fromhub-1]->id : useron.alias,p);
			strupr(str);
			update_qwkroute(str); 
		}
		else {
			if(fromhub)
				strcpy(str,cfg.qhub[fromhub-1]->id);
			else
				strcpy(str,useron.alias); 
		}
		strupr(str);
		j=NET_QWK;
		smb_hfield(&msg,SENDERNETTYPE,2,&j);
		smb_hfield_str(&msg,SENDERNETADDR,str);
		sprintf(str,"%25.25s",hdrblk+46);  /* From user */
		truncsp(str);
	} else {
		sprintf(str,"%u",useron.number);
		smb_hfield_str(&msg,SENDEREXT,str);
		if((uint)subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_NAME)
			strcpy(str,useron.name);
		else
			strcpy(str,useron.alias);
	}
	smb_hfield_str(&msg,SENDER,str);
	if((uint)subnum==INVALID_SUB) {
		if(useron.rest&FLAG('Q') || fromhub)
			msg.idx.from=0;
		else
			msg.idx.from=useron.number; 
	} else {
		strlwr(str);
		msg.idx.from=crc16(str,0); 
	}

	if(!strnicmp(header+skip,"@MSGID:",7)) {
		if(!fromhub)
			set_qwk_flag(QWK_MSGID);
		p=strchr(header+skip, '\n');
		i=skip;
		if(p) {
			*p=0;
			skip+=strlen(header+i)+1; 
		}
		p=header+i+7;					/* Skip "@MSGID:" */
		while(*p && *p<=' ') p++;		/* Skip any spaces */
		truncstr(p," ");				/* Truncate at first space char */
		smb_hfield_str(&msg,RFC822MSGID,p);
	}
	if(!strnicmp(header+skip,"@REPLY:",7)) {
		if(!fromhub)
			set_qwk_flag(QWK_MSGID);
		p=strchr(header+skip, '\n');
		i=skip;
		if(p) {
			*p=0;
			skip+=strlen(header+i)+1; 
		}
		p=header+i+7;					/* Skip "@REPLY:" */
		while(*p && *p<=' ') p++;		/* Skip any spaces */
		truncstr(p," ");				/* Truncate at first space char */
		smb_hfield_str(&msg,RFC822REPLYID,p);
	}
	if(!strnicmp(header+skip,"@TZ:",4)) {
		if(!fromhub)
			set_qwk_flag(QWK_TZ);
		p=strchr(header+skip, '\n');
		i=skip;
		if(p) {
			*p=0;
			skip+=strlen(header+i)+1; 
		}
		p=header+i+4;					/* Skip "@TZ:" */
		while(*p && *p<=' ') p++;		/* Skip any spaces */
		msg.hdr.when_written.zone=(short)ahtoul(p); 
	}
	if(!strnicmp(header+skip,"@REPLYTO:",9)) {
		p=strchr(header+skip, '\n');
		i=skip;
		if(p) {
			*p=0;
			skip+=strlen(header+i)+1; 
		}
		p=header+i+9;					/* Skip "@REPLYTO:" */
		while(*p && *p<=' ') p++;		/* Skip any spaces */
		smb_hfield_str(&msg,REPLYTO,p);
	}
	free(header);

	/*****************/
	/* Calculate CRC */
	/*****************/

	if(smb.status.max_crcs) {
		crc=0xffffffffUL;
		for(l=0;l<bodylen;l++)
			crc=ucrc32(body[l],crc);
		crc=~crc;

		/*******************/
		/* Check for dupes */
		/*******************/

		j=smb_addcrc(&smb,crc);
		if(j) {
			if(j==SMB_DUPE_MSG) {
				bprintf("\r\nDuplicate message\r\n");
				if(!fromhub) {
					if(subnum==INVALID_SUB) {
						sprintf(str,"%s duplicate e-mail attempt",useron.alias);
						logline("E!",str); 
					} else {
						sprintf(str,"%s duplicate message attempt in %s %s"
							,useron.alias
							,cfg.grp[cfg.sub[subnum]->grp]->sname
							,cfg.sub[subnum]->lname);
						logline("P!",str); 
					}
				}
			} else
				errormsg(WHERE,ERR_CHK,smb.file,j);

			smb_freemsgmem(&msg);
			LFREE(body);
			LFREE(tail);
			return(false); 
		} 
	}

	if(online==ON_REMOTE)
		bputs(text[WritingIndx]);

	/*************************************/
	/* Write SMB message header and text */
	/*************************************/

	if(subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_LZH && bodylen
		&& bodylen+2+taillen+2>=SDT_BLOCK_LEN
		&& (lzhbuf=(char *)LMALLOC(bodylen*2))!=NULL) {
		length=lzh_encode((uchar *)body,bodylen,(uchar *)lzhbuf);
		if(length>1L
			&& smb_datblocks(length+4L+taillen+2L)
				<smb_datblocks(bodylen+2L+taillen+2L)) {
			bodylen=length; 	/* Compressable */
			length+=4L;
			lzh=1;
			LFREE(body);
			body=lzhbuf; 
		}
		else {					/* Non-compressable */
			length=bodylen+2L;
			LFREE(lzhbuf); 
		} 
	}
	else
		length=bodylen+2L;					 /* +2 for translation string */

	if(taillen)
		length+=taillen+2L;

	if(length&0xfff00000UL) {
		errormsg(WHERE,ERR_LEN,"REP msg",length);
		smb_freemsgmem(&msg);
		LFREE(body);
		LFREE(tail);
		return(false); 
	}

	if((i=smb_locksmbhdr(&smb))!=0) {
		errormsg(WHERE,ERR_LOCK,smb.file,i);
		FREE(body);
		FREE(tail);
		return(false); 
	}

	if(msg.reply_id!=NULL 
		&& smb_getmsgidx_by_msgid(&smb,&remsg,msg.reply_id)==SMB_SUCCESS)
		msg.hdr.thread_back=remsg.idx.number;	/* needed for threading backward */

	if(msg.hdr.thread_back
		&& smb_getstatus(&smb)==SMB_SUCCESS) {

		memset(&remsg,0,sizeof(remsg));
		remsg.hdr.number=msg.hdr.thread_back;

		if(smb_getmsgidx(&smb,&remsg)==SMB_SUCCESS
			&& smb_lockmsghdr(&smb,&remsg)==SMB_SUCCESS) {

			if(smb_getmsghdr(&smb,&remsg)==SMB_SUCCESS) {

				/* Add RFC-822 Reply-ID (generate if necessary) */
				if(msg.reply_id==NULL)
					smb_hfield_str(&msg,RFC822REPLYID,get_msgid(&cfg,smb.subnum,&remsg));

				/* Add FidoNet Reply if original message has FidoNet MSGID */
				if(msg.ftn_reply==NULL && remsg.ftn_msgid!=NULL)
					smb_hfield_str(&msg,FIDOREPLYID,remsg.ftn_msgid);

				smb_updatethread(&smb,&remsg,smb.status.last_msg+1);
			}

			smb_unlockmsghdr(&smb,&remsg);
			smb_freemsgmem(&remsg);
		}
	}

	if(smb.status.attr&SMB_HYPERALLOC) {
		msg.hdr.offset=smb_hallocdat(&smb);
		storage=SMB_HYPERALLOC; 
	}
	else {
		if((i=smb_open_da(&smb))!=0) {
			errormsg(WHERE,ERR_OPEN,smb.file,i);
			FREE(body);
			FREE(tail);
			return(false); 
		}
		if((subnum==INVALID_SUB && cfg.sys_misc&SM_FASTMAIL)
			|| (subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_FAST)) {
			msg.hdr.offset=smb_fallocdat(&smb,length,1);
			storage=SMB_FASTALLOC; 
		}
		else {
			msg.hdr.offset=smb_allocdat(&smb,length,1);
			storage=SMB_SELFPACK; 
		}
		smb_close_da(&smb); 
	}

	if(msg.hdr.offset && msg.hdr.offset<1L) {
		smb_unlocksmbhdr(&smb);
		errormsg(WHERE,ERR_READ,smb.file,msg.hdr.offset);
		smb_freemsgmem(&msg);
		FREE(body);
		FREE(tail); 
	}
	fseek(smb.sdt_fp,msg.hdr.offset,SEEK_SET);
	if(lzh) {
		xlat=XLAT_LZH;
		fwrite(&xlat,2,1,smb.sdt_fp); 
	}
	xlat=XLAT_NONE;
	fwrite(&xlat,2,1,smb.sdt_fp);
	fwrite(body,bodylen,1,smb.sdt_fp);
	smb_dfield(&msg,TEXT_BODY,bodylen+2+(lzh ? 2:0));
	if(taillen) {
		fwrite(&xlat,2,1,smb.sdt_fp);
		fwrite(tail,taillen,1,smb.sdt_fp);
		smb_dfield(&msg,TEXT_TAIL,taillen+2); 
	}
	fflush(smb.sdt_fp);

	if((i=smb_addmsghdr(&smb,&msg,storage))!=0)	// calls smb_unlocksmbhdr() 
		errormsg(WHERE,ERR_WRITE,smb.file,i);

	smb_freemsgmem(&msg);

	LFREE(body);
	LFREE(tail);

	return(true);
}
