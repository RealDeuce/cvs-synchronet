/* smblib.c */

/* Synchronet message base (SMB) library routines */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

/* ANSI C Library headers */

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>		/* malloc */
#include <string.h>
#include <ctype.h>		/* isdigit */
#include <sys/types.h>
#include <sys/stat.h>	/* must come after sys/types.h */

/* SMB-specific headers */
#include "smblib.h"
#include "genwrap.h"
#include "filewrap.h"

/* Use smb_ver() and smb_lib_ver() to obtain these values */
#define SMBLIB_VERSION		"2.40"      /* SMB library version */
#define SMB_VERSION 		0x0121		/* SMB format version */
										/* High byte major, low byte minor */

static char* nulstr="";

int SMBCALL smb_ver(void)
{
	return(SMB_VERSION);
}

char* SMBCALL smb_lib_ver(void)
{
	return(SMBLIB_VERSION);
}

/****************************************************************************/
/* Open a message base of name 'smb->file'                                  */
/* Opens files for READing messages or updating message indices only        */
/****************************************************************************/
int SMBCALL smb_open(smb_t* smb)
{
	int			i;
	time_t		start=0;
	smbhdr_t	hdr;

	/* Set default values, if uninitialized */
	if(!smb->retry_time)
		smb->retry_time=10;		/* seconds */
	if(!smb->retry_delay 
		|| smb->retry_delay>(smb->retry_time*100))	/* at least ten retries */
		smb->retry_delay=250;	/* milliseconds */
	smb->shd_fp=smb->sdt_fp=smb->sid_fp=NULL;
	smb->sha_fp=smb->sda_fp=smb->hash_fp=NULL;
	smb->last_error[0]=0;

	/* Check for message-base lock semaphore file (under maintenance?) */
	while(smb_islocked(smb)) {
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time)
				return(SMB_ERR_TIMEOUT); 
		SLEEP(smb->retry_delay);
	}

	if((i=smb_open_fp(smb,&smb->shd_fp,SH_DENYNO))!=SMB_SUCCESS)
		return(i);

	memset(&(smb->status),0,sizeof(smb->status));
	if(filelength(fileno(smb->shd_fp))>=(long)sizeof(smbhdr_t)) {
		setvbuf(smb->shd_fp,NULL,_IONBF,SHD_BLOCK_LEN);
		if(smb_locksmbhdr(smb)!=SMB_SUCCESS) {
			smb_close(smb);
			/* smb_lockmsghdr set last_error */
			return(SMB_ERR_LOCK); 
		}
		memset(&hdr,0,sizeof(smbhdr_t));
		if(smb_fread(smb,&hdr,sizeof(smbhdr_t),smb->shd_fp)!=sizeof(smbhdr_t)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading header"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			smb_close(smb);
			return(SMB_ERR_READ);
		}
		if(memcmp(hdr.id,SMB_HEADER_ID,LEN_HEADER_ID)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"corrupt SMB header ID: %.*s"
				,LEN_HEADER_ID,hdr.id);
			smb_close(smb);
			return(SMB_ERR_HDR_ID); 
		}
		if(hdr.version<0x110) {         /* Compatibility check */
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"insufficient header version: %X"
				,hdr.version);
			smb_close(smb);
			return(SMB_ERR_HDR_VER); 
		}
		if(smb_fread(smb,&(smb->status),sizeof(smbstatus_t),smb->shd_fp)!=sizeof(smbstatus_t)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading status"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			smb_close(smb);
			return(SMB_ERR_READ); 
		}
		smb_unlocksmbhdr(smb);
		rewind(smb->shd_fp); 
	}

	setvbuf(smb->shd_fp,NULL,_IOFBF,SHD_BLOCK_LEN);

	if((i=smb_open_fp(smb,&smb->sdt_fp,SH_DENYNO))!=SMB_SUCCESS)
		return(i);

	if((i=smb_open_fp(smb,&smb->sid_fp,SH_DENYNO))!=SMB_SUCCESS)
		return(i);

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Closes the currently open message base									*/
/****************************************************************************/
void SMBCALL smb_close(smb_t* smb)
{
	if(smb->shd_fp!=NULL) {
		smb_unlocksmbhdr(smb);		   /* In case it's been locked */
		smb_close_fp(&smb->shd_fp); 
	}
	smb_close_fp(&smb->sid_fp);
	smb_close_fp(&smb->sdt_fp);
	smb_close_fp(&smb->sid_fp);
	smb_close_fp(&smb->sda_fp);
	smb_close_fp(&smb->sha_fp);
	smb_close_fp(&smb->hash_fp);
}

/****************************************************************************/
/* This set of functions is used to exclusively-lock an entire message base	*/
/* against any other process opening any of the message base files.			*/
/* Currently, this is only used while smbutil packs a message base.			*/
/* This is achieved with a semaphore lock file (e.g. mail.lock).			*/
/****************************************************************************/
static char* smb_lockfname(smb_t* smb, char* fname, size_t maxlen)
{
	safe_snprintf(fname,maxlen,"%s.lock",smb->file);
	return(fname);
}

/****************************************************************************/
/* This function is used to lock an entire message base for exclusive		*/
/* (typically for maintenance/repair)										*/
/****************************************************************************/
int SMBCALL smb_lock(smb_t* smb)
{
	char	path[MAX_PATH+1];
	int		file;
	time_t	start=0;

	smb_lockfname(smb,path,sizeof(path)-1);
	while((file=open(path,O_CREAT|O_EXCL|O_RDWR,S_IREAD|S_IWRITE))==-1) {
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time) {
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"%d (%s) creating %s"
					,get_errno(),STRERROR(get_errno()),path);
				return(SMB_ERR_LOCK);
			}
		SLEEP(smb->retry_delay);
	}
	close(file);
	return(SMB_SUCCESS);
}

int SMBCALL smb_unlock(smb_t* smb)
{
	char	path[MAX_PATH+1];

	smb_lockfname(smb,path,sizeof(path)-1);
	if(remove(path)!=0) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) removing %s"
			,get_errno(),STRERROR(get_errno()),path);
		return(SMB_ERR_DELETE);
	}
	return(SMB_SUCCESS);
}

BOOL SMBCALL smb_islocked(smb_t* smb)
{
	char	path[MAX_PATH+1];

	if(access(smb_lockfname(smb,path,sizeof(path)-1),0)!=0)
		return(FALSE);
	safe_snprintf(smb->last_error,sizeof(smb->last_error),"%s exists",path);
	return(TRUE);
}

