/* sexyz.c */

/* Synchronet External X/Y/ZMODEM Transfer Protocols */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2005 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/* 
 * ZMODEM code based on zmtx/zmrx v1.02 (C) Mattheij Computer Service 1994
 * by Jacques Mattheij <jacquesm@hacktic.nl> 
 */

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

/* xpdev */
#include "conwrap.h"
#include "genwrap.h"
#include "semwrap.h"
#include "dirwrap.h"
#include "filewrap.h"
#include "sockwrap.h"
#include "str_list.h"
#include "ini_file.h"
#include "eventwrap.h"
#include "threadwrap.h"

/* sbbs */
#include "ringbuf.h"
#include "telnet.h"

/* sexyz */
#include "sexyz.h"

#define SINGLE_THREADED		FALSE
#define IO_THREAD_BUF_SIZE	4096

/***************/
/* Global Vars */
/***************/
long	mode=0;							/* Program mode 					*/
long	zmode=0L;						/* Zmodem mode						*/
uchar	block[1024];					/* Block buffer 					*/
ulong	block_num;						/* Block number 					*/
time_t	startall;
char*	dszlog;

xmodem_t xm;
zmodem_t zm;

FILE*	errfp;
FILE*	statfp;
FILE*	logfp=NULL;

char	revision[16];

SOCKET	sock=INVALID_SOCKET;

BOOL	telnet=TRUE;
BOOL	terminate=FALSE;
BOOL	debug_tx=FALSE;
BOOL	debug_rx=FALSE;
BOOL	debug_telnet=FALSE;
BOOL	pause_on_exit=FALSE;

RingBuf		inbuf;
RingBuf		outbuf;
#if defined(RINGBUF_EVENT)
	#define		outbuf_empty outbuf.empty_event
#else
	xpevent_t	outbuf_empty;
#endif
unsigned	outbuf_drain_timeout;

unsigned	flows=0;
unsigned	select_errors=0;

#define getcom(t)	recv_byte(NULL,t)
#define putcom(ch)	send_byte(NULL,ch,10)

#ifdef _WINSOCKAPI_

WSADATA WSAData;
static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
		fprintf(statfp,"%s %s\n",WSAData.szDescription, WSAData.szSystemStatus);
		WSAInitialized=TRUE;
		return(TRUE);
	}

    fprintf(errfp,"!WinSock startup ERROR %d\n", status);
	return(FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)	

#endif


void newline(void)
{
	fprintf(statfp,"\n");
}

int lputs(void* unused, int level, const char* str)
{
	FILE*	fp=statfp;

    if(level<LOG_NOTICE)
		fp=errfp;

    return fprintf(fp,"%s\n",str);
}

int lprintf(int level, const char *fmt, ...)
{
	va_list argptr;
	int		retval;
	FILE*	fp=statfp;

    if(level<LOG_NOTICE)
		fp=errfp;

    va_start(argptr,fmt);
    retval = vfprintf(fp,fmt,argptr);
    va_end(argptr);
    return(retval);
}

char *chr(uchar ch)
{
	static char str[25];

	if(mode&ZMODEM) {
		switch(ch) {
			case ZPAD:		return("ZPAD");
			case ZDLE:		return("ZDLE");
			case ZDLEE:		return("ZDLEE");
			case ZBIN:		return("ZBIN");
			case ZHEX:		return("ZHEX");
			case ZBIN32:	return("ZBIN32");
			case ZBINR32:	return("ZBINR32");
			case ZVBIN:		return("ZVBIN");
			case ZVHEX:		return("ZVHEX");
			case ZVBIN32:	return("ZVBIN32");
			case ZVBINR32:	return("ZVBINR32");
			case ZRESC:		return("ZRESC");
		}
	} else {
		switch(ch) {
			case SOH:	return("SOH");
			case STX:	return("STX");
			case ETX:	return("ETX");
			case EOT:	return("EOT");
			case ACK:	return("ACK");
			case NAK:	return("NAK");
			case CAN:	return("CAN");
		}
	}
	if(ch>=' ' && ch<='~')
		sprintf(str,"'%c' (%02Xh)",ch,ch);
	else
		sprintf(str,"%u (%02Xh)",ch,ch);
	return(str); 
}

void send_telnet_cmd(SOCKET sock, uchar cmd, uchar opt)
{
	uchar buf[3];
	
	buf[0]=TELNET_IAC;
	buf[1]=cmd;
	buf[2]=opt;

	if(debug_telnet)
		fprintf(statfp,"Sending telnet command: %s %s"
			,telnet_cmd_desc(buf[1]),telnet_opt_desc(buf[2]));
	if(send(sock,buf,sizeof(buf),0)==sizeof(buf) && debug_telnet)
		fprintf(statfp,"\n");
	else if(debug_telnet)
		fprintf(statfp," FAILED!\n");
}

#define DEBUG_TELNET FALSE

/****************************************************************************/
/* Receive a byte from remote												*/
/****************************************************************************/
int recv_byte(void* unused, unsigned timeout)
{
	int			i;
	long		t;
	uchar		ch;
	fd_set		socket_set;
	time_t		end;
	struct timeval	tv;
	static uchar	telnet_cmd;
	static int		telnet_cmdlen;

	end=msclock()+(timeout*MSCLOCKS_PER_SEC);
	while(1) {

		FD_ZERO(&socket_set);
		FD_SET(sock,&socket_set);
		if((t=end-msclock())<0) t=0;
		tv.tv_sec=t/MSCLOCKS_PER_SEC;
		tv.tv_usec=0;

		if((i=select(sock+1,&socket_set,NULL,NULL,&tv))<1) {
			if(i==SOCKET_ERROR)
				fprintf(errfp,"!ERROR %d selecting socket", ERROR_VALUE);
			if(timeout) {
				newline();
				fprintf(statfp,"!Receive timeout (%u seconds)\n", timeout);
			}
			return(NOINP);
		}
		
		i=recv(sock,&ch,sizeof(ch),0);

		if(i!=sizeof(ch)) {
			newline();
			if(i==0)
				fprintf(statfp,"No carrier\n");
			else
				fprintf(statfp,"!recv error %d (%d)\n",i,ERROR_VALUE);
			return(-2); 
		}

		if(telnet) {
			if(ch==TELNET_IAC) {
#if DEBUG_TELNET
				fprintf(statfp,"T<%s> ",telnet_cmd_desc(ch));
#endif
				if(telnet_cmdlen==0) {
					telnet_cmdlen=1;
					continue;
				}
				if(telnet_cmdlen==1) {
					telnet_cmdlen=0;
					return(TELNET_IAC);
				}
			}
			if(telnet_cmdlen) {
				telnet_cmdlen++;
#if DEBUG_TELNET
				if(telnet_cmdlen==2)
					fprintf(statfp,"T<%s> ",telnet_cmd_desc(ch));
				else
					fprintf(statfp,"T<%s> ",telnet_opt_desc(ch));
#endif
				if(debug_telnet && telnet_cmdlen==3)
					fprintf(statfp,"Received telnet command: %s %s\n"
						,telnet_cmd_desc(telnet_cmd),telnet_opt_desc(ch));
				if(telnet_cmdlen==3 && telnet_cmd==TELNET_DO)
					send_telnet_cmd(sock, TELNET_WILL,ch);
	/*
				else if(telnet_cmdlen==3 && telnet_cmd==TELNET_WILL)
					send_telnet_cmd(sock, TELNET_DO,ch);
	*/
				telnet_cmd=ch;
				if((telnet_cmdlen==2 && ch<TELNET_WILL) || telnet_cmdlen>2) {
					telnet_cmdlen=0;
//					break;
				}
				continue;
			}
		}
		if(debug_rx)
			fprintf(statfp,"RX: %s\n",chr(ch));
		return(ch);
	}

	return(NOINP);
}

