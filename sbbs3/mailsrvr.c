/* mailsrvr.c */

/* Synchronet Mail (SMTP/POP3) server and sendmail threads */

/* $Id: mailsrvr.c,v 1.412 2006/12/29 01:23:05 rswindell Exp $ */

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

/* ANSI C Library headers */
#include <stdio.h>
#include <stdlib.h>			/* ltoa in GNU C lib */
#include <stdarg.h>			/* va_list */
#include <string.h>			/* strrchr */
#include <ctype.h>			/* isdigit */
#include <fcntl.h>			/* Open flags */
#include <errno.h>			/* errno */

/* Synchronet-specific headers */
#undef SBBS	/* this shouldn't be defined unless building sbbs.dll/libsbbs.so */
#include "sbbs.h"
#include "mailsrvr.h"
#include "mime.h"
#include "md5.h"
#include "crc32.h"
#include "base64.h"
#include "ini_file.h"
#include "netwrap.h"	/* getNameServerList() */

/* Constants */
#define FORWARD			"forward:"
#define NO_FORWARD		"local:"

int dns_getmx(char* name, char* mx, char* mx2
			  ,DWORD intf, DWORD ip_addr, BOOL use_tcp, int timeout);

static char* pop_err	=	"-ERR";
static char* ok_rsp		=	"250 OK";
static char* auth_ok	=	"235 User Authenticated";
static char* sys_error	=	"421 System error";
static char* sys_unavail=	"421 System unavailable, try again later";
static char* badarg_rsp =	"501 Bad argument";
static char* badseq_rsp	=	"503 Bad sequence of commands";
static char* badauth_rsp=	"535 Authentication failure";
static char* badrsp_err	=	"%s replied with:\r\n\"%s\"\r\n"
							"instead of the expected reply:\r\n\"%s ...\"";

#define TIMEOUT_THREAD_WAIT		60		/* Seconds */
#define DNSBL_THROTTLE_VALUE	5000	/* Milliseconds */

#define STATUS_WFC	"Listening"

static mail_startup_t* startup=NULL;
static scfg_t	scfg;
static SOCKET	server_socket=INVALID_SOCKET;
static SOCKET	pop3_socket=INVALID_SOCKET;
static DWORD	active_clients=0;
static int		active_sendmail=0;
static DWORD	thread_count=0;
static BOOL		sendmail_running=FALSE;
static DWORD	sockets=0;
static DWORD	served=0;
static BOOL		terminate_server=FALSE;
static BOOL		terminate_sendmail=FALSE;
static sem_t	sendmail_wakeup_sem;
static char		revision[16];
static time_t	uptime;
static str_list_t recycle_semfiles;
static str_list_t shutdown_semfiles;
static int		mailproc_count;

struct mailproc {
	char		cmdline[INI_MAX_VALUE_LEN];
	str_list_t	to;
	BOOL		passthru;
	BOOL		native;
} *mailproc_list;

typedef struct {
	SOCKET			socket;
	SOCKADDR_IN		client_addr;
} smtp_t,pop3_t;

static int lprintf(int level, char *fmt, ...)
{
	va_list argptr;
	char sbuf[1024];

    if(startup==NULL || startup->lputs==NULL)
        return(0);

	va_start(argptr,fmt);
    vsnprintf(sbuf,sizeof(sbuf),fmt,argptr);
	sbuf[sizeof(sbuf)-1]=0;
    va_end(argptr);
    return(startup->lputs(startup->cbdata,level,sbuf));
}

#ifdef _WINSOCKAPI_

static WSADATA WSAData;
#define SOCKLIB_DESC WSAData.szDescription
static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
		lprintf(LOG_INFO,"%s %s",WSAData.szDescription, WSAData.szSystemStatus);
		WSAInitialized=TRUE;
		return (TRUE);
	}

    lprintf(LOG_CRIT,"!WinSock startup ERROR %d", status);
	return (FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)
#define SOCKLIB_DESC NULL

#endif

static void update_clients(void)
{
	if(startup!=NULL && startup->clients!=NULL)
		startup->clients(startup->cbdata,active_clients+active_sendmail);
}

static void client_on(SOCKET sock, client_t* client, BOOL update)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(startup->cbdata,TRUE,sock,client,update);
}

static void client_off(SOCKET sock)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(startup->cbdata,FALSE,sock,NULL,FALSE);
}

static void thread_up(BOOL setuid)
{
	thread_count++;
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,TRUE,setuid);
}

static void thread_down(void)
{
	if(thread_count>0)
		thread_count--;
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,FALSE,FALSE);
}