/****************************************************************************/
/* If the parameter 'push' is non-zero, this function stores the currently  */
/* open message base to the "virtual" smb stack. Up to SMB_STACK_LEN        */
/* message bases may be stored (defined in SMBDEFS.H).						*/
/* The parameter 'op' is the operation to perform on the stack. Either      */
/* SMB_STACK_PUSH, SMB_STACK_POP, or SMB_STACK_XCHNG						*/
/* If the operation is SMB_STACK_POP, this function restores a message base */
/* previously saved with a SMB_STACK_PUSH call to this same function.		*/
/* If the operation is SMB_STACK_XCHNG, then the current message base is	*/
/* exchanged with the message base on the top of the stack (most recently	*/
/* pushed.																	*/
/* If the current message base is not open, the SMB_STACK_PUSH and			*/
/* SMB_STACK_XCHNG operations do nothing									*/
/* Returns 0 on success, non-zero if stack full.                            */
/* If operation is SMB_STACK_POP or SMB_STACK_XCHNG, it always returns 0.	*/
/****************************************************************************/
int SMBCALL smb_stack(smb_t* smb, int op)
{
	static smb_t stack[SMB_STACK_LEN];
	static int	stack_idx;
	smb_t		tmp_smb;

	if(op==SMB_STACK_PUSH) {
		if(stack_idx>=SMB_STACK_LEN) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error),"SMB stack overflow");
			return(SMB_FAILURE);
		}
		if(smb->shd_fp==NULL || smb->sdt_fp==NULL || smb->sid_fp==NULL)
			return(SMB_SUCCESS);	  /* Msg base not open, do nothing */
		memcpy(&stack[stack_idx],smb,sizeof(smb_t));
		stack_idx++;
		return(SMB_SUCCESS); 
	}
	/* pop or xchng */
	if(!stack_idx)	/* Nothing on the stack, so do nothing */
		return(SMB_SUCCESS);
	if(op==SMB_STACK_XCHNG) {
		if(smb->shd_fp==NULL)
			return(SMB_SUCCESS);
		memcpy(&tmp_smb,smb,sizeof(smb_t));
	}

	stack_idx--;
	memcpy(smb,&stack[stack_idx],sizeof(smb_t));
	if(op==SMB_STACK_XCHNG) {
		memcpy(&stack[stack_idx],&tmp_smb,sizeof(smb_t));
		stack_idx++;
	}
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Truncates header file													*/
/* Retrys for smb.retry_time number of seconds								*/
/* Return 0 on success, non-zero otherwise									*/
/****************************************************************************/
int SMBCALL smb_trunchdr(smb_t* smb)
{
	time_t	start=0;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	rewind(smb->shd_fp);
	while(1) {
		if(!chsize(fileno(smb->shd_fp),0L))
			break;
		if(get_errno()!=EACCES && get_errno()!=EAGAIN) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) changing header file size"
				,get_errno(),STRERROR(get_errno()));
			return(SMB_ERR_WRITE);
		}
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time) {	 /* Time-out */
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"timeout changing header file size (retry_time=%ld)"
					,smb->retry_time);
				return(SMB_ERR_TIMEOUT); 
			}
		SLEEP(smb->retry_delay);
	}
	return(SMB_SUCCESS);
}

/*********************************/
/* Message Base Header Functions */
/*********************************/

/****************************************************************************/
/* Attempts for smb.retry_time number of seconds to lock the msg base hdr	*/
/****************************************************************************/
int SMBCALL smb_locksmbhdr(smb_t* smb)
{
	time_t	start=0;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	while(1) {
		if(lock(fileno(smb->shd_fp),0L,sizeof(smbhdr_t)+sizeof(smbstatus_t))==0) {
			smb->locked=TRUE;
			return(SMB_SUCCESS);
		}
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time) 
				break;						
		/* In case we've already locked it */
		if(unlock(fileno(smb->shd_fp),0L,sizeof(smbhdr_t)+sizeof(smbstatus_t))==0)
			smb->locked=FALSE;
		SLEEP(smb->retry_delay);
	}
	safe_snprintf(smb->last_error,sizeof(smb->last_error),"timeout locking header");
	return(SMB_ERR_TIMEOUT);
}

/****************************************************************************/
/* Read the SMB header from the header file and place into smb.status		*/
/****************************************************************************/
int SMBCALL smb_getstatus(smb_t* smb)
{
	int 	i;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	setvbuf(smb->shd_fp,NULL,_IONBF,SHD_BLOCK_LEN);
	clearerr(smb->shd_fp);
	if(fseek(smb->shd_fp,sizeof(smbhdr_t),SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %u in header file"
			,get_errno(),STRERROR(get_errno()),sizeof(smbhdr_t));
		return(SMB_ERR_SEEK);
	}
	i=smb_fread(smb,&(smb->status),sizeof(smbstatus_t),smb->shd_fp);
	setvbuf(smb->shd_fp,NULL,_IOFBF,SHD_BLOCK_LEN);
	if(i==sizeof(smbstatus_t))
		return(SMB_SUCCESS);
	safe_snprintf(smb->last_error,sizeof(smb->last_error)
		,"%d (%s) reading status",ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
	return(SMB_ERR_READ);
}

/****************************************************************************/
/* Writes message base header												*/
/****************************************************************************/
int SMBCALL smb_putstatus(smb_t* smb)
{
	int i;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	clearerr(smb->shd_fp);
	if(fseek(smb->shd_fp,sizeof(smbhdr_t),SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %u in header file"
			,get_errno(),STRERROR(get_errno()),sizeof(smbhdr_t));
		return(SMB_ERR_SEEK);
	}
	i=fwrite(&(smb->status),1,sizeof(smbstatus_t),smb->shd_fp);
	fflush(smb->shd_fp);
	if(i==sizeof(smbstatus_t))
		return(SMB_SUCCESS);
	safe_snprintf(smb->last_error,sizeof(smb->last_error)
		,"%d (%s) writing status",ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
	return(SMB_ERR_WRITE);
}

/****************************************************************************/
/* Unlocks previously locks message base header 							*/
/****************************************************************************/
int SMBCALL smb_unlocksmbhdr(smb_t* smb)
{
	int result;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	result = unlock(fileno(smb->shd_fp),0L,sizeof(smbhdr_t)+sizeof(smbstatus_t));
	if(result==0)
		smb->locked=FALSE;
	return(result);
}

/********************************/
/* Individual Message Functions */
/********************************/

/****************************************************************************/
/* Is the offset a valid message header offset?								*/
/****************************************************************************/
BOOL SMBCALL smb_valid_hdr_offset(smb_t* smb, ulong offset)
{
	if(offset<sizeof(smbhdr_t)+sizeof(smbstatus_t) 
		|| offset<smb->status.header_offset) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"invalid header offset: %lu (0x%lX)"
			,offset,offset);
		return(FALSE);
	}
	return(TRUE);
}

/****************************************************************************/
/* Attempts for smb.retry_time number of seconds to lock the hdr for 'msg'  */
/****************************************************************************/
int SMBCALL smb_lockmsghdr(smb_t* smb, smbmsg_t* msg)
{
	time_t	start=0;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	if(!smb_valid_hdr_offset(smb,msg->idx.offset))
		return(SMB_ERR_HDR_OFFSET);

	while(1) {
		if(!lock(fileno(smb->shd_fp),msg->idx.offset,sizeof(msghdr_t)))
			return(SMB_SUCCESS);
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time) 
				break;
		/* In case we've already locked it */
		unlock(fileno(smb->shd_fp),msg->idx.offset,sizeof(msghdr_t)); 
		SLEEP(smb->retry_delay);
	}
	safe_snprintf(smb->last_error,sizeof(smb->last_error),"timeout locking header");
	return(SMB_ERR_TIMEOUT);
}