#if !SINGLE_THREADED
/*************************/
/* Send a byte to remote */
/*************************/
int send_byte(void* unused, uchar ch, unsigned timeout)
{
	uchar		buf[2] = { TELNET_IAC, TELNET_IAC };
	unsigned	len=1;

	if(telnet && ch==TELNET_IAC)	/* escape IAC char */
		len=2;
	else
		buf[0]=ch;

	if(RingBufFree(&outbuf)<len) {
		fprintf(statfp,"FLOW");
		flows++;
		if(WaitForEvent(outbuf_empty,timeout*1000)!=WAIT_OBJECT_0) {
			fprintf(statfp,"\n!ERROR Waiting for output buffer to flush\n");
			return(-1);
		}
		fprintf(statfp,"\b\b\b\b    \b\b\b\b");
	}

	RingBufWrite(&outbuf,buf,len);
#if !defined(RINGBUF_EVENT)
	ResetEvent(outbuf_empty);
#endif

	if(debug_tx)
		fprintf(statfp,"TX: %s\n",chr(ch));
	return(0);
}

#else

/*************************/
/* Send a byte to remote */
/*************************/
int send_byte(void* unused, uchar ch, unsigned timeout)
{
	uchar		buf[2] = { TELNET_IAC, TELNET_IAC };
	int			len=1;
	int			i;
	fd_set		socket_set;
	struct timeval	tv;

	FD_ZERO(&socket_set);
	FD_SET(sock,&socket_set);
	tv.tv_sec=timeout;
	tv.tv_usec=0;

	if(select(sock+1,NULL,&socket_set,NULL,&tv)<1)
		return(ERROR_VALUE);

	if(telnet && ch==TELNET_IAC)	/* escape IAC char */
		len=2;
	else
		buf[0]=ch;

	i=send(sock,buf,len,0);
	
	if(i==len) {
		if(debug_tx)
			fprintf(statfp,"TX: %s\n",chr(ch));
		return(0);
	}

	return(-1);
}
#endif

#if 1

void output_thread(void* arg)
{
	char		stats[128];
    BYTE		buf[IO_THREAD_BUF_SIZE];
	int			i;
    ulong		avail;
	ulong		total_sent=0;
	ulong		total_pkts=0;
	ulong		short_sends=0;
    ulong		bufbot=0;
    ulong		buftop=0;
	fd_set		socket_set;
	struct timeval tv;

#ifdef _DEBUG
	fprintf(statfp,"output thread started\n");
#endif

	while(sock!=INVALID_SOCKET && !terminate) {

		if(bufbot==buftop)
	    	avail=RingBufFull(&outbuf);
		else
        	avail=buftop-bufbot;

		if(!avail) {
#if !defined(RINGBUF_EVENT)
			SetEvent(outbuf_empty);
#endif
			sem_wait(&outbuf.sem);
			if(outbuf.highwater_mark)
				sem_trywait_block(&outbuf.highwater_sem,outbuf_drain_timeout);
			continue; 
		}

		/* Check socket for writability (using select) */
		tv.tv_sec=0;
		tv.tv_usec=1000;

		FD_ZERO(&socket_set);
		FD_SET(sock,&socket_set);

		i=select(sock+1,NULL,&socket_set,NULL,&tv);
		if(i==SOCKET_ERROR) {
			fprintf(errfp,"!ERROR %d selecting socket %u for send\n"
				,ERROR_VALUE,sock);
			break;
		}
		if(i<1) {
			select_errors++;
			continue;
		}

        if(bufbot==buftop) { // linear buf empty, read from ring buf
            if(avail>sizeof(buf)) {
                fprintf(errfp,"!Insufficient linear output buffer (%lu > %lu)\n"
					,avail, sizeof(buf));
                avail=sizeof(buf);
            }
            buftop=RingBufRead(&outbuf, buf, avail);
            bufbot=0;
        }
		i=sendsocket(sock, (char*)buf+bufbot, buftop-bufbot);
		if(i==SOCKET_ERROR) {
        	if(ERROR_VALUE == ENOTSOCK)
                fprintf(errfp,"!client socket closed on send\n");
            else if(ERROR_VALUE==ECONNRESET) 
				fprintf(errfp,"!connection reset by peer on send\n");
            else if(ERROR_VALUE==ECONNABORTED) 
				fprintf(errfp,"!connection aborted by peer on send\n");
			else
				fprintf(errfp,"!ERROR %d sending on socket %d\n"
                	,ERROR_VALUE, sock);
			break;
		}

		if(i!=(int)(buftop-bufbot)) {
			fprintf(errfp,"!Short socket send (%u instead of %u)\n"
				,i ,buftop-bufbot);
			short_sends++;
		}
		bufbot+=i;
		total_sent+=i;
		total_pkts++;
    }

	if(total_sent)
		sprintf(stats,"(sent %lu bytes in %lu blocks, %lu average, %lu short, %lu errors)"
			,total_sent, total_pkts, total_sent/total_pkts, short_sends, select_errors);
	else
		stats[0]=0;

	fprintf(errfp,"output thread terminated\n%s\n", stats);
}