SOCKET mail_open_socket(int type, const char* protocol)
{
	char	error[256];
	char	section[128];
	SOCKET	sock;

	sock=socket(AF_INET, type, IPPROTO_IP);
	if(sock!=INVALID_SOCKET && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(startup->cbdata,TRUE);
	if(sock!=INVALID_SOCKET) {
		SAFEPRINTF(section,"mail|%s",protocol);
		if(set_socket_options(&scfg, sock, section, error, sizeof(error)))
			lprintf(LOG_ERR,"%04d !ERROR %s",sock,error);

		sockets++;
#if 0 /*def _DEBUG */
		lprintf(LOG_DEBUG,"%04d Socket opened (%d sockets in use)",sock,sockets);
#endif
	}
	return(sock);
}

int mail_close_socket(SOCKET sock)
{
	int		result;

	if(sock==INVALID_SOCKET)
		return(-1);

	shutdown(sock,SHUT_RDWR);	/* required on Unix */
	result=closesocket(sock);
	if(startup!=NULL && startup->socket_open!=NULL)
		startup->socket_open(startup->cbdata,FALSE);
	sockets--;
	if(result!=0) {
		if(ERROR_VALUE!=ENOTSOCK)
			lprintf(LOG_ERR,"%04d !ERROR %d closing socket",sock, ERROR_VALUE);
	}
#if 0 /*def _DEBUG */
	else 
		lprintf(LOG_DEBUG,"%04d Socket closed (%d sockets in use)",sock,sockets);
#endif

	return(result);
}

static void status(char* str)
{
	if(startup!=NULL && startup->status!=NULL)
	    startup->status(startup->cbdata,str);
}

int sockprintf(SOCKET sock, char *fmt, ...)
{
	int		len;
	int		maxlen;
	int		result;
	va_list argptr;
	char	sbuf[1024];
	fd_set	socket_set;
	struct timeval tv;

    va_start(argptr,fmt);
    len=vsnprintf(sbuf,maxlen=sizeof(sbuf)-2,fmt,argptr);
    va_end(argptr);

	if(len<0 || len > maxlen) /* format error or output truncated */
		len=maxlen;
	if(startup->options&MAIL_OPT_DEBUG_TX)
		lprintf(LOG_DEBUG,"%04d TX: %.*s", sock, len, sbuf);
	memcpy(sbuf+len,"\r\n",2);
	len+=2;

	if(sock==INVALID_SOCKET) {
		lprintf(LOG_WARNING,"!INVALID SOCKET in call to sockprintf");
		return(0);
	}

	/* Check socket for writability (using select) */
	tv.tv_sec=300;
	tv.tv_usec=0;

	FD_ZERO(&socket_set);
	FD_SET(sock,&socket_set);

	if((result=select(sock+1,NULL,&socket_set,NULL,&tv))<1) {
		if(result==0)
			lprintf(LOG_NOTICE,"%04d !TIMEOUT selecting socket for send"
				,sock);
		else
			lprintf(LOG_NOTICE,"%04d !ERROR %d selecting socket for send"
				,sock, ERROR_VALUE);
		return(0);
	}

	while((result=sendsocket(sock,sbuf,len))!=len) {
		if(result==SOCKET_ERROR) {
			if(ERROR_VALUE==EWOULDBLOCK) {
				YIELD();
				continue;
			}
			if(ERROR_VALUE==ECONNRESET) 
				lprintf(LOG_NOTICE,"%04d Connection reset by peer on send",sock);
			else if(ERROR_VALUE==ECONNABORTED) 
				lprintf(LOG_NOTICE,"%04d Connection aborted by peer on send",sock);
			else
				lprintf(LOG_NOTICE,"%04d !ERROR %d sending on socket",sock,ERROR_VALUE);
			return(0);
		}
		lprintf(LOG_WARNING,"%04d !ERROR: short send on socket: %d instead of %d",sock,result,len);
	}
	return(len);
}

static time_t checktime(void)
{
	struct tm tm;

    memset(&tm,0,sizeof(tm));
    tm.tm_year=94;
    tm.tm_mday=1;
    return(mktime(&tm)-0x2D24BD00L);
}

static void recverror(SOCKET socket, int rd, int line)
{
	if(rd==0) 
		lprintf(LOG_NOTICE,"%04d Socket closed by peer on receive (line %d)"
			,socket, line);
	else if(rd==SOCKET_ERROR) {
		if(ERROR_VALUE==ECONNRESET) 
			lprintf(LOG_NOTICE,"%04d Connection reset by peer on receive (line %d)"
				,socket, line);
		else if(ERROR_VALUE==ECONNABORTED) 
			lprintf(LOG_NOTICE,"%04d Connection aborted by peer on receive (line %d)"
				,socket, line);
		else
			lprintf(LOG_NOTICE,"%04d !ERROR %d receiving on socket (line %d)"
				,socket, ERROR_VALUE, line);
	} else
		lprintf(LOG_WARNING,"%04d !ERROR: recv on socket returned unexpected value: %d (line %d)"
			,socket, rd, line);
}


static int sockreadline(SOCKET socket, char* buf, int len)
{
	char	ch;
	int		i,rd=0;
	fd_set	socket_set;
	struct	timeval	tv;
	time_t	start;

	buf[0]=0;

	start=time(NULL);

	if(socket==INVALID_SOCKET) {
		lprintf(LOG_WARNING,"!INVALID SOCKET in call to sockreadline");
		return(-1);
	}
	
	while(rd<len-1) {

		if(server_socket==INVALID_SOCKET || terminate_server) {
			lprintf(LOG_WARNING,"%04d !ABORTING sockreadline",socket);
			return(-1);
		}

		tv.tv_sec=startup->max_inactivity;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(socket,&socket_set);

		i=select(socket+1,&socket_set,NULL,NULL,&tv);

		if(i<1) {
			if(i==0) {
				if((time(NULL)-start)>startup->max_inactivity) {
					lprintf(LOG_WARNING,"%04d !SOCKET INACTIVE",socket);
					return(-1);
				}
				continue;
			}
			recverror(socket,i,__LINE__);
			return(-1);
		}
		i=recv(socket, &ch, 1, 0);
		if(i<1) {
			recverror(socket,i,__LINE__);
			return(-1);
		}
		if(ch=='\n' /* && rd>=1 */ ) { /* Mar-9-2003: terminate on sole LF */
			break;
		}	
		buf[rd++]=ch;
	}
	if(rd>0 && buf[rd-1]=='\r')
		rd--;
	buf[rd]=0;
	
	return(rd);
}

static BOOL sockgetrsp(SOCKET socket, char* rsp, char *buf, int len)
{
	int rd;

	while(1) {
		rd = sockreadline(socket, buf, len);
		if(rd<1) 
			return(FALSE);
		if(buf[3]=='-')	{ /* Multi-line response */
			if(startup->options&MAIL_OPT_DEBUG_RX_RSP) 
				lprintf(LOG_DEBUG,"%04d RX: %s",socket,buf);
			continue;
		}
		if(rsp!=NULL && strnicmp(buf,rsp,strlen(rsp))) {
			lprintf(LOG_WARNING,"%04d !INVALID RESPONSE: '%s' Expected: '%s'", socket, buf, rsp);
			return(FALSE);
		}
		break;
	}
	if(startup->options&MAIL_OPT_DEBUG_RX_RSP) 
		lprintf(LOG_DEBUG,"%04d RX: %s",socket,buf);
	return(TRUE);
}

#define MAX_LINE_LEN	1000

static ulong sockmimetext(SOCKET socket, smbmsg_t* msg, char* msgtxt, ulong maxlines
						  ,str_list_t file_list, char* mime_boundary)
{
	char		toaddr[256]="";
	char		fromaddr[256]="";
	char		fromhost[256];
	char		date[64];
	char*		p;
	char*		tp;
	char*		content_type=NULL;
	int			i;
	int			s;
	ulong		lines;

	/* HEADERS (in recommended order per RFC822 4.1) */
	if(!sockprintf(socket,"Date: %s",msgdate(msg->hdr.when_written,date)))
		return(0);

	if((p=smb_get_hfield(msg,RFC822FROM,NULL))!=NULL)
		s=sockprintf(socket,"From: %s",p);	/* use original RFC822 header field */
	else {
		if(msg->from_net.type==NET_QWK && msg->from_net.addr!=NULL)
			SAFEPRINTF2(fromaddr,"%s!%s"
				,(char*)msg->from_net.addr
				,usermailaddr(&scfg,fromhost,msg->from));
		else if(msg->from_net.type==NET_FIDO && msg->from_net.addr!=NULL)
			SAFECOPY(fromaddr,smb_faddrtoa((faddr_t *)msg->from_net.addr,NULL));
		else if(msg->from_net.type!=NET_NONE && msg->from_net.addr!=NULL)
			SAFECOPY(fromaddr,(char*)msg->from_net.addr);
		else 
			usermailaddr(&scfg,fromaddr,msg->from);
		if(fromaddr[0]=='<')
			s=sockprintf(socket,"From: \"%s\" %s",msg->from,fromaddr);
		else
			s=sockprintf(socket,"From: \"%s\" <%s>",msg->from,fromaddr);
	}
	if(!s)
		return(0);

	if(msg->from_org!=NULL || msg->from_net.type==NET_NONE)
		if(!sockprintf(socket,"Organization: %s"
			,msg->from_org==NULL ? scfg.sys_name : msg->from_org))
			return(0);

	if(!sockprintf(socket,"Subject: %s",msg->subj))
		return(0);

	if((p=smb_get_hfield(msg,RFC822TO,NULL))!=NULL)
		s=sockprintf(socket,"To: %s",p);	/* use original RFC822 header field */
	else {
		if(strchr(msg->to,'@')!=NULL || msg->to_net.addr==NULL)
			s=sockprintf(socket,"To: %s",msg->to);	/* Avoid double-@ */
		else if(msg->to_net.type==NET_INTERNET || msg->to_net.type==NET_QWK) {
			if(strchr((char*)msg->to_net.addr,'<')!=NULL)
				s=sockprintf(socket,"To: %s",(char*)msg->to_net.addr);
			else
				s=sockprintf(socket,"To: \"%s\" <%s>",msg->to,(char*)msg->to_net.addr);
		} else {
			usermailaddr(&scfg,toaddr,msg->to);
			s=sockprintf(socket,"To: \"%s\" <%s>",msg->to,toaddr);
		}
	}
	if(!s)
		return(0);
	if((p=smb_get_hfield(msg,RFC822REPLYTO,NULL))==NULL) {
		if(msg->replyto_net.type==NET_INTERNET)
			p=msg->replyto_net.addr;
		else if(msg->replyto!=NULL)
			p=msg->replyto;
	}
	if(p!=NULL)
		s=sockprintf(socket,"Reply-To: %s",p);	/* use original RFC822 header field */
	if(!s)
		return(0);
	if(!sockprintf(socket,"Message-ID: %s",get_msgid(&scfg,INVALID_SUB,msg)))
		return(0);
	if(msg->reply_id!=NULL)
		if(!sockprintf(socket,"In-Reply-To: %s",msg->reply_id))
			return(0);

    for(i=0;i<msg->total_hfields;i++) { 
		if(msg->hfield[i].type==RFC822HEADER) { 
			if(strnicmp((char*)msg->hfield_dat[i],"Content-Type:",13)==0)
				content_type=msg->hfield_dat[i];
			if(!sockprintf(socket,"%s",(char*)msg->hfield_dat[i]))
				return(0);
        }
    }
	/* Default MIME Content-Type for non-Internet messages */
	if(msg->from_net.type!=NET_INTERNET && content_type==NULL && startup->default_charset[0]) {
		/* No content-type specified, so assume IBM code-page 437 (full ex-ASCII) */
		sockprintf(socket,"Content-Type: text/plain; charset=%s", startup->default_charset);
		sockprintf(socket,"Content-Transfer-Encoding: 8bit");
	}

	if(strListCount(file_list)) {	/* File attachments */
        mimeheaders(socket,mime_boundary);
        sockprintf(socket,"");
        mimeblurb(socket,mime_boundary);
        sockprintf(socket,"");
        mimetextpartheader(socket,mime_boundary);
	}
	if(!sockprintf(socket,""))	/* Header Terminator */
		return(0);

	/* MESSAGE BODY */
	lines=0;
#if 0	/* This is now handled in smb_getmsgtxt() */
    for(i=0;i<msg->total_hfields;i++) {			/* delivery failure notification? */
		if(msg->hfield[i].type==SMTPSYSMSG || msg->hfield[i].type==SMB_COMMENT) { 
			if(!sockprintf(socket,"%s",(char*)msg->hfield_dat[i]))
				return(0);
			lines++;
		}
    }
#endif
	p=msgtxt;
	while(*p && lines<maxlines) {
		tp=strchr(p,'\n');
		if(tp) {
			if(tp-p>MAX_LINE_LEN)
				tp=p+MAX_LINE_LEN;
			*tp=0;
		}
		truncsp(p);	/* Takes care of '\r' or spaces */
		if(*p=='.')
			i=sockprintf(socket,".%.*s",MAX_LINE_LEN,p);
		else
			i=sockprintf(socket,"%.*s",MAX_LINE_LEN,p);
		if(!i)
			break;
		if(tp==NULL)
			break;
		p=tp+1;
		lines++;
		/* release time-slices every x lines */
		if(startup->lines_per_yield
			&& !(lines%startup->lines_per_yield))	
			YIELD();
	}
	if(file_list!=NULL) {
		for(i=0;file_list[i];i++) { 
			sockprintf(socket,"");
			lprintf(LOG_INFO,"%04u MIME Encoding and sending %s",socket,file_list[i]);
			if(!mimeattach(socket,mime_boundary,file_list[i]))
				lprintf(LOG_ERR,"%04u !ERROR opening/encoding/sending %s",socket,file_list[i]);
			else {
				endmime(socket,mime_boundary);
				if(msg->hdr.auxattr&MSG_KILLFILE)
					if(remove(file_list[i])!=0)
						lprintf(LOG_WARNING,"%04u !ERROR %d removing %s",socket,errno,file_list[i]);
			}
		}
	}
    sockprintf(socket,".");	/* End of text */
	return(lines);
}

static ulong sockmsgtxt(SOCKET socket, smbmsg_t* msg, char* msgtxt, ulong maxlines)
{
	char		filepath[MAX_PATH+1];
	ulong		retval;
	char*		boundary=NULL;
	unsigned	i;
	str_list_t	file_list=NULL;
	str_list_t	split;

	if(msg->hdr.auxattr&MSG_FILEATTACH) {

		boundary = mimegetboundary();
		file_list = strListInit();

		/* Parse header fields */
		for(i=0;i<msg->total_hfields;i++)
	        if(msg->hfield[i].type==FILEATTACH) 
				strListPush(&file_list,(char*)msg->hfield_dat[i]);

		/* Parse subject (if necessary) */
		if(!strListCount(file_list)) {	/* filename(s) stored in subject */
			split=strListSplitCopy(NULL,msg->subj," ");
			if(split!=NULL) {
				for(i=0;split[i];i++) {
					if(msg->idx.to!=0)
						SAFEPRINTF3(filepath,"%sfile/%04u.in/%s"
							,scfg.data_dir,msg->idx.to,getfname(truncsp(split[i])));
					else
						SAFEPRINTF3(filepath,"%sfile/%04u.out/%s"
							,scfg.data_dir,msg->idx.from,getfname(truncsp(split[i])));
					strListPush(&file_list,filepath);
				}
				strListFree(&split);
			}
		}
    }

	retval = sockmimetext(socket,msg,msgtxt,maxlines,file_list,boundary);

	strListFree(&file_list);

	if(boundary!=NULL)
		free(boundary);

	return(retval);
}

static u_long resolve_ip(char *inaddr)
{
	char*		p;
	char*		addr;
	char		buf[128];
	HOSTENT*	host;

	SAFECOPY(buf,inaddr);
	addr=buf;
	if(*addr=='[' && *(p=lastchar(addr))==']') { /* Support [ip_address] notation */
		addr++;
		*p=0;
	}

	if(*addr==0)
		return((u_long)INADDR_NONE);

	for(p=addr;*p;p++)
		if(*p!='.' && !isdigit(*p))
			break;
	if(!(*p))
		return(inet_addr(addr));

	if((host=gethostbyname(inaddr))==NULL) {
		lprintf(LOG_WARNING,"0000 !ERROR resolving hostname: %s",inaddr);
		return((u_long)INADDR_NONE);
	}
	return(*((ulong*)host->h_addr_list[0]));
}


static void pop3_thread(void* arg)
{
	char*		p;
	char		str[128];
	char		buf[512];
	char		host_name[128];
	char		host_ip[64];
	char		username[LEN_ALIAS+1];
	char		password[LEN_PASS+1];
	char		challenge[256];
	char		digest[MD5_DIGEST_SIZE];
	char*		response="";
	char*		msgtxt;
	int			i;
	int			rd;
	BOOL		activity=FALSE;
	BOOL		apop=FALSE;
	long		l;
	ulong		lines;
	ulong		lines_sent;
	long		msgs,msgnum;
	ulong		bytes;
	SOCKET		socket;
	HOSTENT*	host;
	smb_t		smb;
	smbmsg_t	msg;
	user_t		user;
	client_t	client;
	mail_t*		mail;
	pop3_t		pop3=*(pop3_t*)arg;

	SetThreadName("POP3 Thread");
	thread_up(TRUE /* setuid */);

	free(arg);

	socket=pop3.socket;

	if(startup->options&MAIL_OPT_DEBUG_POP3)
		lprintf(LOG_DEBUG,"%04d POP3 session thread started", socket);

#ifdef _WIN32
	if(startup->pop3_sound[0] && !(startup->options&MAIL_OPT_MUTE)) 
		PlaySound(startup->pop3_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	SAFECOPY(host_ip,inet_ntoa(pop3.client_addr.sin_addr));

	if(startup->options&MAIL_OPT_DEBUG_POP3)
		lprintf(LOG_INFO,"%04d POP3 connection accepted from: %s port %u"
			,socket, host_ip, ntohs(pop3.client_addr.sin_port));

	if(startup->options&MAIL_OPT_NO_HOST_LOOKUP)
		host=NULL;
	else
		host=gethostbyaddr((char *)&pop3.client_addr.sin_addr
			,sizeof(pop3.client_addr.sin_addr),AF_INET);

	if(host!=NULL && host->h_name!=NULL)
		SAFECOPY(host_name,host->h_name);
	else
		strcpy(host_name,"<no name>");

	if(startup->options&MAIL_OPT_DEBUG_POP3
		&& !(startup->options&MAIL_OPT_NO_HOST_LOOKUP)) {
		lprintf(LOG_INFO,"%04d POP3 Hostname: %s", socket, host_name);
		for(i=0;host!=NULL && host->h_aliases!=NULL && host->h_aliases[i]!=NULL;i++)
			lprintf(LOG_INFO,"%04d POP3 HostAlias: %s", socket, host->h_aliases[i]);
	}

	if(trashcan(&scfg,host_ip,"ip")) {
		lprintf(LOG_NOTICE,"%04d !POP3 BLOCKED CLIENT IP ADDRESS: %s"
			,socket, host_ip);
		sockprintf(socket,"-ERR Access denied.");
		mail_close_socket(socket);
		thread_down();
		return;
	}

	if(trashcan(&scfg,host_name,"host")) {
		lprintf(LOG_NOTICE,"%04d !POP3 BLOCKED CLIENT HOSTNAME: %s"
			,socket, host_name);
		sockprintf(socket,"-ERR Access denied.");
		mail_close_socket(socket);
		thread_down();
		return;
	}

	active_clients++, update_clients();

	/* Initialize client display */
	client.size=sizeof(client);
	client.time=time(NULL);
	SAFECOPY(client.addr,host_ip);
	SAFECOPY(client.host,host_name);
	client.port=ntohs(pop3.client_addr.sin_port);
	client.protocol="POP3";
	client.user="<unknown>";
	client_on(socket,&client,FALSE /* update */);

	SAFEPRINTF(str,"POP3: %s", host_ip);
	status(str);

	mail=NULL;

	do {
		memset(&smb,0,sizeof(smb));
		memset(&msg,0,sizeof(msg));
		memset(&user,0,sizeof(user));
		password[0]=0;

		srand(time(NULL) ^ (DWORD)GetCurrentThreadId());	/* seed random number generator */
		rand();	/* throw-away first result */
		safe_snprintf(challenge,sizeof(challenge),"<%x%x%lx%lx@%.128s>"
			,rand(),socket,(ulong)time(NULL),clock(),startup->host_name);

		sockprintf(socket,"+OK Synchronet POP3 Server %s-%s Ready %s"
			,revision,PLATFORM_DESC,challenge);

		/* Requires USER command first */
		for(i=3;i;i--) {
			if(!sockgetrsp(socket,NULL,buf,sizeof(buf)))
				break;
			if(!strnicmp(buf,"USER ",5))
				break;
			if(!strnicmp(buf,"APOP ",5)) {
				apop=TRUE;
				break;
			}
			sockprintf(socket,"-ERR USER or APOP command expected");
		}
		if(!i || buf[0]==0)	/* no USER or APOP command received */
			break;

		p=buf+5;
		SKIP_WHITESPACE(p);
		if(apop) {
			if((response=strrchr(p,' '))!=NULL)
				*(response++)=0;
			else
				response=p;
		}
		SAFECOPY(username,p);
		if(!apop) {
			sockprintf(socket,"+OK");
			if(!sockgetrsp(socket,"PASS ",buf,sizeof(buf))) {
				sockprintf(socket,"-ERR PASS command expected");
				break;
			}
			p=buf+5;
			SKIP_WHITESPACE(p);
			SAFECOPY(password,p);
		}
		user.number=matchuser(&scfg,username,FALSE /*sysop_alias*/);
		if(!user.number) {
			if(scfg.sys_misc&SM_ECHO_PW)
				lprintf(LOG_WARNING,"%04d !POP3 UNKNOWN USER: %s (password: %s)"
					,socket, username, password);
			else
				lprintf(LOG_WARNING,"%04d !POP3 UNKNOWN USER: %s"
					,socket, username);
			sockprintf(socket,pop_err);
			break;
		}
		if((i=getuserdat(&scfg, &user))!=0) {
			lprintf(LOG_ERR,"%04d !POP3 ERROR %d getting data on user (%s)"
				,socket, i, username);
			sockprintf(socket, pop_err);
			break;
		}
		if(user.misc&(DELETED|INACTIVE)) {
			lprintf(LOG_WARNING,"%04d !POP3 DELETED or INACTIVE user #%u (%s)"
				,socket, user.number, username);
			sockprintf(socket, pop_err);
			break;
		}
		if(apop) {
			strlwr(user.pass);	/* this is case-sensitive, so convert to lowercase */
			strcat(challenge,user.pass);
			MD5_calc(digest,challenge,strlen(challenge));
			MD5_hex(str,digest);
			if(strcmp(str,response)) {
				lprintf(LOG_WARNING,"%04d !POP3 %s FAILED APOP authentication"
					,socket,username);
#if 0
				lprintf(LOG_DEBUG,"%04d !POP3 digest data: %s",socket,challenge);
				lprintf(LOG_DEBUG,"%04d !POP3 calc digest: %s",socket,str);
				lprintf(LOG_DEBUG,"%04d !POP3 resp digest: %s",socket,response);
#endif
				sockprintf(socket,pop_err);
				break;
			}
		} else if(stricmp(password,user.pass)) {
			if(scfg.sys_misc&SM_ECHO_PW)
				lprintf(LOG_WARNING,"%04d !POP3 FAILED Password attempt for user %s: '%s' expected '%s'"
					,socket, username, password, user.pass);
			else
				lprintf(LOG_WARNING,"%04d !POP3 FAILED Password attempt for user %s"
					,socket, username);
			sockprintf(socket, pop_err);
			break;
		}
		putuserrec(&scfg,user.number,U_COMP,LEN_COMP,host_name);
		putuserrec(&scfg,user.number,U_NOTE,LEN_NOTE,host_ip);

		/* Update client display */
		client.user=user.alias;
		client_on(socket,&client,TRUE /* update */);

		if(startup->options&MAIL_OPT_DEBUG_POP3)		
			lprintf(LOG_INFO,"%04d POP3 %s logged in %s", socket, user.alias, apop ? "via APOP":"");
		SAFEPRINTF(str,"POP3: %s",user.alias);
		status(str);

		sprintf(smb.file,"%smail",scfg.data_dir);
		if(smb_islocked(&smb)) {
			lprintf(LOG_WARNING,"%04d !POP3 MAIL BASE LOCKED: %s",socket,smb.last_error);
			sockprintf(socket,"-ERR database locked, try again later");
			break;
		}
		smb.retry_time=scfg.smb_retry_time;
		smb.subnum=INVALID_SUB;
		if((i=smb_open(&smb))!=SMB_SUCCESS) {
			lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) opening %s",socket,i,smb.last_error,smb.file);
			sockprintf(socket,"-ERR %d opening %s",i,smb.file);
			break;
		}

		mail=loadmail(&smb,&msgs,user.number,MAIL_YOUR,0);

		for(l=bytes=0;l<msgs;l++) {
			msg.hdr.number=mail[l].number;
			if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
				lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
					,socket, i, smb.last_error);
				break;
			}
			if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
				lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
					,socket, i, smb.last_error, msg.hdr.number);
				break; 
			}
			i=smb_getmsghdr(&smb,&msg);
			smb_unlockmsghdr(&smb,&msg);
			if(i!=0) {
				lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
					,socket, i, smb.last_error, msg.hdr.number);
				break;
			}
			bytes+=smb_getmsgtxtlen(&msg);
			smb_freemsgmem(&msg);
		}

		if(l<msgs) {
			sockprintf(socket,"-ERR message #%d: %d (%s)"
				,mail[l].number,i,smb.last_error);
			break;
		}

		sockprintf(socket,"+OK %lu messages (%lu bytes)",msgs,bytes);

		while(1) {	/* TRANSACTION STATE */
			rd = sockreadline(socket, buf, sizeof(buf));
			if(rd<0) 
				break;
			truncsp(buf);
			if(startup->options&MAIL_OPT_DEBUG_POP3)
				lprintf(LOG_DEBUG,"%04d POP3 RX: %s", socket, buf);
			if(!stricmp(buf, "NOOP")) {
				sockprintf(socket,"+OK");
				continue;
			}
			if(!stricmp(buf, "QUIT")) {
				sockprintf(socket,"+OK");
				break;
			}
			if(!stricmp(buf, "STAT")) {
				sockprintf(socket,"+OK %lu %lu",msgs,bytes);
				continue;
			}
			if(!stricmp(buf, "RSET")) {
				if((i=smb_locksmbhdr(&smb))!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) locking message base"
						,socket, i, smb.last_error);
					sockprintf(socket,"-ERR %d locking message base",i);
					continue;
				}
				for(l=0;l<msgs;l++) {
					msg.hdr.number=mail[l].number;
					if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
							,socket, i, smb.last_error);
						break;
					}
					if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						break; 
					}
					if((i=smb_getmsghdr(&smb,&msg))!=SMB_SUCCESS) {
						smb_unlockmsghdr(&smb,&msg);
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						break;
					}
					msg.hdr.attr=mail[l].attr;
					if((i=smb_putmsg(&smb,&msg))!=SMB_SUCCESS)
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) updating message index"
							,socket, i, smb.last_error);
					smb_unlockmsghdr(&smb,&msg);
					smb_freemsgmem(&msg);
				}
				smb_unlocksmbhdr(&smb);

				if(l<msgs)
					sockprintf(socket,"-ERR %d messages reset (ERROR: %d)",l,i);
				else
					sockprintf(socket,"+OK %lu messages (%lu bytes)",msgs,bytes);
				continue;
			}
			if(!strnicmp(buf, "LIST",4) || !strnicmp(buf,"UIDL",4)) {
				p=buf+4;
				SKIP_WHITESPACE(p);
				if(isdigit(*p)) {
					msgnum=atol(p);
					if(msgnum<1 || msgnum>msgs) {
						lprintf(LOG_WARNING,"%04d !POP3 INVALID message #%ld"
							,socket, msgnum);
						sockprintf(socket,"-ERR no such message");
						continue;
					}
					msg.hdr.number=mail[msgnum-1].number;
					if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
							,socket, i, smb.last_error);
						sockprintf(socket,"-ERR %d getting message index",i);
						break;
					}
					if(msg.idx.attr&MSG_DELETE) {
						lprintf(LOG_WARNING,"%04d !POP3 ATTEMPT to list DELETED message"
							,socket);
						sockprintf(socket,"-ERR message deleted");
						continue;
					}
					if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						sockprintf(socket,"-ERR %d locking message header",i);
						continue; 
					}
					i=smb_getmsghdr(&smb,&msg);
					smb_unlockmsghdr(&smb,&msg);
					if(i!=0) {
						smb_freemsgmem(&msg);
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						sockprintf(socket,"-ERR %d getting message header",i);
						continue;
					}
					if(!strnicmp(buf, "LIST",4)) {
						sockprintf(socket,"+OK %lu %lu",msgnum,smb_getmsgtxtlen(&msg));
					} else /* UIDL */
						sockprintf(socket,"+OK %lu %lu",msgnum,msg.hdr.number);

					smb_freemsgmem(&msg);
					continue;
				}
				/* List ALL messages */
				sockprintf(socket,"+OK %lu messages (%lu bytes)",msgs,bytes);
				for(l=0;l<msgs;l++) {
					msg.hdr.number=mail[l].number;
					if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
							,socket, i, smb.last_error);
						break;
					}
					if(msg.idx.attr&MSG_DELETE) 
						continue;
					if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						break; 
					}
					i=smb_getmsghdr(&smb,&msg);
					smb_unlockmsghdr(&smb,&msg);
					if(i!=0) {
						smb_freemsgmem(&msg);
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
							,socket, i, smb.last_error, msg.hdr.number);
						break;
					}
					if(!strnicmp(buf, "LIST",4)) {
						sockprintf(socket,"%lu %lu",l+1,smb_getmsgtxtlen(&msg));
					} else /* UIDL */
						sockprintf(socket,"%lu %lu",l+1,msg.hdr.number);

					smb_freemsgmem(&msg);
				}			
				sockprintf(socket,".");
				continue;
			}
			activity=TRUE;
			if(!strnicmp(buf, "RETR ",5) || !strnicmp(buf,"TOP ",4)) {
				SAFEPRINTF(str,"POP3: %s", user.alias);
				status(str);

				lines=-1;
				p=buf+4;
				SKIP_WHITESPACE(p);
				msgnum=atol(p);

				if(!strnicmp(buf,"TOP ",4)) {
					SKIP_DIGIT(p);
					SKIP_WHITESPACE(p);
					lines=atol(p);
				}
				if(msgnum<1 || msgnum>msgs) {
					lprintf(LOG_WARNING,"%04d !POP3 %s ATTEMPTED to retrieve an INVALID message #%ld"
						,socket, user.alias, msgnum);
					sockprintf(socket,"-ERR no such message");
					continue;
				}
				msg.hdr.number=mail[msgnum-1].number;

				lprintf(LOG_INFO,"%04d POP3 %s retrieving message #%ld"
					,socket, user.alias, msg.hdr.number);

				if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
						,socket, i, smb.last_error);
					sockprintf(socket,"-ERR %d getting message index",i);
					continue;
				}
				if(msg.idx.attr&MSG_DELETE) {
					lprintf(LOG_WARNING,"%04d !POP3 ATTEMPT to retrieve DELETED message"
						,socket);
					sockprintf(socket,"-ERR message deleted");
					continue;
				}
				if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
					lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
						,socket, i, smb.last_error, msg.hdr.number);
					sockprintf(socket,"-ERR %d locking message header",i);
					continue; 
				}
				i=smb_getmsghdr(&smb,&msg);
				smb_unlockmsghdr(&smb,&msg);
				if(i!=0) {
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
						,socket, i, smb.last_error, msg.hdr.number);
					sockprintf(socket,"-ERR %d getting message header",i);
					continue;
				}

				if((msgtxt=smb_getmsgtxt(&smb,&msg,GETMSGTXT_ALL))==NULL) {
					smb_freemsgmem(&msg);
					lprintf(LOG_ERR,"%04d !POP3 ERROR (%s) retrieving message %lu text"
						,socket, smb.last_error, msg.hdr.number);
					sockprintf(socket,"-ERR retrieving message text");
					continue;
				}

				sockprintf(socket,"+OK message follows");
				lprintf(LOG_DEBUG,"%04d POP3 sending message text (%u bytes)"
					,socket,strlen(msgtxt));
				lines_sent=sockmsgtxt(socket,&msg,msgtxt,lines);
				/* if(startup->options&MAIL_OPT_DEBUG_POP3) */
				if(lines!=-1 && lines_sent<lines)	/* could send *more* lines */
					lprintf(LOG_ERR,"%04d !POP3 ERROR sending message text (sent %ld of %ld lines)"
						,socket,lines_sent,lines);
				else {
					lprintf(LOG_DEBUG,"%04d POP3 message transfer complete (%lu lines)"
						,socket,lines_sent);

					if((i=smb_locksmbhdr(&smb))!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) locking message base"
							,socket, i, smb.last_error);
					} else {
						if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
							lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
								,socket, i, smb.last_error);
						} else {
							msg.hdr.attr|=MSG_READ;
							msg.hdr.netattr|=MSG_SENT;

							if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) 
								lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
									,socket, i, smb.last_error, msg.hdr.number);
							if((i=smb_putmsg(&smb,&msg))!=SMB_SUCCESS)
								lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) marking message #%lu as read"
									,socket, i, smb.last_error, msg.hdr.number);
							smb_unlockmsghdr(&smb,&msg);
						}
						smb_unlocksmbhdr(&smb);
					}
				}
				smb_freemsgmem(&msg);
				smb_freemsgtxt(msgtxt);
				continue;
			}
			if(!strnicmp(buf, "DELE ",5)) {
				p=buf+5;
				SKIP_WHITESPACE(p);
				msgnum=atol(p);

				if(msgnum<1 || msgnum>msgs) {
					lprintf(LOG_WARNING,"%04d !POP3 %s ATTEMPTED to delete an INVALID message #%ld"
						,socket, user.alias, msgnum);
					sockprintf(socket,"-ERR no such message");
					continue;
				}
				msg.hdr.number=mail[msgnum-1].number;

				lprintf(LOG_INFO,"%04d POP3 %s deleting message #%ld"
					,socket, user.alias, msg.hdr.number);

				if((i=smb_locksmbhdr(&smb))!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) locking message base"
						,socket, i, smb.last_error);
					sockprintf(socket,"-ERR %d locking message base",i);
					continue;
				}
				if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
					smb_unlocksmbhdr(&smb);
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message index"
						,socket, i, smb.last_error);
					sockprintf(socket,"-ERR %d getting message index",i);
					continue;
				}
				if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
					smb_unlocksmbhdr(&smb);
					lprintf(LOG_WARNING,"%04d !POP3 ERROR %d (%s) locking message header #%lu"
						,socket, i, smb.last_error, msg.hdr.number);
					sockprintf(socket,"-ERR %d locking message header",i);
					continue; 
				}
				if((i=smb_getmsghdr(&smb,&msg))!=SMB_SUCCESS) {
					smb_unlockmsghdr(&smb,&msg);
					smb_unlocksmbhdr(&smb);
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) getting message header #%lu"
						,socket, i, smb.last_error, msg.hdr.number);
					sockprintf(socket,"-ERR %d getting message header",i);
					continue;
				}
				msg.hdr.attr|=MSG_DELETE;

				if((i=smb_putmsg(&smb,&msg))==SMB_SUCCESS && msg.hdr.auxattr&MSG_FILEATTACH)
					delfattach(&scfg,&msg);
				smb_unlockmsghdr(&smb,&msg);
				smb_unlocksmbhdr(&smb);
				smb_freemsgmem(&msg);
				if(i!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"%04d !POP3 ERROR %d (%s) marking message as read"
						, socket, i, smb.last_error);
					sockprintf(socket,"-ERR %d marking message for deletion",i);
					continue;
				}
				sockprintf(socket,"+OK");
				if(startup->options&MAIL_OPT_DEBUG_POP3)
					lprintf(LOG_INFO,"%04d POP3 message deleted", socket);
				continue;
			}
			lprintf(LOG_WARNING,"%04d !POP3 UNSUPPORTED COMMAND from %s: '%s'"
				,socket, user.alias, buf);
			sockprintf(socket,"-ERR UNSUPPORTED COMMAND: %s",buf);
		}
		if(user.number)
			logoutuserdat(&scfg,&user,time(NULL),client.time);

	} while(0);

	if(activity) 
		lprintf(LOG_INFO,"%04d POP3 %s logged out from port %u on %s [%s]"
			,socket, user.alias, ntohs(pop3.client_addr.sin_port), host_name, host_ip);

	status(STATUS_WFC);

	/* Free up resources here */
	if(mail!=NULL)
		freemail(mail);

	smb_freemsgmem(&msg);
	smb_close(&smb);

	if(active_clients)
		active_clients--, update_clients();
	client_off(socket);

	thread_down();
	if(startup->options&MAIL_OPT_DEBUG_POP3)
		lprintf(LOG_DEBUG,"%04d POP3 session thread terminated (%u threads remain, %lu clients served)"
			,socket, thread_count, served);

	/* Must be last */
	mail_close_socket(socket);
}