/****************************************************************************/
/* Fills msg->idx with message index based on msg->hdr.number				*/
/* OR if msg->hdr.number is 0, based on msg->offset (record offset).		*/
/* if msg.hdr.number does not equal 0, then msg->offset is filled too.		*/
/* Either msg->hdr.number or msg->offset must be initialized before 		*/
/* calling this function													*/
/* Returns 1 if message number wasn't found, 0 if it was                    */
/****************************************************************************/
int SMBCALL smb_getmsgidx(smb_t* smb, smbmsg_t* msg)
{
	idxrec_t idx;
	ulong	 l,length,total,bot,top;

	if(smb->sid_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"index not open");
		return(SMB_ERR_NOT_OPEN);
	}
	clearerr(smb->sid_fp);

	length=filelength(fileno(smb->sid_fp));
	if(length<sizeof(idxrec_t)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"invalid index file length: %ld",length);
		return(SMB_ERR_FILE_LEN);
	}
	total=length/sizeof(idxrec_t);
	if(!total) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"invalid index file length: %ld",length);
		return(SMB_ERR_FILE_LEN);
	}

	if(!msg->hdr.number) {
		if(msg->offset*sizeof(idxrec_t)>=length) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"invalid index offset: %lu, byte offset: %lu, length: %lu"
				,msg->offset, msg->offset*sizeof(idxrec_t), length);
			return(SMB_ERR_HDR_OFFSET);
		}
		if(fseek(smb->sid_fp,msg->offset*sizeof(idxrec_t),SEEK_SET)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) seeking to %lu in index file"
				,get_errno(),STRERROR(get_errno())
				,msg->offset*sizeof(idxrec_t));
			return(SMB_ERR_SEEK);
		}
		if(smb_fread(smb,&msg->idx,sizeof(idxrec_t),smb->sid_fp)!=sizeof(idxrec_t)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading index at offset %lu (byte %lu)"
				,ferror(smb->sid_fp),STRERROR(ferror(smb->sid_fp))
				,msg->offset,msg->offset*sizeof(idxrec_t));
			return(SMB_ERR_READ);
		}
		return(SMB_SUCCESS); 
	}

	bot=0;
	top=total;
	l=total/2; /* Start at middle index */
	while(1) {
		if(l>=total) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error),"msg %lu not found"
				,msg->hdr.number);
			return(SMB_ERR_NOT_FOUND);
		}
		if(fseek(smb->sid_fp,l*sizeof(idxrec_t),SEEK_SET)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) seeking to offset %lu (byte %lu) in index file"
				,get_errno(),STRERROR(get_errno())
				,l,l*sizeof(idxrec_t));
			return(SMB_ERR_SEEK);
		}
		if(smb_fread(smb,&idx,sizeof(idxrec_t),smb->sid_fp)!=sizeof(idxrec_t)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading index at offset %lu (byte %lu)"
				,ferror(smb->sid_fp),STRERROR(ferror(smb->sid_fp)),l,l*sizeof(idxrec_t));
			return(SMB_ERR_READ);
		}
		if(bot==top-1 && idx.number!=msg->hdr.number) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error),"msg %lu not found"
				,msg->hdr.number);
			return(SMB_ERR_NOT_FOUND);
		}
		if(idx.number>msg->hdr.number) {
			top=l;
			l=bot+((top-bot)/2);
			continue; 
		}
		if(idx.number<msg->hdr.number) {
			bot=l;
			l=top-((top-bot)/2);
			continue; 
		}
		break; 
	}
	msg->idx=idx;
	msg->offset=l;
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Reads the first index record in the open message base 					*/
/****************************************************************************/
int SMBCALL smb_getfirstidx(smb_t* smb, idxrec_t *idx)
{
	if(smb->sid_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"index not open");
		return(SMB_ERR_NOT_OPEN);
	}
	clearerr(smb->sid_fp);
	if(fseek(smb->sid_fp,0,SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to beginning of index file"
			,get_errno(),STRERROR(get_errno()));
		return(SMB_ERR_SEEK);
	}
	if(smb_fread(smb,idx,sizeof(idxrec_t),smb->sid_fp)!=sizeof(idxrec_t)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) reading first index"
			,ferror(smb->sid_fp),STRERROR(ferror(smb->sid_fp)));
		return(SMB_ERR_READ);
	}
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Reads the last index record in the open message base 					*/
/****************************************************************************/
int SMBCALL smb_getlastidx(smb_t* smb, idxrec_t *idx)
{
	long length;

	if(smb->sid_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"index not open");
		return(SMB_ERR_NOT_OPEN);
	}
	clearerr(smb->sid_fp);
	length=filelength(fileno(smb->sid_fp));
	if(length<(long)sizeof(idxrec_t)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"invalid index file length: %ld",length);
		return(SMB_ERR_FILE_LEN);
	}
	if(fseek(smb->sid_fp,length-sizeof(idxrec_t),SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %u in index file"
			,get_errno(),STRERROR(get_errno())
			,(unsigned)(length-sizeof(idxrec_t)));
		return(SMB_ERR_SEEK);
	}
	if(smb_fread(smb,idx,sizeof(idxrec_t),smb->sid_fp)!=sizeof(idxrec_t)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) reading last index"
			,ferror(smb->sid_fp),STRERROR(ferror(smb->sid_fp)));
		return(SMB_ERR_READ);
	}
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Figures out the total length of the header record for 'msg'              */
/* Returns length 															*/
/****************************************************************************/
ulong SMBCALL smb_getmsghdrlen(smbmsg_t* msg)
{
	int i;
	ulong length;

	/* fixed portion */
	length=sizeof(msghdr_t);
	/* data fields */
	length+=msg->hdr.total_dfields*sizeof(dfield_t);
	/* header fields */
	for(i=0;i<msg->total_hfields;i++) {
		length+=sizeof(hfield_t);
		length+=msg->hfield[i].length; 
	}
	return(length);
}

/****************************************************************************/
/* Figures out the total length of the data buffer for 'msg'                */
/* Returns length															*/
/****************************************************************************/
ulong SMBCALL smb_getmsgdatlen(smbmsg_t* msg)
{
	int i;
	ulong length=0L;

	for(i=0;i<msg->hdr.total_dfields;i++)
		length+=msg->dfield[i].length;
	return(length);
}

/****************************************************************************/
/* Figures out the total length of the text buffer for 'msg'                */
/* Returns length															*/
/****************************************************************************/
ulong SMBCALL smb_getmsgtxtlen(smbmsg_t* msg)
{
	int i;
	ulong length=0L;

	for(i=0;i<msg->total_hfields;i++)
		if(msg->hfield[i].type==SMB_COMMENT || msg->hfield[i].type==SMTPSYSMSG)
			length+=msg->hfield[i].length+2;
	for(i=0;i<msg->hdr.total_dfields;i++)
		if(msg->dfield[i].type==TEXT_BODY || msg->dfield[i].type==TEXT_TAIL)
			length+=msg->dfield[i].length;
	return(length);
}