#else

void output_thread(void* arg)
{
	char		stats[128];
    BYTE		buf[IO_THREAD_BUF_SIZE];
	int			i;
    ulong		avail;
	ulong		total_sent=0;
	ulong		total_pkts=0;
	ulong		short_sends=0;
	ulong		buftop=0;
	fd_set		socket_set;
	struct timeval tv;

#ifdef _DEBUG
	fprintf(statfp,"output thread started\n");
#endif

	while(sock!=INVALID_SOCKET && !terminate) {

    	avail=RingBufFull(&outbuf);

		if(!avail) {
			sem_wait(&outbuf.sem);
			if(outbuf.highwater_mark)
				sem_trywait_block(&outbuf.highwater_sem,outbuf_drain_timeout);
			continue; 
		}

		/* Check socket for writability (using select) */
		tv.tv_sec=0;
		tv.tv_usec=1000;

		FD_ZERO(&socket_set);
		FD_SET(sock,&socket_set);

		i=select(sock+1,NULL,&socket_set,NULL,&tv);
		if(i==SOCKET_ERROR) {
			fprintf(errfp,"!ERROR %d selecting socket %u for send\n"
				,ERROR_VALUE,sock);
			break;
		}
		if(i<1) {
			select_errors++;
			continue;
		}

        if(avail>sizeof(buf)) {
            fprintf(errfp,"!Insufficient linear output buffer (%lu > %lu)\n"
				,avail, sizeof(buf));
            avail=sizeof(buf);
        }
        buftop=RingBufRead(&outbuf, buf, avail);
		i=sendsocket(sock, (char*)buf, buftop);
		if(i==SOCKET_ERROR) {
        	if(ERROR_VALUE == ENOTSOCK)
                fprintf(errfp,"!client socket closed on send\n");
            else if(ERROR_VALUE==ECONNRESET) 
				fprintf(errfp,"!connection reset by peer on send\n");
            else if(ERROR_VALUE==ECONNABORTED) 
				fprintf(errfp,"!connection aborted by peer on send\n");
			else
				fprintf(errfp,"!ERROR %d sending on socket %d\n"
                	,ERROR_VALUE, sock);
			break;
		}

		if(i!=(int)(buftop)) {
			fprintf(errfp,"!Short socket send (%u instead of %u)\n"
				,i ,buftop);
			short_sends++;
		}
		total_sent+=i;
		total_pkts++;
    }

	if(total_sent)
		sprintf(stats,"(sent %lu bytes in %lu blocks, %lu average, %lu short, %lu errors)"
			,total_sent, total_pkts, total_sent/total_pkts, short_sends, select_errors);
	else
		stats[0]=0;

	fprintf(errfp,"output thread terminated\n%s\n", stats);
}

#endif

/****************************************************************************/
/* Returns the number of blocks required to send len bytes					*/
/****************************************************************************/
unsigned num_blocks(ulong len, unsigned block_size)
{
	ulong blocks;

	blocks=len/block_size;
	if(len%block_size)
		blocks++;
	return(blocks);
}

/************************************************/
/* Dump the current block contents - for debug  */
/************************************************/
void dump_block(long block_size)
{
	long l;

	for(l=0;l<block_size;l++)
		fprintf(statfp,"%02X  ",block[l]);
	fprintf(statfp,"\n");
}

void xmodem_progress(void* unused, unsigned block_num, ulong fsize, time_t t)
{
	unsigned	cps;
	unsigned	total_blocks;
	long		l;

	if(t<=0)
		t=1;
	if((cps=(uint)((block_num*(long)xm.block_size)/t))<=0)
		cps=1;			/* cps so far */
	l=fsize/cps;		/* total transfer est time */
	l-=t;				/* now, it's est time left */
	if(l<0) l=0;
	total_blocks=num_blocks(fsize,xm.block_size);
	fprintf(statfp,"\rBlock (%lu%s): %lu/%lu  Byte: %lu  "
		"Time: %lu:%02lu/%lu:%02lu  CPS: %u  %lu%% "
		,xm.block_size%1024L ? xm.block_size: xm.block_size/1024L
		,xm.block_size%1024L ? "" : "k"
		,block_num
		,total_blocks
		,block_num*(long)xm.block_size
		,t/60L
		,t%60L
		,l/60L
		,l%60L
		,cps
		,(long)(((float)block_num/(float)total_blocks)*100.0)
		);
}

/* 
 * show the progress of the transfer like this:
 * zmtx: sending file "garbage" 4096 bytes ( 20%)
 */
void zmodem_progress(void* unused, ulong offset, ulong fsize, time_t t)
{
	unsigned	cps;
	long		l;

	if(t<=0)
		t=1;
	if((cps=offset/t)==0)
		cps=1;		/* cps so far */
	l=fsize/cps;	/* total transfer est time */
	l-=t;			/* now, it's est time left */
	if(l<0) l=0;
	fprintf(statfp,"\rKByte: %lu/%lu  "
		"Time: %lu:%02lu/%lu:%02lu  CPS: %u  %lu%% "
		,offset/1024
		,fsize/1024
		,t/60L
		,t%60L
		,l/60L
		,l%60L
		,cps
		,(long)(((float)offset/(float)fsize)*100.0)
		);
}