static ulong rblchk(SOCKET sock, DWORD mail_addr_n, const char* rbl_addr)
{
	char		name[256];
	DWORD		mail_addr;
	HOSTENT*	host;
	struct in_addr dnsbl_result;

	mail_addr=ntohl(mail_addr_n);
	safe_snprintf(name,sizeof(name),"%ld.%ld.%ld.%ld.%.128s"
		,mail_addr&0xff
		,(mail_addr>>8)&0xff
		,(mail_addr>>16)&0xff
		,(mail_addr>>24)&0xff
		,rbl_addr
		);

	if(startup->options&MAIL_OPT_DNSBL_DEBUG)
		lprintf(LOG_DEBUG,"%04d SMTP DNSBL Query: %s",sock,name);

	if((host=gethostbyname(name))==NULL)
		return(0);

	dnsbl_result.s_addr = *((ulong*)host->h_addr_list[0]);
	lprintf(LOG_INFO,"%04d SMTP DNSBL Query: %s resolved to: %s"
		,sock,name,inet_ntoa(dnsbl_result));

	return(dnsbl_result.s_addr);
}

static ulong dns_blacklisted(SOCKET sock, IN_ADDR addr, char* host_name, char* list, char* dnsbl_ip)
{
	char	fname[MAX_PATH+1];
	char	str[256];
	char*	p;
	char*	tp;
	FILE*	fp;
	ulong	found=0;

	sprintf(fname,"%sdnsbl_exempt.cfg",scfg.ctrl_dir);
	if(findstr(inet_ntoa(addr),fname))
		return(FALSE);
	if(findstr(host_name,fname))
		return(FALSE);

	sprintf(fname,"%sdns_blacklist.cfg", scfg.ctrl_dir);
	if((fp=fopen(fname,"r"))==NULL)
		return(FALSE);

	while(!feof(fp) && !found) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		truncsp(str);

		p=str;
		SKIP_WHITESPACE(p);
		if(*p==';' || *p==0) /* comment or blank line */
			continue;

		sprintf(list,"%.100s",p);

		/* terminate */
		tp = p;
		FIND_WHITESPACE(tp);
		*tp=0;	

		found = rblchk(sock, addr.s_addr, p);
	}
	fclose(fp);
	if(found)
		strcpy(dnsbl_ip, inet_ntoa(addr));

	return(found);
}


static BOOL chk_email_addr(SOCKET socket, char* p, char* host_name, char* host_ip
						   ,char* to, char* from, char* source)
{
	char	addr[64];
	char	tmp[128];

	SKIP_WHITESPACE(p);
	if(*p=='<') p++;		/* Skip '<' */
	SAFECOPY(addr,p);
	truncstr(addr,">( ");

	if(!trashcan(&scfg,addr,"email"))
		return(TRUE);

	lprintf(LOG_NOTICE,"%04d !SMTP BLOCKED %s e-mail address: %s"
		,socket, source, addr);
	SAFEPRINTF2(tmp,"Blocked %s e-mail address: %s", source, addr);
	spamlog(&scfg, "SMTP", "REFUSED", tmp, host_name, host_ip, to, from);

	return(FALSE);
}

static void signal_smtp_sem(void)
{
	int file;

	if(scfg.smtpmail_sem[0]==0) 
		return; /* do nothing */

	if((file=open(scfg.smtpmail_sem,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE))!=-1)
		close(file);
}

/*****************************************************************************/
/* Returns command line generated from instr with %c replacments             */
/*****************************************************************************/
static char* mailcmdstr(char* instr, char* msgpath, char* lstpath, char* errpath
						,char* host, char* ip, uint usernum
						,char* sender, char* sender_addr, char* reverse_path, char* cmd)
{
	char	str[1024];
    int		i,j,len;

    len=strlen(instr);
    for(i=j=0;i<len;i++) {
        if(instr[i]=='%') {
            i++;
            cmd[j]=0;
            switch(toupper(instr[i])) {
				case 'E':
					strcat(cmd,errpath);
					break;
				case 'H':
					strcat(cmd,host);
					break;
				case 'I':
					strcat(cmd,ip);
					break;
                case 'G':   /* Temp directory */
                    strcat(cmd,scfg.temp_dir);
                    break;
                case 'J':
                    strcat(cmd,scfg.data_dir);
                    break;
                case 'K':
                    strcat(cmd,scfg.ctrl_dir);
                    break;
				case 'L':
					strcat(cmd,lstpath);
					break;
				case 'F':
				case 'M':
					strcat(cmd,msgpath);
					break;
                case 'O':   /* SysOp */
                    strcat(cmd,scfg.sys_op);
                    break;
                case 'Q':   /* QWK ID */
                    strcat(cmd,scfg.sys_id);
                    break;
				case 'R':	/* reverse path */
					strcat(cmd,reverse_path);
					break;
				case 'S':	/* sender name */
					strcat(cmd,sender);
					break;
				case 'A':	/* sender address */
					strcat(cmd,sender_addr);
					break;
                case 'V':   /* Synchronet Version */
                    sprintf(str,"%s%c",VERSION,REVISION);
					strcat(cmd,str);
                    break;
                case 'Z':
                    strcat(cmd,scfg.text_dir);
                    break;
                case '!':   /* EXEC Directory */
                    strcat(cmd,scfg.exec_dir);
                    break;
                case '@':   /* EXEC Directory for DOS/OS2/Win32, blank for Unix */
#ifndef __unix__
                    strcat(cmd,scfg.exec_dir);
#endif
                    break;
                case '%':   /* %% for percent sign */
                    strcat(cmd,"%");
                    break;
				case '?':	/* Platform */
#ifdef __OS2__
					SAFECOPY(str,"OS2");
#else
					SAFECOPY(str,PLATFORM_DESC);
#endif
					strlwr(str);
					strcat(cmd,str);
					break;
				case 'U':	/* User number */
					sprintf(str,"%u",usernum);
					strcat(cmd,str);
					break;
                default:    /* unknown specification */
                    break; 
			}
            j=strlen(cmd); 
		}
        else
            cmd[j++]=instr[i]; 
	}
    cmd[j]=0;

    return(cmd);
}
#ifdef JAVASCRIPT

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	char*	warning;
	SOCKET*		sock;

	if((sock=(SOCKET*)JS_GetContextPrivate(cx))==NULL)
		return;

	if(report==NULL) {
		lprintf(LOG_ERR,"!JavaScript: %s", message);
		return;
    }

	if(report->filename)
		SAFEPRINTF(file," %s",report->filename);
	else
		file[0]=0;

	if(report->lineno)
		SAFEPRINTF(line," line %u",report->lineno);
	else
		line[0]=0;

	if(JSREPORT_IS_WARNING(report->flags)) {
		if(JSREPORT_IS_STRICT(report->flags))
			warning="strict warning";
		else
			warning="warning";
	} else
		warning="";

	lprintf(LOG_ERR,"%04d !JavaScript %s%s%s: %s"
		,*sock, warning ,file, line, message);
}

static JSBool
js_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i=0;
	int32		level=LOG_INFO;
    JSString*	str=NULL;
	SOCKET*		sock;

	if((sock=(SOCKET*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(JSVAL_IS_NUMBER(argv[i]))
		JS_ValueToInt32(cx,argv[i++],&level);

	for(; i<argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
			return(JS_FALSE);
		lprintf(level,"%04d JavaScript: %s",*sock,JS_GetStringBytes(str));
	}

	if(str==NULL)
		*rval = JSVAL_VOID;
	else
		*rval = STRING_TO_JSVAL(str);

    return(JS_TRUE);
}

static JSFunctionSpec js_global_functions[] = {
	{"write",			js_log,				0},
	{"writeln",			js_log,				0},
	{"print",			js_log,				0},
	{"log",				js_log,				0},
    {0}
};

static BOOL
js_mailproc(SOCKET sock, client_t* client, user_t* user
			,char* cmdline
			,char* msgtxt_fname, char* rcptlst_fname, char* proc_err_fname
			,char* sender, char* sender_addr, char* reverse_path)
{
	char*		p;
	char		fname[MAX_PATH+1];
	char		path[MAX_PATH+1];
	char		arg[MAX_PATH+1];
	BOOL		success=FALSE;
	JSRuntime*	js_runtime=NULL;
	JSContext*	js_cx=NULL;
	JSObject*	js_glob;
	JSObject*	argv;
	jsuint		argc;
	JSScript*	js_script;
	js_branch_t	js_branch;
	jsval		val;
	jsval		rval=JSVAL_VOID;

	ZERO_VAR(js_branch);

	SAFECOPY(fname,cmdline);
	truncstr(fname," \t");
	if(getfext(fname)==NULL) /* No extension specified, assume '.js' */
		strcat(fname,".js");

	SAFECOPY(path,fname);
	if(getfname(path)==path) /* No path specified, assume exec_dir */
		sprintf(path,"%s%s",scfg.exec_dir,fname);

	do {

		lprintf(LOG_DEBUG,"%04d JavaScript: Creating runtime: %lu bytes\n"
			,sock, startup->js.max_bytes);

		if((js_runtime = JS_NewRuntime(startup->js.max_bytes))==NULL)
			break;

		lprintf(LOG_DEBUG,"%04d JavaScript: Initializing context (stack: %lu bytes)\n"
			,sock, startup->js.cx_stack);

		if((js_cx = JS_NewContext(js_runtime, startup->js.cx_stack))==NULL)
			break;

		JS_SetErrorReporter(js_cx, js_ErrorReporter);

		JS_SetContextPrivate(js_cx, &sock);

		/* Global Object */
		if((js_glob=js_CreateGlobalObject(js_cx, &scfg, NULL))==NULL)
			break;

		if (!JS_DefineFunctions(js_cx, js_glob, js_global_functions))
			break;

		/* Internal JS Object */
		if(js_CreateInternalJsObject(js_cx, js_glob, &js_branch)==NULL)
			break;

		/* Client Object */
		if(js_CreateClientObject(js_cx, js_glob, "client", client, sock)==NULL)
			break;

		/* System Object */
		if(js_CreateSystemObject(js_cx, js_glob, &scfg, uptime, startup->host_name, SOCKLIB_DESC)==NULL)
			break;

		/* Socket Class */
		if(js_CreateSocketClass(js_cx, js_glob)==NULL)
			break;

		/* MsgBase Class */
		if(js_CreateMsgBaseClass(js_cx, js_glob, &scfg)==NULL)
			break;

		/* File Class */
		if(js_CreateFileClass(js_cx, js_glob)==NULL)
			break;

		/* User class */
		if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
			break;

		/* Area and "user" Objects */
		if(!js_CreateUserObjects(js_cx, js_glob, &scfg, user, NULL, NULL)) 
			break;

		/* Convert command-line to argv/argc */
		argv=JS_NewArrayObject(js_cx, 0, NULL);
		JS_DefineProperty(js_cx, js_glob, "argv", OBJECT_TO_JSVAL(argv)
			,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

		p=cmdline;
		FIND_WHITESPACE(p); 
		SKIP_WHITESPACE(p);
		for(argc=0;*p;argc++) {
			SAFECOPY(arg,p);
			truncstr(arg," \t");
			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,arg));
			if(!JS_SetElement(js_cx, argv, argc, &val))
				break;
			FIND_WHITESPACE(p);
			SKIP_WHITESPACE(p);
		}
		JS_DefineProperty(js_cx, js_glob, "argc", INT_TO_JSVAL(argc)
			,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

		/* Mailproc "API" filenames */
		JS_DefineProperty(js_cx, js_glob, "message_text_filename"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,msgtxt_fname))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		JS_DefineProperty(js_cx, js_glob, "recipient_list_filename"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,rcptlst_fname))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		JS_DefineProperty(js_cx, js_glob, "processing_error_filename"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,proc_err_fname))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		JS_DefineProperty(js_cx, js_glob, "sender_name"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,sender))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		JS_DefineProperty(js_cx, js_glob, "sender_address"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,sender_addr))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		JS_DefineProperty(js_cx, js_glob, "reverse_path"
			,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,reverse_path))
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		if((js_script=JS_CompileFile(js_cx, js_glob, path))==NULL)
			break;

		success=JS_ExecuteScript(js_cx, js_glob, js_script, &rval);

		js_EvalOnExit(js_cx, js_glob, &js_branch);

	} while(0);

	if(js_cx!=NULL)
		JS_DestroyContext(js_cx);
	if(js_runtime!=NULL)
		JS_DestroyRuntime(js_runtime);

	return(success);
}
#endif


static int parse_header_field(uchar* buf, smbmsg_t* msg, ushort* type)
{
	char*	p;
	char	field[128];
	int		len;
	ushort	nettype;

	if(buf[0]<=' ' && *type!=UNKNOWN) {	/* folded header, append to previous */
		p=buf;
		truncsp(p);
		smb_hfield_append_str(msg,*type,"\r\n");
		return smb_hfield_append_str(msg, *type, p);
	}

	if((p=strchr(buf,':'))==NULL)
		return smb_hfield_str(msg, *type=RFC822HEADER, buf);

	len=(ulong)p-(ulong)buf;
	if(len>sizeof(field)-1)
		len=sizeof(field)-1;
	sprintf(field,"%.*s",len,buf);
	truncsp(field);

	p++;	/* skip colon */
	SKIP_WHITESPACE(p);
	truncsp(p);

	if(!stricmp(field, "TO"))
		return smb_hfield_str(msg, *type=RFC822TO, p);

	if(!stricmp(field, "REPLY-TO")) {
		smb_hfield_str(msg, *type=RFC822REPLYTO, p);
		if(*p=='<')  {
			p++;
			truncstr(p,">");
		}
		nettype=NET_INTERNET;
		smb_hfield(msg, REPLYTONETTYPE, sizeof(nettype), &nettype);
		return smb_hfield_str(msg, *type=REPLYTONETADDR, p);
	}
	if(!stricmp(field, "FROM"))
		return smb_hfield_str(msg, *type=RFC822FROM, p);

	if(!stricmp(field, "ORGANIZATION"))
		return smb_hfield_str(msg, *type=SENDERORG, p);

	if(!stricmp(field, "DATE")) {
		msg->hdr.when_written=rfc822date(p);
		*type=UNKNOWN;
		return(0);
	}
	if(!stricmp(field, "MESSAGE-ID"))
		return smb_hfield_str(msg, *type=RFC822MSGID, p);

	if(!stricmp(field, "IN-REPLY-TO"))
		return smb_hfield_str(msg, *type=RFC822REPLYID, p);

	/* Fall-through */
	return smb_hfield_str(msg, *type=RFC822HEADER, buf);
}

static int chk_received_hdr(SOCKET socket,const char *buf,IN_ADDR *dnsbl_result, char *dnsbl, char *dnsbl_ip)
{
	char		host_name[128];
	IN_ADDR		check_addr;
	char		*fromstr;
	char		ip[16];
	char		*p;
	char		*p2;
	char		*last;

	fromstr=(char *)malloc(strlen(buf)+1);
	if(fromstr==NULL)
		return(0);
	strcpy(fromstr,buf);
	strlwr(fromstr);
	do {
		p=strstr(fromstr,"from ");
		if(p==NULL)
			break;
		p+=4;
		SKIP_WHITESPACE(p);
		if(*p==0)
			break;
		p2=host_name;
		for(;*p && !isspace(*p) && p2<host_name+126;p++)  {
			*p2++=*p;
		}
		*p2=0;
		p=strtok_r(fromstr,"[",&last);
		if(p==NULL)
			break;
		p=strtok_r(NULL,"]",&last);
		if(p==NULL)
			break;
		strncpy(ip,p,16);
		ip[15]=0;
		check_addr.s_addr = inet_addr(ip);
		if(startup->options&MAIL_OPT_DNSBL_DEBUG)
			lprintf(LOG_DEBUG,"%04d DEBUG checking %s (%s)",socket,host_name,ip);
		if((dnsbl_result->s_addr=dns_blacklisted(socket,check_addr,host_name,dnsbl,dnsbl_ip))!=0)
				lprintf(LOG_WARNING,"%04d !SMTP BLACKLISTED SERVER on %s: %s [%s] = %s"
					,socket, dnsbl, host_name, ip, inet_ntoa(*dnsbl_result));
	} while(0);
	free(fromstr);
	return(dnsbl_result->s_addr);
}

static void parse_mail_address(char* p
							   ,char* name, size_t name_len
							   ,char* addr, size_t addr_len)
{
	char*	tp;
	char	tmp[128];

	SKIP_WHITESPACE(p);

	/* Get the address */
	if((tp=strchr(p,'<'))!=NULL)
		tp++;
	else
		tp=p;
	SKIP_WHITESPACE(tp);
	sprintf(addr,"%.*s",addr_len,tp);
	truncstr(addr,">( ");

	SAFECOPY(tmp,p);
	p=tmp;
	/* Get the "name" (if possible) */
	if((tp=strchr(p,'('))!=NULL) {			/* name in parenthesis? */
		p=tp+1;
		tp=strchr(p,')');
	} else if((tp=strchr(p,'"'))!=NULL) {	/* name in quotes? */
		p=tp+1;
		tp=strchr(p,'"');
	} else if(*p=='<') {					/* address in brackets? */
		p++;
		tp=strchr(p,'>');
	} else									/* name, then address in brackets */
		tp=strchr(p,'<');
	if(tp) *tp=0;
	sprintf(name,"%.*s",name_len,p);
	truncsp(name);
}