static void set_convenience_ptr(smbmsg_t* msg, ushort hfield_type, void* hfield_dat)
{
	switch(hfield_type) {	/* convenience variables */
		case SENDER:
			if(!msg->from) {
				msg->from=(char*)hfield_dat;
				break; 
			}
		case FORWARDED: 	/* fall through */
			msg->forwarded=TRUE;
			break;
		case SENDERAGENT:
			if(!msg->forwarded)
				msg->from_agent=*(ushort *)hfield_dat;
			break;
		case SENDEREXT:
			if(!msg->forwarded)
				msg->from_ext=(char*)hfield_dat;
			break;
		case SENDERORG:
			if(!msg->forwarded)
				msg->from_org=(char*)hfield_dat;
			break;
		case SENDERNETTYPE:
			if(!msg->forwarded)
				msg->from_net.type=*(ushort *)hfield_dat;
			break;
		case SENDERNETADDR:
			if(!msg->forwarded)
				msg->from_net.addr=(char*)hfield_dat;
			break;
		case REPLYTO:
			msg->replyto=(char*)hfield_dat;
			break;
		case REPLYTOEXT:
			msg->replyto_ext=(char*)hfield_dat;
			break;
		case REPLYTOAGENT:
			msg->replyto_agent=*(ushort *)hfield_dat;
			break;
		case REPLYTONETTYPE:
			msg->replyto_net.type=*(ushort *)hfield_dat;
			break;
		case REPLYTONETADDR:
			msg->replyto_net.addr=(char*)hfield_dat;
			break;
		case RECIPIENT:
			msg->to=(char*)hfield_dat;
			break;
		case RECIPIENTEXT:
			msg->to_ext=(char*)hfield_dat;
			break;
		case RECIPIENTAGENT:
			msg->to_agent=*(ushort *)hfield_dat;
			break;
		case RECIPIENTNETTYPE:
			msg->to_net.type=*(ushort *)hfield_dat;
			break;
		case RECIPIENTNETADDR:
			msg->to_net.addr=(char*)hfield_dat;
			break;
		case SUBJECT:
			msg->subj=(char*)hfield_dat;
			break;
		case SMB_SUMMARY:
			msg->summary=(char*)hfield_dat;
			break;
		case SMB_EXPIRATION:
			msg->expiration=*(time_t*)hfield_dat;
			break;
		case SMB_PRIORITY:
			msg->priority=*(ulong*)hfield_dat;
			break;
		case SMB_COST:
			msg->cost=*(ulong*)hfield_dat;
			break;
		case RFC822MSGID:
			msg->id=(char*)hfield_dat;
			break;
		case RFC822REPLYID:
			msg->reply_id=(char*)hfield_dat;
			break;
		case SMTPREVERSEPATH:
			msg->reverse_path=(char*)hfield_dat;
			break;
		case USENETPATH:
			msg->path=(char*)hfield_dat;
			break;
		case USENETNEWSGROUPS:
			msg->newsgroups=(char*)hfield_dat;
			break;
		case FIDOMSGID:
			msg->ftn_msgid=(char*)hfield_dat;
			break;
		case FIDOREPLYID:
			msg->ftn_reply=(char*)hfield_dat;
			break;
		case FIDOAREA:
			msg->ftn_area=(char*)hfield_dat;
			break;
		case FIDOPID:
			msg->ftn_pid=(char*)hfield_dat;
			break;
		case FIDOTID:
			msg->ftn_tid=(char*)hfield_dat;
			break;
		case FIDOFLAGS:
			msg->ftn_flags=(char*)hfield_dat;
			break;
	}
}

static void clear_convenience_ptrs(smbmsg_t* msg)
{
	msg->from=NULL;
	msg->from_ext=NULL;
	msg->from_org=NULL;
	memset(&msg->from_net,0,sizeof(net_t));

	msg->replyto=NULL;
	msg->replyto_ext=NULL;
	memset(&msg->replyto_net,0,sizeof(net_t));

	msg->to=NULL;
	msg->to_ext=NULL;
	memset(&msg->to_net,0,sizeof(net_t));

	msg->subj=NULL;
	msg->summary=NULL;
	msg->id=NULL;
	msg->reply_id=NULL;
	msg->reverse_path=NULL;
	msg->path=NULL;
	msg->newsgroups=NULL;

	msg->ftn_msgid=NULL;
	msg->ftn_reply=NULL;
	msg->ftn_area=NULL;
	msg->ftn_pid=NULL;
	msg->ftn_tid=NULL;
	msg->ftn_flags=NULL;
}