int send_files(char** fname, uint fnames)
{
	char	path[MAX_PATH+1];
	int		i;
	uint	errors;
	uint	fnum;
	uint	cps;
	glob_t	g;
	int		gi;
	BOOL	success=TRUE;
	long	l;
	long	fsize;
	long	block_len;
	uint	total_files=0,sent_files=0;
	ulong	total_bytes=0,sent_bytes=0;
	ulong	total_blocks;
	size_t	n;
	time_t	t,startfile;
	time_t	now;
	time_t	last_status;
	FILE*	fp;

	/****************************************************/
	/* Search through all to find total files and bytes */
	/****************************************************/
	for(fnum=0;fnum<fnames;fnum++) {
		if(glob(fname[fnum],0,NULL,&g)) {
			fprintf(statfp,"%s not found\n",fname[fnum]);
			continue;
		}
		for(i=0;i<(int)g.gl_pathc;i++) {
			if(isdir(g.gl_pathv[i]))
				continue;
			total_files++;
			total_bytes+=flength(g.gl_pathv[i]);
		} 
		globfree(&g);
	}

	if(fnames>1)
		fprintf(statfp,"Sending %u files (%lu KB total)\n"
			,total_files,total_bytes/1024);

	zm.n_files_remaining = total_files;
	zm.n_bytes_remaining = total_bytes;

	/***********************************************/
	/* Send every file matching names or filespecs */
	/***********************************************/
	for(fnum=0;fnum<fnames;fnum++) {
		if(glob(fname[fnum],0,NULL,&g)) {
			fprintf(statfp,"%s not found\n",fname[fnum]);
			continue;
		}
		for(gi=0;gi<(int)g.gl_pathc;gi++) {
			SAFECOPY(path,g.gl_pathv[gi]);
			if(isdir(path))
				continue;

			if((fp=fopen(path,"rb"))==NULL) {
				fprintf(statfp,"!Error %d opening %s for read\n",errno,path);
				continue;
			}

			fsize=filelength(fileno(fp));

			errors=0;
			success=FALSE;
			startfile=time(NULL);

			do { /* try */

				if(!(mode&ZMODEM)) { /* X/Ymodem */
					if(!xmodem_get_mode(&xm)) {
						xmodem_cancel(&xm);
						break;
					}
				}

				fprintf(statfp,"Sending %s (%lu KB) via %s\n"
					,path,fsize/1024
					,mode&XMODEM ? "Xmodem" : mode&YMODEM ? mode&GMODE ? "Ymodem-G"
						: "Ymodem" : "Zmodem");

				if(mode&ZMODEM) {

					success=zmodem_send_file(&zm,getfname(path),fp,fnum==0);

				} else {	/* X/Ymodem */

					if(!(mode&XMODEM)) {
						t=fdate(path);
						memset(block,0,sizeof(block));
						SAFECOPY(block,getfname(path));
						i=sprintf(block+strlen(block)+1,"%lu %lo 0 0 %d %ld"
							,fsize,t,total_files-sent_files,total_bytes-sent_bytes);
						
						fprintf(statfp,"Sending Ymodem header block: '%s'\n",block+strlen(block)+1);
						
						block_len=strlen(block)+1+i;
						for(errors=0;errors<xm.max_errors;errors++) {
							xmodem_put_block(&xm, block, block_len <=128 ? 128:1024, 0  /* block_num */);
							if(xmodem_get_ack(&xm,1,0))
								break; 
						}
						if(errors==xm.max_errors) {
							newline();
							fprintf(statfp,"Failed to send header block\n");
							xmodem_cancel(&xm);
							break; 
						}
						if(!xmodem_get_mode(&xm)) {
							xmodem_cancel(&xm);
							break;
						}
					}
					startfile=time(NULL);	/* reset time, don't count header block */
					block_num=1;
					errors=0;
					while((block_num-1)*xm.block_size<(ulong)fsize && errors<xm.max_errors) {
						fseek(fp,(block_num-1)*(long)xm.block_size,SEEK_SET);
						memset(block,CPMEOF,xm.block_size);
						if((n=fread(block,1,xm.block_size,fp))!=xm.block_size
							&& block_num*xm.block_size<(ulong)fsize) {
							fprintf(errfp,"\n!READ ERROR %d instead of %d at offset %lu\n"
								,n,xm.block_size,(block_num-1)*(long)xm.block_size);
							errors++;
							continue;
						}
						xmodem_put_block(&xm, block, xm.block_size, block_num);
						now=time(NULL);
						if((t=now-startfile)<=0)
							t=1;	/* t is time so far */
						if((cps=(uint)((block_num*(long)xm.block_size)/t))<=0)
							cps=1;	/* cps so far */
						total_blocks=num_blocks(fsize,xm.block_size);
						if(now-last_status>=xm.progress_interval || block_num==total_blocks) {
							xmodem_progress(xm.cbdata,block_num,fsize,t);
							last_status=now;
						}
						if(!xmodem_get_ack(&xm,5,block_num)) {
							errors++;
#if 0
							dump_block(xm.block_size);
#endif
							fprintf(errfp,"!Error #%d at offset %ld\n",errors,ftell(fp)-xm.block_size);
						} else
							block_num++; 
					}
					if((long)(block_num-1)*(long)xm.block_size>=fsize) {
						fprintf(statfp,"\n");

#if !SINGLE_THREADED
						lprintf(LOG_DEBUG,"Waiting for output buffer to empty... ");
						if(WaitForEvent(outbuf_empty,5000)==WAIT_OBJECT_0)
							lprintf(LOG_DEBUG,"\n");
						else
							lprintf(LOG_DEBUG,"FAILURE\n");
#endif
						success=xmodem_put_eot(&xm);	/* end-of-text, wait for ACK */
					}
				}
			} while(0);
			/* finally */
			fclose(fp);

			if((t=time(NULL)-startfile)<=0) 
				t=1;
			if(success) {
				sent_files++;
				sent_bytes+=fsize;
				fprintf(statfp,"\nSuccessful - Time: %lu:%02lu  CPS: %lu\n"
						,t/60,t%60,fsize/t);
			} else
				fprintf(errfp,"\n!Failure\n");

			if(total_files>1)
				fprintf(statfp,"Remaining - Time: %lu:%02lu  Files: %u  KBytes: %lu\n"
					,((total_bytes-sent_bytes)/cps)/60
					,((total_bytes-sent_bytes)/cps)%60
					,total_files-sent_files
					,(total_bytes-sent_bytes)/1024
					);

			/* DSZLOG entry */
			if(logfp) {
				fprintf(statfp,"Updating DSZLOG: %s\n", dszlog);
				if(mode&ZMODEM)
					l=zm.sent_successfully;
				else {
					l=(block_num-1)*(long)xm.block_size;
					if(l>fsize)
						l=fsize;
				}
				fprintf(logfp,"%c %7lu %5u bps %6lu cps %3u errors %5u %4u "
					"%s -1\n"
					,success ? (mode&ZMODEM ? 'z':'S') 
						: (mode&ZMODEM && zm.file_skipped) ? 's' 
						: 'E'
					,l
					,115200 /* baud */
					,l/t
					,errors
					,flows
					,xm.block_size
					,path); 
			}
		} /* while(gi<(int)g.gl_pathc) */

		if(gi<(int)g.gl_pathc)/* error occurred */
			break;
	}
	if(fnum<fnames) /* error occurred */
		return(-1);

	if(!success)
		return(-1);

	if(mode&XMODEM)
		return(0);
	if(mode&ZMODEM)
		zmodem_send_zfin(&zm);
	else {	/* YMODEM */

		if(xmodem_get_mode(&xm)) {

			fprintf(statfp,"Sending Ymodem termination block\n");

			memset(block,0,128);	/* send short block for terminator */
			xmodem_put_block(&xm, block, 128 /* block_size */, 0 /* block_num */);
			if(!xmodem_get_ack(&xm,6,0)) {
				newline();
				fprintf(statfp,"Failed to receive ACK after terminating block\n"); 
			} 
		}
	}
	if(total_files>1) {
		t=time(NULL)-startall;
		if(!t) t=1;
		newline();
		fprintf(statfp,"Overall - Time %02lu:%02lu  KBytes: %lu  CPS: %lu\n"
			,t/60,t%60,sent_bytes/1024,sent_bytes/t); 
	}
	return(0);	/* success */
}