static void smtp_thread(void* arg)
{
	int			i,j;
	int			rd;
	char		str[512];
	char		tmp[128];
	char		path[MAX_PATH+1];
	char		value[INI_MAX_VALUE_LEN];
	str_list_t	sec_list;
	char*		section;
	char		buf[1024],*p,*tp,*cp;
	char		hdrfield[512];
	char		alias_buf[128];
	char		name_alias_buf[128];
	char		reverse_path[128];
	char		date[64];
	char		qwkid[32];
	char		rcpt_name[128];
	char		rcpt_addr[128];
	char		sender[128];
	char		sender_addr[128];
	char		hello_name[128];
	char		user_name[LEN_ALIAS+1];
	char		user_pass[LEN_PASS+1];
	char		relay_list[MAX_PATH+1];
	char		domain_list[MAX_PATH+1];
	char		spam_bait[MAX_PATH+1];
	char		spam_block[MAX_PATH+1];
	char		host_name[128];
	char		host_ip[64];
	char		dnsbl[256];
	char		dnsbl_ip[64];
	char*		telegram_buf;
	char*		msgbuf;
	char		challenge[256];
	char		response[128];
	char		secret[64];
	char		md5_data[384];
	char		digest[MD5_DIGEST_SIZE];
	char		dest_host[128];
	char*		errmsg;
	ushort		dest_port;
	socklen_t	addr_len;
	ushort		hfield_type;
	ushort		nettype;
	ushort		agent;
	uint		usernum;
	ulong		lines=0;
	ulong		hdr_lines=0;
	ulong		hdr_len=0;
	ulong		length;
	ulong		badcmds=0;
	BOOL		esmtp=FALSE;
	BOOL		telegram=FALSE;
	BOOL		forward=FALSE;
	BOOL		no_forward=FALSE;
	BOOL		auth_login;
	BOOL		routed=FALSE;
	BOOL		dnsbl_recvhdr;
	BOOL		msg_handled;
	uint		subnum=INVALID_SUB;
	FILE*		msgtxt=NULL;
	char		msgtxt_fname[MAX_PATH+1];
	FILE*		rcptlst;
	char		rcptlst_fname[MAX_PATH+1];
	ushort		rcpt_count=0;
	FILE*		proc_err;
	char		proc_err_fname[MAX_PATH+1];
	char		session_id[MAX_PATH+1];
	FILE*		spy=NULL;
	SOCKET		socket;
	HOSTENT*	host;
	int			smb_error;
	smb_t		smb;
	smbmsg_t	msg;
	smbmsg_t	newmsg;
	user_t		user;
	user_t		relay_user;
	node_t		node;
	client_t	client;
	smtp_t		smtp=*(smtp_t*)arg;
	SOCKADDR_IN server_addr;
	IN_ADDR		dnsbl_result;
	BOOL*		mailproc_match;
	enum {
			 SMTP_STATE_INITIAL
			,SMTP_STATE_HELO
			,SMTP_STATE_MAIL_FROM
			,SMTP_STATE_RCPT_TO
			,SMTP_STATE_DATA_HEADER
			,SMTP_STATE_DATA_BODY

	} state = SMTP_STATE_INITIAL;

	enum {
			 SMTP_CMD_NONE
			,SMTP_CMD_MAIL
			,SMTP_CMD_SEND
			,SMTP_CMD_SOML
			,SMTP_CMD_SAML

	} cmd = SMTP_CMD_NONE;

	SetThreadName("SMTP Thread");
	thread_up(TRUE /* setuid */);

	free(arg);

	socket=smtp.socket;

	lprintf(LOG_DEBUG,"%04d SMTP RX Session thread started", socket);

#ifdef _WIN32
	if(startup->inbound_sound[0] && !(startup->options&MAIL_OPT_MUTE)) 
		PlaySound(startup->inbound_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	addr_len=sizeof(server_addr);
	if((i=getsockname(socket, (struct sockaddr *)&server_addr,&addr_len))!=0) {
		lprintf(LOG_ERR,"%04d !SMTP ERROR %d (%d) getting address/port"
			,socket, i, ERROR_VALUE);
		sockprintf(socket,sys_error);
		mail_close_socket(socket);
		thread_down();
		return;
	} 

	if((mailproc_match=alloca(sizeof(BOOL)*mailproc_count))==NULL) {
		lprintf(LOG_ERR,"%04d !SMTP ERROR allocating memory for mailproc_match", socket);
		sockprintf(socket,sys_error);
		mail_close_socket(socket);
		thread_down();
		return;
	} 
	memset(mailproc_match,FALSE,sizeof(BOOL)*mailproc_count);

	memset(&smb,0,sizeof(smb));
	memset(&msg,0,sizeof(msg));
	memset(&user,0,sizeof(user));
	memset(&relay_user,0,sizeof(relay_user));

	SAFECOPY(host_ip,inet_ntoa(smtp.client_addr.sin_addr));

	lprintf(LOG_INFO,"%04d SMTP Connection accepted from: %s port %u"
		, socket, host_ip, ntohs(smtp.client_addr.sin_port));

	if(startup->options&MAIL_OPT_NO_HOST_LOOKUP)
		host=NULL;
	else
		host=gethostbyaddr ((char *)&smtp.client_addr.sin_addr
			,sizeof(smtp.client_addr.sin_addr),AF_INET);

	if(host!=NULL && host->h_name!=NULL)
		SAFECOPY(host_name,host->h_name);
	else
		strcpy(host_name,"<no name>");

	if(!(startup->options&MAIL_OPT_NO_HOST_LOOKUP)) {
		lprintf(LOG_INFO,"%04d SMTP Hostname: %s", socket, host_name);
		for(i=0;host!=NULL && host->h_aliases!=NULL && host->h_aliases[i]!=NULL;i++)
			lprintf(LOG_INFO,"%04d SMTP HostAlias: %s", socket, host->h_aliases[i]);
#if 0
		if(host!=NULL) {
			ip=resolve_ip(host_name);
			if(ip!=smtp.client_addr.sin_addr.s_addr) {
				smtp.client_addr.sin_addr.s_addr=ip;
				lprintf(LOG_WARNING,"%04d !SMTP DNS/IP ADDRESS MISMATCH: %s vs %s"
					,socket, inet_ntoa(smtp.client_addr.sin_addr), host_ip);
				sockprintf(socket,"550 DNS and IP address mismatch");
				mail_close_socket(socket);
				thread_down();
				return;
			}
		}
#endif
	}

	SAFECOPY(hello_name,host_name);

	sprintf(spam_bait,"%sspambait.cfg",scfg.ctrl_dir);
	sprintf(spam_block,"%sspamblock.cfg",scfg.ctrl_dir);

	if(trashcan(&scfg,host_ip,"ip") || findstr(host_ip,spam_block)) {
		lprintf(LOG_NOTICE,"%04d !SMTP BLOCKED SERVER IP ADDRESS: %s"
			,socket, host_ip);
		sockprintf(socket,"550 Access denied.");
		mail_close_socket(socket);
		thread_down();
		return;
	}

	if(trashcan(&scfg,host_name,"host") || findstr(host_name,spam_block)) {
		lprintf(LOG_NOTICE,"%04d !SMTP BLOCKED SERVER HOSTNAME: %s"
			,socket, host_name);
		sockprintf(socket,"550 Access denied.");
		mail_close_socket(socket);
		thread_down();
		return;
	}

	active_clients++, update_clients();

	/*  SPAM Filters (mail-abuse.org) */
	dnsbl_result.s_addr = dns_blacklisted(socket,smtp.client_addr.sin_addr,host_name,dnsbl,dnsbl_ip);
	if(dnsbl_result.s_addr) {
		lprintf(LOG_WARNING,"%04d !SMTP BLACKLISTED SERVER on %s: %s [%s] = %s"
			,socket, dnsbl, host_name, dnsbl_ip, inet_ntoa(dnsbl_result));
		if(startup->options&MAIL_OPT_DNSBL_REFUSE) {
			SAFEPRINTF2(str,"Listed on %s as %s", dnsbl, inet_ntoa(dnsbl_result));
			spamlog(&scfg, "SMTP", "SESSION REFUSED", str, host_name, dnsbl_ip, NULL, NULL);
			sockprintf(socket
				,"550 Mail from %s refused due to listing at %s"
				,dnsbl_ip, dnsbl);
			mail_close_socket(socket);
			lprintf(LOG_WARNING,"%04d !SMTP REFUSED SESSION from blacklisted server"
				,socket);
			thread_down();
			if(active_clients)
				active_clients--, update_clients();
			return;
		}
	}

	sprintf(smb.file,"%smail",scfg.data_dir);
	if(smb_islocked(&smb)) {
		lprintf(LOG_WARNING,"%04d !SMTP MAIL BASE LOCKED: %s"
			,socket, smb.last_error);
		sockprintf(socket,sys_unavail);
		mail_close_socket(socket);
		thread_down();
		if(active_clients)
			active_clients--, update_clients();
		return;
	}

	srand(time(NULL) ^ (DWORD)GetCurrentThreadId());	/* seed random number generator */
	rand();	/* throw-away first result */
	SAFEPRINTF3(session_id,"%x%x%lx",socket,rand(),clock());

	SAFEPRINTF2(rcptlst_fname,"%sSBBS_SMTP.%s.lst", scfg.temp_dir, session_id);
	rcptlst=fopen(rcptlst_fname,"w+");
	if(rcptlst==NULL) {
		lprintf(LOG_ERR,"%04d !SMTP ERROR %d creating recipient list: %s"
			,socket, errno, rcptlst_fname);
		sockprintf(socket,sys_error);
		mail_close_socket(socket);
		thread_down();
		if(active_clients)
			active_clients--, update_clients();
		return;
	}

	if(trashcan(&scfg,host_name,"smtpspy") 
		|| trashcan(&scfg,host_ip,"smtpspy")) {
		sprintf(str,"%ssmtpspy.txt", scfg.logs_dir);
		spy=fopen(str,"a");
	}

	/* Initialize client display */
	client.size=sizeof(client);
	client.time=time(NULL);
	SAFECOPY(client.addr,host_ip);
	SAFECOPY(client.host,host_name);
	client.port=ntohs(smtp.client_addr.sin_port);
	client.protocol="SMTP";
	client.user="<unknown>";
	client_on(socket,&client,FALSE /* update */);

	SAFEPRINTF(str,"SMTP: %s",host_ip);
	status(str);

	sockprintf(socket,"220 %s Synchronet SMTP Server %s-%s Ready"
		,startup->host_name,revision,PLATFORM_DESC);
	while(1) {
		rd = sockreadline(socket, buf, sizeof(buf));
		if(rd<0) 
			break;
		truncsp(buf);
		if(spy!=NULL)
			fprintf(spy,"%s\n",buf);
		if(relay_user.number==0 && dnsbl_result.s_addr && startup->options&MAIL_OPT_DNSBL_THROTTLE)
			mswait(DNSBL_THROTTLE_VALUE);
		if(state>=SMTP_STATE_DATA_HEADER) {
			if(!strcmp(buf,".")) {

				state=SMTP_STATE_HELO;	/* RESET state machine here in case of error */
				cmd=SMTP_CMD_NONE;

				if(msgtxt==NULL) {
					lprintf(LOG_ERR,"%04d !SMTP NO MESSAGE TEXT FILE POINTER?", socket);
					sockprintf(socket,"554 No message text");
					continue;
				}

				if(ftell(msgtxt)<1) {
					lprintf(LOG_ERR,"%04d !SMTP INVALID MESSAGE LENGTH: %ld (%lu lines)"
						, socket, ftell(msgtxt), lines);
					sockprintf(socket,"554 No message text");
					continue;
				}

				lprintf(LOG_INFO,"%04d SMTP End of message (body: %lu lines, %lu bytes, header: %lu lines, %lu bytes)"
					, socket, lines, ftell(msgtxt)-hdr_len, hdr_lines, hdr_len);

				/* Twit-listing (sender's name and e-mail addresses) here */
				sprintf(path,"%stwitlist.cfg",scfg.ctrl_dir);
				if(fexist(path) && (findstr(sender,path) || findstr(sender_addr,path))) {
					lprintf(LOG_NOTICE,"%04d !SMTP FILTERING TWIT-LISTED SENDER: %s <%s>"
						,socket, sender, sender_addr);
					SAFEPRINTF2(tmp,"Twit-listed sender: %s <%s>", sender, sender_addr);
					spamlog(&scfg, "SMTP", "REFUSED", tmp, host_name, host_ip, rcpt_addr, reverse_path);
					sockprintf(socket, "554 Sender not allowed.");
					continue;
				}

				if(telegram==TRUE) {		/* Telegram */
					const char* head="\1n\1h\1cInstant Message\1n from \1h\1y";
					const char* tail="\1n:\r\n\1h";
					rewind(msgtxt);
					length=filelength(fileno(msgtxt));
					
					p=strchr(sender_addr,'@');
					if(p==NULL || resolve_ip(p+1)!=smtp.client_addr.sin_addr.s_addr) 
						/* Append real IP and hostname if different */
						safe_snprintf(str,sizeof(str),"%s%s\r\n\1w[\1n%s\1h] (\1n%s\1h)%s"
							,head,sender_addr,host_ip,host_name,tail);
					else
						safe_snprintf(str,sizeof(str),"%s%s%s",head,sender_addr,tail);
					
					if((telegram_buf=(char*)malloc(length+strlen(str)+1))==NULL) {
						lprintf(LOG_CRIT,"%04d !SMTP ERROR allocating %lu bytes of memory for telegram from %s"
							,socket,length+strlen(str)+1,sender_addr);
						sockprintf(socket, "452 Insufficient system storage");
						continue; 
					}
					strcpy(telegram_buf,str);	/* can't use SAFECOPY here */
					if(fread(telegram_buf+strlen(str),1,length,msgtxt)!=length) {
						lprintf(LOG_ERR,"%04d !SMTP ERROR reading %lu bytes from telegram file"
							,socket,length);
						sockprintf(socket, "452 Insufficient system storage");
						free(telegram_buf);
						continue; 
					}
					telegram_buf[length+strlen(str)]=0;	/* Need ASCIIZ */

					/* Send telegram to users */
					sec_list=iniReadSectionList(rcptlst,NULL);	/* Each section is a recipient */
					for(rcpt_count=0; sec_list!=NULL
						&& sec_list[rcpt_count]!=NULL 
						&& rcpt_count<startup->max_recipients; rcpt_count++) {

						section=sec_list[rcpt_count];

						SAFECOPY(rcpt_name,iniReadString(rcptlst,section	,smb_hfieldtype(RECIPIENT),"unknown",value));
						usernum=iniReadInteger(rcptlst,section				,smb_hfieldtype(RECIPIENTEXT),0);
						SAFECOPY(rcpt_addr,iniReadString(rcptlst,section	,smb_hfieldtype(RECIPIENTNETADDR),rcpt_name,value));

						if((i=putsmsg(&scfg,usernum,telegram_buf))==0)
							lprintf(LOG_INFO,"%04d SMTP Created telegram (%ld/%u bytes) from %s to %s <%s>"
								,socket, length, strlen(telegram_buf), sender_addr, rcpt_name, rcpt_addr);
						else
							lprintf(LOG_ERR,"%04d !SMTP ERROR %d creating telegram from %s to %s <%s>"
								,socket, i, sender_addr, rcpt_name, rcpt_addr);
					}
					iniFreeStringList(sec_list);
					free(telegram_buf);
					sockprintf(socket,ok_rsp);
					telegram=FALSE;
					continue;
				}

				fclose(msgtxt), msgtxt=NULL;
				fclose(rcptlst), rcptlst=NULL;

				/* External Mail Processing here */
				msg_handled=FALSE;
				if(mailproc_count) {
					SAFEPRINTF2(proc_err_fname,"%sSBBS_SMTP.%s.err", scfg.temp_dir, session_id);
					remove(proc_err_fname);

					for(i=0;i<mailproc_count;i++) {

						/* This processor is for specific recipients only and did not match */
						if(strListCount(mailproc_list[i].to) && !mailproc_match[i])
							continue;

						if(!mailproc_list[i].passthru)
							msg_handled=TRUE;

						mailcmdstr(mailproc_list[i].cmdline
							,msgtxt_fname, rcptlst_fname, proc_err_fname
							,host_name, host_ip, relay_user.number
							,sender, sender_addr, reverse_path, str);
						lprintf(LOG_DEBUG,"%04d SMTP Executing external process: %s"
							,socket, str);

						if(mailproc_list[i].native) {
							if((j=system(str))!=0)
								lprintf(LOG_WARNING,"%04d !SMTP system(%s) returned %d (errno: %d)"
									,socket, str, j, errno);
						} else  /* JavaScript */
							js_mailproc(socket, &client, &relay_user, str /* cmdline */
								,msgtxt_fname, rcptlst_fname, proc_err_fname
								,sender, sender_addr, reverse_path);
						if(flength(proc_err_fname)>0)
							break;
						if(!fexist(msgtxt_fname) || !fexist(rcptlst_fname))
							break;
					}
					if(flength(proc_err_fname)>0 
						&& (proc_err=fopen(proc_err_fname,"r"))!=NULL) {
						while(!feof(proc_err)) {
							if(!fgets(str,sizeof(str),proc_err))
								break;
							truncsp(str);
							lprintf(LOG_WARNING,"%04d !SMTP external process error: %s", socket, str);
							i=atoi(str);
							if(i>=100 && i<1000)
								sockprintf(socket,"%s", str);
							else
								sockprintf(socket,"554%c%s"
									,ftell(proc_err)<filelength(fileno(proc_err)) ? '-' : ' '
									,str);
						}
						fclose(proc_err);
						msg_handled=TRUE;
					}
					else if(!fexist(msgtxt_fname) || !fexist(rcptlst_fname)) {
						lprintf(LOG_WARNING,"%04d SMTP External process removed %s file"
							,socket, fexist(msgtxt_fname)==FALSE ? "message text" : "recipient list");
						sockprintf(socket,ok_rsp);
						msg_handled=TRUE;
					}
					else if(msg_handled)
						sockprintf(socket,ok_rsp);
					remove(proc_err_fname);	/* Remove error file here */
				}

				/* Re-open files */
				/* We must do this before continuing for handled msgs */
				/* to prevent freopen(NULL) and orphaned temp files */
				if((rcptlst=fopen(rcptlst_fname,fexist(rcptlst_fname) ? "r":"w+"))==NULL) {
					lprintf(LOG_ERR,"%04d !SMTP ERROR %d re-opening recipient list: %s"
						,socket, errno, rcptlst_fname);
					if(!msg_handled)
						sockprintf(socket,sys_error);
					continue;
				}
			
				if(msg_handled) {
					lprintf(LOG_NOTICE,"%04d SMTP Message handled by external mail processor"
						,socket);
					continue;
				}

				if((msgtxt=fopen(msgtxt_fname,"rb"))==NULL) {
					lprintf(LOG_ERR,"%04d !SMTP ERROR %d re-opening message file: %s"
						,socket, errno, msgtxt_fname);
					sockprintf(socket,sys_error);
					continue;
				}

				/* Initialize message header */
				smb_freemsgmem(&msg);
				memset(&msg,0,sizeof(smbmsg_t));		

				/* Parse message header here */
				hfield_type=UNKNOWN;
				smb_error=SMB_SUCCESS; /* no SMB error */
				errmsg="452 Insufficient system storage";
				while(!feof(msgtxt)) {
					if(!fgets(buf,sizeof(buf),msgtxt))
						break;
					truncsp(buf);
					if(buf[0]==0)	/* blank line marks end of header */
						break;

					if(!strnicmp(buf, "SUBJECT:",8)) {
						p=buf+8;
						SKIP_WHITESPACE(p);
						if(relay_user.number==0	&& dnsbl_result.s_addr && startup->dnsbl_tag[0]
							&& !(startup->options&MAIL_OPT_DNSBL_IGNORE)) {
							safe_snprintf(str,sizeof(str),"%.*s: %.*s"
								,(int)sizeof(str)/2, startup->dnsbl_tag
								,(int)sizeof(str)/2, p);
							p=str;
							lprintf(LOG_WARNING,"%04d !SMTP TAGGED MAIL SUBJECT from blacklisted server with: %s"
								,socket, startup->dnsbl_tag);
						}
						smb_hfield_str(&msg, SUBJECT, p);
						continue;
					}
					if(!strnicmp(buf, "FROM:", 5)
						&& !chk_email_addr(socket,buf+5,host_name,host_ip,rcpt_addr,reverse_path,"FROM")) {
						errmsg="554 Sender not allowed.";
						smb_error=SMB_FAILURE;
						break;
					}
					if(!strnicmp(buf, "TO:", 3)
						&& !chk_email_addr(socket,buf+3,host_name,host_ip,rcpt_addr,reverse_path,"TO")) {
						errmsg="550 Unknown user.";
						smb_error=SMB_FAILURE;
						break;
					}
					if((smb_error=parse_header_field(buf,&msg,&hfield_type))!=SMB_SUCCESS) {
						if(smb_error==SMB_ERR_HDR_LEN)
							lprintf(LOG_WARNING,"%04d !SMTP MESSAGE HEADER EXCEEDS %u BYTES"
								,socket, SMB_MAX_HDR_LEN);
						else
							lprintf(LOG_ERR,"%04d !SMTP ERROR %d adding header field: %s"
								,socket, smb_error, buf);
						break;
					}
				}
				if(smb_error!=SMB_SUCCESS) {	/* SMB Error */
					sockprintf(socket, errmsg);
					continue;
				}
				if((p=smb_get_hfield(&msg, RFC822TO, NULL))!=NULL) {
					if(*p=='<')	p++;
					SAFECOPY(rcpt_name,p);
					truncstr(rcpt_name,">");
				}
				if((p=smb_get_hfield(&msg, RFC822FROM, NULL))!=NULL) {
					parse_mail_address(p 
						,sender		,sizeof(sender)-1
						,sender_addr,sizeof(sender_addr)-1);
				}

				/* SPAM Filtering/Logging */
				if(relay_user.number==0 && msg.subj!=NULL && trashcan(&scfg,msg.subj,"subject")) {
					lprintf(LOG_WARNING,"%04d !SMTP BLOCKED SUBJECT (%s) from: %s"
						,socket, msg.subj, reverse_path);
					SAFEPRINTF2(tmp,"Blocked subject (%s) from: %s"
						,msg.subj, reverse_path);
					spamlog(&scfg, "SMTP", "REFUSED"
						,tmp, host_name, host_ip, rcpt_addr, reverse_path);
					sockprintf(socket, "554 Subject not allowed.");
					continue;
				}
				dnsbl_recvhdr=FALSE;
				if(startup->options&MAIL_OPT_DNSBL_CHKRECVHDRS)  {
					for(i=0;!dnsbl_result.s_addr && i<msg.total_hfields;i++)  {
						if(msg.hfield[i].type == RFC822HEADER
							&& strnicmp(msg.hfield_dat[i],"Received:",9)==0)  {
							if(chk_received_hdr(socket,msg.hfield_dat[i],&dnsbl_result,dnsbl,dnsbl_ip)) {
								dnsbl_recvhdr=TRUE;
								break;
							}
						}
					}
				}
				if(relay_user.number==0 && dnsbl_result.s_addr) {
					if(startup->options&MAIL_OPT_DNSBL_IGNORE) {
						lprintf(LOG_WARNING,"%04d !SMTP IGNORED MAIL from blacklisted server"
							,socket);
						SAFEPRINTF2(str,"Listed on %s as %s", dnsbl, inet_ntoa(dnsbl_result));
						spamlog(&scfg, "SMTP", "IGNORED"
							,str, host_name, dnsbl_ip, rcpt_addr, reverse_path);
						/* pretend we received it */
						sockprintf(socket,ok_rsp);
						continue;
					}
					/* tag message as spam */
					if(startup->dnsbl_hdr[0]) {
						safe_snprintf(str,sizeof(str),"%s: %s is listed on %s as %s"
							,startup->dnsbl_hdr, dnsbl_ip
							,dnsbl, inet_ntoa(dnsbl_result));
						smb_hfield_str(&msg, RFC822HEADER, str);
						lprintf(LOG_WARNING,"%04d !SMTP TAGGED MAIL HEADER from blacklisted server with: %s"
							,socket, startup->dnsbl_hdr);
					}
					if(startup->dnsbl_hdr[0] || startup->dnsbl_tag[0]) {
						SAFEPRINTF2(str,"Listed on %s as %s", dnsbl, inet_ntoa(dnsbl_result));
						spamlog(&scfg, "SMTP", "TAGGED", str, host_name, dnsbl_ip, rcpt_addr, reverse_path);
					}
				}
				if(dnsbl_recvhdr)			/* DNSBL-listed IP found in Received header? */
					dnsbl_result.s_addr=0;	/* Reset DNSBL look-up result between messages */

				if(sender[0]==0) {
					lprintf(LOG_WARNING,"%04d !SMTP MISSING mail header 'FROM' field", socket);
					sockprintf(socket, "554 Mail header missing 'FROM' field");
					subnum=INVALID_SUB;
					continue;
				}
				nettype=NET_INTERNET;
				smb_hfield_str(&msg, SMTPREVERSEPATH, reverse_path);
				smb_hfield_str(&msg, SENDER, sender);
				smb_hfield(&msg, SENDERNETTYPE, sizeof(nettype), &nettype);
				smb_hfield_str(&msg, SENDERNETADDR, sender_addr);
				if(msg.subj==NULL)
					smb_hfield(&msg, SUBJECT, 0, NULL);

				length=filelength(fileno(msgtxt))-ftell(msgtxt);

				if(startup->max_msg_size && length>startup->max_msg_size) {
					lprintf(LOG_WARNING,"%04d !SMTP message size (%lu) exceeds maximum: %lu bytes"
						,socket,length,startup->max_msg_size);
					sockprintf(socket, "552 Message size (%lu) exceeds maximum: %lu bytes"
						,length,startup->max_msg_size);
					subnum=INVALID_SUB;
					continue;
				}

				if((msgbuf=(char*)malloc(length+1))==NULL) {
					lprintf(LOG_CRIT,"%04d !SMTP ERROR allocating %d bytes of memory"
						,socket,length+1);
					sockprintf(socket, "452 Insufficient system storage");
					subnum=INVALID_SUB;
					continue;
				}
				fread(msgbuf,length,1,msgtxt);
				msgbuf[length]=0;	/* ASCIIZ */

				/* Do external JavaScript processing here? */

				if(subnum!=INVALID_SUB) {	/* Message Base */
					if(relay_user.number==0)
						memset(&relay_user,0,sizeof(relay_user));

					if(!chk_ar(&scfg,scfg.grp[scfg.sub[subnum]->grp]->ar, &relay_user)
						|| !chk_ar(&scfg,scfg.sub[subnum]->ar, &relay_user)
						|| !chk_ar(&scfg,scfg.sub[subnum]->post_ar, &relay_user)) {
						lprintf(LOG_WARNING,"%04d !SMTP %s has insufficient access to post on %s"
							,socket, sender_addr, scfg.sub[subnum]->sname);
						sockprintf(socket,"550 Insufficient access");
						subnum=INVALID_SUB;
						continue;
					}

					if(rcpt_name[0]==0)
						strcpy(rcpt_name,"All");
					smb_hfield_str(&msg, RECIPIENT, rcpt_name);

					smb.subnum=subnum;
					if((i=savemsg(&scfg, &smb, &msg, &client, msgbuf))!=SMB_SUCCESS) {
						lprintf(LOG_WARNING,"%04d !SMTP ERROR %d (%s) saving message"
							,socket,i,smb.last_error);
						sockprintf(socket, "452 ERROR %d (%s) saving message"
							,i,smb.last_error);
					} else {
						lprintf(LOG_INFO,"%04d SMTP %s posted a message on %s"
							,socket, sender_addr, scfg.sub[subnum]->sname);
						sockprintf(socket,ok_rsp);
						signal_smtp_sem();
					}
					free(msgbuf);
					smb_close(&smb);
					subnum=INVALID_SUB;
					continue;
				}

				/* E-mail */
				smb.subnum=INVALID_SUB;
				i=savemsg(&scfg, &smb, &msg, &client, msgbuf);
				free(msgbuf);
				if(i!=SMB_SUCCESS) {
					smb_close(&smb);
					lprintf(LOG_ERR,"%04d !SMTP ERROR %d (%s) saving message"
						,socket,i,smb.last_error);
					sockprintf(socket, "452 ERROR %d (%s) saving message"
						,i,smb.last_error);
					continue;
				}

				sec_list=iniReadSectionList(rcptlst,NULL);	/* Each section is a recipient */
				for(rcpt_count=0; sec_list!=NULL
					&& sec_list[rcpt_count]!=NULL 
					&& rcpt_count<startup->max_recipients; rcpt_count++) {
				
					section=sec_list[rcpt_count];

					SAFECOPY(rcpt_name,iniReadString(rcptlst,section	,smb_hfieldtype(RECIPIENT),"unknown",value));
					usernum=iniReadInteger(rcptlst,section				,smb_hfieldtype(RECIPIENTEXT),0);
					agent=iniReadShortInt(rcptlst,section				,smb_hfieldtype(RECIPIENTAGENT),AGENT_PERSON);
					nettype=iniReadShortInt(rcptlst,section				,smb_hfieldtype(RECIPIENTNETTYPE),NET_NONE);
					sprintf(str,"#%u",usernum);
					SAFECOPY(rcpt_addr,iniReadString(rcptlst,section	,smb_hfieldtype(RECIPIENTNETADDR),str,value));

					if(nettype==NET_NONE /* Local destination */ && usernum==0) {
						lprintf(LOG_ERR,"%04d !SMTP can't deliver mail to user #0"
							,socket);
						break;
					}

					if((i=smb_copymsgmem(&smb,&newmsg,&msg))!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !SMTP ERROR %d (%s) copying message"
							,socket, i, smb.last_error);
						break;
					}

					snprintf(hdrfield,sizeof(hdrfield),
						"Received: from %s (%s [%s])\r\n"
						"          by %s [%s] (Synchronet Mail Server %s-%s) with %s\r\n"
						"          for %s; %s"
						,host_name,hello_name,host_ip
						,startup->host_name,inet_ntoa(server_addr.sin_addr)
						,revision,PLATFORM_DESC
						,esmtp ? "ESMTP" : "SMTP"
						,rcpt_name,msgdate(msg.hdr.when_imported,date));
					smb_hfield_str(&newmsg, RFC822HEADER, hdrfield);

					smb_hfield_str(&newmsg, RECIPIENT, rcpt_name);

					if(usernum) {	/* Local destination or QWKnet routed */
						/* This is required for fixsmb to be able to rebuild the index */
						sprintf(str,"%u",usernum);
						smb_hfield_str(&newmsg, RECIPIENTEXT, str);
					}
					if(nettype!=NET_NONE) {
						smb_hfield(&newmsg, RECIPIENTNETTYPE, sizeof(nettype), &nettype);
						smb_hfield_str(&newmsg, RECIPIENTNETADDR, rcpt_addr);
					}
					if(agent!=newmsg.to_agent)
						smb_hfield(&newmsg, RECIPIENTAGENT, sizeof(agent), &agent);

					i=smb_addmsghdr(&smb,&newmsg,SMB_SELFPACK);
					smb_freemsgmem(&newmsg);
					if(i!=SMB_SUCCESS) {
						lprintf(LOG_ERR,"%04d !SMTP ERROR %d (%s) adding message header"
							,socket, i, smb.last_error);
						break;
					}
					lprintf(LOG_INFO,"%04d SMTP Created message #%ld from %s to %s <%s>"
						,socket, newmsg.hdr.number, sender, rcpt_name, rcpt_addr);
					if(!(startup->options&MAIL_OPT_NO_NOTIFY) && usernum
						&& !dnsbl_recvhdr && !dnsbl_result.s_addr) {
						safe_snprintf(str,sizeof(str)
							,"\7\1n\1hOn %.24s\r\n\1m%s \1n\1msent you e-mail from: "
							"\1h%s\1n\r\n"
							,timestr(&scfg,(time_t*)&newmsg.hdr.when_imported.time,tmp)
							,sender,sender_addr);
						if(!newmsg.idx.to) {	/* Forwarding */
							strcat(str,"\1mand it was automatically forwarded to: \1h");
							strcat(str,user.netmail);
							strcat(str,"\1n\r\n");
						}
						putsmsg(&scfg, usernum, str);
					}
				}
				iniFreeStringList(sec_list);
				if(rcpt_count<1) {
					smb_freemsg_dfields(&smb,&msg,SMB_ALL_REFS);
					sockprintf(socket, "452 Insufficient system storage");
				}
				else {
					if(rcpt_count>1)
						smb_incmsg_dfields(&smb,&msg,(ushort)(rcpt_count-1));
					sockprintf(socket,ok_rsp);
					signal_smtp_sem();
				}
#if 0 /* This shouldn't be necessary here */
				smb_close_da(&smb);
#endif
				smb_close(&smb);
				continue;
			}
			if(buf[0]==0 && state==SMTP_STATE_DATA_HEADER) {	
				state=SMTP_STATE_DATA_BODY;	/* Null line separates header and body */
				lines=0;
				if(msgtxt!=NULL) {
					fprintf(msgtxt, "\r\n");
					hdr_len=ftell(msgtxt);
				}
				continue;
			}
			if(state==SMTP_STATE_DATA_BODY) {
				p=buf;
				if(*p=='.') p++;	/* Transparency (RFC821 4.5.2) */
				if(msgtxt!=NULL) 
					fprintf(msgtxt, "%s\r\n", p);
				lines++;
				/* release time-slices every x lines */
				if(startup->lines_per_yield &&
					!(lines%startup->lines_per_yield))	
					YIELD();
				continue;
			}
			/* RFC822 Header parsing */
			if(startup->options&MAIL_OPT_DEBUG_RX_HEADER)
				lprintf(LOG_DEBUG,"%04d SMTP %s",socket, buf);

			if(!strnicmp(buf, "FROM:", 5))
				parse_mail_address(buf+5
					,sender,		sizeof(sender)-1
					,sender_addr,	sizeof(sender_addr)-1);

			if(msgtxt!=NULL) 
				fprintf(msgtxt, "%s\r\n", buf);
			hdr_lines++;
			continue;
		}
		strip_ctrl(buf);
		lprintf(LOG_DEBUG,"%04d SMTP RX: %s", socket, buf);
		if(!strnicmp(buf,"HELO",4)) {
			p=buf+4;
			SKIP_WHITESPACE(p);
			SAFECOPY(hello_name,p);
			sockprintf(socket,"250 %s",startup->host_name);
			esmtp=FALSE;
			state=SMTP_STATE_HELO;
			cmd=SMTP_CMD_NONE;
			telegram=FALSE;
			subnum=INVALID_SUB;
			continue;
		}
		if(!strnicmp(buf,"EHLO",4)) {
			p=buf+4;
			SKIP_WHITESPACE(p);
			SAFECOPY(hello_name,p);
			sockprintf(socket,"250-%s",startup->host_name);
			sockprintf(socket,"250 AUTH PLAIN LOGIN CRAM-MD5");
			esmtp=TRUE;
			state=SMTP_STATE_HELO;
			cmd=SMTP_CMD_NONE;
			telegram=FALSE;
			subnum=INVALID_SUB;
			continue;
		}
		if((auth_login=(stricmp(buf,"AUTH LOGIN")==0))==TRUE 
			|| stricmp(buf,"AUTH PLAIN")==0) {
			if(auth_login)
				sockprintf(socket,"334 VXNlcm5hbWU6");	/* Base64-encoded "Username:" */
			else
				sockprintf(socket,"334 Username:");
			if((rd=sockreadline(socket, buf, sizeof(buf)))<1) {
				sockprintf(socket,badarg_rsp);
				continue;
			}
			if(startup->options&MAIL_OPT_DEBUG_RX_RSP) 
				lprintf(LOG_DEBUG,"%04d RX: %s",socket,buf);
			if(auth_login) {
				if(b64_decode(user_name,sizeof(user_name),buf,rd)<1) {
					sockprintf(socket,badarg_rsp);
					continue;
				}
			} else
				SAFECOPY(user_name,buf);

			if(auth_login)
				sockprintf(socket,"334 UGFzc3dvcmQ6");	/* Base64-encoded "Password:" */
			else
				sockprintf(socket,"334 Password:");
			if((rd=sockreadline(socket, buf, sizeof(buf)))<1) {
				sockprintf(socket,badarg_rsp);
				continue;
			}
			if(startup->options&MAIL_OPT_DEBUG_RX_RSP) 
				lprintf(LOG_DEBUG,"%04d RX: %s",socket,buf);
			if(auth_login) {
				if(b64_decode(user_pass,sizeof(user_pass),buf,rd)<1) {
					sockprintf(socket,badarg_rsp);
					continue;
				}
			} else
				SAFECOPY(user_pass,buf);
			if((relay_user.number=matchuser(&scfg,user_name,FALSE))==0) {
				if(scfg.sys_misc&SM_ECHO_PW)
					lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN USER: %s (password: %s)"
						,socket, user_name, user_pass);
				else
					lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN USER: %s"
						,socket, user_name);
				sockprintf(socket,badauth_rsp);
				continue;
			}
			if((i=getuserdat(&scfg, &relay_user))!=0) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d getting data on user (%s)"
					,socket, i, user_name);
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				continue;
			}
			if(relay_user.misc&(DELETED|INACTIVE)) {
				lprintf(LOG_WARNING,"%04d !SMTP DELETED or INACTIVE user #%u (%s)"
					,socket, relay_user.number, user_name);
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				break;
			}
			if(stricmp(user_pass,relay_user.pass)) {
				if(scfg.sys_misc&SM_ECHO_PW)
					lprintf(LOG_WARNING,"%04d !SMTP FAILED Password attempt for user %s: '%s' expected '%s'"
						,socket, user_name, user_pass, relay_user.pass);
				else
					lprintf(LOG_WARNING,"%04d !SMTP FAILED Password attempt for user %s"
						,socket, user_name);
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				break;
			}
			/* Update client display */
			client.user=relay_user.alias;
			client_on(socket,&client,TRUE /* update */);

			lprintf(LOG_INFO,"%04d SMTP %s authenticated using %s authentication"
				,socket,relay_user.alias,auth_login ? "LOGIN" : "PLAIN");
			sockprintf(socket,auth_ok);
			continue;
		}
		if(!stricmp(buf,"AUTH CRAM-MD5")) {
			safe_snprintf(challenge,sizeof(challenge),"<%x%x%lx%lx@%s>"
				,rand(),socket,(ulong)time(NULL),clock(),startup->host_name);
#if 0
			lprintf(LOG_DEBUG,"%04d SMTP CRAM-MD5 challenge: %s"
				,socket,challenge);
#endif
			b64_encode(str,sizeof(str),challenge,0);
			sockprintf(socket,"334 %s",str);
			if((rd=sockreadline(socket, buf, sizeof(buf)))<1) {
				sockprintf(socket,badarg_rsp);
				continue;
			}
			if(startup->options&MAIL_OPT_DEBUG_RX_RSP) 
				lprintf(LOG_DEBUG,"%04d RX: %s",socket,buf);

			if(b64_decode(response,sizeof(response),buf,rd)<1) {
				sockprintf(socket,badarg_rsp);
				continue;
			}
#if 0
			lprintf(LOG_DEBUG,"%04d SMTP CRAM-MD5 response: %s"
				,socket,response);
#endif
			if((p=strrchr(response,' '))!=NULL)
				*(p++)=0;
			else
				p=response;
			SAFECOPY(user_name,response);
			if((relay_user.number=matchuser(&scfg,user_name,FALSE))==0) {
				lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN USER: %s"
					,socket, user_name);
				sockprintf(socket,badauth_rsp);
				continue;
			}
			if((i=getuserdat(&scfg, &relay_user))!=0) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d getting data on user (%s)"
					,socket, i, user_name);
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				continue;
			}
			if(relay_user.misc&(DELETED|INACTIVE)) {
				lprintf(LOG_WARNING,"%04d !SMTP DELETED or INACTIVE user #%u (%s)"
					,socket, relay_user.number, user_name);
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				continue;
			}
			/* Calculate correct response */
			memset(secret,0,sizeof(secret));
			SAFECOPY(secret,relay_user.pass);
			strlwr(secret);	/* this is case sensitive, so convert to lowercase first */
			for(i=0;i<sizeof(secret);i++)
				md5_data[i]=secret[i]^0x36;	/* ipad */
			strcpy(md5_data+i,challenge);
			MD5_calc(digest,md5_data,sizeof(secret)+strlen(challenge));
			for(i=0;i<sizeof(secret);i++)
				md5_data[i]=secret[i]^0x5c;	/* opad */
			memcpy(md5_data+i,digest,sizeof(digest));
			MD5_calc(digest,md5_data,sizeof(secret)+sizeof(digest));
			MD5_hex(str,digest);
			if(strcmp(p,str)) {
				lprintf(LOG_WARNING,"%04d !SMTP %s FAILED CRAM-MD5 authentication"
					,socket,relay_user.alias);
#if 0
				lprintf(LOG_DEBUG,"%04d !SMTP calc digest: %s"
					,socket,str);
				lprintf(LOG_DEBUG,"%04d !SMTP resp digest: %s"
					,socket,p);
#endif
				sockprintf(socket,badauth_rsp);
				relay_user.number=0;
				continue;
			}
			/* Update client display */
			client.user=relay_user.alias;
			client_on(socket,&client,TRUE /* update */);

			lprintf(LOG_INFO,"%04d SMTP %s authenticated using CRAM-MD5 authentication"
				,socket,relay_user.alias);
			sockprintf(socket,auth_ok);
			continue;
		}
		if(!strnicmp(buf,"AUTH",4)) {
			sockprintf(socket,"504 Unrecognized authentication type.");
			continue;
		}
		if(!stricmp(buf,"QUIT")) {
			sockprintf(socket,"221 %s Service closing transmission channel",startup->host_name);
			break;
		} 
		if(!stricmp(buf,"NOOP")) {
			sockprintf(socket, ok_rsp);
			badcmds=0;
			continue;
		}
		if(state<SMTP_STATE_HELO) {
			/* RFC 821 4.1.1 "The first command in a session must be the HELO command." */
			lprintf(LOG_WARNING,"%04d !SMTP MISSING 'HELO' command",socket);
			sockprintf(socket, badseq_rsp);
			continue;
		}
		if(!stricmp(buf,"TURN")) {
			sockprintf(socket,"502 command not supported");
			badcmds=0;
			continue;
		}
		if(!stricmp(buf,"RSET")) {
			smb_freemsgmem(&msg);
			memset(&msg,0,sizeof(smbmsg_t));		/* Initialize message header */
			reverse_path[0]=0;
			state=SMTP_STATE_HELO;
			cmd=SMTP_CMD_NONE;
			telegram=FALSE;
			subnum=INVALID_SUB;

			/* reset recipient list */
			if((rcptlst=freopen(rcptlst_fname,"w+",rcptlst))==NULL) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d re-opening %s"
					,socket, errno, rcptlst_fname);
				sockprintf(socket,sys_error);
				break;
			}
			rcpt_count=0;

			sockprintf(socket,ok_rsp);
			badcmds=0;
			lprintf(LOG_INFO,"%04d SMTP Session reset",socket);
			continue;
		}
		if(!strnicmp(buf,"MAIL FROM:",10)
			|| !strnicmp(buf,"SEND FROM:",10)	/* Send a Message (Telegram) to a local ONLINE user */
			|| !strnicmp(buf,"SOML FROM:",10)	/* Send OR Mail a Message to a local user */
			|| !strnicmp(buf,"SAML FROM:",10)	/* Send AND Mail a Message to a local user */
			) {
			p=buf+10;
			if(!chk_email_addr(socket,p,host_name,host_ip,NULL,NULL,"REVERSE PATH")) {
				sockprintf(socket, "554 Sender not allowed.");
				break;
			}
			SKIP_WHITESPACE(p);
			SAFECOPY(reverse_path,p);

			/* Update client display */
			if(relay_user.number==0) {
				client.user=reverse_path;
				client_on(socket,&client,TRUE /* update */);
			}

			/* Setup state */
			state=SMTP_STATE_MAIL_FROM;
			if(!strnicmp(buf,"MAIL FROM:",10))
				cmd=SMTP_CMD_MAIL;
			else if(!strnicmp(buf,"SEND FROM:",10))
				cmd=SMTP_CMD_SEND;
			else if(!strnicmp(buf,"SOML FROM:",10))
				cmd=SMTP_CMD_SOML;
			else if(!strnicmp(buf,"SAML FROM:",10))
				cmd=SMTP_CMD_SAML;

			/* reset recipient list */
			if((rcptlst=freopen(rcptlst_fname,"w+",rcptlst))==NULL) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d re-opening %s"
					,socket, errno, rcptlst_fname);
				sockprintf(socket,sys_error);
				break;
			}
			rcpt_count=0;
			sockprintf(socket,ok_rsp);
			badcmds=0;
			continue;
		}