/****************************************************************************/
/* Read header information into 'msg' structure                             */
/* msg->idx.offset must be set before calling this function 				*/
/* Must call smb_freemsgmem() to free memory allocated for var len strs 	*/
/* Returns 0 on success, non-zero if error									*/
/****************************************************************************/
int SMBCALL smb_getmsghdr(smb_t* smb, smbmsg_t* msg)
{
	void	*vp,**vpp;
	ushort	i;
	ulong	l,offset;
	idxrec_t idx;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}

	if(!smb_valid_hdr_offset(smb,msg->idx.offset))
		return(SMB_ERR_HDR_OFFSET);

	rewind(smb->shd_fp);
	if(fseek(smb->shd_fp,msg->idx.offset,SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %lu in header"
			,get_errno(),STRERROR(get_errno())
			,msg->idx.offset);
		return(SMB_ERR_SEEK);
	}

	idx=msg->idx;
	offset=msg->offset;
	memset(msg,0,sizeof(smbmsg_t));
	msg->idx=idx;
	msg->offset=offset;
	if(smb_fread(smb,&msg->hdr,sizeof(msghdr_t),smb->shd_fp)!=sizeof(msghdr_t)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) reading msg header"
			,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
		return(SMB_ERR_READ);
	}
	if(memcmp(msg->hdr.id,SHD_HEADER_ID,LEN_HEADER_ID)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"corrupt message header ID: %.*s at offset %lu"
			,LEN_HEADER_ID,msg->hdr.id,msg->idx.offset);
		return(SMB_ERR_HDR_ID);
	}
	if(msg->hdr.version<0x110) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"insufficient header version: %X"
			,msg->hdr.version);
		return(SMB_ERR_HDR_VER);
	}
	l=sizeof(msghdr_t);
	if(msg->hdr.total_dfields && (msg->dfield
		=(dfield_t *)MALLOC(sizeof(dfield_t)*msg->hdr.total_dfields))==NULL) {
		smb_freemsgmem(msg);
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"malloc failure of %d bytes for %d data fields"
			,(int)sizeof(dfield_t)*msg->hdr.total_dfields, msg->hdr.total_dfields);
		return(SMB_ERR_MEM); 
	}
	i=0;
	while(i<msg->hdr.total_dfields && l<(ulong)msg->hdr.length) {
		if(smb_fread(smb,&msg->dfield[i],sizeof(dfield_t),smb->shd_fp)!=sizeof(dfield_t)) {
			smb_freemsgmem(msg);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading data field %d"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)),i);
			return(SMB_ERR_READ); 
		}
		i++;
		l+=sizeof(dfield_t); 
	}
	if(i<msg->hdr.total_dfields) {
		smb_freemsgmem(msg);
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"insufficient data fields read (%d instead of %d)"
			,i,msg->hdr.total_dfields);
		return(SMB_ERR_READ); 
	}
	while(l<(ulong)msg->hdr.length) {
		i=msg->total_hfields;
		if((vpp=(void* *)REALLOC(msg->hfield_dat,sizeof(void* )*(i+1)))==NULL) {
			smb_freemsgmem(msg);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"realloc failure of %d bytes for header field data"
				,(int)sizeof(void*)*(i+1));
			return(SMB_ERR_MEM); 
		}
		msg->hfield_dat=vpp;
		if((vp=(hfield_t *)REALLOC(msg->hfield,sizeof(hfield_t)*(i+1)))==NULL) {
			smb_freemsgmem(msg);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"realloc failure of %d bytes for header fields"
				,(int)sizeof(hfield_t)*(i+1));
			return(SMB_ERR_MEM); 
		}
		msg->hfield=vp;
		msg->total_hfields++;
		if(smb_fread(smb,&msg->hfield[i],sizeof(hfield_t),smb->shd_fp)!=sizeof(hfield_t)) {
			smb_freemsgmem(msg);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading header field"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_READ); 
		}
		l+=sizeof(hfield_t);
		if((msg->hfield_dat[i]=(char*)MALLOC(msg->hfield[i].length+1))
			==NULL) {			/* Allocate 1 extra for ASCIIZ terminator */
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"malloc failure of %d bytes for header field %d"
				,msg->hfield[i].length+1, i);
			smb_freemsgmem(msg);  /* or 0 length field */
			return(SMB_ERR_MEM); 
		}
		memset(msg->hfield_dat[i],0,msg->hfield[i].length+1);  /* init to NULL */
		if(msg->hfield[i].length
			&& smb_fread(smb,msg->hfield_dat[i],msg->hfield[i].length,smb->shd_fp)
				!=(size_t)msg->hfield[i].length) {
			smb_freemsgmem(msg);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading header field data"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_READ); 
		}
		set_convenience_ptr(msg,msg->hfield[i].type,msg->hfield_dat[i]);

		l+=msg->hfield[i].length; 
	}

	/* These convenience pointers must point to something */
	if(msg->from==NULL)	msg->from=nulstr;
	if(msg->to==NULL)	msg->to=nulstr;
	if(msg->subj==NULL)	msg->subj=nulstr;

	/* If no reverse path specified, use sender's address */
	if(msg->reverse_path == NULL)
		msg->reverse_path = msg->from_net.addr;

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Frees memory allocated for variable-length header fields in 'msg'        */
/****************************************************************************/
void SMBCALL smb_freemsghdrmem(smbmsg_t* msg)
{
	ushort	i;

	for(i=0;i<msg->total_hfields;i++)
		if(msg->hfield_dat[i]) {
			FREE(msg->hfield_dat[i]);
			msg->hfield_dat[i]=NULL;
		}
	msg->total_hfields=0;
	if(msg->hfield) {
		FREE(msg->hfield);
		msg->hfield=NULL;
	}
	if(msg->hfield_dat) {
		FREE(msg->hfield_dat);
		msg->hfield_dat=NULL;
	}
	clear_convenience_ptrs(msg);	/* don't leave pointers to freed memory */
}

/****************************************************************************/
/* Frees memory allocated for 'msg'                                         */
/****************************************************************************/
void SMBCALL smb_freemsgmem(smbmsg_t* msg)
{
	if(msg->dfield) {
		FREE(msg->dfield);
		msg->dfield=NULL;
	}
	msg->hdr.total_dfields=0;
	smb_freemsghdrmem(msg);
}

/****************************************************************************/
/* Copies memory allocated for 'srcmsg' to 'msg'							*/
/****************************************************************************/
int SMBCALL smb_copymsgmem(smb_t* smb, smbmsg_t* msg, smbmsg_t* srcmsg)
{
	int i;

	memcpy(msg,srcmsg,sizeof(smbmsg_t));

	/* data field types/lengths */
	if(msg->hdr.total_dfields>0) {
		if((msg->dfield=(dfield_t *)MALLOC(msg->hdr.total_dfields*sizeof(dfield_t)))==NULL) {
			if(smb!=NULL)
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"malloc failure of %d bytes for %d data fields"
					,msg->hdr.total_dfields*sizeof(dfield_t), msg->hdr.total_dfields);
			return(SMB_ERR_MEM);
		}
		memcpy(msg->dfield,srcmsg->dfield,msg->hdr.total_dfields*sizeof(dfield_t));
	}

	/* header field types/lengths */
	if(msg->total_hfields>0) {
		if((msg->hfield=(hfield_t *)MALLOC(msg->total_hfields*sizeof(hfield_t)))==NULL) {
			if(smb!=NULL)
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"malloc failure of %d bytes for %d header fields"
					,msg->total_hfields*sizeof(hfield_t), msg->total_hfields);
			return(SMB_ERR_MEM);
		}
		memcpy(msg->hfield,srcmsg->hfield,msg->total_hfields*sizeof(hfield_t));

		/* header field data */
		if((msg->hfield_dat=(void**)MALLOC(msg->total_hfields*sizeof(void*)))==NULL) {
			if(smb!=NULL)
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"malloc failure of %d bytes for %d header fields"
					,msg->total_hfields*sizeof(void*), msg->total_hfields);
			return(SMB_ERR_MEM);
		}

		for(i=0;i<msg->total_hfields;i++) {
			if((msg->hfield_dat[i]=(void*)MALLOC(msg->hfield[i].length+1))==NULL) {
				if(smb!=NULL)
					safe_snprintf(smb->last_error,sizeof(smb->last_error)
						,"malloc failure of %d bytes for header field #%d"
						,msg->hfield[i].length+1, i+1);
				return(SMB_ERR_MEM);
			}
			memset(msg->hfield_dat[i],0,msg->hfield[i].length+1);
			memcpy(msg->hfield_dat[i],srcmsg->hfield_dat[i],msg->hfield[i].length);
		}
	}

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Unlocks header for 'msg'                                                 */
/****************************************************************************/
int SMBCALL smb_unlockmsghdr(smb_t* smb, smbmsg_t* msg)
{
	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	if(!smb_valid_hdr_offset(smb,msg->idx.offset))
		return(SMB_ERR_HDR_OFFSET);
	return(unlock(fileno(smb->shd_fp),msg->idx.offset,sizeof(msghdr_t)));
}