int receive_files(char** fname, int fnames)
{
	char	str[MAX_PATH+1];
	int		i;
	int		fnum=0;
	uint	errors;
	uint	total_files;
	uint	cps;
	uint	hdr_block_num;
	long	b,l,m;
	long	serial_num=-1;
	ulong	file_bytes=0,file_bytes_left=0;
	ulong	total_bytes=0;
	FILE*	fp;
	time_t	t,startfile,ftime;
	time_t	now,last_status=0;

	if(fnames>1)
		fprintf(statfp,"Receiving %u files\n",fnames);

	outbuf.highwater_mark=0;	/* don't delay ACK/NAK transmits */

	while(1) {
		if(mode&XMODEM) {
			SAFECOPY(str,fname[0]);
			file_bytes=file_bytes_left=0x7fffffff;
		}

		else if(mode&YMODEM) {
			for(errors=0;errors<xm.max_errors;errors++) {
				fprintf(statfp,"Fetching Ymodem header block, requesting: ");
				if(errors>(xm.max_errors/2) && mode&CRC && !(mode&GMODE))
					mode&=~CRC;
				if(mode&GMODE) {		/* G for Ymodem-G */
					fprintf(statfp,"G (Streaming CRC) mode\n");
					putcom('G');
				} else if(mode&CRC) {	/* C for CRC */
					fprintf(statfp,"CRC mode\n");
					putcom('C');
				} else {				/* NAK for checksum */
					fprintf(statfp,"Checksum mode\n");
					putcom(NAK);
				}

				if(xmodem_get_block(&xm, block,TRUE)==0) { 	 /* block received successfully */
					putcom(ACK);
					break; 
				} 
			}
			if(errors==xm.max_errors) {
				fprintf(statfp,"!Error fetching Ymodem header block\n");
				xmodem_cancel(&xm);
				return(1); 
			}
			if(!block[0]) {
				fprintf(statfp,"Received Ymodem termination block\n");
				return(0); 
			}
			sscanf(block+strlen(block)+1,"%ld %lo %lo %lo %d %ld"
				,&file_bytes			/* file size (decimal) */
				,&ftime 				/* file time (octal unix format) */
				,&m 					/* file mode (not used) */
				,&serial_num			/* program serial number */
				,&total_files			/* remaining files to be sent */
				,&total_bytes			/* remaining bytes to be sent */
				);
			if(!file_bytes)
				file_bytes=0x7fffffff;
			file_bytes_left=file_bytes;
			if(!total_files)
				total_files=fnames-fnum;
			if(!total_files)
				total_files=1;
			if(total_bytes<file_bytes)
				total_bytes=file_bytes;
			if(!serial_num)
				serial_num=-1;
			fprintf(statfp,"Incoming filename: %.64s ",block);
			if(mode&DIR)
				sprintf(str,"%s%s",fname[0],getfname(block));
			else {
				SAFECOPY(str,getfname(block));
				for(i=0;i<fnames;i++) {
					if(!fname[i][0])	/* name blank or already used */
						continue;
					if(!stricmp(getfname(fname[i]),str)) {
						SAFECOPY(str,fname[i]);
						fname[i][0]=0;
						break; 
					} 
				}
				if(i==fnames) { 				/* Not found in list */
					if(fnames)
						fprintf(statfp," - Not in receive list!");
					if(!fnames || fnum>=fnames || !fname[fnum][0])
						SAFECOPY(str,getfname(block));	/* worst case */
					else {
						SAFECOPY(str,fname[fnum]);
						fname[fnum][0]=0; 
					} 
				} 
			}
			fprintf(statfp,"\n"); 
//			getchar();
		}

		else {	/* Zmodem */
#if 0
			tryzhdrtype=ZRINIT;
			while(1) {
				Txhdr[ZF0]=(CANFC32|CANFDX|CANOVIO|CANRLE);
				/* add CANBRK if we can send break signal */
				if(zmode&CTRL_ESC)
					Txhdr[ZF0]|=TESCCTL;
				Txhdr[ZF1]=CANVHDR;
				Txhdr[ZP0]=0;
				Txhdr[ZP1]=0;
				putzhhdr(tryzhdrtype);
				done=0;
				while(!done) {
					done=1;
					switch(getzhdr()) {
						case ZRQINIT:
							if(Rxhdr[ZF3]&0x80)
								zmode|=VAR_HDRS;   /* we can var header */
							break;
						case ZFILE:
							zconv=Rxhdr[ZF0];
							zmanag=Rxhdr[ZF1];
							ztrans=Rxhdr[ZF2];
							if(Rxhdr[ZF3]&ZCANVHDR)
								zmode|=VAR_HDRS;
							tryzhdrtype=ZRINIT;
							if(getzdata(block, 1024)==GOTCRCW) {
								/* something */
								done=1; 
							}
							putzhhdr(ZNAK);
							done=0;
							break;
						case ZSINIT:
							if(Rxhdr[ZF0]&TESCCTL)
								zmode|=CTRL_ESC;
							if (getzdata(attn,ZATTNLEN)==GOTCRCW) {
								ltohdr(1L);
								putzhhdr(ZACK); 
							}
							else
								putzhhdr(ZNAK);
							done=0;
							break;
						case ZFREECNT:
							ltohdr(0);			/* should be free disk space */
							putzhhdr(ZACK);
							done=0;
							break;
						case ZCOMMAND:
/***
							cmdzack1flg = Rxhdr[ZF0];
							if(getzdata(block,1024)==GOTCRCW) {
								if (cmdzack1flg & ZCACK1)
									ltohdr(0L);
								else
									ltohdr((long)sys2(block));
								purgeline();	/* dump impatient questions */
								do {
									zshhdr(4,ZCOMPL, Txhdr);
								}
								while (++errors<20 && zgethdr(Rxhdr,1)!=ZFIN);
								ackbibi();
								if (cmdzack1flg & ZCACK1)
									exec2(block);
								return ZCOMPL;
							}
***/
							putzhhdr(ZNAK);
							done=0;
							break;
						case ZCOMPL:
							done=0;
							break;
						case ZFIN:
							ackbibi();
							return ZCOMPL;
						case ZCAN:
							return ERROR; 
				} 
			}
#endif
		}

		fnum++;

		if(!(mode&DIR) && fnames && fnum>fnames) {
			newline();
			fprintf(statfp,"Attempt to send more files than specified\n");
			xmodem_cancel(&xm);
			break; 
		}

		if(fexist(str) && !(mode&OVERWRITE)) {
			fprintf(statfp,"%s already exists\n",str);
			xmodem_cancel(&xm);
			return(1); 
		}
		if((fp=fopen(str,"wb"))==NULL) {
			fprintf(statfp,"!Error creating %s\n",str);
			xmodem_cancel(&xm);
			return(1); 
		}
		setvbuf(fp,NULL,_IOFBF,8*1024);
		startfile=time(NULL);
		fprintf(statfp,"Receiving %s (%lu KB) via %s %s\n"
			,str
			,mode&XMODEM ? 0 : file_bytes/1024
			,mode&XMODEM ? "Xmodem" : mode&YMODEM ? mode&GMODE ? "Ymodem-G"
			: "Ymodem" :"Zmodem"
			,mode&CRC ? "CRC-16":"Checksum");

		errors=0;
		block_num=0;
		if(mode&GMODE)		/* G for Ymodem-G */
			putcom('G');
		else if(mode&CRC)	/* C for CRC */
			putcom('C');
		else				/* NAK for checksum */
			putcom(NAK);
		while(errors<xm.max_errors) {
			if(errors && mode&GMODE) {
				xmodem_cancel(&xm);
				return(1); 
			}
			if(block_num && !(mode&GMODE))
				putcom(ACK);
			i=xmodem_get_block(&xm, block,FALSE); 	/* returns block num */
			if(i<0) {
				if(i==-EOT)			/* end of transfer */
					break;
				/* other error */
				xmodem_cancel(&xm);
				return(1); 
			}
			hdr_block_num=i;
			if(file_bytes_left<=0L)  { /* No more bytes to send */
				newline();
				fprintf(statfp,"Attempt to send more than header specified\n");
				break; 
			}
			if(hdr_block_num==(uchar)((block_num+1)&0xff)) {	/* correct block */
				block_num++;
				if(file_bytes_left<(ulong)xm.block_size) {
					if(fwrite(block,1,file_bytes_left,fp)
						!=file_bytes_left) {
						newline();
						fprintf(statfp,"!Error writing to file\n");
						xmodem_cancel(&xm);
						return(1); 
					} 
				}
				else {
					if(fwrite(block,1,xm.block_size,fp)
						!=(uint)xm.block_size) {
						newline();
						fprintf(statfp,"!Error writing to file\n");
						xmodem_cancel(&xm);
						return(1); 
					} 
				}
				file_bytes_left-=xm.block_size; 
			}
			else {
				newline();
				fprintf(statfp,"Block number %u instead of %u\n"
					,hdr_block_num,(block_num+1)&0xff);
//				dump_block(xm.block_size);
				errors++; 
			}
			now=time(NULL);
			t=now-startfile;
			if(!t) t=1;
			cps=(uint)((block_num*(long)xm.block_size)/t); 	/* cps so far */
			if(!cps) cps=1;
			l=file_bytes/cps;  /* total transfer est time */
			l-=t;				/* now, it's est time left */
			if(l<0) l=0;
			b=num_blocks(file_bytes, xm.block_size);
			if(now!=last_status) {
				if(mode&YMODEM)
					fprintf(statfp,"\rBlock (%lu%s): %lu/%lu  Byte: %lu  Time: %lu:%02lu/"
						"%lu:%02lu  CPS: %u  %lu%% "
						,xm.block_size%1024L ? xm.block_size: xm.block_size/1024L
						,xm.block_size%1024L ? "" : "k"
						,block_num
						,b
						,block_num*(long)xm.block_size
						,t/60L
						,t%60L
						,l/60L
						,l%60L
						,cps
						,(long)(((float)block_num/(float)b)*100.0)
						);
				else	/* Xmodem */
					fprintf(statfp,"\rBlock (%lu%s): %lu  Byte: %lu  Time: %lu:%02lu  "
						"CPS: %u "
						,xm.block_size%1024L ? xm.block_size: xm.block_size/1024L
						,xm.block_size%1024L ? "" : "k"
						,block_num
						,block_num*(long)xm.block_size
						,t/60L
						,t%60L
						,cps
						);
				last_status=now;
			}
		}

		putcom(ACK);
		if(!(mode&XMODEM) && ftime)
			setfdate(str,ftime); 
		/* Use correct file size */
		fflush(fp);
		if(file_bytes < (ulong)filelength(fileno(fp)));
			chsize(fileno(fp),file_bytes);
		fclose(fp);
		t=time(NULL)-startfile;
		if(!t) t=1;
#if 0
		l=(block_num-1)*xm.block_size;
		if(l>(long)file_bytes)
#endif
			l=file_bytes;
		newline();
		fprintf(statfp,"Successful - Time: %lu:%02lu  CPS: %lu\n"
			,t/60,t%60,l/t);
		if(logfp) {
			fprintf(statfp,"Updating DSZLOG: %s\n", dszlog);
			fprintf(logfp,"%c %6lu %5u bps %4lu cps %3u errors %5u %4u "
				"%s %d\n"
				,mode&ZMODEM ? 'Z' : 'R'
				,l
				,115200	/* baud */
				,l/t
				,errors
				,flows
				,xm.block_size
				,str
				,serial_num); 
		}
		if(mode&XMODEM)
			break;
		total_files--;
		total_bytes-=file_bytes;
		if(total_files>1 && total_bytes)
			fprintf(statfp,"Remaining - Time: %lu:%02lu  Files: %u  KBytes: %lu\n"
				,(total_bytes/cps)/60
				,(total_bytes/cps)%60
				,total_files
				,total_bytes/1024
				);
	}
	return(0);	/* success */
}