#if 0	/* No one uses this command */
		if(!strnicmp(buf,"VRFY",4)) {
			p=buf+4;
			SKIP_WHITESPACE(p);
			if(*p==0) {
				sockprintf(socket,"550 No user specified.");
				continue;
			}
#endif

		/* Add to Recipient list */
		if(!strnicmp(buf,"RCPT TO:",8)) {

			if(state<SMTP_STATE_MAIL_FROM) {
				lprintf(LOG_WARNING,"%04d !SMTP MISSING 'MAIL' command",socket);
				sockprintf(socket, badseq_rsp);
				continue;
			}

			if(spy==NULL && trashcan(&scfg,reverse_path,"smtpspy")) {
				sprintf(str,"%ssmtpspy.txt", scfg.logs_dir);
				spy=fopen(str,"a");
			}

			p=buf+8;
			SKIP_WHITESPACE(p);
			SAFECOPY(str,p);
			p=strrchr(str,'<');
			if(p==NULL)
				p=str;
			else
				p++;

			truncstr(str,">");	/* was truncating at space too */

			routed=FALSE;
			forward=FALSE;
			no_forward=FALSE;
			if(!strnicmp(p,FORWARD,strlen(FORWARD))) {
				forward=TRUE;		/* force forward to user's netmail address */
				p+=strlen(FORWARD);
			}
			if(!strnicmp(p,NO_FORWARD,strlen(NO_FORWARD))) {
				no_forward=TRUE;	/* do not forward to user's netmail address */
				p+=strlen(NO_FORWARD);
			}

			if(*p==0) {
				lprintf(LOG_WARNING,"%04d !SMTP NO RECIPIENT SPECIFIED"
					,socket);
				sockprintf(socket, "500 No recipient specified");
				continue;
			}

			rcpt_name[0]=0;
			SAFECOPY(rcpt_addr,p);

			/* Check recipient counter */
			if(rcpt_count>=startup->max_recipients) {
				lprintf(LOG_WARNING,"%04d !SMTP MAXIMUM RECIPIENTS (%d) REACHED"
					,socket, startup->max_recipients);
				sprintf(tmp,"Maximum recipient count (%d)",startup->max_recipients);
				spamlog(&scfg, "SMTP", "REFUSED", tmp
					,host_name, host_ip, rcpt_addr, reverse_path);
				sockprintf(socket, "552 Too many recipients");
				continue;
			}

			/* Check for SPAM bait recipient */
			if(findstr(rcpt_addr,spam_bait)) {
				SAFEPRINTF(str,"SPAM BAIT (%s) taken", rcpt_addr);
				lprintf(LOG_WARNING,"%04d !SMTP %s by: %s"
					,socket, str, reverse_path);
				strcpy(tmp,"REFUSED");
				if(dnsbl_result.s_addr==0)	{ /* Don't double-filter */
					lprintf(LOG_WARNING,"%04d !BLOCKING IP ADDRESS: %s in %s", socket, host_ip, spam_block);
					filter_ip(&scfg, "SMTP", str, host_name, host_ip, reverse_path, spam_block);
					strcat(tmp," and BLOCKED");
				}
				spamlog(&scfg, "SMTP", tmp, "Attempted recipient in SPAM BAIT list"
					,host_name, host_ip, rcpt_addr, reverse_path);
				break;
			}

			/* Check for blocked recipients */
			if(!chk_email_addr(socket,rcpt_addr,host_name,host_ip,rcpt_addr,reverse_path,"RECIPIENT")) {
				sockprintf(socket, "550 Unknown User:%s", buf+8);
				continue;
			}

			if(relay_user.number==0 && dnsbl_result.s_addr && startup->options&MAIL_OPT_DNSBL_BADUSER) {
				lprintf(LOG_WARNING,"%04d !SMTP REFUSED MAIL from blacklisted server"
					,socket);
				SAFEPRINTF2(str,"Listed on %s as %s", dnsbl, inet_ntoa(dnsbl_result));
				spamlog(&scfg, "SMTP", "REFUSED", str, host_name, host_ip, rcpt_addr, reverse_path);
				sockprintf(socket
					,"550 Mail from %s refused due to listing at %s"
					,host_ip, dnsbl);
				continue;
			}

			/* Check for full address aliases */
			p=alias(&scfg,p,alias_buf);
			if(p==alias_buf) 
				lprintf(LOG_INFO,"%04d SMTP ADDRESS ALIAS: %s",socket,p);

			tp=strrchr(p,'@');
			if(cmd==SMTP_CMD_MAIL && tp!=NULL) {
				
				/* RELAY */
				dest_port=server_addr.sin_port;
				SAFECOPY(dest_host,tp+1);
				cp=strrchr(dest_host,':');
				if(cp!=NULL) {
					*cp=0;
					dest_port=atoi(cp+1);
				}
				sprintf(domain_list,"%sdomains.cfg",scfg.ctrl_dir);
				if((stricmp(dest_host,scfg.sys_inetaddr)!=0
						&& stricmp(dest_host,startup->host_name)!=0
						&& resolve_ip(dest_host)!=server_addr.sin_addr.s_addr
						&& findstr(dest_host,domain_list)==FALSE)
					|| dest_port!=server_addr.sin_port) {

					sprintf(relay_list,"%srelay.cfg",scfg.ctrl_dir);
					if(relay_user.number==0 /* not authenticated, search for IP */
						&& startup->options&MAIL_OPT_SMTP_AUTH_VIA_IP) { 
						relay_user.number=userdatdupe(&scfg, 0, U_NOTE, LEN_NOTE, host_ip, FALSE);
						if(relay_user.number) {
							getuserdat(&scfg,&relay_user);
							if(relay_user.laston < time(NULL)-(60*60))	/* logon in past hour? */
								relay_user.number=0;
						}
					} else
						getuserdat(&scfg,&relay_user);
					if(p!=alias_buf /* forced relay by alias */ &&
						(!(startup->options&MAIL_OPT_ALLOW_RELAY)
							|| relay_user.number==0
							|| relay_user.rest&(FLAG('G')|FLAG('M'))) &&
						!findstr(host_name,relay_list) && 
						!findstr(host_ip,relay_list)) {
						lprintf(LOG_WARNING,"%04d !SMTP ILLEGAL RELAY ATTEMPT from %s [%s] to %s"
							,socket, reverse_path, host_ip, p);
						SAFEPRINTF(tmp,"Relay attempt to: %s", p);
						spamlog(&scfg, "SMTP", "REFUSED", tmp, host_name, host_ip, rcpt_addr, reverse_path);
						if(startup->options&MAIL_OPT_ALLOW_RELAY)
							sockprintf(socket, "553 Relaying through this server "
							"requires authentication.  "
							"Please authenticate before sending.");
						else
							sockprintf(socket, "550 Relay not allowed.");
						break;
					}

					if(relay_user.number==0)
						SAFECOPY(relay_user.alias,"Unknown User");

					lprintf(LOG_INFO,"%04d SMTP %s relaying to external mail service: %s"
						,socket, relay_user.alias, tp+1);

					fprintf(rcptlst,"[%u]\n",rcpt_count++);
					fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENT),rcpt_addr);
					fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTNETTYPE),NET_INTERNET);
					fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENTNETADDR),p);

					sockprintf(socket,ok_rsp);
					state=SMTP_STATE_RCPT_TO;
					continue;
				}
			}
			if(tp!=NULL)
				*tp=0;	/* truncate at '@' */

			tp=strchr(p,'!');	/* Routed QWKnet mail in <qwkid!user@host> format */
			if(tp!=NULL) {
				*(tp++)=0;
				SKIP_CHAR(tp,'"');				/* Skip '"' */
				truncstr(tp,"\"");				/* Strip '"' */
				SAFECOPY(rcpt_addr,tp);
				routed=TRUE;
			}

			FIND_ALPHANUMERIC(p);				/* Skip '<' or '"' */
			truncstr(p,"\"");	

			p=alias(&scfg,p,name_alias_buf);
			if(p==name_alias_buf) 
				lprintf(LOG_INFO,"%04d SMTP NAME ALIAS: %s",socket,p);
		
			if(!strnicmp(p,"sub:",4)) {		/* Post on a sub-board */
				p+=4;
				for(i=0;i<scfg.total_subs;i++)
					if(!stricmp(p,scfg.sub[i]->code))
						break;
				if(i>=scfg.total_subs) {
					lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN SUB-BOARD: %s", socket, p);
					sockprintf(socket, "550 Unknown sub-board: %s", p);
					continue;
				}
				subnum=i;
				sockprintf(socket,ok_rsp);
				state=SMTP_STATE_RCPT_TO;
				rcpt_count++;
				continue;
			}

			memset(mailproc_match,FALSE,sizeof(BOOL)*mailproc_count);
			for(i=0;i<mailproc_count;i++) {
				if(mailproc_list[i].to!=NULL) {
					for(j=0;mailproc_list[i].to[j]!=NULL;j++) {
						if(stricmp(p,mailproc_list[i].to[j])==0) {
							mailproc_match[i]=TRUE;
							if(!mailproc_list[i].passthru)
								break;
						}
					}
					if(mailproc_list[i].to[j]!=NULL)
						break;
				}
			}
			/* destined for an external mail processor */
			if(i<mailproc_count) {
				fprintf(rcptlst,"[%u]\n",rcpt_count++);
				fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENT),rcpt_addr);