/****************************************************************************/
/* Adds a header field to the 'msg' structure (in memory only)              */
/****************************************************************************/
int SMBCALL smb_hfield(smbmsg_t* msg, ushort type, size_t length, void* data)
{
	void**		vpp;
	hfield_t*	hp;
	int i;

	if(smb_getmsghdrlen(msg)+sizeof(hfield_t)+length>SMB_MAX_HDR_LEN)
		return(SMB_ERR_HDR_LEN);

	i=msg->total_hfields;
	if((hp=(hfield_t *)REALLOC(msg->hfield,sizeof(hfield_t)*(i+1)))==NULL) 
		return(SMB_ERR_MEM);

	msg->hfield=hp;
	if((vpp=(void* *)REALLOC(msg->hfield_dat,sizeof(void* )*(i+1)))==NULL) 
		return(SMB_ERR_MEM);
	
	msg->hfield_dat=vpp;
	msg->total_hfields++;
	msg->hfield[i].type=type;
	msg->hfield[i].length=length;
	if(length) {
		if((msg->hfield_dat[i]=(void* )MALLOC(length+1))==NULL) 
			return(SMB_ERR_MEM);	/* Allocate 1 extra for ASCIIZ terminator */
		memset(msg->hfield_dat[i],0,length+1);
		memcpy(msg->hfield_dat[i],data,length); 
		set_convenience_ptr(msg,type,msg->hfield_dat[i]);
	}
	else
		msg->hfield_dat[i]=NULL;

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Adds a list of header fields to the 'msg' structure (in memory only)     */
/****************************************************************************/
int	SMBCALL smb_hfield_addlist(smbmsg_t* msg, hfield_t** hfield_list, void** hfield_dat)
{
	int			retval;
	unsigned	n;

	if(hfield_list==NULL)
		return(SMB_FAILURE);

	for(n=0;hfield_list[n]!=NULL;n++)
		if((retval=smb_hfield(msg
			,hfield_list[n]->type,hfield_list[n]->length,hfield_dat[n]))!=SMB_SUCCESS)
			return(retval);

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Convenience function to add an ASCIIZ string header field				*/
/****************************************************************************/
int SMBCALL smb_hfield_str(smbmsg_t* msg, ushort type, const char* str)
{
	return smb_hfield(msg, type, strlen(str), (void*)str);
}

/****************************************************************************/
/* Appends data to an existing header field (in memory only)				*/
/****************************************************************************/
int SMBCALL smb_hfield_append(smbmsg_t* msg, ushort type, size_t length, void* data)
{
	int		i;
	BYTE*	p;

	if(length==0)	/* nothing to append */
		return(SMB_SUCCESS);

	if(msg->total_hfields<1)
		return(SMB_ERR_NOT_FOUND);

	/* search for the last header field of this type */
	for(i=msg->total_hfields-1;i>=0;i--)
		if(msg->hfield[i].type == type)
			break;
	if(i<0)
		return(SMB_ERR_NOT_FOUND);

	if(smb_getmsghdrlen(msg)+length>SMB_MAX_HDR_LEN)
		return(SMB_ERR_HDR_LEN);

	if((p=(BYTE*)REALLOC(msg->hfield_dat[i],msg->hfield[i].length+length+1))==NULL) 
		return(SMB_ERR_MEM);	/* Allocate 1 extra for ASCIIZ terminator */

	msg->hfield_dat[i]=p;
	p+=msg->hfield[i].length;	/* skip existing data */
	memset(p,0,length+1);
	memcpy(p,data,length);		/* append */
	msg->hfield[i].length+=length;
	set_convenience_ptr(msg,type,msg->hfield_dat[i]);

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Appends data to an existing ASCIIZ header field (in memory only)			*/
/****************************************************************************/
int SMBCALL smb_hfield_append_str(smbmsg_t* msg, ushort type, const char* str)
{
	return smb_hfield_append(msg, type, strlen(str), (void*)str);
}

/****************************************************************************/
/* Searches for a specific header field (by type) and returns it			*/
/****************************************************************************/
void* SMBCALL smb_get_hfield(smbmsg_t* msg, ushort type, hfield_t* hfield)
{
	int i;

	for(i=0;i<msg->total_hfields;i++)
		if(msg->hfield[i].type == type) {
			if(hfield != NULL)
				*hfield = msg->hfield[i];
			return(msg->hfield_dat[i]);
		}

	return(NULL);
}

/****************************************************************************/
/* Adds a data field to the 'msg' structure (in memory only)                */
/* Automatically figures out the offset into the data buffer from existing	*/
/* dfield lengths															*/
/****************************************************************************/
int SMBCALL smb_dfield(smbmsg_t* msg, ushort type, ulong length)
{
	dfield_t* dp;
	int i,j;

	i=msg->hdr.total_dfields;
	if((dp=(dfield_t *)REALLOC(msg->dfield,sizeof(dfield_t)*(i+1)))==NULL) 
		return(SMB_ERR_MEM);
	
	msg->dfield=dp;
	msg->hdr.total_dfields++;
	msg->dfield[i].type=type;
	msg->dfield[i].length=length;
	for(j=msg->dfield[i].offset=0;j<i;j++)
		msg->dfield[i].offset+=msg->dfield[j].length;
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Checks CRC history file for duplicate crc. If found, returns 1.			*/
/* If no dupe, adds to CRC history and returns 0, or negative if error. 	*/
/****************************************************************************/
int SMBCALL smb_addcrc(smb_t* smb, ulong crc)
{
	char	str[MAX_PATH+1];
	int 	file;
	int		wr;
	long	length;
	long	newlen;
	ulong	l,*buf;
	time_t	start=0;

	if(!smb->status.max_crcs)
		return(SMB_SUCCESS);

	SAFEPRINTF(str,"%s.sch",smb->file);
	while(1) {
		if((file=sopen(str,O_RDWR|O_CREAT|O_BINARY,SH_DENYRW,S_IREAD|S_IWRITE))!=-1)
			break;
		if(get_errno()!=EACCES && get_errno()!=EAGAIN) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) opening %s"
				,get_errno(),STRERROR(get_errno()),str);
			return(SMB_ERR_OPEN);
		}
		if(!start)
			start=time(NULL);
		else
			if(time(NULL)-start>=(time_t)smb->retry_time) {
				safe_snprintf(smb->last_error,sizeof(smb->last_error)
					,"timeout opening %s (retry_time=%ld)"
					,str,smb->retry_time);
				return(SMB_ERR_TIMEOUT); 
			}
		SLEEP(smb->retry_delay);
	}

	length=filelength(file);
	if(length<0L || length%sizeof(long)) {
		close(file);
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"invalid file length: %ld", length);
		return(SMB_ERR_FILE_LEN); 
	}

	if(length!=0) {
		if((buf=(ulong*)MALLOC(length))==NULL) {
			close(file);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"malloc failure of %ld bytes"
				,length);
			return(SMB_ERR_MEM); 
		}

		if(read(file,buf,length)!=length) {
			close(file);
			FREE(buf);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) reading %ld bytes"
				,get_errno(),STRERROR(get_errno()),length);
			return(SMB_ERR_READ);
		}

		for(l=0;l<length/sizeof(long);l++)
			if(crc==buf[l])
				break;
		if(l<length/sizeof(long)) {					/* Dupe CRC found */
			close(file);
			FREE(buf);
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"duplicate message text CRC detected");
			return(SMB_DUPE_MSG);
		} 

		if(length>=(long)(smb->status.max_crcs*sizeof(long))) {
			newlen=(smb->status.max_crcs-1)*sizeof(long);
			chsize(file,0);	/* truncate it */
			lseek(file,0L,SEEK_SET);
			write(file,buf+(length-newlen),newlen); 
		}
		FREE(buf);
	}
	wr=write(file,&crc,sizeof(crc));	/* Write to the end */
	close(file);

	if(wr!=sizeof(crc)) {	
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) writing %u bytes"
			,get_errno(),STRERROR(get_errno()),sizeof(crc));
		return(SMB_ERR_WRITE);
	}

	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Creates a new message header record in the header file.					*/