void exiting(void)
{
	if(pause_on_exit) {
		printf("Hit enter to continue...");
		getchar();
	}
}

static const char* usage=
	"usage: sexyz <socket> [-opts] <cmd> [file | path | +list]\n"
	"\n"
	"socket = TCP socket descriptor\n"
	"\n"
	"opts   = -o  to overwrite files when receiving\n"
	"         -a  to sound alarm at start and stop of transfer\n"
	"         -!  to pause after abnormal exit (error)\n"
	"         -telnet to enable Telnet mode\n"
	"         -rlogin to enable RLogin (pass-through) mode\n"
	"\n"
	"cmd    = v  to display detailed version information\n"
	"         sx to send Xmodem     rx to recv Xmodem\n"
	"         sX to send Xmodem-1k  rc to recv Xmodem-CRC\n"
	"         sy to send Ymodem     ry to recv Ymodem\n"
	"         sY to send Ymodem-1k  rg to recv Ymodem-G\n"
	"         sz to send Zmodem     rz to recv Zmodem\n"
	"\n"
	"file   = filename to send or receive\n"
	"path   = directory to receive files into\n"
	"list   = name of text file with list of filenames to send or receive\n";

/***************/
/* Entry Point */
/***************/
int main(int argc, char **argv)
{
	char	str[MAX_PATH+1];
	char	fname[MAX_PATH+1];
	char	ini_fname[MAX_PATH+1];
	char*	p;
	int 	i;
	int		retval;
	uint	fnames=0;
	FILE*	fp;
	BOOL	tcp_nodelay;
	BOOL	pause_on_abend=FALSE;
	char	compiler[32];
	str_list_t fname_list;

	fname_list=strListInit();

	DESCRIBE_COMPILER(compiler);

	errfp=stderr;
	statfp=stdout;

	sscanf("$Revision$", "%*s %s", revision);

	fprintf(statfp,"\nSynchronet External X/Y/Zmodem  v%s-%s"
		"  Copyright 2005 Rob Swindell\n\n"
		,revision
		,PLATFORM_DESC
		);

	RingBufInit(&inbuf, IO_THREAD_BUF_SIZE);
	RingBufInit(&outbuf, IO_THREAD_BUF_SIZE);

	xmodem_init(&xm,NULL,&mode,lputs,xmodem_progress,send_byte,recv_byte);
	zmodem_init(&zm,NULL,&mode,lputs,zmodem_progress,send_byte,recv_byte);

	/* Generate path/sexyz[.host].ini from path/sexyz[.exe] */
	SAFECOPY(str,argv[0]);
	p=getfname(str);
	SAFECOPY(fname,p);
	*p=0;
	if((p=getfext(fname))!=NULL) 
		*p=0;
	strcat(fname,".ini");
	
	iniFileName(ini_fname,sizeof(ini_fname),str,fname);
	if((fp=fopen(ini_fname,"r"))!=NULL)
		fprintf(statfp,"Reading %s\n",ini_fname);

	tcp_nodelay				=iniReadBool(fp,ROOT_SECTION,"TCP_NODELAY",TRUE);

	debug_tx				=iniReadBool(fp,ROOT_SECTION,"DebugTx",FALSE);
	debug_rx				=iniReadBool(fp,ROOT_SECTION,"DebugRx",FALSE);
	debug_telnet			=iniReadBool(fp,ROOT_SECTION,"DebugTelnet",FALSE);

	pause_on_exit			=iniReadBool(fp,ROOT_SECTION,"PauseOnExit",FALSE);
	pause_on_abend			=iniReadBool(fp,ROOT_SECTION,"PauseOnAbend",FALSE);

	outbuf.highwater_mark	=iniReadInteger(fp,ROOT_SECTION,"OutbufHighwaterMark",1100);
	outbuf_drain_timeout	=iniReadInteger(fp,ROOT_SECTION,"OutbufDrainTimeout",10);

	xm.progress_interval	=iniReadInteger(fp,"Xmodem","ProgressInterval",xm.progress_interval);
	xm.send_timeout			=iniReadInteger(fp,"Xmodem","SendTimeout",xm.send_timeout);	/* seconds */
	xm.recv_timeout			=iniReadInteger(fp,"Xmodem","RecvTimeout",xm.recv_timeout);	/* seconds */
	xm.byte_timeout			=iniReadInteger(fp,"Xmodem","ByteTimeout",xm.byte_timeout);	/* seconds */
	xm.ack_timeout			=iniReadInteger(fp,"Xmodem","AckTimeout",xm.ack_timeout);	/* seconds */
	xm.block_size			=iniReadInteger(fp,"Xmodem","BlockSize",xm.block_size);		/* 128 or 1024 */
	xm.max_errors			=iniReadInteger(fp,"Xmodem","MaxErrors",xm.max_errors);
	xm.g_delay				=iniReadInteger(fp,"Xmodem","G_Delay",xm.g_delay);

	zm.progress_interval	=iniReadInteger(fp,"Zmodem","ProgressInterval",zm.progress_interval);
	zm.send_timeout			=iniReadInteger(fp,"Zmodem","SendTimeout",zm.send_timeout);	/* seconds */
	zm.recv_timeout			=iniReadInteger(fp,"Zmodem","RecvTimeout",zm.recv_timeout);	/* seconds */
	zm.max_errors			=iniReadInteger(fp,"Zmodem","MaxErrors",zm.max_errors);

	if(fp!=NULL)
		fclose(fp);

	atexit(exiting);

#if !defined(RINGBUF_EVENT)
	outbuf_empty=CreateEvent(NULL,/* ManualReset */TRUE, /*InitialState */TRUE,NULL);
#endif

#if 0
	if(argc>1) {
		fprintf(statfp,"Command line: ");
		for(i=1;i<argc;i++)
			fprintf(statfp,"%s ",argv[i]);
		fprintf(statfp,"\n",statfp);
	}
#endif


	for(i=1;i<argc;i++) {

		if(sock==INVALID_SOCKET && isdigit(argv[i][0])) {
			sock=atoi(argv[i]);
			continue;
		}

		if(!(mode&(SEND|RECV))) {
			if(toupper(argv[i][0])=='S' || toupper(argv[i][0])=='R') { /* cmd */
				if(toupper(argv[i][0])=='R')
					mode|=RECV;
				else
					mode|=SEND;

				switch(argv[i][1]) {
					case 'c':
					case 'C':
						mode|=CRC;
					case 'x':
						xm.block_size=128;
					case 'X':
						mode|=XMODEM;
						break;
					case 'b':	/* sz/rz compatible */
					case 'B':
					case 'y':
						xm.block_size=128;
					case 'Y':
						mode|=(YMODEM|CRC);
						break;
					case 'g':
					case 'G':
						mode|=(YMODEM|CRC|GMODE);
						break;
					case 'z':
					case 'Z':
						mode|=(ZMODEM|CRC);
						break;
					default:
						fprintf(statfp,"Unrecognized command '%s'\n\n",argv[i]);
						fprintf(statfp,usage);
						exit(1); 
				} 
				continue;
			}

			if(toupper(argv[i][0])=='V') {

				fprintf(statfp,"%-8s %s\n",getfname(__FILE__)		,revision);
				fprintf(statfp,"%-8s %s\n",getfname(xmodem_source()),xmodem_ver(str));
				fprintf(statfp,"%-8s %s\n",getfname(zmodem_source()),zmodem_ver(str));
#ifdef _DEBUG
				fprintf(statfp,"Debug\n");
#endif
				fprintf(statfp,"Compiled %s %.5s with %s\n",__DATE__,__TIME__,compiler);
				fprintf(statfp,"%s\n",os_version(str));
				exit(1);
			}


			if(argv[i][0]=='-') {
				if(stricmp(argv[i]+1,"telnet")==0) {
					telnet=TRUE;
					continue;
				}
				if(stricmp(argv[i]+1,"rlogin")==0) {
					telnet=FALSE;
					continue;
				}
				switch(toupper(argv[i][1])) {
					case 'K':	/* sz/rz compatible */
						xm.block_size=1024;
						break;
					case 'G':	/* Ymodem-G */
						mode|=GMODE;
						break;
					case 'O':
						mode|=OVERWRITE;
						break;
					case 'A':
						mode|=ALARM;
						break;
					case '!':
						pause_on_abend=TRUE;
						break;
					case 'D':
						mode|=DEBUG; 
						break;
				}
			}
		}

		else if(argv[i][0]=='+') {
			if(mode&DIR) {
				fprintf(statfp,"!Cannot specify both directory and filename\n");
				exit(1); 
			}
			sprintf(str,"%s",argv[i]+1);
			if((fp=fopen(str,"r"))==NULL) {
				fprintf(statfp,"!Error %d opening filelist: %s\n",errno,str);
				exit(1); 
			}
			while(!feof(fp) && !ferror(fp)) {
				if(!fgets(str,sizeof(str),fp))
					break;
				truncsp(str);
				strListAppend(&fname_list,strdup(str),fnames++);
			}
			fclose(fp); 
		}

		else if(mode&(SEND|RECV)){
			if(isdir(argv[i])) { /* is a directory */
				if(mode&DIR) {
					fprintf(statfp,"!Only one directory can be specified\n");
					exit(1); 
				}
				if(fnames) {
					fprintf(statfp,"!Cannot specify both directory and filename\n");
					exit(1); 
				}
				if(mode&SEND) {
					fprintf(statfp,"!Cannot send directory '%s'\n",argv[i]);
					exit(1);
				}
				mode|=DIR; 
			}
			strListAppend(&fname_list,argv[i],fnames++);
		} 
	}

	if(sock==INVALID_SOCKET || sock<1) {
		fprintf(statfp,"!No socket descriptor specified\n\n");
		fprintf(errfp,usage);
		exit(1);
	}

	if(!(mode&(SEND|RECV))) {
		fprintf(statfp,"!No command specified\n\n");
		fprintf(statfp,usage);
		exit(1); 
	}

	if(mode&(SEND|XMODEM) && !fnames) { /* Sending with any or recv w/Xmodem */
		fprintf(statfp,"!Must specify filename or filelist\n\n");
		fprintf(statfp,usage);
		exit(1); 
	}


//	if(mode&DIR)
//		backslash(fname[0]);

	if(mode&ALARM) {
		BEEP(1000,500);
		BEEP(2000,500);
	}

	if(!winsock_startup())
		return(-1);

	/* Enable the Nagle Algorithm */
	lprintf(LOG_DEBUG,"Setting TCP_NODELAY to %d\n",tcp_nodelay);
	setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char*)&tcp_nodelay,sizeof(tcp_nodelay));

	if(!socket_check(sock, NULL, NULL, 0)) {
		newline();
		fprintf(statfp,"!No socket connection\n");
		return(-1); 
	}

	if((dszlog=getenv("DSZLOG"))!=NULL) {
		if((logfp=fopen(dszlog,"w"))==NULL) {
			fprintf(statfp,"!Error opening DSZLOG file: %s\n",dszlog);
			return(-1); 
		}
	}

	startall=time(NULL);

#if !SINGLE_THREADED
	_beginthread(output_thread,0,NULL);
#endif

	if(mode&RECV)
		retval=receive_files(fname_list, fnames);
	else
		retval=send_files(fname_list, fnames);

#if !SINGLE_THREADED
	lprintf(LOG_DEBUG,"Waiting for output buffer to empty... ");
	if(WaitForEvent(outbuf_empty,5000)==WAIT_OBJECT_0)
		lprintf(LOG_DEBUG,"\n");
	else
		lprintf(LOG_DEBUG,"FAILURE\n");
#endif

	terminate=TRUE;	/* stop output thread */
//	sem_post(outbuf.sem);
//	sem_post(outbuf.highwater_sem);

	if(mode&ALARM) {
		BEEP(2000,500);
		BEEP(1000,500);
	}
	fprintf(statfp,"Exiting - Error level: %d, flows: %u, select_errors=%u"
		,retval, flows, select_errors);
	fprintf(statfp,"\n");

	if(logfp!=NULL)
		fclose(logfp);

	if(retval && pause_on_abend) {
		printf("Hit enter to continue...");
		getchar();
		pause_on_exit=FALSE;
	}

	return(retval);
}