#if 0	/* should we fall-through to the sysop account? */
				fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTEXT),1);
#endif
				lprintf(LOG_INFO,"%04d SMTP Routing mail for %s to External Mail Processor: %s"
					,socket, rcpt_addr, mailproc_list[i].cmdline);
				sockprintf(socket,ok_rsp);
				state=SMTP_STATE_RCPT_TO;
				continue;
			}

			usernum=0;	/* unknown user at this point */

			if(routed) {
				SAFECOPY(qwkid,p);
				truncstr(qwkid,"/");
				/* Search QWKnet hub-IDs for route destination */
				for(i=0;i<scfg.total_qhubs;i++) {
					if(!stricmp(qwkid,scfg.qhub[i]->id))
						break;
				}
				if(i<scfg.total_qhubs) {	/* found matching QWKnet Hub */

					lprintf(LOG_INFO,"%04d SMTP Routing mail for %s <%s> to QWKnet Hub: %s"
						,socket, rcpt_addr, p, scfg.qhub[i]->id);

					fprintf(rcptlst,"[%u]\n",rcpt_count++);
					fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENT),rcpt_addr);
					fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTNETTYPE),NET_QWK);
					fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENTNETADDR),p);

					sockprintf(socket,ok_rsp);
					state=SMTP_STATE_RCPT_TO;
					continue;
				}
			}

			if((p==name_alias_buf || startup->options&MAIL_OPT_ALLOW_RX_BY_NUMBER)
				&& isdigit(*p)) {
				usernum=atoi(p);			/* RX by user number */
				/* verify usernum */
				username(&scfg,usernum,str);
				if(!str[0] || !stricmp(str,"DELETED USER"))
					usernum=0;
				p=str;
			} else {
				/* RX by "user alias", "user.alias" or "user_alias" */
				usernum=matchuser(&scfg,p,startup->options&MAIL_OPT_ALLOW_SYSOP_ALIASES);	

				if(!usernum) { /* RX by "real name", "real.name", or "sysop.alias" */
					
					/* convert "user.name" to "user name" */
					SAFECOPY(rcpt_name,p);
					for(tp=rcpt_name;*tp;tp++)	
						if(*tp=='.') *tp=' ';

					if(!stricmp(p,scfg.sys_op) || !stricmp(rcpt_name,scfg.sys_op))
						usernum=1;			/* RX by "sysop.alias" */

					if(!usernum && scfg.msg_misc&MM_REALNAME)	/* RX by "real name" */
						usernum=userdatdupe(&scfg, 0, U_NAME, LEN_NAME, p, FALSE);

					if(!usernum && scfg.msg_misc&MM_REALNAME)	/* RX by "real.name" */
						usernum=userdatdupe(&scfg, 0, U_NAME, LEN_NAME, rcpt_name, FALSE);
				}
			}
			if(!usernum && startup->default_user[0]) {
				usernum=matchuser(&scfg,startup->default_user,TRUE /* sysop_alias */);
				if(usernum)
					lprintf(LOG_INFO,"%04d SMTP Forwarding mail for UNKNOWN USER to default user: %s #%u"
						,socket,startup->default_user,usernum);
				else
					lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN DEFAULT USER: %s"
						,socket,startup->default_user);
			}

			if(!usernum) {
				lprintf(LOG_WARNING,"%04d !SMTP UNKNOWN USER:%s", socket, buf+8);
				sockprintf(socket, "550 Unknown User:%s", buf+8);
				continue;
			}
			user.number=usernum;
			if((i=getuserdat(&scfg, &user))!=0) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d getting data on user #%u (%s)"
					,socket, i, usernum, p);
				sockprintf(socket, "550 Unknown User:%s", buf+8);
				continue;
			}
			if(user.misc&(DELETED|INACTIVE)) {
				lprintf(LOG_WARNING,"%04d !SMTP DELETED or INACTIVE user #%u (%s)"
					,socket, usernum, p);
				sockprintf(socket, "550 Unknown User:%s", buf+8);
				continue;
			}
			if(cmd==SMTP_CMD_SEND) { /* Check if user online */
				for(i=0;i<scfg.sys_nodes;i++) {
					getnodedat(&scfg, i+1, &node, 0);
					if(node.status==NODE_INUSE && node.useron==user.number
						&& !(node.misc&NODE_POFF))
						break;
				}
				if(i>=scfg.sys_nodes) {
					lprintf(LOG_WARNING,"%04d !Attempt to send telegram to unavailable user #%u (%s)"
						,socket, user.number, user.alias);
					sockprintf(socket,"450 User unavailable");
					continue;
				}
			}
			if(cmd==SMTP_CMD_MAIL) {
#if 0	/* implement later */
				if(useron.etoday>=cfg.level_emailperday[useron.level]
					&& !(useron.rest&FLAG('Q'))) {
					bputs(text[TooManyEmailsToday]);
					continue; 
				}
#endif
			} else
				telegram=TRUE;

			fprintf(rcptlst,"[%u]\n",rcpt_count++);
			fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENT),rcpt_addr);
			fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTEXT),user.number);


			/* Forward to Internet */
			tp=strrchr(user.netmail,'@');
			if(!telegram
				&& !routed
				&& !no_forward
				&& scfg.sys_misc&SM_FWDTONET 
				&& (user.misc&NETMAIL || forward)
				&& tp!=NULL && smb_netaddr_type(user.netmail)==NET_INTERNET 
				&& !strstr(tp,scfg.sys_inetaddr)) {
				lprintf(LOG_INFO,"%04d SMTP Forwarding to: %s"
					,socket, user.netmail);
				fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTNETTYPE),NET_INTERNET);
				fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENTNETADDR),user.netmail);
				sockprintf(socket,"251 User not local; will forward to %s", user.netmail);
			} else { /* Local (no-forward) */
				if(routed) { /* QWKnet */
					fprintf(rcptlst,"%s=%u\n",smb_hfieldtype(RECIPIENTNETTYPE),NET_QWK);
					fprintf(rcptlst,"%s=%s\n",smb_hfieldtype(RECIPIENTNETADDR),user.alias);
				}						
				sockprintf(socket,ok_rsp);
			}
			state=SMTP_STATE_RCPT_TO;
			continue;
		}
		/* Message Data (header and body) */
		if(!strnicmp(buf,"DATA",4)) {
			if(state<SMTP_STATE_RCPT_TO) {
				lprintf(LOG_WARNING,"%04d !SMTP MISSING 'RCPT TO' command", socket);
				sockprintf(socket, badseq_rsp);
				continue;
			}
			if(msgtxt!=NULL) {
				fclose(msgtxt), msgtxt=NULL;
				if(!(startup->options&MAIL_OPT_DEBUG_RX_BODY))
					unlink(msgtxt_fname);
			}
			SAFEPRINTF2(msgtxt_fname,"%sSBBS_SMTP.%s.msg", scfg.temp_dir, session_id);
			if((msgtxt=fopen(msgtxt_fname,"w+b"))==NULL) {
				lprintf(LOG_ERR,"%04d !SMTP ERROR %d opening %s"
					,socket, errno, msgtxt_fname);
				sockprintf(socket, "452 Insufficient system storage");
				continue;
			}
			/* These vars are potentially over-written by parsing an RFC822 header */
			/* get sender_addr */
			p=strchr(reverse_path,'<');
			if(p==NULL)	
				p=reverse_path;
			else 
				p++;
			SAFECOPY(sender_addr,p);
			truncstr(sender_addr,">");
			/* get sender */
			SAFECOPY(sender,sender_addr);
			if(truncstr(sender,"@")==NULL)
				sender[0]=0;

			sockprintf(socket, "354 send the mail data, end with <CRLF>.<CRLF>");
			if(telegram)
				state=SMTP_STATE_DATA_BODY;	/* No RFC headers in Telegrams */
			else
				state=SMTP_STATE_DATA_HEADER;
			hdr_lines=0;
			continue;
		}
		sockprintf(socket,"500 Syntax error");
		lprintf(LOG_WARNING,"%04d !SMTP UNSUPPORTED COMMAND: '%s'", socket, buf);
		if(++badcmds>9) {
			lprintf(LOG_WARNING,"%04d !TOO MANY INVALID COMMANDS (%u)",socket,badcmds);
			break;
		}
	}

	/* Free up resources here */
	smb_freemsgmem(&msg);

	if(msgtxt!=NULL) {
		fclose(msgtxt);
		if(!(startup->options&MAIL_OPT_DEBUG_RX_BODY))
			unlink(msgtxt_fname);
	}
	if(rcptlst!=NULL) {
		fclose(rcptlst);
		unlink(rcptlst_fname);
	}
	if(spy!=NULL)
		fclose(spy);

	status(STATUS_WFC);

	if(active_clients)
		active_clients--, update_clients();
	client_off(socket);

	thread_down();
	lprintf(LOG_DEBUG,"%04d SMTP RX Session thread terminated (%u threads remain, %lu clients served)"
		,socket, thread_count, served);

	/* Must be last */
	mail_close_socket(socket);
}

BOOL bounce(smb_t* smb, smbmsg_t* msg, char* err, BOOL immediate)
{
	char		str[128];
	char		attempts[64];
	int			i;
	ushort		agent=AGENT_SMTPSYSMSG;
	smbmsg_t	newmsg;

	msg->hdr.delivery_attempts++;
	lprintf(LOG_WARNING,"0000 !Delivery attempt #%u FAILED (%s) for message #%lu from %s to %s"
		,msg->hdr.delivery_attempts
		,err
		,msg->hdr.number
		,msg->from
		,msg->to_net.addr);

	if((i=smb_updatemsg(smb,msg))!=SMB_SUCCESS) {
		lprintf(LOG_ERR,"0000 !BOUNCE ERROR %d (%s) incrementing delivery attempt counter"
			,i, smb->last_error);
		return(FALSE);
	}

	if(!immediate && msg->hdr.delivery_attempts < startup->max_delivery_attempts)
		return(TRUE);

	newmsg=*msg;
	/* Mark original message as deleted */
	msg->hdr.attr|=MSG_DELETE;

	i=smb_updatemsg(smb,msg);
	if(msg->hdr.auxattr&MSG_FILEATTACH)
		delfattach(&scfg,msg);
	if(i!=SMB_SUCCESS) {
		lprintf(LOG_ERR,"0000 !BOUNCE ERROR %d (%s) deleting message"
			,i, smb->last_error);
		return(FALSE);
	}

	if(msg->from_agent!=AGENT_PERSON	/* don't bounce 'bounce messages' */
		|| (msg->idx.from==0 && msg->from_net.type==NET_NONE)
		|| (msg->reverse_path!=NULL && *msg->reverse_path==0)) {
		lprintf(LOG_WARNING,"0000 !Deleted undeliverable message from %s", msg->from);
		return(TRUE);
	}
	
	lprintf(LOG_WARNING,"0000 !Bouncing message back to %s", msg->from);

	newmsg.hfield=NULL;
	newmsg.hfield_dat=NULL;
	newmsg.total_hfields=0;
	newmsg.hdr.delivery_attempts=0;

	SAFEPRINTF(str,"Delivery failure: %s",newmsg.subj);
	smb_hfield_str(&newmsg, SUBJECT, str);
	smb_hfield_str(&newmsg, RECIPIENT, newmsg.from);
	if(newmsg.from_ext!=NULL) { /* Back to sender */
		smb_hfield_str(&newmsg, RECIPIENTEXT, newmsg.from_ext);
		newmsg.from_ext=NULL;	/* Clear the sender extension */
	}

	if((newmsg.from_net.type==NET_QWK || newmsg.from_net.type==NET_INTERNET)
		&& newmsg.reverse_path!=NULL) {
		smb_hfield(&newmsg, RECIPIENTNETTYPE, sizeof(newmsg.from_net.type), &newmsg.from_net.type);
		smb_hfield_str(&newmsg, RECIPIENTNETADDR, newmsg.reverse_path);
	}
	strcpy(str,"Mail Delivery Subsystem");
	smb_hfield_str(&newmsg, SENDER, str);
	smb_hfield(&newmsg, SENDERAGENT, sizeof(agent), &agent);
	
	/* Put error message in subject for now */
	if(msg->hdr.delivery_attempts>1)
		sprintf(attempts,"after %u attempts", msg->hdr.delivery_attempts);
	else
		attempts[0]=0;
	SAFEPRINTF2(str,"%s reporting delivery failure of message %s"
		,startup->host_name, attempts);
	smb_hfield_str(&newmsg, SMB_COMMENT, str);
	SAFEPRINTF2(str,"from %s to %s\r\n"
		,msg->reverse_path==NULL ? msg->from : msg->reverse_path
		,(char*)msg->to_net.addr);
	smb_hfield_str(&newmsg, SMB_COMMENT, str);
	strcpy(str,"Reason:");
	smb_hfield_str(&newmsg, SMB_COMMENT, str);
	smb_hfield_str(&newmsg, SMB_COMMENT, err);
	smb_hfield_str(&newmsg, SMB_COMMENT, "\r\nOriginal message text follows:\r\n");

	if((i=smb_addmsghdr(smb,&newmsg,SMB_SELFPACK))!=SMB_SUCCESS)
		lprintf(LOG_ERR,"0000 !BOUNCE ERROR %d (%s) adding message header"
			,i,smb->last_error);
	else {
		lprintf(LOG_WARNING,"0000 !Delivery failure notification (message #%ld) created for %s"
			,newmsg.hdr.number, newmsg.from);
		if((i=smb_incmsg_dfields(smb,&newmsg,1))!=SMB_SUCCESS)
			lprintf(LOG_ERR,"0000 !BOUNCE ERROR %d (%s) incrementing data allocation units"
				,i,smb->last_error);
	}

	newmsg.dfield=NULL;				/* Don't double-free the data fields */
	newmsg.hdr.total_dfields=0;
	smb_freemsgmem(&newmsg);

	return(TRUE);
}