/* If storage is SMB_SELFPACK, self-packing conservative allocation is used */
/* If storage is SMB_FASTALLOC, fast allocation is used 					*/
/* If storage is SMB_HYPERALLOC, no allocation tables are used (fastest)	*/
/* This function will UN-lock the SMB header								*/
/****************************************************************************/
int SMBCALL smb_addmsghdr(smb_t* smb, smbmsg_t* msg, int storage)
{
	int		i;
	long	l;
	ulong	hdrlen;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}

	if(!smb->locked && smb_locksmbhdr(smb)!=SMB_SUCCESS)
		return(SMB_ERR_LOCK);

	hdrlen=smb_getmsghdrlen(msg);
	if(hdrlen>SMB_MAX_HDR_LEN) { /* headers are limited to 64k in size */
		smb_unlocksmbhdr(smb);
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"illegal message header length (%lu > %u)"
			,hdrlen,SMB_MAX_HDR_LEN);
		return(SMB_ERR_HDR_LEN);
	}

	if((i=smb_getstatus(smb))!=SMB_SUCCESS) {
		smb_unlocksmbhdr(smb);
		return(i);
	}
	msg->idx.number=msg->hdr.number=smb->status.last_msg+1;

	if(!(msg->flags&MSG_FLAG_HASHED) /* not already hashed */
		&& (i=smb_hashmsg(smb,msg,NULL,/* update? */FALSE))!=SMB_SUCCESS) {
		smb_unlocksmbhdr(smb);
		return(i);	/* Duplicate message? */
	}

	if(storage!=SMB_HYPERALLOC && (i=smb_open_ha(smb))!=SMB_SUCCESS) {
		smb_unlocksmbhdr(smb);
		return(i);
	}

	msg->hdr.length=(ushort)hdrlen;
	if(storage==SMB_HYPERALLOC)
		l=smb_hallochdr(smb);
	else if(storage==SMB_FASTALLOC)
		l=smb_fallochdr(smb,msg->hdr.length);
	else
		l=smb_allochdr(smb,msg->hdr.length);
	if(storage!=SMB_HYPERALLOC)
		smb_close_ha(smb);
	if(l<0) {
		smb_unlocksmbhdr(smb);
		return(l); 
	}

	msg->idx.offset=smb->status.header_offset+l;
	msg->idx.time=msg->hdr.when_imported.time;
	msg->idx.attr=msg->hdr.attr;
	msg->offset=smb->status.total_msgs;
	i=smb_putmsg(smb,msg);
	if(i==SMB_SUCCESS) {
		smb->status.last_msg++;
		smb->status.total_msgs++;
		smb_putstatus(smb);
	}
	smb_unlocksmbhdr(smb);
	return(i);
}


/****************************************************************************/
/* Writes both header and index information for msg 'msg'                   */
/****************************************************************************/
int SMBCALL smb_putmsg(smb_t* smb, smbmsg_t* msg)
{
	int i;

	i=smb_putmsghdr(smb,msg);
	if(i)
		return(i);
	return(smb_putmsgidx(smb,msg));
}