static int remove_msg_intransit(smb_t* smb, smbmsg_t* msg)
{
	int i;

	if((i=smb_lockmsghdr(smb,msg))!=SMB_SUCCESS) {
		lprintf(LOG_WARNING,"0000 !SEND ERROR %d (%s) locking message header #%lu"
			,i, smb->last_error, msg->idx.number);
		return(i);
	}
	msg->hdr.netattr&=~MSG_INTRANSIT;
	i=smb_putmsghdr(smb,msg);
	smb_unlockmsghdr(smb,msg);
	
	if(i!=0)
		lprintf(LOG_ERR,"0000 !SEND ERROR %d (%s) writing message header #%lu"
			,i, smb->last_error, msg->idx.number);

	return(i);
}

void get_dns_server(char* dns_server, size_t len)
{
	str_list_t	list;
	size_t		count;

	sprintf(dns_server,"%.*s",len,startup->dns_server);
	if(!isalnum(dns_server[0])) {
		if((list=getNameServerList())!=NULL) {
			if((count=strListCount(list))>0) {
				sprintf(dns_server,"%.*s",len,list[xp_random(count)]);
				lprintf(LOG_DEBUG,"0000 SEND using auto-detected DNS server address: %s"
					,dns_server);
			}
			freeNameServerList(list);
		}
	}
}

#ifdef __BORLANDC__
#pragma argsused
#endif
static void sendmail_thread(void* arg)
{
	int			i,j;
	char		to[128];
	char		mx[128];
	char		mx2[128];
	char		err[1024];
	char		buf[512];
	char		str[128];
	char		resp[512];
	char		toaddr[256];
	char		fromaddr[256];
	char		challenge[256];
	char		secret[64];
	char		md5_data[384];
	char		digest[MD5_DIGEST_SIZE];
	char		numeric_ip[16];
	char		domain_list[MAX_PATH+1];
	char		dns_server[16];
	char*		server;
	char*		msgtxt=NULL;
	char*		p;
	char*		tp;
	ushort		port;
	ulong		last_msg=0;
	ulong		ip_addr;
	ulong		dns;
	ulong		lines;
	BOOL		success;
	BOOL		first_cycle=TRUE;
	SOCKET		sock=INVALID_SOCKET;
	SOCKADDR_IN	addr;
	SOCKADDR_IN	server_addr;
	time_t		last_scan=0;
	smb_t		smb;
	smbmsg_t	msg;
	mail_t*		mail;
	long		msgs;
	long		l;
	BOOL		sending_locally=FALSE;

	SetThreadName("SendMail Thread");
	thread_up(TRUE /* setuid */);

	sendmail_running=TRUE;
	terminate_sendmail=FALSE;

	lprintf(LOG_DEBUG,"0000 SendMail thread started");

	memset(&msg,0,sizeof(msg));
	memset(&smb,0,sizeof(smb));

	while(server_socket!=INVALID_SOCKET && !terminate_sendmail) {

		if(startup->options&MAIL_OPT_NO_SENDMAIL) {
			sem_trywait_block(&sendmail_wakeup_sem,1000);
			continue;
		}

		if(active_sendmail!=0)
			active_sendmail=0, update_clients();

		smb_close(&smb);

		if(sock!=INVALID_SOCKET) {
			mail_close_socket(sock);
			sock=INVALID_SOCKET;
		}

		if(msgtxt!=NULL) {
			smb_freemsgtxt(msgtxt);
			msgtxt=NULL;
		}

		smb_freemsgmem(&msg);

		/* Don't delay on first loop */
		if(first_cycle)
			first_cycle=FALSE;
		else
			sem_trywait_block(&sendmail_wakeup_sem,startup->sem_chk_freq*1000);

		sprintf(smb.file,"%smail",scfg.data_dir);
		smb.retry_time=scfg.smb_retry_time;
		smb.subnum=INVALID_SUB;
		if((i=smb_open(&smb))!=SMB_SUCCESS) 
			continue;
		if((i=smb_locksmbhdr(&smb))!=SMB_SUCCESS)
			continue;
		i=smb_getstatus(&smb);
		smb_unlocksmbhdr(&smb);
		if(i!=0)
			continue;
		if(smb.status.last_msg==last_msg && time(NULL)-last_scan<startup->rescan_frequency)
			continue;
		last_msg=smb.status.last_msg;
		last_scan=time(NULL);
		mail=loadmail(&smb,&msgs,/* to network */0,MAIL_YOUR,0);
		for(l=0; l<msgs; l++) {
			if(active_sendmail!=0)
				active_sendmail=0, update_clients();

			if(server_socket==INVALID_SOCKET || terminate_sendmail)	/* server stopped */
				break;

			if(sock!=INVALID_SOCKET) {
				mail_close_socket(sock);
				sock=INVALID_SOCKET;
			}

			if(msgtxt!=NULL) {
				smb_freemsgtxt(msgtxt);
				msgtxt=NULL;
			}

			smb_freemsgmem(&msg);

			msg.hdr.number=mail[l].number;
			if((i=smb_getmsgidx(&smb,&msg))!=SMB_SUCCESS) {
				lprintf(LOG_ERR,"0000 !SEND ERROR %d (%s) getting message index #%lu"
					,i, smb.last_error, mail[l].number);
				break;
			}
			if((i=smb_lockmsghdr(&smb,&msg))!=SMB_SUCCESS) {
				lprintf(LOG_WARNING,"0000 !SEND ERROR %d (%s) locking message header #%lu"
					,i, smb.last_error, msg.idx.number);
				continue;
			}
			if((i=smb_getmsghdr(&smb,&msg))!=SMB_SUCCESS) {
				smb_unlockmsghdr(&smb,&msg);
				lprintf(LOG_ERR,"0000 !SEND ERROR %d (%s) reading message header #%lu"
					,i, smb.last_error, msg.idx.number);
				continue; 
			}
			if(msg.hdr.attr&MSG_DELETE || msg.to_net.type!=NET_INTERNET || msg.to_net.addr==NULL) {
				smb_unlockmsghdr(&smb,&msg);
				continue;
			}

			if(!(startup->options&MAIL_OPT_SEND_INTRANSIT) && msg.hdr.netattr&MSG_INTRANSIT) {
				smb_unlockmsghdr(&smb,&msg);
				lprintf(LOG_ERR,"0000 SEND Message #%lu from %s to %s - in transit"
					,msg.hdr.number, msg.from, msg.to_net.addr);
				continue;
			}
			msg.hdr.netattr|=MSG_INTRANSIT;	/* Prevent another sendmail thread from sending this msg */
			smb_putmsghdr(&smb,&msg);
			smb_unlockmsghdr(&smb,&msg);

			active_sendmail=1, update_clients();

			lprintf(LOG_INFO,"0000 SEND Message #%lu from %s to %s"
				,msg.hdr.number, msg.from, msg.to_net.addr);
			status("Sending");
#ifdef _WIN32
			if(startup->outbound_sound[0] && !(startup->options&MAIL_OPT_MUTE)) 
				PlaySound(startup->outbound_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

			lprintf(LOG_DEBUG,"0000 SEND getting message text");
			if((msgtxt=smb_getmsgtxt(&smb,&msg,GETMSGTXT_ALL))==NULL) {
				remove_msg_intransit(&smb,&msg);
				lprintf(LOG_ERR,"0000 !SEND ERROR (%s) retrieving message text",smb.last_error);
				continue;
			}

			port=0;

			/* Check if this is a local email ToDo */
			SAFECOPY(to,(char*)msg.to_net.addr);
			truncstr(to,"> ");

			p=strrchr(to,'@');
			if(p==NULL) {
				remove_msg_intransit(&smb,&msg);
				lprintf(LOG_WARNING,"0000 !SEND INVALID destination address: %s", to);
				SAFEPRINTF(err,"Invalid destination address: %s", to);
				bounce(&smb,&msg,err,TRUE);
				continue;
			}
			p++;
			sprintf(domain_list,"%sdomains.cfg",scfg.ctrl_dir);
			if(stricmp(p,scfg.sys_inetaddr)==0
					|| stricmp(p,startup->host_name)==0
					|| findstr(p,domain_list)) {
				/* This is a local message... no need to send to remote */
				port = startup->smtp_port;
				if(startup->interface_addr==0)
					server="127.0.0.1";
				else {
					sprintf(numeric_ip, "%u.%u.%u.%u"
							, startup->interface_addr >> 24
							, (startup->interface_addr >> 16) & 0xff
							, (startup->interface_addr >> 8) & 0xff
							, startup->interface_addr & 0xff);
					server = numeric_ip;
				}
				sending_locally=TRUE;
			}
			else {
				if(startup->options&MAIL_OPT_RELAY_TX) { 
					server=startup->relay_server;
					port=startup->relay_port;
				} else {
					server=p;
					tp=strrchr(p,':');	/* non-standard SMTP port */
					if(tp!=NULL) {
						*tp=0;
						port=atoi(tp+1);
					}
					if(port==0) {	/* No port specified, use MX look-up */
						get_dns_server(dns_server,sizeof(dns_server));
						if((dns=resolve_ip(dns_server))==INADDR_NONE) {
							remove_msg_intransit(&smb,&msg);
							lprintf(LOG_WARNING,"0000 !SEND INVALID DNS server address: %s"
								,dns_server);
							continue;
						}
						lprintf(LOG_DEBUG,"0000 SEND getting MX records for %s from %s",p,dns_server);
						if((i=dns_getmx(p, mx, mx2, startup->interface_addr, dns
							,startup->options&MAIL_OPT_USE_TCP_DNS ? TRUE : FALSE
							,TIMEOUT_THREAD_WAIT/2))!=0) {
							remove_msg_intransit(&smb,&msg);
							lprintf(LOG_WARNING,"0000 !SEND ERROR %d obtaining MX records for %s from %s"
								,i,p,startup->dns_server);
							SAFEPRINTF2(err,"Error %d obtaining MX record for %s",i,p);
							bounce(&smb,&msg,err,FALSE);
							continue;
						}
						server=mx;
					}
				}
			}
			if(!port)
				port=IPPORT_SMTP;

			if((sock=mail_open_socket(SOCK_STREAM,"smtp|sendmail"))==INVALID_SOCKET) {
				remove_msg_intransit(&smb,&msg);
				lprintf(LOG_ERR,"0000 !SEND ERROR %d opening socket", ERROR_VALUE);
				continue;
			}

			memset(&addr,0,sizeof(addr));
			addr.sin_addr.s_addr = htonl(startup->interface_addr);
			addr.sin_family = AF_INET;

			if(startup->seteuid!=NULL)
				startup->seteuid(FALSE);
			i=bind(sock,(struct sockaddr *)&addr, sizeof(addr));
			if(startup->seteuid!=NULL)
				startup->seteuid(TRUE);
			if(i!=0) {
				remove_msg_intransit(&smb,&msg);
				lprintf(LOG_ERR,"%04d !SEND ERROR %d (%d) binding socket", sock, i, ERROR_VALUE);
				continue;
			}

			strcpy(err,"UNKNOWN ERROR");
			success=FALSE;
			for(j=0;j<2 && !success;j++) {
				if(j) {
					if(startup->options&MAIL_OPT_RELAY_TX || !mx2[0])
						break;
					server=mx2;	/* Give second mx record a try */
				}
				
				lprintf(LOG_DEBUG,"%04d SEND resolving SMTP hostname: %s", sock, server);
				ip_addr=resolve_ip(server);
				if(ip_addr==INADDR_NONE) {
					SAFEPRINTF(err,"Failed to resolve SMTP hostname: %s",server);
					continue;
				}

				memset(&server_addr,0,sizeof(server_addr));
				server_addr.sin_addr.s_addr = ip_addr;
				server_addr.sin_family = AF_INET;
				server_addr.sin_port = htons(port);

				if((server==mx || server==mx2) 
					&& ((ip_addr&0xff)==127 || ip_addr==0)) {
					SAFEPRINTF2(err,"Bad IP address (%s) for MX server: %s"
						,inet_ntoa(server_addr.sin_addr),server);
					continue;
				}
				
				lprintf(LOG_INFO,"%04d SEND connecting to port %u on %s [%s]"
					,sock
					,ntohs(server_addr.sin_port)
					,server,inet_ntoa(server_addr.sin_addr));
				if((i=connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)))!=0) {
					i=ERROR_VALUE;
					lprintf(LOG_WARNING,"%04d !SEND ERROR %d connecting to SMTP server: %s"
						,sock
						,i, server);
					SAFEPRINTF2(err,"Error %d connecting to SMTP server: %s"
						,i, server);
					continue;
				}
				success=TRUE;
			}
			if(!success) {	/* Failed to send, so bounce */
				remove_msg_intransit(&smb,&msg);
				bounce(&smb,&msg,err,FALSE);	
				continue;
			}

			lprintf(LOG_DEBUG,"%04d SEND connected to %s",sock,server);

			/* HELO */
			if(!sockgetrsp(sock,"220",buf,sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"220");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}
			if(startup->options&MAIL_OPT_RELAY_TX 
				&& (startup->options&MAIL_OPT_RELAY_AUTH_MASK)!=0)	/* Requires ESMTP */
				sockprintf(sock,"EHLO %s",startup->host_name);
			else
				sockprintf(sock,"HELO %s",startup->host_name);
			if(!sockgetrsp(sock,"250", buf, sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"250");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}

			/* AUTH */
			if(startup->options&MAIL_OPT_RELAY_TX 
				&& (startup->options&MAIL_OPT_RELAY_AUTH_MASK)!=0 && !sending_locally) {
				switch(startup->options&MAIL_OPT_RELAY_AUTH_MASK) {
					case MAIL_OPT_RELAY_AUTH_PLAIN:
						p="PLAIN";
						break;
					case MAIL_OPT_RELAY_AUTH_LOGIN:
						p="LOGIN";
						break;
					case MAIL_OPT_RELAY_AUTH_CRAM_MD5:
						p="CRAM-MD5";
						break;
					default:
						p="<unknown>";
						break;
				}
				sockprintf(sock,"AUTH %s",p);
				if(!sockgetrsp(sock,"334",buf,sizeof(buf))) {
					SAFEPRINTF3(err,badrsp_err,server,buf,"334 Username/Challenge");
					bounce(&smb,&msg,err,buf[0]=='5');
					continue;
				}
				switch(startup->options&MAIL_OPT_RELAY_AUTH_MASK) {
					case MAIL_OPT_RELAY_AUTH_PLAIN:
						p=startup->relay_user;
						break;
					case MAIL_OPT_RELAY_AUTH_LOGIN:
						b64_encode(p=resp,sizeof(resp),startup->relay_user,0);
						break;
					case MAIL_OPT_RELAY_AUTH_CRAM_MD5:
						p=buf;
						FIND_WHITESPACE(p);
						SKIP_WHITESPACE(p);
						b64_decode(challenge,sizeof(challenge),p,0);

						/* Calculate response */
						memset(secret,0,sizeof(secret));
						SAFECOPY(secret,startup->relay_pass);
						for(i=0;i<sizeof(secret);i++)
							md5_data[i]=secret[i]^0x36;	/* ipad */
						strcpy(md5_data+i,challenge);
						MD5_calc(digest,md5_data,sizeof(secret)+strlen(challenge));
						for(i=0;i<sizeof(secret);i++)
							md5_data[i]=secret[i]^0x5c;	/* opad */
						memcpy(md5_data+i,digest,sizeof(digest));
						MD5_calc(digest,md5_data,sizeof(secret)+sizeof(digest));
						
						safe_snprintf(buf,sizeof(buf),"%s %s",startup->relay_user,MD5_hex(str,digest));
						b64_encode(p=resp,sizeof(resp),buf,0);
						break;
					default:
						p="<unknown>";
						break;
				}
				sockprintf(sock,"%s",p);
				if((startup->options&MAIL_OPT_RELAY_AUTH_MASK)!=MAIL_OPT_RELAY_AUTH_CRAM_MD5) {
					if(!sockgetrsp(sock,"334",buf,sizeof(buf))) {
						SAFEPRINTF3(err,badrsp_err,server,buf,"334 Password");
						bounce(&smb,&msg,err,buf[0]=='5');
						continue;
					}
					switch(startup->options&MAIL_OPT_RELAY_AUTH_MASK) {
						case MAIL_OPT_RELAY_AUTH_PLAIN:
							p=startup->relay_pass;
							break;
						case MAIL_OPT_RELAY_AUTH_LOGIN:
							b64_encode(p=buf,sizeof(buf),startup->relay_pass,0);
							break;
						default:
							p="<unknown>";
							break;
					}
					sockprintf(sock,"%s",p);
				}
				if(!sockgetrsp(sock,"235",buf,sizeof(buf))) {
					SAFEPRINTF3(err,badrsp_err,server,buf,"235");
					bounce(&smb,&msg,err,buf[0]=='5');
					continue;
				}
			}

			/* MAIL */
			if(msg.from_net.type==NET_INTERNET && msg.reverse_path!=NULL)
				SAFECOPY(fromaddr,msg.reverse_path);
			else 
				usermailaddr(&scfg,fromaddr,msg.from);
			truncstr(fromaddr," ");
			if(fromaddr[0]=='<')
				sockprintf(sock,"MAIL FROM: %s",fromaddr);
			else
				sockprintf(sock,"MAIL FROM: <%s>",fromaddr);
			if(!sockgetrsp(sock,"250", buf, sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"250");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}
			/* RCPT */
			if(msg.forward_path!=NULL) {
				SAFECOPY(toaddr,msg.forward_path);
			} else {
				if((p=strrchr((char*)msg.to_net.addr,'<'))!=NULL)
					p++;
				else
					p=(char*)msg.to_net.addr;
				SAFECOPY(toaddr,p);
				truncstr(toaddr,"> ");
				if((p=strrchr(toaddr,'@'))!=NULL && (tp=strrchr(toaddr,':'))!=NULL
					&& tp > p)
					*tp=0;	/* Remove ":port" designation from envelope */
			}
			sockprintf(sock,"RCPT TO: <%s>", toaddr);
			if(!sockgetrsp(sock,"25", buf, sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"25*");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}
			/* DATA */
			sockprintf(sock,"DATA");
			if(!sockgetrsp(sock,"354", buf, sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"354");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}
			lprintf(LOG_DEBUG,"%04d SEND sending message text (%u bytes)"
				,sock, strlen(msgtxt));
			lines=sockmsgtxt(sock,&msg,msgtxt,-1);
			if(!sockgetrsp(sock,"250", buf, sizeof(buf))) {
				remove_msg_intransit(&smb,&msg);
				SAFEPRINTF3(err,badrsp_err,server,buf,"250");
				bounce(&smb,&msg,err,buf[0]=='5');
				continue;
			}
			lprintf(LOG_DEBUG,"%04d SEND message transfer complete (%lu lines)", sock, lines);

			/* Now lets mark this message for deletion without corrupting the index */
			msg.hdr.attr|=MSG_DELETE;
			msg.hdr.netattr&=~MSG_INTRANSIT;
			if((i=smb_updatemsg(&smb,&msg))!=SMB_SUCCESS)
				lprintf(LOG_ERR,"%04d !SEND ERROR %d (%s) deleting message #%lu"
					,sock, i, smb.last_error, msg.hdr.number);
			if(msg.hdr.auxattr&MSG_FILEATTACH)
				delfattach(&scfg,&msg);

			/* QUIT */
			sockprintf(sock,"QUIT");
			sockgetrsp(sock,"221", buf, sizeof(buf));
			mail_close_socket(sock);
			sock=INVALID_SOCKET;
		}				
		status(STATUS_WFC);
		/* Free up resources here */
		if(mail!=NULL)
			freemail(mail);
	}
	if(sock!=INVALID_SOCKET)
		mail_close_socket(sock);

	smb_freemsgtxt(msgtxt);
	smb_freemsgmem(&msg);
	smb_close(&smb);

	if(active_sendmail!=0)
		active_sendmail=0, update_clients();

	thread_down();
	lprintf(LOG_DEBUG,"0000 SendMail thread terminated (%u threads remain)", thread_count);

	sendmail_running=FALSE;
}

void DLLCALL mail_terminate(void)
{
  	lprintf(LOG_DEBUG,"%04d Mail Server terminate",server_socket);
	terminate_server=TRUE;
}

static void cleanup(int code)
{
	int i;

	free_cfg(&scfg);

	semfile_list_free(&recycle_semfiles);
	semfile_list_free(&shutdown_semfiles);

	if(mailproc_list!=NULL) {
		for(i=0;i<mailproc_count;i++)
			strListFree(&mailproc_list[i].to);
		FREE_AND_NULL(mailproc_list);
	}

	if(server_socket!=INVALID_SOCKET) {
		mail_close_socket(server_socket);
		server_socket=INVALID_SOCKET;
	}

	if(pop3_socket!=INVALID_SOCKET) {
		mail_close_socket(pop3_socket);
		pop3_socket=INVALID_SOCKET;
	}

	update_clients();

#ifdef _WINSOCKAPI_	
	if(WSAInitialized && WSACleanup()!=0) 
		lprintf(LOG_ERR,"0000 !WSACleanup ERROR %d",ERROR_VALUE);
#endif

	thread_down();
	status("Down");
	if(terminate_server || code)
		lprintf(LOG_INFO,"#### Mail Server thread terminated (%u threads remain, %lu clients served)"
			,thread_count, served);
	if(startup!=NULL && startup->terminated!=NULL)
		startup->terminated(startup->cbdata,code);
}

const char* DLLCALL mail_ver(void)
{
	static char ver[256];
	char compiler[32];

	DESCRIBE_COMPILER(compiler);

	sscanf("$Revision: 1.412 $", "%*s %s", revision);

	sprintf(ver,"Synchronet Mail Server %s%s  SMBLIB %s  "
		"Compiled %s %s with %s"
		,revision
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,smb_lib_ver()
		,__DATE__, __TIME__, compiler
		);

	return(ver);
}

void DLLCALL mail_server(void* arg)
{
	char*			p;
	char			path[MAX_PATH+1];
	char			str[256];
	char			error[256];
	char			compiler[32];
	SOCKADDR_IN		server_addr;
	SOCKADDR_IN		client_addr;
	socklen_t		client_addr_len;
	SOCKET			client_socket;
	int				i;
	int				result;
	ulong			l;
	time_t			t;
	time_t			start;
	time_t			initialized=0;
	fd_set			socket_set;
	SOCKET			high_socket_set;
	pop3_t*			pop3;
	smtp_t*			smtp;
	struct timeval	tv;
	FILE*			fp;
	str_list_t		sec_list;

	mail_ver();

	startup=(mail_startup_t*)arg;

#ifdef _THREAD_SUID_BROKEN
	if(thread_suid_broken)
		startup->seteuid(TRUE);
#endif

    if(startup==NULL) {
    	sbbs_beep(100,500);
    	fprintf(stderr, "No startup structure passed!\n");
    	return;
    }

	if(startup->size!=sizeof(mail_startup_t)) {	/* verify size */
		sbbs_beep(100,500);
		sbbs_beep(300,500);
		sbbs_beep(100,500);
		fprintf(stderr, "Invalid startup structure!\n");
		return;
	}

	/* Setup intelligent defaults */
	if(startup->relay_port==0)				startup->relay_port=IPPORT_SMTP;
	if(startup->smtp_port==0)				startup->smtp_port=IPPORT_SMTP;
	if(startup->pop3_port==0)				startup->pop3_port=IPPORT_POP3;
	if(startup->rescan_frequency==0)		startup->rescan_frequency=3600;	/* 60 minutes */
	if(startup->max_delivery_attempts==0)	startup->max_delivery_attempts=50;
	if(startup->max_inactivity==0) 			startup->max_inactivity=120; /* seconds */
	if(startup->max_recipients==0) 			startup->max_recipients=100;
	if(startup->sem_chk_freq==0)			startup->sem_chk_freq=2;

#ifdef JAVASCRIPT
	if(startup->js.max_bytes==0)			startup->js.max_bytes=JAVASCRIPT_MAX_BYTES;
	if(startup->js.cx_stack==0)				startup->js.cx_stack=JAVASCRIPT_CONTEXT_STACK;
#endif

	uptime=0;
	served=0;
	startup->recycle_now=FALSE;
	startup->shutdown_now=FALSE;
	terminate_server=FALSE;

	SetThreadName("Mail Server");

	do {

		thread_up(FALSE /* setuid */);

		status("Initializing");

		memset(&scfg, 0, sizeof(scfg));

		lprintf(LOG_INFO,"Synchronet Mail Server Revision %s%s"
			,revision
#ifdef _DEBUG
			," Debug"
#else
			,""
#endif
			);

		DESCRIBE_COMPILER(compiler);

		lprintf(LOG_INFO,"Compiled %s %s with %s", __DATE__, __TIME__, compiler);

		lprintf(LOG_INFO,"SMBLIB %s (format %x.%02x)",smb_lib_ver(),smb_ver()>>8,smb_ver()&0xff);

		sbbs_srand();

		if(!winsock_startup()) {
			cleanup(1);
			return;
		}

		t=time(NULL);
		lprintf(LOG_INFO,"Initializing on %.24s with options: %lx"
			,CTIME_R(&t,str),startup->options);

		/* Initial configuration and load from CNF files */
		SAFECOPY(scfg.ctrl_dir,startup->ctrl_dir);
		lprintf(LOG_INFO,"Loading configuration files from %s", scfg.ctrl_dir);
		scfg.size=sizeof(scfg);
		SAFECOPY(error,UNKNOWN_LOAD_ERROR);
		if(!load_cfg(&scfg, NULL, TRUE, error)) {
			lprintf(LOG_ERR,"!ERROR %s",error);
			lprintf(LOG_ERR,"!Failed to load configuration files");
			cleanup(1);
			return;
		}

		if(startup->temp_dir[0])
			SAFECOPY(scfg.temp_dir,startup->temp_dir);
		else
			SAFECOPY(scfg.temp_dir,"../temp");
	   	prep_dir(scfg.ctrl_dir, scfg.temp_dir, sizeof(scfg.temp_dir));
		MKDIR(scfg.temp_dir);
		lprintf(LOG_DEBUG,"Temporary file directory: %s", scfg.temp_dir);
		if(!isdir(scfg.temp_dir)) {
			lprintf(LOG_ERR,"!Invalid temp directory: %s", scfg.temp_dir);
			cleanup(1);
			return;
		}

		/* Parse the mailproc[.host].ini */
		mailproc_list=NULL;
		mailproc_count=0;
		iniFileName(path,sizeof(path),scfg.ctrl_dir,"mailproc.ini");
		if((fp=iniOpenFile(path, /* create? */FALSE))!=NULL) {
			lprintf(LOG_DEBUG,"Reading %s",path);
			sec_list = iniReadSectionList(fp,/* prefix */NULL);
			if((mailproc_count=strListCount(sec_list))!=0
				&& (mailproc_list=malloc(mailproc_count*sizeof(struct mailproc)))!=NULL) {
				for(i=0;i<mailproc_count;i++) {
					memset(&mailproc_list[i],0,sizeof(struct mailproc));
					SAFECOPY(mailproc_list[i].cmdline,sec_list[i]);
					mailproc_list[i].to =
						iniReadStringList(fp,sec_list[i],"to",",",NULL);
					mailproc_list[i].passthru =
						iniReadBool(fp,sec_list[i],"passthru",TRUE);
					mailproc_list[i].native =
						iniReadBool(fp,sec_list[i],"native",FALSE);
				}
			}
			iniFreeStringList(sec_list);
			iniCloseFile(fp);
		}

		if(startup->host_name[0]==0)
			SAFECOPY(startup->host_name,scfg.sys_inetaddr);

		if(!(scfg.sys_misc&SM_LOCAL_TZ) && !(startup->options&MAIL_OPT_LOCAL_TIMEZONE)) {
			if(putenv("TZ=UTC0"))
				lprintf(LOG_ERR,"!putenv() FAILED");
			tzset();

			if((t=checktime())!=0) {   /* Check binary time */
				lprintf(LOG_ERR,"!TIME PROBLEM (%ld)",t);
				cleanup(1);
				return;
			}
		}

		if(uptime==0)
			uptime=time(NULL);	/* this must be done *after* setting the timezone */

		if(startup->max_clients==0) {
			startup->max_clients=scfg.sys_nodes;
			if(startup->max_clients<10)
				startup->max_clients=10;
		}

		lprintf(LOG_DEBUG,"Maximum clients: %u",startup->max_clients);

		lprintf(LOG_DEBUG,"Maximum inactivity: %u seconds",startup->max_inactivity);

		active_clients=0,update_clients();

		/* open a socket and wait for a client */

		server_socket = mail_open_socket(SOCK_STREAM,"smtp");

		if(server_socket == INVALID_SOCKET) {
			lprintf(LOG_ERR,"!ERROR %d opening socket", ERROR_VALUE);
			cleanup(1);
			return;
		}

		lprintf(LOG_DEBUG,"%04d SMTP socket opened",server_socket);

		/*****************************/
		/* Listen for incoming calls */
		/*****************************/
		memset(&server_addr, 0, sizeof(server_addr));

		server_addr.sin_addr.s_addr = htonl(startup->interface_addr);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port   = htons(startup->smtp_port);

		if(startup->seteuid!=NULL)
			startup->seteuid(FALSE);
		result = retry_bind(server_socket,(struct sockaddr *)&server_addr,sizeof(server_addr)
			,startup->bind_retry_count,startup->bind_retry_delay,"SMTP Server",lprintf);
		if(startup->seteuid!=NULL)
			startup->seteuid(TRUE);
		if(result != 0) {
			lprintf(LOG_ERR,"%04d %s",server_socket, BIND_FAILURE_HELP);
			cleanup(1);
			return;
		}

		lprintf(LOG_DEBUG,"%04d SMTP socket bound to port %u"
			,server_socket, startup->smtp_port);

		result = listen (server_socket, 1);

		if(result != 0) {
			lprintf(LOG_ERR,"%04d !ERROR %d (%d) listening on socket"
				,server_socket, result, ERROR_VALUE);
			cleanup(1);
			return;
		}

		if(startup->options&MAIL_OPT_ALLOW_POP3) {

			/* open a socket and wait for a client */

			pop3_socket = mail_open_socket(SOCK_STREAM,"pop3");

			if(pop3_socket == INVALID_SOCKET) {
				lprintf(LOG_ERR,"!ERROR %d opening POP3 socket", ERROR_VALUE);
				cleanup(1);
				return;
			}

			lprintf(LOG_DEBUG,"%04d POP3 socket opened",pop3_socket);

			/*****************************/
			/* Listen for incoming calls */
			/*****************************/
			memset(&server_addr, 0, sizeof(server_addr));

			server_addr.sin_addr.s_addr = htonl(startup->interface_addr);
			server_addr.sin_family = AF_INET;
			server_addr.sin_port   = htons(startup->pop3_port);

			if(startup->seteuid!=NULL)
				startup->seteuid(FALSE);
			result = retry_bind(pop3_socket,(struct sockaddr *)&server_addr,sizeof(server_addr)
				,startup->bind_retry_count,startup->bind_retry_delay,"POP3 Server",lprintf);
			if(startup->seteuid!=NULL)
				startup->seteuid(TRUE);
			if(result != 0) {
				lprintf(LOG_ERR,"%04d %s",pop3_socket,BIND_FAILURE_HELP);
				cleanup(1);
				return;
			}

			lprintf(LOG_DEBUG,"%04d POP3 socket bound to port %u"
				,pop3_socket, startup->pop3_port);

			result = listen (pop3_socket, 1);

			if(result != 0) {
				lprintf(LOG_ERR,"%04d !ERROR %d (%d) listening on POP3 socket"
					,pop3_socket, result, ERROR_VALUE);
				cleanup(1);
				return;
			}
		}

		sem_init(&sendmail_wakeup_sem,0,0);

		if(!(startup->options&MAIL_OPT_NO_SENDMAIL))
			_beginthread(sendmail_thread, 0, NULL);

		lprintf(LOG_NOTICE,"%04d Mail Server thread started",server_socket);
		status(STATUS_WFC);

		/* Setup recycle/shutdown semaphore file lists */
		shutdown_semfiles=semfile_list_init(scfg.ctrl_dir,"shutdown","mail");
		recycle_semfiles=semfile_list_init(scfg.ctrl_dir,"recycle","mail");
		SAFEPRINTF(path,"%smailsrvr.rec",scfg.ctrl_dir);	/* legacy */
		semfile_list_add(&recycle_semfiles,path);
		if(!initialized) {
			initialized=time(NULL);
			semfile_list_check(&initialized,recycle_semfiles);
			semfile_list_check(&initialized,shutdown_semfiles);
		}

		/* signal caller that we've started up successfully */
		if(startup->started!=NULL)
    		startup->started(startup->cbdata);

		while(server_socket!=INVALID_SOCKET && !terminate_server) {

			if(active_clients==0) {
				if(!(startup->options&MAIL_OPT_NO_RECYCLE)) {
					if((p=semfile_list_check(&initialized,recycle_semfiles))!=NULL) {
						lprintf(LOG_INFO,"%04d Recycle semaphore file (%s) detected"
							,server_socket,p);
						break;
					}
#if 0	/* unused */
					if(startup->recycle_sem!=NULL && sem_trywait(&startup->recycle_sem)==0)
						startup->recycle_now=TRUE;
#endif
					if(startup->recycle_now==TRUE) {
						lprintf(LOG_NOTICE,"%04d Recycle semaphore signaled", server_socket);
						startup->recycle_now=FALSE;
						break;
					}
				}
				if(((p=semfile_list_check(&initialized,shutdown_semfiles))!=NULL
						&& lprintf(LOG_INFO,"%04d Shutdown semaphore file (%s) detected"
						,server_socket,p))
					|| (startup->shutdown_now==TRUE
						&& lprintf(LOG_INFO,"%04d Shutdown semaphore signaled",server_socket))) {
					startup->shutdown_now=FALSE;
					terminate_server=TRUE;
					break;
				}
			}

			/* now wait for connection */

			FD_ZERO(&socket_set);
			FD_SET(server_socket,&socket_set);
			high_socket_set=server_socket+1;
			if(startup->options&MAIL_OPT_ALLOW_POP3 
				&& pop3_socket!=INVALID_SOCKET) {
				FD_SET(pop3_socket,&socket_set);
				if(pop3_socket+1>high_socket_set)
					high_socket_set=pop3_socket+1;
			}

			tv.tv_sec=startup->sem_chk_freq;
			tv.tv_usec=0;

			if((i=select(high_socket_set,&socket_set,NULL,NULL,&tv))<1) {
				if(i==0)
					continue;
				if(ERROR_VALUE==EINTR)
					lprintf(LOG_DEBUG,"%04d Mail Server listening interrupted",server_socket);
				else if(ERROR_VALUE == ENOTSOCK)
            		lprintf(LOG_NOTICE,"%04d Mail Server sockets closed",server_socket);
				else
					lprintf(LOG_WARNING,"%04d !ERROR %d selecting sockets",server_socket,ERROR_VALUE);
				continue;
			}

			if(server_socket!=INVALID_SOCKET && !terminate_server
				&& FD_ISSET(server_socket,&socket_set)) {

				client_addr_len = sizeof(client_addr);
				client_socket = accept(server_socket, (struct sockaddr *)&client_addr
        			,&client_addr_len);

				if(client_socket == INVALID_SOCKET)
				{
#if 0	/* is this necessary still? */
					if(ERROR_VALUE == ENOTSOCK || ERROR_VALUE == EINVAL) {
            			lprintf(LOG_NOTICE,"%04d SMTP socket closed while listening"
							,server_socket);
						break;
					}
#endif
					lprintf(LOG_WARNING,"%04d SMTP !ERROR %d accepting connection"
						,server_socket, ERROR_VALUE);
#ifdef _WIN32
					if(WSAGetLastError()==WSAENOBUFS)	/* recycle (re-init WinSock) on this error */
						break;
#endif
					continue;
				}
				if(startup->socket_open!=NULL)
					startup->socket_open(startup->cbdata,TRUE);
				sockets++;

				if(trashcan(&scfg,inet_ntoa(client_addr.sin_addr),"ip-silent")) {
					mail_close_socket(client_socket);
					continue;
				}

				if(active_clients>=startup->max_clients) {
					lprintf(LOG_WARNING,"%04d SMTP !MAXIMUM CLIENTS (%u) reached, access denied"
						,client_socket, startup->max_clients);
					sockprintf(client_socket,"421 Maximum active clients reached, please try again later.");
					mswait(3000);
					mail_close_socket(client_socket);
					continue;
				}

				l=1;

				if((i=ioctlsocket(client_socket, FIONBIO, &l))!=0) {
					lprintf(LOG_ERR,"%04d SMTP !ERROR %d (%d) disabling blocking on socket"
						,client_socket, i, ERROR_VALUE);
					mail_close_socket(client_socket);
					continue;
				}

				if((smtp=malloc(sizeof(smtp_t)))==NULL) {
					lprintf(LOG_CRIT,"%04d SMTP !ERROR allocating %u bytes of memory for smtp_t"
						,client_socket, sizeof(smtp_t));
					mail_close_socket(client_socket);
					continue;
				}

				smtp->socket=client_socket;
				smtp->client_addr=client_addr;
				_beginthread (smtp_thread, 0, smtp);
				served++;
			}

			if(pop3_socket!=INVALID_SOCKET
				&& FD_ISSET(pop3_socket,&socket_set)) {

				client_addr_len = sizeof(client_addr);
				client_socket = accept(pop3_socket, (struct sockaddr *)&client_addr
        			,&client_addr_len);

				if(client_socket == INVALID_SOCKET)
				{
#if 0	/* is this necessary still? */
					if(ERROR_VALUE == ENOTSOCK || ERROR_VALUE == EINVAL) {
            			lprintf(LOG_NOTICE,"%04d POP3 socket closed while listening",pop3_socket);
						break;
					}
#endif
					lprintf(LOG_WARNING,"%04d POP3 !ERROR %d accepting connection"
						,pop3_socket, ERROR_VALUE);
#ifdef _WIN32
					if(WSAGetLastError()==WSAENOBUFS)	/* recycle (re-init WinSock) on this error */
						break;
#endif
					continue;
				}
				if(startup->socket_open!=NULL)
					startup->socket_open(startup->cbdata,TRUE);
				sockets++;

				if(trashcan(&scfg,inet_ntoa(client_addr.sin_addr),"ip-silent")) {
					mail_close_socket(client_socket);
					continue;
				}

				if(active_clients>=startup->max_clients) {
					lprintf(LOG_WARNING,"%04d POP3 !MAXIMUM CLIENTS (%u) reached, access denied"
						,client_socket, startup->max_clients);
					sockprintf(client_socket,"-ERR Maximum active clients reached, please try again later.");
					mswait(3000);
					mail_close_socket(client_socket);
					continue;
				}


				l=1;

				if((i=ioctlsocket(client_socket, FIONBIO, &l))!=0) {
					lprintf(LOG_ERR,"%04d POP3 !ERROR %d (%d) disabling blocking on socket"
						,client_socket, i, ERROR_VALUE);
					sockprintf(client_socket,"-ERR System error, please try again later.");
					mswait(3000);
					mail_close_socket(client_socket);
					continue;
				}

				if((pop3=malloc(sizeof(pop3_t)))==NULL) {
					lprintf(LOG_CRIT,"%04d POP3 !ERROR allocating %u bytes of memory for pop3_t"
						,client_socket,sizeof(pop3_t));
					sockprintf(client_socket,"-ERR System error, please try again later.");
					mswait(3000);
					mail_close_socket(client_socket);
					continue;
				}

				pop3->socket=client_socket;
				pop3->client_addr=client_addr;

				_beginthread (pop3_thread, 0, pop3);
				served++;
			}
		}

		if(active_clients) {
			lprintf(LOG_DEBUG,"%04d Waiting for %d active clients to disconnect..."
				,server_socket, active_clients);
			start=time(NULL);
			while(active_clients) {
				if(time(NULL)-start>startup->max_inactivity) {
					lprintf(LOG_WARNING,"%04d !TIMEOUT waiting for %d active clients"
						,server_socket, active_clients);
					break;
				}
				mswait(100);
			}
		}

		if(sendmail_running) {
			terminate_sendmail=TRUE;
			sem_post(&sendmail_wakeup_sem);
			mswait(100);
		}
		if(sendmail_running) {
			lprintf(LOG_DEBUG,"%04d Waiting for SendMail thread to terminate..."
				,server_socket);
			start=time(NULL);
			while(sendmail_running) {
				if(time(NULL)-start>TIMEOUT_THREAD_WAIT) {
					lprintf(LOG_WARNING,"%04d !TIMEOUT waiting for sendmail thread to terminate"
						,server_socket);
					break;
				}
				mswait(500);
			}
		}
		if(!sendmail_running) {
			while(sem_destroy(&sendmail_wakeup_sem)==-1 && errno==EBUSY) {
				mswait(1);
				sem_post(&sendmail_wakeup_sem);
			}
		}

		cleanup(0);

		if(!terminate_server) {
			lprintf(LOG_INFO,"Recycling server...");
			mswait(2000);
			if(startup->recycle!=NULL)
				startup->recycle(startup->cbdata);
		}

	} while(!terminate_server);
}