/****************************************************************************/
/* Writes index information for 'msg'                                       */
/* msg->idx 																*/
/* and msg->offset must be set prior to calling to this function			*/
/* Returns 0 if everything ok                                               */
/****************************************************************************/
int SMBCALL smb_putmsgidx(smb_t* smb, smbmsg_t* msg)
{
	if(smb->sid_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"index not open");
		return(SMB_ERR_NOT_OPEN);
	}
	clearerr(smb->sid_fp);
	if(fseek(smb->sid_fp,msg->offset*sizeof(idxrec_t),SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %u in header"
			,get_errno(),STRERROR(get_errno())
			,(unsigned)(msg->offset*sizeof(idxrec_t)));
		return(SMB_ERR_SEEK);
	}
	if(!fwrite(&msg->idx,sizeof(idxrec_t),1,smb->sid_fp)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) writing index"
			,ferror(smb->sid_fp),STRERROR(ferror(smb->sid_fp)));
		return(SMB_ERR_WRITE);
	}
	fflush(smb->sid_fp);
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Writes header information for 'msg'                                      */
/* msg->hdr.length															*/
/* msg->idx.offset															*/
/* and msg->offset must be set prior to calling to this function			*/
/* Returns 0 if everything ok                                               */
/****************************************************************************/
int SMBCALL smb_putmsghdr(smb_t* smb, smbmsg_t* msg)
{
	ushort	i;
	ulong	hdrlen;

	if(smb->shd_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}

	if(!smb_valid_hdr_offset(smb,msg->idx.offset))
		return(SMB_ERR_HDR_OFFSET);

	clearerr(smb->shd_fp);
	if(fseek(smb->shd_fp,msg->idx.offset,SEEK_SET)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) seeking to %lu in index"
			,get_errno(),STRERROR(get_errno()),msg->idx.offset);
		return(SMB_ERR_SEEK);
	}

	/* Verify that the number of blocks required to stored the actual 
	   (calculated) header length does not exceed the number allocated. */
	hdrlen=smb_getmsghdrlen(msg);
	if(hdrlen>SMB_MAX_HDR_LEN) { /* headers are limited to 64k in size */
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"illegal message header length (%lu > %u)"
			,hdrlen,SMB_MAX_HDR_LEN);
		return(SMB_ERR_HDR_LEN);
	}
	if(smb_hdrblocks(hdrlen) > smb_hdrblocks(msg->hdr.length)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"illegal header length increase: %lu (%lu blocks) vs %hu (%lu blocks)"
			,hdrlen, smb_hdrblocks(hdrlen)
			,msg->hdr.length, smb_hdrblocks(msg->hdr.length));
		return(SMB_ERR_HDR_LEN);
	}
	msg->hdr.length=(ushort)hdrlen; /* store the actual header length */

	/**********************************/
	/* Set the message header ID here */
	/**********************************/
	memcpy(&msg->hdr.id,SHD_HEADER_ID,LEN_HEADER_ID);

	/************************************************/
	/* Write the fixed portion of the header record */
	/************************************************/
	if(!fwrite(&msg->hdr,sizeof(msghdr_t),1,smb->shd_fp)) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error)
			,"%d (%s) writing fixed portion of header record"
			,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
		return(SMB_ERR_WRITE);
	}

	/************************************************/
	/* Write the data fields (each is fixed length) */
	/************************************************/
	for(i=0;i<msg->hdr.total_dfields;i++)
		if(!fwrite(&msg->dfield[i],sizeof(dfield_t),1,smb->shd_fp)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) writing data field"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_WRITE);
		}

	/*******************************************/
	/* Write the variable length header fields */
	/*******************************************/
	for(i=0;i<msg->total_hfields;i++) {
		if(!fwrite(&msg->hfield[i],sizeof(hfield_t),1,smb->shd_fp)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) writing header field"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_WRITE);
		}
		if(msg->hfield[i].length					 /* more then 0 bytes long */
			&& !fwrite(msg->hfield_dat[i],msg->hfield[i].length,1,smb->shd_fp)) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) writing header field data"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_WRITE); 
		}
	}

	while(hdrlen%SHD_BLOCK_LEN) {
		if(fputc(0,smb->shd_fp)!=0) {
			safe_snprintf(smb->last_error,sizeof(smb->last_error)
				,"%d (%s) padding header block"
				,ferror(smb->shd_fp),STRERROR(ferror(smb->shd_fp)));
			return(SMB_ERR_WRITE); 			   /* pad block with NULL */
		}
		hdrlen++; 
	}
	fflush(smb->shd_fp);
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Creates a sub-board's initial header file                                */
/* Truncates and deletes other associated SMB files 						*/
/****************************************************************************/
int SMBCALL smb_create(smb_t* smb)
{
    char        str[MAX_PATH+1];
	smbhdr_t	hdr;

	if(smb->shd_fp==NULL || smb->sdt_fp==NULL || smb->sid_fp==NULL) {
		safe_snprintf(smb->last_error,sizeof(smb->last_error),"msgbase not open");
		return(SMB_ERR_NOT_OPEN);
	}
	if(filelength(fileno(smb->shd_fp))>=(long)(sizeof(smbhdr_t)+sizeof(smbstatus_t))
		&& smb_locksmbhdr(smb)!=SMB_SUCCESS)  /* header exists, so lock it */
		return(SMB_ERR_LOCK);
	memset(&hdr,0,sizeof(smbhdr_t));
	memcpy(hdr.id,SMB_HEADER_ID,LEN_HEADER_ID);     
	hdr.version=SMB_VERSION;
	hdr.length=sizeof(smbhdr_t)+sizeof(smbstatus_t);
	smb->status.last_msg=smb->status.total_msgs=0;
	smb->status.header_offset=sizeof(smbhdr_t)+sizeof(smbstatus_t);
	rewind(smb->shd_fp);
	fwrite(&hdr,1,sizeof(smbhdr_t),smb->shd_fp);
	fwrite(&(smb->status),1,sizeof(smbstatus_t),smb->shd_fp);
	rewind(smb->shd_fp);
	chsize(fileno(smb->shd_fp),sizeof(smbhdr_t)+sizeof(smbstatus_t));
	fflush(smb->shd_fp);

	rewind(smb->sdt_fp);
	chsize(fileno(smb->sdt_fp),0L);
	rewind(smb->sid_fp);
	chsize(fileno(smb->sid_fp),0L);

	SAFEPRINTF(str,"%s.sda",smb->file);
	remove(str);						/* if it exists, delete it */
	SAFEPRINTF(str,"%s.sha",smb->file);
	remove(str);                        /* if it exists, delete it */
	SAFEPRINTF(str,"%s.sch",smb->file);
	remove(str);
	SAFEPRINTF(str,"%s.hash",smb->file);
	remove(str);
	smb_unlocksmbhdr(smb);
	return(SMB_SUCCESS);
}

/****************************************************************************/
/* Returns number of data blocks required to store "length" amount of data  */
/****************************************************************************/
ulong SMBCALL smb_datblocks(ulong length)
{
	ulong blocks;

	blocks=length/SDT_BLOCK_LEN;
	if(length%SDT_BLOCK_LEN)
		blocks++;
	return(blocks);
}

/****************************************************************************/
/* Returns number of header blocks required to store "length" size header   */
/****************************************************************************/
ulong SMBCALL smb_hdrblocks(ulong length)
{
	ulong blocks;

	blocks=length/SHD_BLOCK_LEN;
	if(length%SHD_BLOCK_LEN)
		blocks++;
	return(blocks);
}

/****************************************************************************/
/* Returns difference from specified timezone and UTC/GMT					*/
/****************************************************************************/
int SMBCALL smb_tzutc(short zone)
{
	int tz;

	if(OTHER_ZONE(zone))
		return(zone);

	tz=zone&0xfff;
	if(zone&(WESTERN_ZONE|US_ZONE))	/* West of UTC? */
		return(-tz);
	return(tz);
}

/****************************************************************************/
/****************************************************************************/
int SMBCALL smb_updatethread(smb_t* smb, smbmsg_t* remsg, ulong newmsgnum)
{
	int			retval=SMB_ERR_NOT_FOUND;
	ulong		nextmsgnum;
	smbmsg_t	nextmsg;

	if(!remsg->hdr.thread_first) {	/* New msg is first reply */
		remsg->hdr.thread_first=newmsgnum;
		if((retval=smb_lockmsghdr(smb,remsg))!=SMB_SUCCESS)
			return(retval);
		retval=smb_putmsghdr(smb,remsg);
		smb_unlockmsghdr(smb,remsg);
		return(retval);
	}
	
	/* Search for last reply and extend chain */
	memset(&nextmsg,0,sizeof(nextmsg));
	nextmsgnum=remsg->hdr.thread_first;	/* start with first reply */
	while(1) {
		nextmsg.idx.offset=0;
		nextmsg.hdr.number=nextmsgnum;
		if(smb_getmsgidx(smb, &nextmsg)!=SMB_SUCCESS) /* invalid thread origin */
			break;
		if(smb_lockmsghdr(smb,&nextmsg)!=SMB_SUCCESS)
			break;
		if(smb_getmsghdr(smb, &nextmsg)!=SMB_SUCCESS) {
			smb_unlockmsghdr(smb,&nextmsg); 
			break;
		}
		if(nextmsg.hdr.thread_next && nextmsg.hdr.thread_next!=nextmsgnum) {
			nextmsgnum=nextmsg.hdr.thread_next;
			smb_unlockmsghdr(smb,&nextmsg);
			smb_freemsgmem(&nextmsg);
			continue; 
		}
		nextmsg.hdr.thread_next=newmsgnum;
		retval=smb_putmsghdr(smb,&nextmsg);
		smb_unlockmsghdr(smb,&nextmsg);
		smb_freemsgmem(&nextmsg);
		break; 
	}

	return(retval);
}

/* End of SMBLIB.C */
