/* ftpsrvr.c */

/* Synchronet FTP server */

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

/* Platform-specific headers */
#ifdef _WIN32

	#include <share.h>		/* SH_DENYNO */
	#include <direct.h>		/* _mkdir/_rmdir() */
	#include <process.h>	/* _beginthread */
	#include <windows.h>	/* required for mmsystem.h */
	#include <mmsystem.h>	/* SND_ASYNC */

#elif defined(__unix__)

	#include <signal.h>		/* signal/SIGPIPE */

#endif


/* ANSI C Library headers */
#include <stdio.h>
#include <stdlib.h>			/* ltoa in GNU C lib */
#include <stdarg.h>			/* va_list, varargs */
#include <string.h>			/* strrchr */
#include <fcntl.h>			/* O_WRONLY, O_RDONLY, etc. */
#include <errno.h>			/* EACCES */
#include <ctype.h>			/* toupper */
#include <sys/stat.h>		/* S_IWRITE */

/* Synchronet-specific headers */
#include "sbbs.h"
#include "ftpsrvr.h"
#include "telnet.h"

/* Constants */

#define FTP_SERVER				"Synchronet FTP Server"
#ifdef  JAVASCRIPT
#define FTP_VERSION				"1.10"
#else
#define FTP_VERSION				"1.05"
#endif

#define STATUS_WFC				"Listening"

#define BBS_VIRTUAL_PATH		"bbs:/""/"	/* this is actually bbs:<slash><slash> */
#define LOCAL_FSYS_DIR			"local:"
#define BBS_FSYS_DIR			"bbs:"

#define TIMEOUT_THREAD_WAIT		15		/* Seconds */

#define TIMEOUT_SOCKET_LISTEN	30		/* Seconds */

#define XFER_REPORT_INTERVAL	60		/* Seconds */

#define INDEX_FNAME_LEN			15

#define	NAME_LEN				15		/* User name length for listings */

static ftp_startup_t* startup=NULL;
static scfg_t	scfg;
static SOCKET	server_socket=INVALID_SOCKET;
static DWORD	active_clients=0;
static DWORD	sockets=0;
static HANDLE	socket_mutex=NULL;
static BYTE 	socket_debug[20000]={0};

#define	SOCKET_DEBUG_CTRL		(1<<0)	// 0x01
#define SOCKET_DEBUG_CONNECT	(1<<1)	// 0x02
#define SOCKET_DEBUG_READLINE	(1<<2)	// 0x04
#define SOCKET_DEBUG_ACCEPT		(1<<3)	// 0x08
#define SOCKET_DEBUG_DOWNLOAD	(1<<4)	// 0x10
#define SOCKET_DEBUG_SELECT		(1<<5)	// 0x20
#define SOCKET_DEBUG_RECV_CHAR	(1<<6)	// 0x40
#define SOCKET_DEBUG_RECV_BUF	(1<<7)	// 0x80

typedef struct {
	SOCKET			socket;
	SOCKADDR_IN		client_addr;
} ftp_t;


static const char *mon[]={"Jan","Feb","Mar","Apr","May","Jun"
            ,"Jul","Aug","Sep","Oct","Nov","Dec"};

BOOL direxist(char *dir)
{
	if(access(dir,0)==0)
		return(TRUE);
	else
		return(FALSE);
}

BOOL dir_op(scfg_t* cfg, user_t* user, uint dirnum)
{
	return(user->level>=SYSOP_LEVEL || user->exempt&FLAG('R')
		|| (cfg->dir[dirnum]->op_ar[0] && chk_ar(cfg,cfg->dir[dirnum]->op_ar,user)));
}

static int lprintf(char *fmt, ...)
{
	int		result;
	va_list argptr;
	char sbuf[1024];

    if(startup==NULL || startup->lputs==NULL)
        return(0);

    va_start(argptr,fmt);
    vsprintf(sbuf,fmt,argptr);
    va_end(argptr);
    result=startup->lputs(sbuf);

	return(result);
}

#ifdef _WINSOCKAPI_

static WSADATA WSAData;
static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
		lprintf("%s %s",WSAData.szDescription, WSAData.szSystemStatus);
		WSAInitialized=TRUE;
		return (TRUE);
	}

    lprintf("!WinSock startup ERROR %d", status);
	return (FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)

#endif

static void status(char* str)
{
	if(startup!=NULL && startup->status!=NULL)
	    startup->status(str);
}

static void update_clients(void)
{
	if(startup!=NULL && startup->clients!=NULL)
		startup->clients(active_clients);
}

static void client_on(SOCKET sock, client_t* client)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(TRUE,sock,client);
}

static void client_off(SOCKET sock)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(FALSE,sock,NULL);
}

static void thread_up(void)
{
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(TRUE);
}

static void thread_down(void)
{
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(FALSE);
}

static SOCKET ftp_open_socket(int type)
{
	SOCKET sock;

	sock=socket(AF_INET, type, IPPROTO_IP);
	if(sock!=INVALID_SOCKET && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(TRUE);
	if(sock!=INVALID_SOCKET) {
		sockets++;
#ifdef _DEBUG
		lprintf("%04d Socket opened (%d sockets in use)",sock,sockets);
#endif
	}
	return(sock);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
static int ftp_close_socket(SOCKET* sock, int line)
{
	int		result;

#ifndef _WIN32
#define ReleaseMutex(x)
#else
	if((result=WaitForSingleObject(socket_mutex,5000))!=WAIT_OBJECT_0) 
		lprintf("%04d !ERROR %d getting socket mutex from line %d"
			,*sock,ERROR_VALUE,line);

	if(IsBadWritePtr(sock,sizeof(SOCKET))) {
		ReleaseMutex(socket_mutex);
		lprintf("0000 !BAD socket pointer in close_socket from line %d",line);
		return(-1);
	}
#endif
	if((*sock)==INVALID_SOCKET) {
		ReleaseMutex(socket_mutex);
		lprintf("0000 !INVALID_SOCKET in close_socket from line %d",line);
		return(-1);
	}

	shutdown(*sock,SHUT_RDWR);	/* required on Unix */

	result=closesocket(*sock);
	if(result==0 && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(FALSE);

	sockets--;

	if(result!=0) {
		if(ERROR_VALUE!=ENOTSOCK)
			lprintf("%04d !ERROR %d closing socket from line %d",*sock,ERROR_VALUE,line);
	}
#ifdef _DEBUG
	else 
		lprintf("%04d Socket closed (%d sockets in use) from line %d",*sock,sockets,line);
#endif
	*sock=INVALID_SOCKET;
	ReleaseMutex(socket_mutex);

	return(result);
}

static int sockprintf(SOCKET sock, char *fmt, ...)
{
	int		len;
	int		result;
	va_list argptr;
	char	sbuf[1024];

    va_start(argptr,fmt);
    len=vsprintf(sbuf,fmt,argptr);
	if(startup!=NULL && startup->options&FTP_OPT_DEBUG_TX)
		lprintf("%04d TX: %s", sock, sbuf);
	strcat(sbuf,"\r\n");
	len+=2;
    va_end(argptr);
	while((result=send(sock,sbuf,len,0))!=len) {
		if(result==SOCKET_ERROR) {
			if(ERROR_VALUE==EWOULDBLOCK) {
				mswait(1);
				continue;
			}
			if(ERROR_VALUE==ECONNRESET) 
				lprintf("%04d Connection reset by peer on send",sock);
			else if(ERROR_VALUE==ECONNABORTED)
				lprintf("%04d Connection aborted by peer on send",sock);
			else
				lprintf("%04d !ERROR %d sending",sock,ERROR_VALUE);
			return(0);
		}
		lprintf("%04d !ERROR: short send: %d instead of %d",sock,result,len);
	}
	return(len);
}

/************************************************/
/* Truncates white-space chars off end of 'str' */
/************************************************/
static void truncsp(char *str)
{
	uint c;

c=strlen(str);
while(c && (uchar)str[c-1]<=' ') c--;
str[c]=0;
}

/* Returns the directory index of a virtual lib/dir path (e.g. main/games/filename) */
int getdir(char* p, user_t* user)
{
	char*	tp;
	char	path[MAX_PATH+1];
	int		dir;
	int		lib;

	sprintf(path,"%.*s",(int)sizeof(path)-1,p);
	p=path;

	if(*p=='/') 
		p++;
	else if(!strncmp(p,"./",2))
		p+=2;

	tp=strchr(p,'/');
	if(tp) *tp=0;
	for(lib=0;lib<scfg.total_libs;lib++) {
		if(!chk_ar(&scfg,scfg.lib[lib]->ar,user))
			continue;
		if(!stricmp(scfg.lib[lib]->sname,p))
			break;
	}
	if(lib>=scfg.total_libs) 
		return(-1);

	if(tp!=NULL)
		p=tp+1;

	tp=strchr(p,'/');
	if(tp) *tp=0;
	for(dir=0;dir<scfg.total_dirs;dir++) {
		if(scfg.dir[dir]->lib!=lib)
			continue;
		if(dir!=scfg.sysop_dir && dir!=scfg.upload_dir 
			&& !chk_ar(&scfg,scfg.dir[dir]->ar,user))
			continue;
		if(!stricmp(scfg.dir[dir]->code,p))
			break;
	}
	if(dir>=scfg.total_dirs) 
		return(-1);

	return(dir);
}

/*********************************/
/* JavaScript Data and Functions */
/*********************************/
#ifdef JAVASCRIPT

JSRuntime* js_runtime=NULL;

static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i;
    JSString *	str;
	FILE*	fp;

	if((fp=(FILE*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    for (i = 0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[i]);
		if (!str)
		    return JS_FALSE;
		fprintf(fp,"%s",JS_GetStringBytes(str));
	}

	return(JS_TRUE);
}

static JSFunctionSpec js_global_functions[] = {
	{"write",           js_write,           1},		/* write to HTML file */
	{0}
};

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	char*	warning;
	FILE*	fp;

	fp=(FILE*)JS_GetContextPrivate(cx);
	
	if(report==NULL) {
		lprintf("!JavaScript: %s", message);
		if(fp!=NULL)
			fprintf(fp,"!JavaScript: %s", message);
		return;
    }

	if(report->filename)
		sprintf(file," %s",report->filename);
	else
		file[0]=0;

	if(report->lineno)
		sprintf(line," line %d",report->lineno);
	else
		line[0]=0;

	if(JSREPORT_IS_WARNING(report->flags)) {
		if(JSREPORT_IS_STRICT(report->flags))
			warning="strict warning";
		else
			warning="warning";
	} else
		warning="";

	lprintf("!JavaScript %s%s%s: %s",warning,file,line,message);
	if(fp!=NULL)
		fprintf(fp,"!JavaScript %s%s%s: %s",warning,file,line,message);
}

static JSClass js_server_class = {
        "FtpServer",0, 
        JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub, 
        JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub 
}; 

static JSClass js_ftp_class = {
        "Ftp",0, 
        JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub, 
        JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub 
}; 

JSContext* js_initcx(SOCKET sock, JSObject** glob, JSObject** ftp)
{
	char		ver[256];
	JSContext*	js_cx;
	JSObject*	js_glob;
	JSObject*	server;
	jsval		val;
	BOOL		success=FALSE;

	lprintf("%04d JavaScript: Initializing context",sock);

    if((js_cx = JS_NewContext(js_runtime, JAVASCRIPT_CONTEXT_STACK))==NULL)
		return(NULL);

	lprintf("%04d JavaScript: Context created",sock);

	JS_BeginRequest(js_cx);	/* Required for multi-thread support */

    JS_SetErrorReporter(js_cx, js_ErrorReporter);

	do {

		lprintf("%04d JavaScript: Initializing Global object",sock);
		if((js_glob=js_CreateGlobalObject(js_cx, &scfg))==NULL) 
			break;

		if (!JS_DefineFunctions(js_cx, js_glob, js_global_functions)) 
			break;

		lprintf("%04d JavaScript: Initializing System object",sock);
		if(js_CreateSystemObject(js_cx, js_glob, &scfg)==NULL) 
			break;

		if((*ftp=JS_DefineObject(js_cx, js_glob, "ftp", &js_ftp_class
			,NULL,0))==NULL)
			break;

		if((server=JS_DefineObject(js_cx, js_glob, "server", &js_server_class
			,NULL,0))==NULL)
			break;

		sprintf(ver,"%s v%s",FTP_SERVER,FTP_VERSION);
		val = STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, ver));
		if(!JS_SetProperty(js_cx, server, "version", &val))
			break;

		val = STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, ftp_ver()));
		if(!JS_SetProperty(js_cx, server, "version_detail", &val))
			break;

		if(glob!=NULL)
			*glob=js_glob;

		success=TRUE;

	} while(0);

	JS_EndRequest(js_cx);		/* Required for multi-thread support */

	if(!success) {
		JS_DestroyContext(js_cx);
		return(NULL);
	}

	return(js_cx);
}

static JSClass js_file_class = {
        "FtpFile",0, 
        JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub, 
        JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub 
}; 

BOOL js_add_file(JSContext* js_cx, JSObject* array, 
				 char* name, char* desc, char* ext_desc,
				 ulong size, ulong credits, 
				 time_t time, time_t uploaded, time_t last_downloaded, 
				 ulong times_downloaded, ulong misc, 
				 char* uploader, char* link)
{
	JSObject*	file;
	jsval		val;
	jsint		index;

	if((file=JS_NewObject(js_cx, &js_file_class, NULL, NULL))==NULL)
		return(FALSE);

	val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, name));
	if(!JS_SetProperty(js_cx, file, "name", &val))
		return(FALSE);

	val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, desc));
	if(!JS_SetProperty(js_cx, file, "description", &val))
		return(FALSE);

	val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, ext_desc));
	if(!JS_SetProperty(js_cx, file, "extended_description", &val))
		return(FALSE);

	val=INT_TO_JSVAL(size);
	if(!JS_SetProperty(js_cx, file, "size", &val))
		return(FALSE);

	val=INT_TO_JSVAL(credits);
	if(!JS_SetProperty(js_cx, file, "credits", &val))
		return(FALSE);

	val=INT_TO_JSVAL(time);
	if(!JS_SetProperty(js_cx, file, "time", &val))
		return(FALSE);

	val=INT_TO_JSVAL(uploaded);
	if(!JS_SetProperty(js_cx, file, "uploaded", &val))
		return(FALSE);

	val=INT_TO_JSVAL(last_downloaded);
	if(!JS_SetProperty(js_cx, file, "last_downloaded", &val))
		return(FALSE);

	val=INT_TO_JSVAL(times_downloaded);
	if(!JS_SetProperty(js_cx, file, "times_downloaded", &val))
		return(FALSE);

	val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, uploader));
	if(!JS_SetProperty(js_cx, file, "uploader", &val))
		return(FALSE);

	val=INT_TO_JSVAL(misc);
	if(!JS_SetProperty(js_cx, file, "settings", &val))
		return(FALSE);

	val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, link));
	if(!JS_SetProperty(js_cx, file, "link", &val))
		return(FALSE);

	if(!JS_GetArrayLength(js_cx, array, &index))
		return(FALSE);

	val=OBJECT_TO_JSVAL(file);
	return(JS_SetElement(js_cx, array, index, &val));
}

BOOL js_generate_index(JSContext* js_cx, JSObject* parent, 
					   SOCKET sock, FILE* fp, int lib, int dir, user_t* user)
{
	char		str[256];
	char		path[MAX_PATH+1];
	char		spath[MAX_PATH+1];	/* script path */
	char		vpath[MAX_PATH+1];	/* virtual path */
	char		aliasfile[MAX_PATH+1];
	char		extdesc[513];
	char*		p;
	char*		tp;
	char*		np;
	char*		dp;
	char		aliasline[512];
	BOOL		alias_dir;
	BOOL		success=FALSE;
	FILE*		alias_fp;
	uint		i;
	file_t		f;
	glob_t		g;
	jsval		val;
	jsval		rval;
	JSObject*	lib_obj=NULL;
	JSObject*	dir_obj=NULL;
	JSObject*	file_array=NULL;
	JSObject*	dir_array=NULL;
	JSScript*	js_script=NULL;

	JS_SetContextPrivate(js_cx, fp);

	do {	/* pseudo try/catch */

		if((file_array=JS_NewArrayObject(js_cx, 0, NULL))==NULL) {
			lprintf("%04d !JavaScript FAILED to create file_array",sock);
			break;
		}

		if((dir_array=JS_NewArrayObject(js_cx, 0, NULL))==NULL) {
			lprintf("%04d !JavaScript FAILED to create dir_array",sock);
			break;
		}

		/* Add extension if not specified */
		if(!strchr(startup->html_index_script,BACKSLASH))
			sprintf(spath,"%s%s",scfg.exec_dir,startup->html_index_script);
		else
			sprintf(spath,"%.*s",sizeof(spath)-4,startup->html_index_script);
		if(!strchr(spath,'.'))
			strcat(spath,".js");

		if(!fexist(spath)) {
			lprintf("%04d !HTML JavaScript (%s) doesn't exist",sock,spath);
			break;
		}

		val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, startup->html_index_file));
		if(!JS_SetProperty(js_cx, parent, "html_index_file", &val)) {
			lprintf("%04d !JavaScript FAILED to set html_index_file property",sock);
			break;
		}

		/* file[] */
		val=OBJECT_TO_JSVAL(file_array);
		if(!JS_SetProperty(js_cx, parent, "file_list", &val)) {
			lprintf("%04d !JavaScript FAILED to set file property",sock);
			break;
		}

		/* dir[] */
		val=OBJECT_TO_JSVAL(dir_array);
		if(!JS_SetProperty(js_cx, parent, "dir_list", &val)) {
			lprintf("%04d !JavaScript FAILED to set dir property",sock);
			break;
		}

		/* curlib */
		if((lib_obj=JS_NewObject(js_cx, &js_file_class, 0, NULL))==NULL) {
			lprintf("%04d !JavaScript FAILED to create lib_obj",sock);
			break;
		}

		val=OBJECT_TO_JSVAL(lib_obj);
		if(!JS_SetProperty(js_cx, parent, "curlib", &val)) {
			lprintf("%04d !JavaScript FAILED to set curlib property",sock);
			break;
		}

		/* curdir */
		if((dir_obj=JS_NewObject(js_cx, &js_file_class, 0, NULL))==NULL) {
			lprintf("%04d !JavaScript FAILED to create dir_obj",sock);
			break;
		}

		val=OBJECT_TO_JSVAL(dir_obj);
		if(!JS_SetProperty(js_cx, parent, "curdir", &val)) {
			lprintf("%04d !JavaScript FAILED to set curdir property",sock);
			break;
		}

		strcpy(vpath,"/");

		if(lib>=0) { /* root */

			strcat(vpath,scfg.lib[lib]->sname);
			strcat(vpath,"/");

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, scfg.lib[lib]->sname));
			if(!JS_SetProperty(js_cx, lib_obj, "name", &val)) {
				lprintf("%04d !JavaScript FAILED to set curlib.name property",sock);
				break;
			}

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, scfg.lib[lib]->lname));
			if(!JS_SetProperty(js_cx, lib_obj, "description", &val)) {
				lprintf("%04d !JavaScript FAILED to set curlib.desc property",sock);
				break;
			}
		}

		if(dir>=0) { /* 1st level */
			strcat(vpath,scfg.dir[dir]->code);
			strcat(vpath,"/");

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, scfg.dir[dir]->code));
			if(!JS_SetProperty(js_cx, dir_obj, "code", &val)) {
				lprintf("%04d !JavaScript FAILED to set curdir.code property",sock);
				break;
			}

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, scfg.dir[dir]->sname));
			if(!JS_SetProperty(js_cx, dir_obj, "name", &val)) {
				lprintf("%04d !JavaScript FAILED to set curdir.name property",sock);
				break;
			}

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, scfg.dir[dir]->lname));
			if(!JS_SetProperty(js_cx, dir_obj, "description", &val)) {
				lprintf("%04d !JavaScript FAILED to set curdir.desc property",sock);
				break;
			}

			val=INT_TO_JSVAL(scfg.dir[dir]->misc);
			if(!JS_SetProperty(js_cx, dir_obj, "settings", &val)) {
				lprintf("%04d !JavaScript FAILED to set curdir.misc property",sock);
				break;
			}
		}

		val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, vpath));
		if(!JS_SetProperty(js_cx, parent, "path", &val)) {
			lprintf("%04d !JavaScript FAILED to set curdir property",sock);
			break;
		}

		if(lib<0) {	/* root dir */

			/* File Aliases */
			sprintf(path,"%sftpalias.cfg",scfg.ctrl_dir);
			if((alias_fp=fopen(path,"r"))!=NULL) {

				while(!feof(alias_fp)) {
					if(!fgets(aliasline,sizeof(aliasline)-1,alias_fp))
						break;

					p=aliasline;	/* alias pointer */
					while(*p && *p<=' ') p++;

					if(*p==';')	/* comment */
						continue;

					tp=p;		/* terminator pointer */
					while(*tp && *tp>' ') tp++;
					if(*tp) *tp=0;

					np=tp+1;	/* filename pointer */
					while(*np && *np<=' ') np++;

					tp=np;		/* terminator pointer */
					while(*tp && *tp>' ') tp++;
					if(*tp) *tp=0;

					dp=tp+1;	/* description pointer */
					while(*dp && *dp<=' ') dp++;
					truncsp(dp);

					alias_dir=FALSE;

					/* Virtual Path? */
					if(!strnicmp(np,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
						if((dir=getdir(np+strlen(BBS_VIRTUAL_PATH),user))<0)
							continue; /* No access or invalid virtual path */
						tp=strrchr(np,'/');
						if(tp==NULL) 
							continue;
						tp++;
						if(*tp) {
							sprintf(aliasfile,"%s%s",scfg.dir[dir]->path,tp);
							np=aliasfile;
						}
						else 
							alias_dir=TRUE;
					}

					if(!alias_dir && !fexist(np))
						continue;

					if(alias_dir) {
						if(!chk_ar(&scfg,scfg.dir[dir]->ar,user))
							continue;
						sprintf(vpath,"/%s/%s",p,startup->html_index_file);
					} else
						strcpy(vpath,p);
					js_add_file(js_cx
						,alias_dir ? dir_array : file_array
						,p				/* filename */
						,dp				/* description */
						,NULL			/* extdesc */
						,flength(np)	/* size */
						,0				/* credits */
						,fdate(np)		/* time */
						,fdate(np)		/* uploaded */
						,0				/* last downloaded */
						,0				/* times downloaded */
						,0				/* misc */
						,scfg.sys_id	/* uploader */
						,vpath			/* link */
						);
				}

				fclose(alias_fp);
			}

			/* QWK Packet */
			if(startup->options&FTP_OPT_ALLOW_QWK /* && fexist(qwkfile) */) {
				sprintf(str,"%s.qwk",scfg.sys_id);
				sprintf(vpath,"/%s",str);
				js_add_file(js_cx
					,file_array 
					,str						/* filename */
					,"QWK Message Packet"		/* description */
					,NULL						/* extdesc */
					,10240						/* size */
					,0							/* credits */
					,time(NULL)					/* time */
					,0							/* uploaded */
					,0							/* last downloaded */
					,0							/* times downloaded */
					,0							/* misc */
					,scfg.sys_id				/* uploader */
					,str						/* link */
					);
			}

			/* Library Folders */
			for(i=0;i<scfg.total_libs;i++) {
				if(!chk_ar(&scfg,scfg.lib[i]->ar,user))
					continue;
				sprintf(vpath,"/%s/%s",scfg.lib[i]->sname,startup->html_index_file);
				js_add_file(js_cx
					,dir_array 
					,scfg.lib[i]->sname			/* filename */
					,scfg.lib[i]->lname			/* description */
					,NULL						/* extdesc */
					,0,0,0,0,0,0,0				/* unused */
					,scfg.sys_id				/* uploader */
					,vpath						/* link */
					);
			}
		} else if(dir<0) {
			/* Directories */
			for(i=0;i<scfg.total_dirs;i++) {
				if(scfg.dir[i]->lib!=lib)
					continue;
				if(/* i!=scfg.sysop_dir && i!=scfg.upload_dir && */
					!chk_ar(&scfg,scfg.dir[i]->ar,user))
					continue;
				sprintf(vpath,"/%s/%s/%s"
					,scfg.lib[scfg.dir[i]->lib]->sname
					,scfg.dir[i]->code
					,startup->html_index_file);
				js_add_file(js_cx
					,dir_array 
					,scfg.dir[i]->sname			/* filename */
					,scfg.dir[i]->lname			/* description */
					,NULL						/* extdesc */
					,getfiles(&scfg,i)			/* size */
					,0,0,0,0,0					/* unused */
					,scfg.dir[i]->misc			/* misc */
					,scfg.sys_id				/* uploader */
					,vpath						/* link */
					);

			}
		} else if(chk_ar(&scfg,scfg.dir[dir]->ar,user)){
			sprintf(path,"%s*",scfg.dir[dir]->path);
			glob(path,0,NULL,&g);
			for(i=0;i<(int)g.gl_pathc;i++) {
				if(isdir(g.gl_pathv[i]))
					continue;
	#ifdef _WIN32
				GetShortPathName(g.gl_pathv[i], str, sizeof(str));
	#else
				strcpy(str,g.gl_pathv[i]);
	#endif
				padfname(getfname(str),f.name);
				strupr(f.name);
				f.dir=dir;
				if(getfileixb(&scfg,&f)) {
					f.size=0; /* flength(g.gl_pathv[i]); */
					getfiledat(&scfg,&f);
					if(f.misc&FM_EXTDESC) {
						extdesc[0]=0;
						getextdesc(&scfg, dir, f.datoffset, extdesc);
						/* Remove Ctrl-A Codes and Ex-ASCII code */
						remove_ctrl_a(extdesc,NULL);
					}
					sprintf(vpath,"/%s/%s/%s"
						,scfg.lib[scfg.dir[dir]->lib]->sname
						,scfg.dir[dir]->code
						,getfname(g.gl_pathv[i]));
					js_add_file(js_cx
						,file_array 
						,getfname(g.gl_pathv[i])	/* filename */
						,f.desc						/* description */
						,f.misc&FM_EXTDESC ? extdesc : NULL
						,f.size						/* size */
						,f.cdt						/* credits */
						,f.date						/* time */
						,f.dateuled					/* uploaded */
						,f.datedled					/* last downloaded */
						,f.timesdled				/* times downloaded */
						,f.misc						/* misc */
						,f.uler						/* uploader */
						,getfname(g.gl_pathv[i])	/* link */
						);
				}
			}
			globfree(&g);
		}

		/* RUN SCRIPT */
		if((js_script=JS_CompileFile(js_cx, parent, spath))==NULL) {
			lprintf("%04d !JavaScript FAILED to compile script (%s)",sock,spath);
			break;
		}

		if((success=JS_ExecuteScript(js_cx, parent, js_script, &rval))!=TRUE) {
			lprintf("%04d !JavaScript FAILED to execute script (%s)",sock,spath);
			break;
		}

	} while(0);


	if(js_script!=NULL) 
		JS_DestroyScript(js_cx, js_script);

	JS_DeleteProperty(js_cx, parent, "path");
	JS_DeleteProperty(js_cx, parent, "sort");
	JS_DeleteProperty(js_cx, parent, "reverse");
	JS_DeleteProperty(js_cx, parent, "file_list");
	JS_DeleteProperty(js_cx, parent, "dir_list");
	JS_DeleteProperty(js_cx, parent, "curlib");
	JS_DeleteProperty(js_cx, parent, "curdir");
	JS_DeleteProperty(js_cx, parent, "html_index_file");

	return(success);
}


#endif

static int nopen(char *str, int access)
{
	int file,share,count=0;

    if(access&O_DENYNONE) {
        share=SH_DENYNO;
        access&=~O_DENYNONE; }
    else if(access==O_RDONLY) share=SH_DENYWR;
    else share=SH_DENYRW;
    while(((file=sopen(str,O_BINARY|access,share))==-1)
        && errno==EACCES && count++<LOOP_NOPEN)
        if(count)
            mswait(100);
    return(file);
}

time_t gettimeleft(scfg_t* cfg, user_t* user, time_t starttime)
{
	time_t	now;
    long    tleft;
	time_t	timeleft;

	now=time(NULL);

	if(user->exempt&FLAG('T')) {   /* Time online exemption */
		timeleft=cfg->level_timepercall[user->level]*60;
		if(timeleft<10)             /* never get below 10 for exempt users */
			timeleft=10; }
	else {
		tleft=(((long)cfg->level_timeperday[user->level]-user->ttoday)
			+user->textra)*60L;
		if(tleft<0) tleft=0;
		if(tleft>cfg->level_timepercall[user->level]*60)
			tleft=cfg->level_timepercall[user->level]*60;
		tleft+=user->min*60L;
		tleft-=now-starttime;
		if(tleft>0x7fffL)
			timeleft=0x7fff;
		else
			timeleft=tleft; }

	return(timeleft);
}

static time_t checktime(void)
{
	struct tm tm;

    memset(&tm,0,sizeof(tm));
    tm.tm_year=94;
    tm.tm_mday=1;
    return(mktime(&tm)-0x2D24BD00L);
}

BOOL upload_stats(ulong bytes)
{
	char	str[MAX_PATH+1];
	int		file;
	ulong	val;

	sprintf(str,"%sdsts.dab",scfg.ctrl_dir);
	if((file=nopen(str,O_RDWR))==-1) 
		return(FALSE);

	lseek(file,20L,SEEK_SET);   /* Skip timestamp, logons and logons today */
	read(file,&val,4);        /* Uploads today         */
	val++;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	read(file,&val,4);        /* Upload bytes today    */
	val+=bytes;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	close(file);
	return(TRUE);
}

BOOL download_stats(ulong bytes)
{
	char	str[MAX_PATH+1];
	int		file;
	ulong	val;

	sprintf(str,"%sdsts.dab",scfg.ctrl_dir);
	if((file=nopen(str,O_RDWR))==-1) 
		return(FALSE);

	lseek(file,28L,SEEK_SET);   /* Skip timestamp, logons and logons today */
	read(file,&val,4);        /* Downloads today         */
	val++;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	read(file,&val,4);        /* Download bytes today    */
	val+=bytes;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	close(file);
	return(TRUE);
}

void recverror(SOCKET socket, int rd, int line)
{
	if(rd==0) 
		lprintf("%04d Socket closed by peer on receive (line %d)"
			,socket, line);
	else if(rd==SOCKET_ERROR) {
		if(ERROR_VALUE==ECONNRESET) 
			lprintf("%04d Connection reset by peer on receive (line %d)"
				,socket, line);
		else if(ERROR_VALUE==ECONNABORTED) 
			lprintf("%04d Connection aborted by peer on receive (line %d)"
				,socket, line);
		else
			lprintf("%04d !ERROR %d receiving on socket (line %d)"
				,socket, ERROR_VALUE, line);
	} else
		lprintf("%04d !ERROR: recv on socket returned unexpected value: %d (line %d)"
			,socket, rd, line);
}

int sockreadline(SOCKET socket, char* buf, int len, time_t* lastactive)
{
	char	ch;
	int		i,rd=0;
	fd_set	socket_set;
	struct timeval	tv;
	
	while(rd<len-1) {

		tv.tv_sec=0;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(socket,&socket_set);

		i=select(socket+1,&socket_set,NULL,NULL,&tv);

		if(server_socket==INVALID_SOCKET) {
			sockprintf(socket,"421 Server downed, aborting.");
			lprintf("%04d Server downed, aborting.",socket);
			return(0);
		}
		if(i<1) {
			if(i==0) {
				if((time(NULL)-(*lastactive))>startup->max_inactivity) {
					lprintf("%04d Disconnecting due to to inactivity.",socket);
					sockprintf(socket,"421 Disconnecting due to inactivity (%u seconds)."
						,startup->max_inactivity);
					return(0);
				}
				mswait(1);
				continue;
			}
			recverror(socket,i,__LINE__);
			return(i);
		}
		socket_debug[socket]|=SOCKET_DEBUG_RECV_CHAR;
		i=recv(socket, &ch, 1, 0);
		socket_debug[socket]&=~SOCKET_DEBUG_RECV_CHAR;
		if(i<1) {
#if 0
			if(ERROR_VALUE==EWOULDBLOCK) {
				mswait(1);
				continue;
			}
#endif
			recverror(socket,i,__LINE__);
			return(i);
		}
		if(ch=='\n' && rd>=1) {
			break;
		}	
		buf[rd++]=ch;
	}
	buf[rd-1]=0;
	
	return(rd);
}

/*****************************************************************************/
/* Returns command line generated from instr with %c replacments             */
/*****************************************************************************/
char * cmdstr(user_t* user, char *instr, char *fpath, char *fspec, char *cmd)
{
	char	str[256];
    int		i,j,len;
#ifdef _WIN32
	char sfpath[MAX_PATH];
#endif

    len=strlen(instr);
    for(i=j=0;i<len;i++) {
        if(instr[i]=='%') {
            i++;
            cmd[j]=0;
            switch(toupper(instr[i])) {
                case 'A':   /* User alias */
                    strcat(cmd,user->alias);
                    break;
                case 'B':   /* Baud (DTE) Rate */
                case 'C':   /* Connect Description */
                case 'D':   /* Connect (DCE) Rate */
                case 'E':   /* Estimated Rate */
                case 'H':   /* Port Handle or Hardware Flow Control */
                case 'P':   /* COM Port */
                case 'R':   /* Rows */
                case 'T':   /* Time left in seconds */
                case '&':   /* Address of msr */
                case 'Y':	/* COMSPEC */
					/* UNSUPPORTED */
					break;
                case 'F':   /* File path */
                    strcat(cmd,fpath);
                    break;
                case 'G':   /* Temp directory */
                    strcat(cmd,scfg.temp_dir);
                    break;
                case 'I':   /* UART IRQ Line */
                    strcat(cmd,ultoa(scfg.com_irq,str,10));
                    break;
                case 'J':
                    strcat(cmd,scfg.data_dir);
                    break;
                case 'K':
                    strcat(cmd,scfg.ctrl_dir);
                    break;
                case 'L':   /* Lines per message */
                    strcat(cmd,ultoa(scfg.level_linespermsg[user->level],str,10));
                    break;
                case 'M':   /* Minutes (credits) for user */
                    strcat(cmd,ultoa(user->min,str,10));
                    break;
                case 'N':   /* Node Directory (same as SBBSNODE environment var) */
                    strcat(cmd,scfg.node_dir);
                    break;
                case 'O':   /* SysOp */
                    strcat(cmd,scfg.sys_op);
                    break;
                case 'Q':   /* QWK ID */
                    strcat(cmd,scfg.sys_id);
                    break;
                case 'S':   /* File Spec */
                    strcat(cmd,fspec);
                    break;
                case 'U':   /* UART I/O Address (in hex) */
                    strcat(cmd,ultoa(scfg.com_base,str,16));
                    break;
                case 'V':   /* Synchronet Version */
                    sprintf(str,"%s%c",VERSION,REVISION);
                    break;
                case 'W':   /* Time-slice API type (mswtype) */
#if 0 //ndef __FLAT__
                    strcat(cmd,ultoa(mswtyp,str,10));
#endif
                    break;
                case 'X':
                    strcat(cmd,scfg.shell[user->shell]->code);
                    break;
                case 'Z':
                    strcat(cmd,scfg.text_dir);
                    break;
				case '~':	/* DOS-compatible (8.3) filename */
#ifdef _WIN32
					strcpy(sfpath,fpath);
					GetShortPathName(fpath,sfpath,sizeof(sfpath));
					strcat(cmd,sfpath);
#else
                    strcat(cmd,fpath);
#endif			
					break;
                case '!':   /* EXEC Directory */
                    strcat(cmd,scfg.exec_dir);
                    break;
                case '#':   /* Node number (same as SBBSNNUM environment var) */
                    sprintf(str,"%d",scfg.node_num);
                    strcat(cmd,str);
                    break;
                case '*':
                    sprintf(str,"%03d",scfg.node_num);
                    strcat(cmd,str);
                    break;
                case '$':   /* Credits */
                    strcat(cmd,ultoa(user->cdt+user->freecdt,str,10));
                    break;
                case '%':   /* %% for percent sign */
                    strcat(cmd,"%");
                    break;
				case '?':	/* Platform */
#ifdef __OS2__
					strcpy(str,"OS2");
#else
					strcpy(str,PLATFORM_DESC);
#endif
					strlwr(str);
					strcat(cmd,str);
					break;
                default:    /* unknown specification */
                    if(isdigit(instr[i])) {
                        sprintf(str,"%0*d",instr[i]&0xf,user->number);
                        strcat(cmd,str); }
                    break; }
            j=strlen(cmd); }
        else
            cmd[j++]=instr[i]; }
    cmd[j]=0;

    return(cmd);
}


void DLLCALL ftp_terminate(void)
{
	if(server_socket!=INVALID_SOCKET) {
    	lprintf("%04d FTP Terminate: closing socket",server_socket);
		ftp_close_socket(&server_socket,__LINE__);
	    server_socket=INVALID_SOCKET;
    }
}


typedef struct {
	SOCKET	ctrl_sock;
	SOCKET*	data_sock;
	BOOL*	inprogress;
	BOOL*	aborted;
	BOOL	delfile;
	BOOL	tmpfile;
	BOOL	credits;
	BOOL	append;
	long	filepos;
	char	filename[MAX_PATH+1];
	time_t*	lastactive;
	user_t*	user;
	int		dir;
	char*	desc;
} xfer_t;

static void send_thread(void* arg)
{
	char		buf[8192];
	char		fname[MAX_PATH+1];
	int			i;
	int			rd;
	int			wr;
	ulong		total=0;
	ulong		dur;
	ulong		cps;
	ulong		length;
	BOOL		error=FALSE;
	FILE*		fp;
	file_t		f;
	xfer_t		xfer;
	time_t		now;
	time_t		start;
	time_t		last_report;

	xfer=*(xfer_t*)arg;

	length=flength(xfer.filename);

	if((fp=fopen(xfer.filename,"rb"))==NULL) {
		lprintf("%04d !DATA ERROR %d opening %s",xfer.ctrl_sock,errno,xfer.filename);
		sockprintf(xfer.ctrl_sock,"450 ERROR %d opening %s.",errno,xfer.filename);
		if(xfer.tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
			remove(xfer.filename);
		ftp_close_socket(xfer.data_sock,__LINE__);
		*xfer.inprogress=FALSE;
		return;
	}
	thread_up();

	*xfer.inprogress=TRUE;
	*xfer.aborted=FALSE;
	if(startup->options&FTP_OPT_DEBUG_DATA || xfer.filepos)
		lprintf("%04d DATA socket %d sending %s from offset %ld"
			,xfer.ctrl_sock,*xfer.data_sock,xfer.filename,xfer.filepos);

	fseek(fp,xfer.filepos,SEEK_SET);
	last_report=start=time(NULL);
	while(!feof(fp)) {
		now=time(NULL);
		if(total && now>=last_report+XFER_REPORT_INTERVAL) {
			lprintf("%04d Sent %ld bytes (%ld total) of %s (%lu cps)"
				,xfer.ctrl_sock,xfer.filepos+total,length,xfer.filename
				,now-start ? total/(now-start) : total*2);
			last_report=now;
		}

		if(*xfer.aborted==TRUE) {
			lprintf("%04d !DATA Transfer aborted",xfer.ctrl_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer aborted.");
			error=TRUE;
			break;
		}
		if(server_socket==INVALID_SOCKET) {
			lprintf("%04d !DATA Transfer locally aborted",xfer.ctrl_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer locally aborted.");
			error=TRUE;
			break;
		}
		rd=fread(buf,sizeof(char),sizeof(buf),fp);
		if(rd<1) /* EOF or READ error */
			break;

		wr=send(*xfer.data_sock,buf,rd,0);
		if(wr!=rd) {
			if(wr==SOCKET_ERROR) {
				if(ERROR_VALUE==ECONNRESET) 
					lprintf("%04d DATA Connection reset by peer, sending on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else if(ERROR_VALUE==ECONNABORTED) 
					lprintf("%04d DATA Connection aborted by peer, sending on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else
					lprintf("%04d !DATA ERROR %d sending on data socket %d"
						,xfer.ctrl_sock,ERROR_VALUE,*xfer.data_sock);
				sockprintf(xfer.ctrl_sock,"426 Error %d sending on DATA channel",ERROR_VALUE);
				error=TRUE;
				break;
			}
			if(wr==0) {
				lprintf("%04d !DATA socket %d disconnected",xfer.ctrl_sock, *xfer.data_sock);
				sockprintf(xfer.ctrl_sock,"426 DATA channel disconnected");
				error=TRUE;
				break;
			}
			lprintf("%04d !DATA ERROR sent %d instead of %d on socket %d"
				,xfer.ctrl_sock,wr,rd,*xfer.data_sock);
			sockprintf(xfer.ctrl_sock,"451 Short DATA transfer");
			error=TRUE;
			break;
		}
		total+=wr;
		*xfer.lastactive=time(NULL);	/* exception here */
		mswait(1);
	}

	if((i=ferror(fp))!=0) 
		lprintf("%04d !FILE ERROR %d (%d)",xfer.ctrl_sock,i,errno);

	ftp_close_socket(xfer.data_sock,__LINE__);	/* Signal end of file */
	if(startup->options&FTP_OPT_DEBUG_DATA)
		lprintf("%04d DATA socket closed",xfer.ctrl_sock);
	
	if(!error) {
		dur=time(NULL)-start;
		cps=dur ? total/dur : total*2;
		lprintf("%04d Transfer successful: %lu bytes sent in %lu seconds (%lu cps)"
			,xfer.ctrl_sock
			,total,dur,cps);
		sockprintf(xfer.ctrl_sock,"226 Download complete (%lu cps).",cps);

		if(xfer.dir>=0) {
			memset(&f,0,sizeof(f));
#ifdef _WIN32
			GetShortPathName(xfer.filename,fname,sizeof(fname));
#else
			strcpy(fname,xfer.filename);
#endif
			padfname(getfname(fname),f.name);
			strupr(f.name);
			f.dir=xfer.dir;
			f.size=total;
			if(getfileixb(&scfg,&f)==TRUE && getfiledat(&scfg,&f)==TRUE) {
				f.timesdled++;
				putfiledat(&scfg,&f);
			lprintf("%04d %s downloaded: %s (%lu times total)"
				,xfer.ctrl_sock
				,xfer.user->alias
				,xfer.filename
				,f.timesdled);
			}
			/* Need to update datedled in index */
		}	

		if(xfer.credits) {
			xfer.user->dls=(ushort)adjustuserrec(&scfg, xfer.user->number,U_DLS,5,1);
			xfer.user->dlb=adjustuserrec(&scfg, xfer.user->number,U_DLB,10,total);
			if(xfer.dir>=0 && !(scfg.dir[xfer.dir]->misc&DIR_FREE) 
				/* && !chk_ar(&scfg, scfg.dir[xfer.dir]->ex_ar, xfer.user) */
				&& !(xfer.user->exempt&FLAG('D')))
				subtract_cdt(&scfg, xfer.user, xfer.credits);
		}
		if(!xfer.tmpfile && !xfer.delfile)
			download_stats(total);
	}

	fclose(fp);
	*xfer.inprogress=FALSE;
	if(xfer.tmpfile) {
		if(!(startup->options&FTP_OPT_KEEP_TEMP_FILES))
			remove(xfer.filename);
	} 
	else if(xfer.delfile && !error)
		remove(xfer.filename);

	thread_down();
}

static void receive_thread(void* arg)
{
	char*		p;
	char		buf[8192];
	char		ext[F_EXBSIZE+1];
	char		desc[F_EXBSIZE+1];
	char		cmd[MAX_PATH*2];
	char		tmp[MAX_PATH+1];
	char		fname[MAX_PATH+1];
	int			i;
	int			rd;
	int			file;
	ulong		total=0;
	ulong		dur;
	ulong		cps;
	BOOL		error=FALSE;
	FILE*		fp;
	file_t		f;
	xfer_t		xfer;
	time_t		now;
	time_t		start;
	time_t		last_report;

	xfer=*(xfer_t*)arg;

	if((fp=fopen(xfer.filename,xfer.append ? "ab" : "wb"))==NULL) {
		lprintf("%04d !DATA ERROR %d opening %s",xfer.ctrl_sock,errno,xfer.filename);
		sockprintf(xfer.ctrl_sock,"450 ERROR %d opening %s.",errno,xfer.filename);
		ftp_close_socket(xfer.data_sock,__LINE__);
		return;
	}

	thread_up();

	*xfer.inprogress=TRUE;
	*xfer.aborted=FALSE;
	if(xfer.filepos || startup->options&FTP_OPT_DEBUG_DATA)
		lprintf("%04d DATA socket %d receiving from offset %ld"
			,xfer.ctrl_sock,*xfer.data_sock,xfer.filepos);

	fseek(fp,xfer.filepos,SEEK_SET);
	last_report=start=time(NULL);
	while(1) {

		now=time(NULL);
		if(total && now>=last_report+XFER_REPORT_INTERVAL) {
			lprintf("%04d Received %ld bytes of %s (%lu cps)"
				,xfer.ctrl_sock,total,xfer.filename
				,now-start? total/(now-start) : total*2);
			last_report=now;
		}
		if(*xfer.aborted==TRUE) {
			lprintf("%04d !DATA Transfer aborted",xfer.ctrl_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"426 Transfer aborted.");
			error=TRUE;
			break;
		}
		if(server_socket==INVALID_SOCKET) {
			lprintf("%04d !DATA Transfer locally aborted",xfer.ctrl_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"426 Transfer locally aborted.");
			error=TRUE;
			break;
		}
		socket_debug[xfer.ctrl_sock]|=SOCKET_DEBUG_RECV_BUF;
		rd=recv(*xfer.data_sock,buf,sizeof(buf),0);
		socket_debug[xfer.ctrl_sock]&=~SOCKET_DEBUG_RECV_BUF;
		if(rd<1) {
			if(rd==0) { /* Socket closed */
				if(startup->options&FTP_OPT_DEBUG_DATA)
					lprintf("%04d DATA socket %d closed by client"
						,xfer.ctrl_sock,*xfer.data_sock);
				break;
			}
			if(rd==SOCKET_ERROR) {
				if(ERROR_VALUE==ECONNRESET) 
					lprintf("%04d Connection reset by peer, receiving on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else if(ERROR_VALUE==ECONNABORTED) 
					lprintf("%04d Connection aborted by peer, receiving on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else
					lprintf("%04d !DATA ERROR %d receiving on data socket %d"
						,xfer.ctrl_sock,ERROR_VALUE,*xfer.data_sock);
				/* Send NAK */
				sockprintf(xfer.ctrl_sock,"426 Error %d receiving on DATA channel"
					,ERROR_VALUE);
				error=TRUE;
				break;
			}
			lprintf("%04d !DATA ERROR recv returned %d on socket %d"
				,xfer.ctrl_sock,rd,*xfer.data_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"451 Unexpected socket error: %d",rd);
			error=TRUE;
			break;
		}
		fwrite(buf,1,rd,fp);
		total+=rd;
		*xfer.lastactive=time(NULL);
		mswait(1);
	}

	*xfer.inprogress=FALSE;
	fclose(fp);

	ftp_close_socket(xfer.data_sock,__LINE__);
	if(error && startup->options&FTP_OPT_DEBUG_DATA)
		lprintf("%04d DATA socket %d closed",xfer.ctrl_sock,*xfer.data_sock);
	
	if(!error) {
		dur=time(NULL)-start;
		cps=dur ? total/dur : total*2;
		lprintf("%04d Transfer successful: %lu bytes received in %lu seconds (%lu cps)"
			,xfer.ctrl_sock
			,total,dur,cps);

		if(xfer.dir>=0) {
			memset(&f,0,sizeof(f));
#ifdef _WIN32
			GetShortPathName(xfer.filename,fname,sizeof(fname));
#else
			strcpy(fname,xfer.filename);
#endif
			padfname(getfname(fname),f.name);
			strupr(f.name);
			f.dir=xfer.dir;
			if(scfg.dir[f.dir]->misc&DIR_AONLY)  /* Forced anonymous */
				f.misc|=FM_ANON;
			f.cdt=total;
			f.dateuled=time(NULL);
			f.timesdled=0;
			f.datedled=0L;
			f.opencount=0;

			/* Desciption specified with DESC command? */
			if(xfer.desc!=NULL && *xfer.desc!=0)	
				sprintf(f.desc,"%.*s",(int)sizeof(f.desc)-1,xfer.desc);

			/* FILE_ID.DIZ support */
			p=strrchr(f.name,'.');
			if(p!=NULL && scfg.dir[f.dir]->misc&DIR_DIZ) {
				for(i=0;i<scfg.total_fextrs;i++)
					if(!stricmp(scfg.fextr[i]->ext,p+1) 
						&& chk_ar(&scfg,scfg.fextr[i]->ar,xfer.user))
						break;
				if(i<scfg.total_fextrs) {
					sprintf(tmp,"%sFILE_ID.DIZ",scfg.temp_dir);
					remove(tmp);
					system(cmdstr(xfer.user,scfg.fextr[i]->cmd,fname,"FILE_ID.DIZ",cmd));
					if(!fexist(tmp)) {
						sprintf(tmp,"%sDESC.SDI",scfg.temp_dir);
						remove(tmp);
						system(cmdstr(xfer.user,scfg.fextr[i]->cmd,fname,"DESC.SDI",cmd)); 
					}
					if((file=nopen(tmp,O_RDONLY))!=-1) {
						memset(ext,0,sizeof(ext));
						read(file,ext,sizeof(ext)-1);
						for(i=sizeof(ext)-1;i;i--)	/* trim trailing spaces */
							if(ext[i-1]>SP)
								break;
						ext[i]=0;
						if(!f.desc[0]) {			/* use for normal description */
							strcpy(desc,ext);
							strip_exascii(desc);	/* strip extended ASCII chars */
							strip_ctrl(desc);		/* strip control chars */
							for(i=0;desc[i];i++)	/* find approprate first char */
								if(isalnum(desc[i]))
									break;
							sprintf(f.desc,"%.*s",LEN_FDESC,desc+i); 
						}
						close(file);
						remove(tmp);
						f.misc|=FM_EXTDESC; 
					} 
				} 
			} /* FILE_ID.DIZ support */

			if(f.desc[0]==0) 	/* no description given, use (long) filename */
				sprintf(f.desc,"%.*s",(int)sizeof(f.desc)-1,getfname(xfer.filename));

			strcpy(f.uler,xfer.user->alias);
			if(!addfiledat(&scfg,&f))
				lprintf("%04d !ERROR adding file (%s) to database",xfer.ctrl_sock,f.name);

			if(f.misc&FM_EXTDESC)
				putextdesc(&scfg,f.dir,f.datoffset,ext);

			if(scfg.dir[f.dir]->upload_sem[0])
				if((file=sopen(scfg.dir[f.dir]->upload_sem,O_WRONLY|O_CREAT|O_TRUNC,SH_DENYNO))!=-1)
					close(file);
			/**************************/
			/* Update Uploader's Info */
			/**************************/
			xfer.user->uls=(short)adjustuserrec(&scfg, xfer.user->number,U_ULS,5,1);
			xfer.user->ulb=adjustuserrec(&scfg, xfer.user->number,U_ULB,10,total);
			if(scfg.dir[f.dir]->up_pct && scfg.dir[f.dir]->misc&DIR_CDTUL) { /* credit for upload */
				if(scfg.dir[f.dir]->misc&DIR_CDTMIN && cps)    /* Give min instead of cdt */
					xfer.user->min=adjustuserrec(&scfg,xfer.user->number,U_MIN,10
						,((ulong)(total*(scfg.dir[f.dir]->up_pct/100.0))/cps)/60);
				else
					xfer.user->cdt=adjustuserrec(&scfg,xfer.user->number,U_CDT,10
						,(ulong)(f.cdt*(scfg.dir[f.dir]->up_pct/100.0))); 
			}
			upload_stats(total);
		}
		/* Send ACK */
		sockprintf(xfer.ctrl_sock,"226 Upload complete (%lu cps).",cps);
	}

	thread_down();
}



static void filexfer(SOCKADDR_IN* addr, SOCKET ctrl_sock, SOCKET pasv_sock, SOCKET* data_sock
					,char* filename, long filepos, BOOL* inprogress, BOOL* aborted
					,BOOL delfile, BOOL tmpfile
					,time_t* lastactive
					,user_t* user
					,int dir
					,BOOL receiving
					,BOOL credits
					,BOOL append
					,char* desc)
{
	int			result;
	int			addr_len;
	SOCKADDR_IN	server_addr;
	BOOL		reuseaddr;
	static xfer_t xfer;
	struct timeval	tv;
	fd_set			socket_set;

	if((*inprogress)==TRUE) {
		lprintf("%04d !TRANSFER already in progress",ctrl_sock);
		sockprintf(ctrl_sock,"425 Transfer already in progress.");
		return;
	}
	*inprogress=TRUE;

	if(*data_sock!=INVALID_SOCKET)
		ftp_close_socket(data_sock,__LINE__);

	if(pasv_sock==INVALID_SOCKET) {	/* !PASV */

		if((*data_sock=socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET) {
			lprintf("%04d !DATA ERROR %d opening socket", ctrl_sock, ERROR_VALUE);
			sockprintf(ctrl_sock,"425 Error %d opening socket",ERROR_VALUE);
			if(tmpfile)
				remove(filename);
			*inprogress=FALSE;
			return;
		}
		if(startup->socket_open!=NULL)
			startup->socket_open(TRUE);
		sockets++;
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf("%04d DATA socket %d opened",ctrl_sock,*data_sock);

		/* Use port-1 for all data connections */
		reuseaddr=TRUE;
		setsockopt(*data_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseaddr,sizeof(reuseaddr));

		memset(&server_addr, 0, sizeof(server_addr));

		server_addr.sin_addr.s_addr = htonl(startup->interface_addr);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port   = htons((WORD)(startup->port-1));	/* 20? */

		if((result=bind(*data_sock, (struct sockaddr *) &server_addr
			,sizeof(server_addr)))!=0) {
			lprintf ("%04d !DATA ERROR %d (%d) binding socket %d"
				,ctrl_sock, result, ERROR_VALUE, *data_sock);
			sockprintf(ctrl_sock,"425 Error %d binding socket",ERROR_VALUE);
			if(tmpfile)
				remove(filename);
			*inprogress=FALSE;
			ftp_close_socket(data_sock,__LINE__);
			return;
		}

		socket_debug[ctrl_sock]|=SOCKET_DEBUG_CONNECT;
		result=connect(*data_sock, (struct sockaddr *)addr,sizeof(struct sockaddr));
		socket_debug[ctrl_sock]&=~SOCKET_DEBUG_CONNECT;

		if(result!=0) {
			lprintf("%04d !DATA ERROR %d (%d) connecting to client %s port %d on socket %d"
					,ctrl_sock,result,ERROR_VALUE
					,inet_ntoa(addr->sin_addr),ntohs(addr->sin_port),*data_sock);
			sockprintf(ctrl_sock,"425 Error %d connecting to socket",ERROR_VALUE);
			if(tmpfile)
				remove(filename);
			*inprogress=FALSE;
			ftp_close_socket(data_sock,__LINE__);
			return;
		}
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf("%04d DATA socket %d connected to %s port %d"
				,ctrl_sock,*data_sock,inet_ntoa(addr->sin_addr),ntohs(addr->sin_port));

	} else {	/* PASV */

		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf("%04d PASV DATA socket %d listening on %s port %d"
					,ctrl_sock,pasv_sock,inet_ntoa(addr->sin_addr),ntohs(addr->sin_port));

		/* Setup for select() */
		tv.tv_sec=TIMEOUT_SOCKET_LISTEN;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(pasv_sock,&socket_set);

		socket_debug[ctrl_sock]|=SOCKET_DEBUG_SELECT;
		result=select(pasv_sock+1,&socket_set,NULL,NULL,&tv);
		socket_debug[ctrl_sock]&=~SOCKET_DEBUG_SELECT;
		if(result<1) {
			lprintf("%04d !PASV select returned %d (error: %d)",ctrl_sock,result,ERROR_VALUE);
			sockprintf(ctrl_sock,"425 Error %d selecting socket for connection",ERROR_VALUE);
			if(tmpfile)
				remove(filename);
			*inprogress=FALSE;
			return;
		}
			
		addr_len=sizeof(SOCKADDR_IN);
		socket_debug[ctrl_sock]|=SOCKET_DEBUG_ACCEPT;
		*data_sock=accept(pasv_sock,(struct sockaddr*)addr,&addr_len);
		socket_debug[ctrl_sock]&=~SOCKET_DEBUG_ACCEPT;

		if(*data_sock==INVALID_SOCKET) {
			lprintf("%04d !PASV DATA ERROR %d accepting connection on socket %d"
				,ctrl_sock,ERROR_VALUE,pasv_sock);
			sockprintf(ctrl_sock,"425 Error %d accepting connection",ERROR_VALUE);
			if(tmpfile)
				remove(filename);
			*inprogress=FALSE;
			return;
		}
		if(startup->socket_open!=NULL)
			startup->socket_open(TRUE);
		sockets++;
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf("%04d PASV DATA socket %d connected to %s port %d"
				,ctrl_sock,*data_sock,inet_ntoa(addr->sin_addr),ntohs(addr->sin_port));
	}


	/* Get Mutex here? */
	memset(&xfer,0,sizeof(xfer));
	xfer.ctrl_sock=ctrl_sock;
	xfer.data_sock=data_sock;
	xfer.inprogress=inprogress;
	xfer.aborted=aborted;
	xfer.delfile=delfile;
	xfer.tmpfile=tmpfile;
	xfer.append=append;
	xfer.filepos=filepos;
	xfer.credits=credits;
	xfer.lastactive=lastactive;
	xfer.user=user;
	xfer.dir=dir;
	xfer.desc=desc;
	sprintf(xfer.filename,"%.*s",(int)sizeof(xfer.filename)-1,filename);
	if(receiving)
		_beginthread(receive_thread,0,(void*)&xfer);
	else
		_beginthread(send_thread,0,(void*)&xfer);
}

/* convert "user name" to "user.name" or "mr. user" to "mr._user" */
char* dotname(char* in, char* out)
{
	char	ch;
	int		i;

	if(strchr(in,'.')==NULL)
		ch='.';
	else
		ch='_';
	for(i=0;in[i];i++)
		if(in[i]<=' ')
			out[i]=ch;
		else
			out[i]=in[i];
	out[i]=0;
	return(out);
}

void parsepath(char** pp, user_t* user, int* curlib, int* curdir)
{
	char*	p;
	char*	tp;
	char	path[MAX_PATH+1];
	int		dir=*curdir;
	int		lib=*curlib;

	sprintf(path,"%.*s",(int)sizeof(path)-1,*pp);
	p=path;

	if(*p=='/') {
		p++;
		lib=-1;
	}
	else if(!strncmp(p,"./",2))
		p+=2;

	if(!strncmp(p,"..",2)) {
		p+=2;
		if(dir>=0)
			dir=-1;
		else if(lib>=0)
			lib=-1;
		if(*p=='/')
			p++;
	}

	if(*p==0) {
		*curlib=lib;
		*curdir=dir;
		return;
	}

	if(lib<0) { /* root */
		tp=strchr(p,'/');
		if(tp) *tp=0;
		for(lib=0;lib<scfg.total_libs;lib++) {
			if(!chk_ar(&scfg,scfg.lib[lib]->ar,user))
				continue;
			if(!stricmp(scfg.lib[lib]->sname,p))
				break;
		}
		if(lib>=scfg.total_libs) { /* not found */
			*curlib=-1;
			return;
		}
		*curlib=lib;

		if(tp==NULL) {
			*curdir=-1;
			return;
		}

		p=tp+1;
	}

	tp=strchr(p,'/');
	if(tp!=NULL) {
		*tp=0;
		tp++;
	} else 
		tp=p+strlen(p);

	for(dir=0;dir<scfg.total_dirs;dir++) {
		if(scfg.dir[dir]->lib!=lib)
			continue;
		if(dir!=scfg.sysop_dir && dir!=scfg.upload_dir 
			&& !chk_ar(&scfg,scfg.dir[dir]->ar,user))
			continue;
		if(!stricmp(scfg.dir[dir]->code,p))
			break;
	}

	if(dir>=scfg.total_dirs)  /* not found */
		return;

	*curdir=dir;

	*pp+=tp-path;	/* skip "lib/dir/" */
}

BOOL alias(char* fullalias, char* filename, user_t* user, int* curdir)
{
	char*	p;
	char*	tp;
	char*	fname="";
	char	line[512];
	char	alias[512];
	char	aliasfile[MAX_PATH+1];
	int		dir=-1;
	FILE*	fp;
	BOOL	result=FALSE;

	sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
	if((fp=fopen(aliasfile,"r"))==NULL) 
		return(result);

	sprintf(alias,"%.*s",(int)sizeof(alias)-1,fullalias);
	p=strrchr(alias+1,'/');
	if(p) {
		*p=0;
		fname=p+1;
	}

	while(!feof(fp)) {
		if(!fgets(line,sizeof(line)-1,fp))
			break;

		p=line;	/* alias */
		while(*p && *p<=' ') p++;
		if(*p==';')	/* comment */
			continue;

		tp=p;		/* terminator */
		while(*tp && *tp>' ') tp++;
		if(*tp) *tp=0;

		if(stricmp(p,alias))	/* Not a match */
			continue;

		p=tp+1;		/* filename */
		while(*p && *p<=' ') p++;

		tp=p;		/* terminator */
		while(*tp && *tp>' ') tp++;
		if(*tp) *tp=0;

		if(!strnicmp(p,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
			if((dir=getdir(p+strlen(BBS_VIRTUAL_PATH),user))<0)	{
				lprintf("0000 !Invalid virtual path (%s) for %s",p,user->alias);
				/* invalid or no access */
				continue;
			}
			p=strrchr(p,'/');
			if(p!=NULL) p++;
			if(p!=NULL && filename!=NULL) {
				if(*p)
					sprintf(filename,"%s%s",scfg.dir[dir]->path,p);
				else
					sprintf(filename,"%s%s",scfg.dir[dir]->path,fname);
			}
		} else if(filename!=NULL)
			strcpy(filename,p);

		result=TRUE;	/* success */
		break;
	}
	fclose(fp);
	if(curdir!=NULL)
		*curdir=dir;
	return(result);
}

char* root_dir(char* path)
{
	char*	p;
	static char	root[MAX_PATH+1];

	sprintf(root,"%.*s",(int)sizeof(root)-1,path);

	if(!strncmp(root,"\\\\",2)) {	/* network path */
		p=strchr(root+2,'\\');
		if(p) p=strchr(p+1,'\\');
		if(p) *(p+1)=0;				/* truncate at \\computer\sharename\ */
	} 
	else if(!strncmp(root+1,":/",2) || !strncmp(root+1,":\\",2))
		root[3]=0;
	else if(*root=='/' || *root=='\\')
		root[1]=0;

	return(root);
}

char* vpath(int lib, int dir, char* str)
{
	strcpy(str,"/");
	if(lib<0)
		return(str);
	strcat(str,scfg.lib[lib]->sname);
	strcat(str,"/");
	if(dir<0)
		return(str);
	strcat(str,scfg.dir[dir]->code);
	strcat(str,"/");
	return(str);
}

static void ctrl_thread(void* arg)
{
	char		buf[512];
	char		str[128];
	char*		cmd;
	char*		p;
	char*		np;
	char*		tp;
	char		password[64];
	char		fname[MAX_PATH+1];
	char		qwkfile[MAX_PATH+1];
	char		aliasfile[MAX_PATH+1];
	char		aliasline[512];
	char		desc[501]="";
	char		sys_pass[128];
	char*		host_name;
	char		host_ip[64];
	char		path[MAX_PATH+1];
	char		local_dir[MAX_PATH+1];
	char		ren_from[MAX_PATH+1]="";
	char		html_index_ext[MAX_PATH+1];
	WORD		port;
	ulong		ip_addr;
	int			addr_len;
	DWORD		h1,h2,h3,h4;
	u_short		p1,p2;	/* For PORT command */
	int			i;
	int			rd;
	int			file;
	int			result;
	int			lib;
	int			dir;
	int			curlib=-1;
	int			curdir=-1;
	int			orglib;
	int			orgdir;
	long		filepos=0L;
	long		timeleft;
	ulong		l;
	ulong		avail;
	BOOL		detail;
	BOOL		success;
	BOOL		getdate;
	BOOL		getsize;
	BOOL		delecmd;
	BOOL		delfile;
	BOOL		tmpfile;
	BOOL		credits;
	BOOL		filedat;
	BOOL		transfer_inprogress;
	BOOL		transfer_aborted;
	BOOL		sysop=FALSE;
	BOOL		local_fsys=FALSE;
	BOOL		alias_dir;
	FILE*		fp;
	FILE*		alias_fp;
	SOCKET		sock;
	SOCKET		pasv_sock=INVALID_SOCKET;
	SOCKET		data_sock=INVALID_SOCKET;
	HOSTENT*	host;
	SOCKADDR_IN	addr;
	SOCKADDR_IN	data_addr;
	SOCKADDR_IN	pasv_addr;
	ftp_t		ftp=*(ftp_t*)arg;
	user_t		user;
	time_t		t;
	time_t		now;
	time_t		logintime;
	time_t		lastactive;
	file_t		f;
	glob_t		g;
	node_t		node;
	client_t	client;
	struct tm	tm;
	struct tm *	tm_p;
	struct tm 	cur_tm;
#ifdef JAVASCRIPT
	jsval		js_val;
	JSContext*	js_cx;
	JSObject*	js_glob;
	JSObject*	js_ftp;
#endif

	thread_up();

	lastactive=time(NULL);

	sock=ftp.socket;
	data_addr=ftp.client_addr;
	/* Default data port is ctrl port-1 */
	data_addr.sin_port=ntohs(data_addr.sin_port)-1;
	data_addr.sin_port=htons(data_addr.sin_port);
	
	lprintf("%04d CTRL thread started", sock);

	free(arg);

#ifdef _WIN32
	if(startup->answer_sound[0] && !(startup->options&FTP_OPT_MUTE)) 
		PlaySound(startup->answer_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

#ifdef JAVASCRIPT
	if(((js_cx=js_initcx(sock,&js_glob,&js_ftp))==NULL)) {
		lprintf("%04d !ERROR initializing JavaScript context",sock);
		sockprintf(sock,"425 Error initializing JavaScript context");
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}
#endif

	transfer_inprogress = FALSE;
	transfer_aborted = FALSE;

	l=1;

	if((i=ioctlsocket(sock, FIONBIO, &l))!=0) {
		lprintf("%04d !ERROR %d (%d) disabling socket blocking"
			,sock, i, ERROR_VALUE);
		sockprintf(sock,"425 Error %d disabling socket blocking"
			,ERROR_VALUE);
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	memset(&user,0,sizeof(user));

	strcpy(host_ip,inet_ntoa(ftp.client_addr.sin_addr));

	lprintf ("%04d CTRL connection accepted from: %s port %u"
		,sock, host_ip, ntohs(ftp.client_addr.sin_port));

	if(startup->options&FTP_OPT_NO_HOST_LOOKUP)
		host=NULL;
	else
		host=gethostbyaddr ((char *)&ftp.client_addr.sin_addr
			,sizeof(ftp.client_addr.sin_addr),AF_INET);

	if(host!=NULL && host->h_name!=NULL)
		host_name=host->h_name;
	else
		host_name="<no name>";

	lprintf("%04d Host name: %s", sock, host_name);

	if(trashcan(&scfg,host_ip,"ip")) {
		lprintf("%04d !Client blocked in ip.can: %s", sock, host_ip);
		sockprintf(sock,"550 Access denied.");
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	if(trashcan(&scfg,host_name,"host")) {
		lprintf("%04d !Client blocked in host.can: %s", sock, host_name);
		sockprintf(sock,"550 Access denied.");
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	/* For PASV mode */
	addr_len=sizeof(pasv_addr);
	if((result=getsockname(sock, (struct sockaddr *)&pasv_addr,&addr_len))!=0) {
		lprintf("%04d !ERROR %d (%d) getting address/port", sock, result, ERROR_VALUE);
		sockprintf(sock,"425 Error %d getting address/port",ERROR_VALUE);
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	} 

	active_clients++;
	update_clients();

	/* Initialize client display */
	client.size=sizeof(client);
	client.time=time(NULL);
	sprintf(client.addr,"%.*s",(int)sizeof(client.addr)-1,host_ip);
	sprintf(client.host,"%.*s",(int)sizeof(client.host)-1,host_name);
	client.port=ntohs(ftp.client_addr.sin_port);
	client.protocol="FTP";
	client.user="<unknown>";
	client_on(sock,&client);

	sockprintf(sock,"220-%s (%s)",scfg.sys_name, scfg.sys_inetaddr);
	sockprintf(sock," Synchronet FTP Server for %s v%s Ready"
		,PLATFORM_DESC,FTP_VERSION);
	sprintf(str,"%sftplogin.txt",scfg.text_dir);
	if((fp=fopen(str,"rb"))!=NULL) {
		while(!feof(fp)) {
			if(!fgets(buf,sizeof(buf),fp))
				break;
			truncsp(buf);
			sockprintf(sock," %s",buf);
		}
		fclose(fp);
	}
	sockprintf(sock,"220 Please enter your user name.");

	socket_debug[sock]|=SOCKET_DEBUG_CTRL;
	while(1) {

		socket_debug[sock]|=SOCKET_DEBUG_READLINE;
		rd = sockreadline(sock, buf, sizeof(buf), &lastactive);
		socket_debug[sock]&=~SOCKET_DEBUG_READLINE;
		if(rd<1) {
			if(transfer_inprogress==TRUE)
				transfer_aborted=TRUE;
			break;
		}
		truncsp(buf);
		lastactive=time(NULL);
		cmd=buf;
		while(((BYTE)*cmd)==TELNET_IAC) {
			cmd++;
			lprintf("%04d RX: Telnet cmd %d",sock,(BYTE)*cmd);
			cmd++;
		}
		while(*cmd && *cmd<' ') {
			lprintf("%04d RX: %d (0x%02X)",sock,(BYTE)*cmd,(BYTE)*cmd);
			cmd++;
		}
		if(!(*cmd))
			continue;
		if(startup->options&FTP_OPT_DEBUG_RX)
			lprintf("%04d RX: %s", sock, cmd);
		if(!stricmp(cmd, "NOOP")) {
			sockprintf(sock,"200 NOOP command successful.");
			continue;
		}
		if(!stricmp(cmd, "HELP SITE") || !stricmp(cmd, "SITE HELP")) {
			sockprintf(sock,"214-The following SITE commands are recognized (* => unimplemented):");
			sockprintf(sock," HELP    WHO");
			sockprintf(sock,"214 Direct comments to sysop@%s.",scfg.sys_inetaddr);
			continue;
		}
		if(!strnicmp(cmd, "HELP",4)) {
			sockprintf(sock,"214-The following commands are recognized (* => unimplemented, # => extension):");
			sockprintf(sock," USER    PASS    CWD     XCWD    CDUP    XCUP    PWD     XPWD");
			sockprintf(sock," QUIT    REIN    PORT    PASV    LIST    NLST    NOOP    HELP");
			sockprintf(sock," SIZE    MDTM    RETR    STOR    REST    ALLO    ABOR    SYST");
			sockprintf(sock," TYPE    STRU    MODE    SITE    RNFR*   RNTO*   DELE*   DESC#");
			sockprintf(sock," FEAT#   OPTS#");
			sockprintf(sock,"214 Direct comments to sysop@%s.",scfg.sys_inetaddr);
			continue;
		}
		if(!stricmp(cmd, "FEAT")) {
			sockprintf(sock,"211-The following additional (post-RFC949) features are supported:");
			sockprintf(sock," DESC");
			sockprintf(sock," MDTM");
			sockprintf(sock," SIZE");
			sockprintf(sock," REST STREAM");
			sockprintf(sock,"211 End");
			continue;
		}
		if(!strnicmp(cmd, "OPTS",4)) {
			sockprintf(sock,"501 No options supported.");
			continue;
		}
		if(!stricmp(cmd, "QUIT")) {
			sprintf(str,"%sftpbye.txt",scfg.text_dir);
			if((fp=fopen(str,"rb"))!=NULL) {
				i=0;
				while(!feof(fp)) {
					if(!fgets(buf,sizeof(buf),fp))
						break;
					truncsp(buf);
					if(!i)
						sockprintf(sock,"221-%s",buf);
					else
						sockprintf(sock," %s",buf);
					i++;
				}
				fclose(fp);
			}
			sockprintf(sock,"221 Goodbye. Closing control connection.");
			break;
		}
		if(!strnicmp(cmd, "USER ",5)) {
			sysop=FALSE;
			user.number=0;
			p=cmd+5;
			while(*p && *p<=' ') p++;
			truncsp(p);
			sprintf(user.alias,"%.*s",(int)sizeof(user.alias)-1,p);
			if(!stricmp(user.alias,"anonymous"))
				strcpy(user.alias,"guest");
			user.number=matchuser(&scfg,user.alias);
			if(user.number && getuserdat(&scfg, &user)==0 && user.pass[0]==0) 
				sockprintf(sock,"331 User name okay, give your full e-mail address as password.");
			else
				sockprintf(sock,"331 User name okay, need password.");
			continue;
		}
		if(!strnicmp(cmd, "PASS ",5) && user.alias[0]) {
			user.number=0;
			p=cmd+5;
			while(*p && *p<=' ') p++;

			sprintf(password,"%.*s",(int)sizeof(password)-1,p);
			user.number=matchuser(&scfg,user.alias);
			if(!user.number) {
				lprintf("%04d !FTP: UNKNOWN USER: %s",sock,user.alias);
				sockprintf(sock,"530 Password not accepted.");
				continue;
			}
			if((i=getuserdat(&scfg, &user))!=0) {
				lprintf("%04d !FTP: ERROR %d getting data on user #%d (%s)"
					,sock,i,user.number,user.alias);
				sockprintf(sock,"530 Database error %d",i);
				user.number=0;
				continue;
			}
			if(user.misc&(DELETED|INACTIVE)) {
				lprintf("%04d !FTP: DELETED or INACTIVE user #%d (%s)"
					,sock,user.number,user.alias);
				sockprintf(sock,"530 Password not accepted.");
				user.number=0;
				continue;
			}
			if(user.rest&FLAG('T')) {
				lprintf("%04d !FTP: 'T' RESTRICTED user #%d (%s)"
					,sock,user.number,user.alias);
				sockprintf(sock,"530 Password not accepted.");
				user.number=0;
				continue;
			}
			if(user.ltoday>scfg.level_callsperday[user.level]
				&& !(user.exempt&FLAG('L'))) {
				lprintf("%04d !FTP: Max logons (%d) reached"
					,sock,scfg.level_callsperday[user.level]);
				sockprintf(sock,"530 Maximum logons reached.");
				user.number=0;
				continue;
			}
			if(user.rest&FLAG('L') && user.ltoday>1) {
				lprintf("%04d !FTP: L Restricted user already on today"
					,sock);
				sockprintf(sock,"530 Maximum logons reached.");
				user.number=0;
				continue;
			}

			sprintf(sys_pass,"%s:%s",user.pass,scfg.sys_pass);
			if(!user.pass[0]) {	/* Guest/Anonymous */
				lprintf("%04d Guest: %s",sock,password);
				putuserrec(&scfg,user.number,U_NETMAIL,LEN_NETMAIL,password);
			}
			else if(user.level>=SYSOP_LEVEL && !stricmp(password,sys_pass)) {
				lprintf("%04d Sysop access granted to %s", sock, user.alias);
				sysop=TRUE;
			}
			else if(stricmp(password,user.pass)) {
				if(scfg.sys_misc&SM_ECHO_PW)
					lprintf("%04d !FTP: FAILED Password attempt for user %s: '%s' expected '%s'"
						,sock, user.alias, password, user.pass);
				else
					lprintf("%04d !FTP: FAILED Password attempt for user %s"
						,sock, user.alias);
				sockprintf(sock,"530 Password not accepted.");
				user.number=0;
				continue;
			}

			/* Update client display */
			client.user=user.alias;
			client_on(sock,&client);


			lprintf("%04d %s logged in.",sock,user.alias);
			logintime=time(NULL);
			timeleft=gettimeleft(&scfg,&user,logintime);
			sprintf(str,"%sftphello.txt",scfg.text_dir);
			if((fp=fopen(str,"rb"))!=NULL) {
				i=0;
				while(!feof(fp)) {
					if(!fgets(buf,sizeof(buf),fp))
						break;
					truncsp(buf);
					if(!i)
						sockprintf(sock,"230-%s",buf);
					else
						sockprintf(sock," %s",buf);
					i++;
				}
				fclose(fp);
			}

#ifdef JAVASCRIPT
			JS_BeginRequest(js_cx);	/* Required for multi-thread support */

			if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
				lprintf("%04d !JavaScript ERROR creating user class",sock);

			if(js_CreateUserObject(js_cx, js_glob, &scfg, "user", user.number)==NULL) 
				lprintf("%04d !JavaScript ERROR creating user object",sock);

			if(js_CreateClientObject(js_cx, js_glob, "client", &client, sock)==NULL) 
				lprintf("%04d !JavaScript ERROR creating client object",sock);

			if(js_CreateFileAreaObject(js_cx, js_glob, &scfg, &user
				,startup->html_index_file)==NULL) 
				lprintf("%04d !JavaScript ERROR creating file area object",sock);

			JS_EndRequest(js_cx);	/* Required for multi-thread support */
#endif

			if(sysop)
				sockprintf(sock,"230-Sysop access granted.");
			sockprintf(sock,"230-%s logged in.",user.alias);
			if(!(user.exempt&FLAG('D')) && (user.cdt+user.freecdt)>0)
				sockprintf(sock,"230-You have %lu download credits."
					,user.cdt+user.freecdt);
			sockprintf(sock,"230 You are allowed %ld minutes of use for this session."
				,timeleft/60);
			sprintf(qwkfile,"%sfile/%04d.qwk",scfg.data_dir,user.number);

			/* Adjust User Total Logons/Logons Today */
			adjustuserrec(&scfg,user.number,U_LOGONS,5,1);
			putuserrec(&scfg,user.number,U_LTODAY,5,ultoa(user.ltoday+1,str,10));
			putuserrec(&scfg,user.number,U_MODEM,LEN_MODEM,"FTP");
			putuserrec(&scfg,user.number,U_COMP,LEN_COMP,host_name);
			putuserrec(&scfg,user.number,U_NOTE,LEN_NOTE,host_ip);
			getuserdat(&scfg, &user);	/* make user current */

			continue;
		}

		if(!user.number) {
			sockprintf(sock,"530 Please login with USER and PASS.");
			continue;
		}

		if((timeleft=gettimeleft(&scfg,&user,logintime))<1L) {
			sockprintf(sock,"421 Sorry, you've run out of time.");
			lprintf("%04d Out of time, disconnecting",sock);
			break;
		}

		/********************************/
		/* These commands require login */
		/********************************/

		if(!stricmp(cmd, "REIN")) {
			lprintf("%04d %s reinitialized control session",sock,user.alias);
			user.number=0;
			sysop=FALSE;
			filepos=0;
			sockprintf(sock,"220 Control session re-initialized. Ready for re-login.");
			continue;
		}

		if(!stricmp(cmd, "SITE WHO")) {
			sockprintf(sock,"211-Active users");
			for(i=0;i<scfg.sys_nodes && i<scfg.sys_lastnode;i++) {
				if((result=getnodedat(&scfg, i+1, &node, 0))!=0) {
					sockprintf(sock," Error %d getting data for Telnet Node %d",result,i+1);
					continue;
				}
				if(node.status==NODE_INUSE)
					sockprintf(sock," Telnet Node %3d: %s",i+1, username(&scfg,node.useron,str));
			}
			sockprintf(sock,"211 End");
			continue;
		}
		if(!stricmp(cmd, "SITE DEBUG")) {
			sockprintf(sock,"211-Debug");
			for(i=0;i<sizeof(socket_debug);i++) 
				if(socket_debug[i]!=0)
					sockprintf(sock,"211-socket %d = %X",i,socket_debug[i]);
			sockprintf(sock,"211 End");
			continue;
		}


		if(!strnicmp(cmd, "PORT ",5)) {
			p=cmd+5;
			while(*p && *p<=' ') p++;
			sscanf(p,"%ld,%ld,%ld,%ld,%hd,%hd",&h1,&h2,&h3,&h4,&p1,&p2);
			data_addr.sin_addr.s_addr=htonl((h1<<24)|(h2<<16)|(h3<<8)|h4);
			data_addr.sin_port=htons((u_short)((p1<<8)|p2));
			sockprintf(sock,"200 PORT Command successful.");
			continue;
		}

		if(!stricmp(cmd, "PASV")) {

			if(pasv_sock!=INVALID_SOCKET) 
				ftp_close_socket(&pasv_sock,__LINE__);

			if((pasv_sock=ftp_open_socket(SOCK_STREAM))==INVALID_SOCKET) {
				lprintf("%04d !PASV ERROR %d opening socket", sock,ERROR_VALUE);
				sockprintf(sock,"425 Error %d opening PASV data socket", ERROR_VALUE);
				continue;
			}

			if(startup->options&FTP_OPT_DEBUG_DATA)
				lprintf("%04d PASV DATA socket %d opened",sock,pasv_sock);

			pasv_addr.sin_port = 0;

			if((result=bind(pasv_sock, (struct sockaddr *) &pasv_addr,sizeof(pasv_addr)))!= 0) {
				lprintf("%04d !PASV ERROR %d (%d) binding socket", sock, result, ERROR_VALUE);
				sockprintf(sock,"425 Error %d binding data socket",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			}

			addr_len=sizeof(addr);
			if((result=getsockname(pasv_sock, (struct sockaddr *)&addr,&addr_len))!=0) {
				lprintf("%04d !PASV ERROR %d (%d) getting address/port", sock, result, ERROR_VALUE);
				sockprintf(sock,"425 Error %d getting address/port",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			} 

			if((result=listen(pasv_sock, 1))!= 0) {
				lprintf("%04d !PASV ERROR %d (%d) listening on socket", sock, result, ERROR_VALUE);
				sockprintf(sock,"425 Error %d listening on data socket",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			}

			ip_addr=ntohl(pasv_addr.sin_addr.s_addr);
			port=ntohs(addr.sin_port);
			sockprintf(sock,"227 Entering Passive Mode (%d,%d,%d,%d,%hd,%hd)"
				,(ip_addr>>24)&0xff
				,(ip_addr>>16)&0xff
				,(ip_addr>>8)&0xff
				,ip_addr&0xff
				,(port>>8)&0xff
				,port&0xff
				);
			continue;
		}

		if(!strnicmp(cmd, "TYPE ",5)) {
			sockprintf(sock,"200 All files sent in BINARY mode.");
			continue;
		}

		if(!strnicmp(cmd, "ALLO",4)) {
			p=cmd+5;
			while(*p && *p<=' ') p++;
			if(*p)
				l=atol(p);	
			else
				l=0;
			if(local_fsys)
				avail=getfreediskspace(local_dir);
			else
				avail=getfreediskspace(scfg.data_dir);	/* Change to temp_dir? */
			if(l && l>avail)
				sockprintf(sock,"504 Only %ld bytes available.",avail);
			else
				sockprintf(sock,"200 %ld bytes available.",avail);
			continue;
		}

		if(!strnicmp(cmd, "REST",4)) {
			p=cmd+4;
			while(*p && *p<=' ') p++;
			if(*p)
				filepos=atol(p);
			else
				filepos=0;
			sockprintf(sock,"350 Restarting at %ld. Send STORE or RETRIEVE to initiate transfer."
				,filepos);
			continue;
		}

		if(!strnicmp(cmd, "MODE ",5)) {
			p=cmd+5;
			while(*p && *p<=' ') p++;
			if(toupper(*p)!='S')
				sockprintf(sock,"504 Only STREAM mode supported.");
			else
				sockprintf(sock,"200 STREAM mode.");
			continue;
		}

		if(!strnicmp(cmd, "STRU ",5)) {
			p=cmd+5;
			while(*p && *p<=' ') p++;
			if(toupper(*p)!='F')
				sockprintf(sock,"504 Only FILE structure supported.");
			else
				sockprintf(sock,"200 FILE structure.");
			continue;
		}

		if(!stricmp(cmd, "SYST")) {
			sockprintf(sock,"215 UNIX Type: L8");
			continue;
		}

		if(!stricmp(cmd, "ABOR")) {
			if(!transfer_inprogress)
				sockprintf(sock,"226 No tranfer in progress.");
			else {
				transfer_aborted=TRUE;
				mswait(1); /* give send thread time to abort */
				sockprintf(sock,"226 Transfer aborted.");
			}
			continue;
		}

		if(!strnicmp(cmd,"SMNT ",5) && sysop && !(startup->options&FTP_OPT_NO_LOCAL_FSYS)) {
			p=cmd+5;
			while(*p && *p<=' ') p++;
			if(!stricmp(p,BBS_FSYS_DIR)) 
				local_fsys=FALSE;
			else {
				if(!direxist(p)) {
					sockprintf(sock,"550 Directory does not exist.");
					lprintf("%04d !%s attempted to mount invalid directory: %s"
						,sock, user.alias, p);
					continue;
				}
				local_fsys=TRUE;
				sprintf(local_dir,"%.*s",(int)sizeof(local_dir)-1,p);
			}
			sockprintf(sock,"250 %s file system mounted."
				,local_fsys ? "Local" : "BBS");
			lprintf("%04d %s mounted %s file system"
				,sock, user.alias, local_fsys ? "local" : "BBS");
			continue;
		}

		/****************************/
		/* Local File System Access */
		/****************************/
		if(sysop && local_fsys && !(startup->options&FTP_OPT_NO_LOCAL_FSYS)) {
			if(local_dir[0] 
				&& local_dir[strlen(local_dir)-1]!='\\'
				&& local_dir[strlen(local_dir)-1]!='/')
				strcat(local_dir,"/");

			if(!strnicmp(cmd, "LIST", 4) || !strnicmp(cmd, "NLST", 4)) {	
				sprintf(fname,"%sftp%d.tx", scfg.data_dir, sock);
				if((fp=fopen(fname,"w+b"))==NULL) {
					lprintf("%04d !ERROR %d opening %s",sock,errno,fname);
					sockprintf(sock, "451 Insufficient system storage");
					continue;
				}
				if(!strnicmp(cmd, "LIST", 4))
					detail=TRUE;
				else
					detail=FALSE;

				p=cmd+4;
				while(*p && *p<=' ') p++;
				sprintf(path,"%s%s",local_dir, *p ? p : "*");
				lprintf("%04d %s listing: %s", sock, user.alias, path);
				sockprintf(sock, "150 Directory of %s%s", local_dir, p);
			
				glob(path,0,NULL,&g);
				for(i=0;i<(int)g.gl_pathc;i++) {
					if(detail) {
						f.size=flength(g.gl_pathv[i]);
						t=fdate(g.gl_pathv[i]);
						tm_p=localtime(&t);
						if(tm_p==NULL)
							memset(&tm,0,sizeof(tm));
						else
							tm=*tm_p;
						fprintf(fp,"%crw-r--r--   1 %-8s local %9ld %s %2d "
							,isdir(g.gl_pathv[i]) ? 'd':'-'
							,scfg.sys_id
							,f.size
							,mon[tm.tm_mon],tm.tm_mday);
						if(tm.tm_year==cur_tm.tm_year)
							fprintf(fp,"%02d:%02d %s\r\n"
								,tm.tm_hour,tm.tm_min
								,getfname(g.gl_pathv[i]));
						else
							fprintf(fp,"%5d %s\r\n"
								,1900+tm.tm_year
								,getfname(g.gl_pathv[i]));
					} else
						fprintf(fp,"%s\r\n",getfname(g.gl_pathv[i]));
				}
				globfree(&g);
				fclose(fp);
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,0L
					,&transfer_inprogress,&transfer_aborted
					,TRUE	/* delfile */
					,TRUE	/* tmpfile */
					,&lastactive,&user,-1,FALSE,FALSE,FALSE,NULL);
				continue;
			} /* Local LIST/NLST */
				
			if(!strnicmp(cmd, "CWD ", 4) || !strnicmp(cmd,"XCWD ",5)) {
			    if(!strnicmp(cmd,"CWD ",4))
					p=cmd+4;
				else
					p=cmd+5;
				while(*p && *p<=' ') p++;
				tp=p;
				if(*tp=='/' || *tp=='\\') /* /local: and /bbs: are valid */
					tp++;
				if(!strnicmp(tp,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) {
					local_fsys=FALSE;
					sockprintf(sock,"250 CWD command successful (BBS file system mounted).");
					lprintf("%04d %s mounted BBS file system", sock, user.alias);
					continue;
				}
				if(!strnicmp(tp,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) {
					tp+=strlen(LOCAL_FSYS_DIR);	/* already mounted */
					p=tp;
				}

				if(p[1]==':' || !strncmp(p,"\\\\",2))
					strcpy(path,p);
				else if(*p=='/' || *p=='\\')
					sprintf(path,"%s%s",root_dir(local_dir),p);
				else {
					sprintf(fname,"%s%s",local_dir,p);
					_fullpath(path,fname,sizeof(path));
				}

				if(!direxist(path)) {
					sockprintf(sock,"550 Directory does not exist (%s).",path);
					lprintf("%04d !%s attempted to change to an invalid directory: %s"
						,sock, user.alias, path);
				} else {
					strcpy(local_dir,path);
					sockprintf(sock,"250 CWD command successful (%s).", local_dir);
				}
				continue;
			} /* Local CWD */

			if(!stricmp(cmd,"CDUP") || !stricmp(cmd,"XCUP")) {
				sprintf(path,"%s..",local_dir);
				if(_fullpath(local_dir,path,sizeof(local_dir))==NULL)
					sockprintf(sock,"550 Directory does not exist.");
				else
					sockprintf(sock,"200 CDUP command successful.");
				continue;
			}

			if(!stricmp(cmd, "PWD") || !stricmp(cmd,"XPWD")) {
				if(strlen(local_dir)>3)
					local_dir[strlen(local_dir)-1]=0;	/* truncate '/' */

				sockprintf(sock,"257 \"%s\" is current directory."
					,local_dir);
				continue;
			} /* Local PWD */

			if(!strnicmp(cmd, "MKD ", 4) || !strnicmp(cmd,"XMKD",4)) {
				p=cmd+4;
				while(*p && *p<=' ') p++;
				if(*p=='/')	/* absolute */
					sprintf(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					sprintf(fname,"%s%s",local_dir,p);

				if((i=_mkdir(fname))==0) {
					sockprintf(sock,"257 \"%s\" directory created",fname);
					lprintf("%04d %s created directory: %s",sock,user.alias,fname);
				} else {
					sockprintf(sock,"521 Error %d creating directory: %s",i,fname);
					lprintf("%04d !%s attempted to create directory: %s (Error %d)"
						,sock,user.alias,fname,i);
				}
				continue;
			}

			if(!strnicmp(cmd, "RMD ", 4) || !strnicmp(cmd,"XRMD",4)) {
				p=cmd+4;
				while(*p && *p<=' ') p++;
				if(*p=='/')	/* absolute */
					sprintf(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					sprintf(fname,"%s%s",local_dir,p);

				if((i=_rmdir(fname))==0) {
					sockprintf(sock,"250 \"%s\" directory removed",fname);
					lprintf("%04d %s removed directory: %s",sock,user.alias,fname);
				} else {
					sockprintf(sock,"450 Error %d removing directory: %s",i,fname);
					lprintf("%04d !%s attempted to remove directory: %s (Error %d)"
						,sock,user.alias,fname,i);
				}
				continue;
			}

			if(!strnicmp(cmd, "RNFR ",5)) {
				p=cmd+5;
				while(*p && *p<=' ') p++;
				if(*p=='/')	/* absolute */
					sprintf(ren_from,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					sprintf(ren_from,"%s%s",local_dir,p);
				if(!fexist(ren_from)) {
					sockprintf(sock,"550 File not found: %s",ren_from);
					lprintf("%04d !%s attempted to rename %s (not found)"
						,sock,user.alias,ren_from);
				} else
					sockprintf(sock,"350 File exists, ready for destination name");
				continue;
			}

			if(!strnicmp(cmd, "RNTO ",5)) {
				p=cmd+5;
				while(*p && *p<=' ') p++;
				if(*p=='/')	/* absolute */
					sprintf(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					sprintf(fname,"%s%s",local_dir,p);

				if((i=rename(ren_from, fname))==0) {
					sockprintf(sock,"250 \"%s\" renamed to \"%s\"",ren_from,fname);
					lprintf("%04d %s renamed %s to %s",sock,user.alias,ren_from,fname);
				} else {
					sockprintf(sock,"450 Error %d renaming file: %s",i,ren_from);
					lprintf("%04d !%s attempted to rename file: %s (Error %d)"
						,sock,user.alias,ren_from,i);
				}
				continue;
			}


			if(!strnicmp(cmd, "RETR ", 5) || !strnicmp(cmd,"SIZE ",5) 
				|| !strnicmp(cmd, "MDTM ",5) || !strnicmp(cmd, "DELE ",5)) {
				p=cmd+5;
				while(*p && *p<=' ') p++;

				if(!strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) 
					p+=strlen(LOCAL_FSYS_DIR);	/* already mounted */

				if(p[1]==':')		/* drive specified */
					strcpy(fname,p);
				else if(*p=='/')	/* absolute, current drive */
					sprintf(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					sprintf(fname,"%s%s",local_dir,p);
				if(!fexist(fname)) {
					lprintf("%04d !%s file not found: %s",sock,user.alias,fname);
					sockprintf(sock,"550 File not found: %s",fname);
					continue;
				}
				if(!strnicmp(cmd,"SIZE ",5)) {
					sockprintf(sock,"213 %lu",flength(fname));
					continue;
				}
				if(!strnicmp(cmd,"MDTM ",5)) {
					t=fdate(fname);
					tm_p=gmtime(&t);
					if(tm_p==NULL)
						memset(&tm,0,sizeof(tm));
					else
						tm=*tm_p;
					sockprintf(sock,"213 %u%02u%02u%02u%02u%02u"
						,1900+tm.tm_year,tm.tm_mon+1,tm.tm_mday
						,tm.tm_hour,tm.tm_min,tm.tm_sec);					
					continue;
				}
				if(!strnicmp(cmd,"DELE ",5)) {
					if((i=remove(fname))==0) {
						sockprintf(sock,"250 \"%s\" removed successfully.",fname);
						lprintf("%04d %s deleted file: %s",sock,user.alias,fname);
					} else {
						sockprintf(sock,"450 Error %d removing file: %s",i,fname);
						lprintf("%04d !%s attempted to delete file: %s (Error %d)"
							,sock,user.alias,fname,i);
					}
					continue;
				}
				/* RETR */
				lprintf("%04d %s downloading: %s (%ld bytes) in %s mode"
					,sock,user.alias,fname,flength(fname)
					,pasv_sock==INVALID_SOCKET ? "active":"passive");
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
					,&lastactive,&user,-1,FALSE,FALSE,FALSE,NULL);
				continue;
			} /* Local RETR/SIZE/MDTM */

			if(!strnicmp(cmd, "STOR ", 5) || !strnicmp(cmd, "APPE ", 5)) {
				p=cmd+5;
				while(*p && *p<=' ') p++;

				if(!strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) 
					p+=strlen(LOCAL_FSYS_DIR);	/* already mounted */

				if(p[1]==':')		/* drive specified */
					strcpy(fname,p);
				else if(*p=='/')	/* absolute, current drive */
					sprintf(fname,"%s%s",root_dir(local_dir),p+1);
				else				/* relative */
					sprintf(fname,"%s%s",local_dir,p);

				lprintf("%04d %s uploading: %s in %s mode", sock,user.alias,fname
					,pasv_sock==INVALID_SOCKET ? "active":"passive");
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
					,&lastactive
					,&user
					,-1		/* dir */
					,TRUE	/* uploading */
					,FALSE	/* credits */
					,!strnicmp(cmd,"APPE",4) ? TRUE : FALSE	/* append */
					,NULL	/* desc */
					);
				filepos=0;
				continue;
			} /* Local STOR */
		}

		if(!strnicmp(cmd, "LIST", 4) || !strnicmp(cmd, "NLST", 4)) {	
			dir=curdir;
			lib=curlib;

			if(cmd[4]!=0) 
				lprintf("%04d LIST/NLST: %s",sock,cmd);

			/* path specified? */
			p=cmd+4;
			while(*p && *p<=' ') p++;

			if(*p=='-') {	/* -Letc */
				while(*p && *p>' ') p++;
				while(*p && *p<=' ') p++;
			}

			parsepath(&p,&user,&lib,&dir);

			sprintf(fname,"%sftp%d.tx", scfg.data_dir, sock);
			if((fp=fopen(fname,"w+b"))==NULL) {
				lprintf("%04d !ERROR %d opening %s",sock,errno,fname);
				sockprintf(sock, "451 Insufficient system storage");
				continue;
			}
			if(!strnicmp(cmd, "LIST", 4))
				detail=TRUE;
			else
				detail=FALSE;
			sockprintf(sock,"150 Opening ASCII mode data connection for /bin/ls.");
			now=time(NULL);
			tm_p=localtime(&now);
			if(tm_p==NULL) 
				memset(&cur_tm,0,sizeof(cur_tm));
			else
				cur_tm=*tm_p;

			/* ASCII Index File */
			if(startup->options&FTP_OPT_INDEX_FILE && startup->index_file_name[0]
				&& (!stricmp(p,startup->index_file_name) || *p==0 || *p=='*')) {
				if(detail)
					fprintf(fp,"-rw-r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
						,NAME_LEN
						,scfg.sys_id
						,lib<0 ? scfg.sys_id : dir<0 
							? scfg.lib[lib]->sname : scfg.dir[dir]->code
						,512L
						,mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
						,startup->index_file_name);
				else
					fprintf(fp,"%s\r\n",startup->index_file_name);
			} 
			/* HTML Index File */
			if(startup->options&FTP_OPT_HTML_INDEX_FILE && startup->html_index_file[0]
				&& (!stricmp(p,startup->html_index_file) || *p==0 || *p=='*')) {
				if(detail)
					fprintf(fp,"-rw-r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
						,NAME_LEN
						,scfg.sys_id
						,lib<0 ? scfg.sys_id : dir<0 
							? scfg.lib[lib]->sname : scfg.dir[dir]->code
						,512L
						,mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
						,startup->html_index_file);
				else
					fprintf(fp,"%s\r\n",startup->html_index_file);
			} 

			if(lib<0) { /* Root dir */
				lprintf("%04d %s listing: root",sock,user.alias);

				/* QWK Packet */
				if(startup->options&FTP_OPT_ALLOW_QWK/* && fexist(qwkfile)*/) {
					if(detail) {
						if(fexist(qwkfile)) {
							t=fdate(qwkfile);
							l=flength(qwkfile);
						} else {
							t=time(NULL);
							l=10240;
						};
						tm_p=localtime(&t);
						if(tm_p==NULL) 
							memset(&tm,0,sizeof(tm));
						else
							tm=*tm_p;
						fprintf(fp,"-rw-r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s.qwk\r\n"
							,NAME_LEN
							,scfg.sys_id
							,scfg.sys_id
							,l
							,mon[tm.tm_mon],tm.tm_mday,tm.tm_hour,tm.tm_min
							,scfg.sys_id);
					} else
						fprintf(fp,"%s.qwk\r\n",scfg.sys_id);
				} 

				/* File Aliases */
				sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
				if((alias_fp=fopen(aliasfile,"r"))!=NULL) {

					while(!feof(alias_fp)) {
						if(!fgets(aliasline,sizeof(aliasline)-1,alias_fp))
							break;

						alias_dir=FALSE;

						p=aliasline;		/* alias pointer */
						while(*p && *p<=' ') p++;

						if(*p==';')	/* comment */
							continue;

						tp=p;		/* terminator pointer */
						while(*tp && *tp>' ') tp++;
						if(*tp) *tp=0;

						np=tp+1;	/* filename pointer */
						while(*np && *np<=' ') np++;

						tp=np;		/* terminator pointer */
						while(*tp && *tp>' ') tp++;
						if(*tp) *tp=0;

						/* Virtual Path? */
						if(!strnicmp(np,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
							if((dir=getdir(np+strlen(BBS_VIRTUAL_PATH),&user))<0)
								continue; /* No access or invalid virtual path */
							tp=strrchr(np,'/');
							if(tp==NULL) 
								continue;
							tp++;
							if(*tp) {
								sprintf(aliasfile,"%s%s",scfg.dir[dir]->path,tp);
								np=aliasfile;
							}
							else 
								alias_dir=TRUE;
						}

						if(!alias_dir && !fexist(np))
							continue;

						if(detail) {

							if(alias_dir==TRUE) {
								fprintf(fp,"drwxr-xr-x   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
									,NAME_LEN
									,scfg.sys_id
									,scfg.lib[scfg.dir[dir]->lib]->sname
									,512L
									,mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
									,p);
							}
							else {
								t=fdate(np);
								tm_p=localtime(&t);
								if(tm_p==NULL)
									memset(&tm,0,sizeof(tm));
								else
									tm=*tm_p;
								fprintf(fp,"-rw-r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
									,NAME_LEN
									,scfg.sys_id
									,scfg.sys_id
									,flength(np)
									,mon[tm.tm_mon],tm.tm_mday,tm.tm_hour,tm.tm_min
									,p);
							}
						} else
							fprintf(fp,"%s\r\n",p);

					}

					fclose(alias_fp);
				}

				/* Library folders */
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user))
						continue;
					if(detail)
						fprintf(fp,"drwxr-xr-x   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
							,NAME_LEN
							,scfg.sys_id
							,scfg.sys_id
							,512L
							,mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
							,scfg.lib[i]->sname);
					else
						fprintf(fp,"%s\r\n",scfg.lib[i]->sname);
				}
			} else if(dir<0) {
				lprintf("%04d %s listing: %s library",sock,user.alias,scfg.lib[lib]->sname);
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(i!=scfg.sysop_dir && i!=scfg.upload_dir 
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user))
						continue;
					if(detail)
						fprintf(fp,"drwxr-xr-x   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
							,NAME_LEN
							,scfg.sys_id
							,scfg.lib[lib]->sname
							,512L
							,mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
							,scfg.dir[i]->code);
					else
						fprintf(fp,"%s\r\n",scfg.dir[i]->code);
				}
			} else if(chk_ar(&scfg,scfg.dir[dir]->ar,&user)) {
				lprintf("%04d %s listing: %s/%s directory"
					,sock,user.alias,scfg.lib[lib]->sname,scfg.dir[dir]->code);

				sprintf(path,"%s%s",scfg.dir[dir]->path,*p ? p : "*");
				glob(path,0,NULL,&g);
				for(i=0;i<(int)g.gl_pathc;i++) {
					if(isdir(g.gl_pathv[i]))
						continue;
#ifdef _WIN32
					GetShortPathName(g.gl_pathv[i], str, sizeof(str));
#else
					strcpy(str,g.gl_pathv[i]);
#endif
					padfname(getfname(str),f.name);
					strupr(f.name);
					f.dir=dir;
					if((filedat=getfileixb(&scfg,&f))==FALSE
						&& !(startup->options&FTP_OPT_DIR_FILES))
						continue;
					if(detail) {
						f.size=flength(g.gl_pathv[i]);
						getfiledat(&scfg,&f);
						t=fdate(g.gl_pathv[i]);
						tm_p=localtime(&t);
						if(tm_p==NULL)
							memset(&tm,0,sizeof(tm));
						else
							tm=*tm_p;
						fprintf(fp,"-rw-r--r--   1 %-*s %-8s %9ld %s %2d "
							,NAME_LEN
							,filedat ? dotname(f.uler,str) : scfg.sys_id
							,scfg.dir[dir]->code
							,f.size
							,mon[tm.tm_mon],tm.tm_mday);
						if(tm.tm_year==cur_tm.tm_year)
							fprintf(fp,"%02d:%02d %s\r\n"
								,tm.tm_hour,tm.tm_min
								,getfname(g.gl_pathv[i]));
						else
							fprintf(fp,"%5d %s\r\n"
								,1900+tm.tm_year
								,getfname(g.gl_pathv[i]));
					} else
						fprintf(fp,"%s\r\n",getfname(g.gl_pathv[i]));
				}
				globfree(&g);
			} else 
				lprintf("%04d %s listing: %s/%s directory (empty - no access)"
					,sock,user.alias,scfg.lib[lib]->sname,scfg.dir[dir]->code);

			fclose(fp);
			filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,0L
				,&transfer_inprogress,&transfer_aborted
				,TRUE /* delfile */
				,TRUE /* tmpfile */
				,&lastactive,&user,dir,FALSE,FALSE,FALSE,NULL);
			continue;
		}

		if(!strnicmp(cmd, "RETR ", 5) 
			|| !strnicmp(cmd, "SIZE ",5) 
			|| !strnicmp(cmd, "MDTM ",5)
			|| !strnicmp(cmd, "DELE ",5)) {
			getdate=FALSE;
			getsize=FALSE;
			delecmd=FALSE;
			if(!strnicmp(cmd,"SIZE ",5))
				getsize=TRUE;
			else if(!strnicmp(cmd,"MDTM ",5))
				getdate=TRUE;
			else if(!strnicmp(cmd,"DELE ",5))
				delecmd=TRUE;

			if(!getsize && !getdate && user.rest&FLAG('D')) {
				sockprintf(sock,"550 Insufficient access.");
				filepos=0;
				continue;
			}
			credits=TRUE;
			success=FALSE;
			delfile=FALSE;
			tmpfile=FALSE;
			lib=curlib;
			dir=curdir;

			p=cmd+5;
			while(*p && *p<=' ') p++;

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				lib=-1;
				p++;
			}
			else if(!strncmp(p,"./",2))
				p+=2;

			if(lib<0 && alias(p, fname, &user, &dir)==TRUE) {
				success=TRUE;
				credits=TRUE;	/* include in d/l stats */
				tmpfile=FALSE;
				delfile=FALSE;
				lprintf("%04d %s %.4s by alias: %s"
					,sock,user.alias,cmd,p);
				p=getfname(fname);
				if(dir>=0)
					lib=scfg.dir[dir]->lib;
			}
			if(!success && lib<0 && (tp=strchr(p,'/'))!=NULL) {
				dir=-1;
				*tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) 
					lib=i;
				p=tp+1;
			}
			if(!success && dir<0 && (tp=strchr(p,'/'))!=NULL) {
				*tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(!chk_ar(&scfg,scfg.dir[i]->ar,&user))
						continue;
					if(!stricmp(scfg.dir[i]->code,p))
						break;
				}
				if(i<scfg.total_dirs) 
					dir=i;
				p=tp+1;
			}

			sprintf(html_index_ext,"%s?",startup->html_index_file);

			sprintf(str,"%s.qwk",scfg.sys_id);
			if(lib<0 && startup->options&FTP_OPT_ALLOW_QWK 
				&& !stricmp(p,str) && !delecmd) {
				lprintf("%04d %s creating/updating QWK packet...",sock,user.alias);
				sprintf(str,"%spack%04u.now",scfg.data_dir,user.number);
				if((file=open(str,O_WRONLY|O_CREAT,S_IWRITE))==-1) {
					lprintf("%04d !ERROR %d opening %s",sock, errno, str);
					sockprintf(sock, "451 !ERROR %d creating semaphore file",errno);
					filepos=0;
					continue;
				}
				close(file);
				t=time(NULL);
				while(fexist(str)) {
					if(time(NULL)-t>startup->qwk_timeout)
						break;
					mswait(1000);
				}
				if(fexist(str)) {
					lprintf("%04d !TIMEOUT waiting for QWK packet creation",sock);
					sockprintf(sock,"451 Time-out waiting for packet creation.");
					remove(str);
					filepos=0;
					continue;
				}
				if(!fexist(qwkfile)) {
					lprintf("%04d No QWK Packet created (no new messages)",sock);
					sockprintf(sock,"550 No QWK packet created (no new messages)");
					filepos=0;
					continue;
				}
				strcpy(fname,qwkfile);
				success=TRUE;
				delfile=TRUE;
				credits=FALSE;
				lprintf("%04d %s downloading QWK packet (%ld bytes) in %s mode"
					,sock,user.alias,flength(fname)
					,pasv_sock==INVALID_SOCKET ? "active":"passive");
			/* ASCII Index File */
			} else if(startup->options&FTP_OPT_INDEX_FILE 
				&& !stricmp(p,startup->index_file_name)
				&& !delecmd) {
				sprintf(fname,"%sftp%d.tx", scfg.data_dir, sock);
				if((fp=fopen(fname,"w+b"))==NULL) {
					lprintf("%04d !ERROR %d opening %s",sock,errno,fname);
					sockprintf(sock, "451 Insufficient system storage");
					filepos=0;
					continue;
				}
				if(!getsize && !getdate)
					lprintf("%04d %s downloading index for %s in %s mode"
						,sock,user.alias,vpath(lib,dir,str)
						,pasv_sock==INVALID_SOCKET ? "active":"passive");
				success=TRUE;
				credits=FALSE;
				tmpfile=TRUE;
				delfile=TRUE;
				fprintf(fp,"%-*s File/Folder Descriptions\r\n"
					,INDEX_FNAME_LEN,startup->index_file_name);
				if(startup->options&FTP_OPT_HTML_INDEX_FILE)
					fprintf(fp,"%-*s File/Folder Descriptions (HTML)\r\n"
						,INDEX_FNAME_LEN,startup->html_index_file);
				if(lib<0) {

					/* File Aliases */
					sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
					if((alias_fp=fopen(aliasfile,"r"))!=NULL) {

						while(!feof(alias_fp)) {
							if(!fgets(aliasline,sizeof(aliasline)-1,alias_fp))
								break;

							p=aliasline;	/* alias pointer */
							while(*p && *p<=' ') p++;

							if(*p==';')	/* comment */
								continue;

							tp=p;		/* terminator pointer */
							while(*tp && *tp>' ') tp++;
							if(*tp) *tp=0;

							np=tp+1;	/* filename pointer */
							while(*np && *np<=' ') np++;

							np++;		/* description pointer */
							while(*np && *np>' ') np++;

							while(*np && *np<' ') np++;

							truncsp(np);

							fprintf(fp,"%-*s %s\r\n",INDEX_FNAME_LEN,p,np);
						}

						fclose(alias_fp);
					}

					/* QWK Packet */
					if(startup->options&FTP_OPT_ALLOW_QWK /* && fexist(qwkfile) */) {
						sprintf(str,"%s.qwk",scfg.sys_id);
						fprintf(fp,"%-*s QWK Message Packet\r\n"
							,INDEX_FNAME_LEN,str);
					}

					/* Library Folders */
					for(i=0;i<scfg.total_libs;i++) {
						if(!chk_ar(&scfg,scfg.lib[i]->ar,&user))
							continue;
						fprintf(fp,"%-*s %s\r\n"
							,INDEX_FNAME_LEN,scfg.lib[i]->sname,scfg.lib[i]->lname);
					}
				} else if(dir<0) {
					for(i=0;i<scfg.total_dirs;i++) {
						if(scfg.dir[i]->lib!=lib)
							continue;
						if(i!=scfg.sysop_dir && i!=scfg.upload_dir
							&& !chk_ar(&scfg,scfg.dir[i]->ar,&user))
							continue;
						fprintf(fp,"%-*s %s\r\n"
							,INDEX_FNAME_LEN,scfg.dir[i]->code,scfg.dir[i]->lname);
					}
				} else if(chk_ar(&scfg,scfg.dir[dir]->ar,&user)){
					sprintf(cmd,"%s*",scfg.dir[dir]->path);
					glob(cmd,0,NULL,&g);
					for(i=0;i<(int)g.gl_pathc;i++) {
						if(isdir(g.gl_pathv[i]))
							continue;
#ifdef _WIN32
						GetShortPathName(g.gl_pathv[i], str, sizeof(str));
#else
						strcpy(str,g.gl_pathv[i]);
#endif
						padfname(getfname(str),f.name);
						strupr(f.name);
						f.dir=dir;
						if(getfileixb(&scfg,&f)) {
							f.size=flength(g.gl_pathv[i]);
							getfiledat(&scfg,&f);
							fprintf(fp,"%-*s %s\r\n",INDEX_FNAME_LEN
								,getfname(g.gl_pathv[i]),f.desc);
						}
					}
					globfree(&g);
				}
				fclose(fp);
			/* HTML Index File */
			} else if(startup->options&FTP_OPT_HTML_INDEX_FILE 
				&& (!stricmp(p,startup->html_index_file) 
				|| !strnicmp(p,html_index_ext,strlen(html_index_ext)))
				&& !delecmd) {
#ifdef JAVASCRIPT
				JS_BeginRequest(js_cx);	/* Required for multi-thread support */

				js_val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, "name"));
				JS_SetProperty(js_cx, js_ftp, "sort", &js_val);
				js_val=BOOLEAN_TO_JSVAL(FALSE);
				JS_SetProperty(js_cx, js_ftp, "reverse", &js_val);

				if(!strnicmp(p,html_index_ext,strlen(html_index_ext))) {
					p+=strlen(html_index_ext);
					tp=strrchr(p,'$');
					if(tp!=NULL)
						*tp=0;
					if(!strnicmp(p,"ext=",4)) {
						p+=4;
						if(!strcmp(p,"on"))
							user.misc|=EXTDESC;
						else
							user.misc&=~EXTDESC;
						if(!(user.rest&FLAG('G')))
							putuserrec(&scfg,user.number,U_MISC,8,ultoa(user.misc,str,16));
					} 
					else if(!strnicmp(p,"sort=",5)) {
						p+=5;
						tp=strchr(p,'&');
						if(tp!=NULL) {
							*tp=0;
							tp++;
							if(!stricmp(tp,"reverse")) {
								js_val=BOOLEAN_TO_JSVAL(TRUE);
								JS_SetProperty(js_cx, js_ftp, "reverse", &js_val);
							}
						}
						js_val=STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, p));
						JS_SetProperty(js_cx, js_ftp, "sort", &js_val);
					}
				}
				JS_EndRequest(js_cx);	/* Required for multi-thread support */
#endif
				sprintf(fname,"%sftp%d.tx", scfg.data_dir, sock);
				if((fp=fopen(fname,"w+b"))==NULL) {
					lprintf("%04d !ERROR %d opening %s",sock,errno,fname);
					sockprintf(sock, "451 Insufficient system storage");
					filepos=0;
					continue;
				}
				if(!getsize && !getdate)
					lprintf("%04d %s downloading HTML index for %s in %s mode"
						,sock,user.alias,vpath(lib,dir,str)
						,pasv_sock==INVALID_SOCKET ? "active":"passive");
				success=TRUE;
				credits=FALSE;
				tmpfile=TRUE;
				delfile=TRUE;
#ifdef JAVASCRIPT
				JS_BeginRequest(js_cx);	/* Required for multi-thread support */

				js_val=INT_TO_JSVAL(timeleft);
				if(!JS_SetProperty(js_cx, js_ftp, "time_left", &js_val))
					lprintf("%04d !JavaScript ERROR setting user.time_left",sock);
				js_generate_index(js_cx, js_ftp, sock, fp, lib, dir, &user);

				JS_EndRequest(js_cx);	/* Required for multi-thread support */
#endif
				fclose(fp);
			} else if(dir>=0) {

				if(!chk_ar(&scfg,scfg.dir[dir]->ar,&user)) {
					lprintf("%04d !%s has insufficient access to /%s/%s"
						,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname,scfg.dir[dir]->code);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}

				if(!getsize && !getdate && !delecmd
					&& !chk_ar(&scfg,scfg.dir[dir]->dl_ar,&user)) {
					lprintf("%04d !%s has insufficient access to download from /%s/%s"
						,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname,scfg.dir[dir]->code);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}

				if(delecmd && !dir_op(&scfg,&user,dir)) {
					lprintf("%04d !%s has insufficient access to delete files in /%s/%s"
						,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname,scfg.dir[dir]->code);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}
				sprintf(fname,"%s%s",scfg.dir[dir]->path,p);
#ifdef _WIN32
				GetShortPathName(fname, str, sizeof(str));
#else
				strcpy(str,fname);
#endif
				padfname(getfname(str),f.name);
				strupr(f.name);
				f.dir=dir;
				f.cdt=0;
				f.size=-1;
				filedat=getfileixb(&scfg,&f);
				if(!filedat && !(startup->options&FTP_OPT_DIR_FILES)) {
					sockprintf(sock,"550 File not found: %s",p);
					lprintf("%04d !%s file (%s%s) not in database for %.4s command"
						,sock,user.alias,vpath(lib,dir,str),p,cmd);
					filepos=0;
					continue;
				}

				/* Verify credits */
				if(!getsize && !getdate && !delecmd
					&& !(scfg.dir[dir]->misc&DIR_FREE) 
					&& !(user.exempt&FLAG('D'))) {
					if(filedat)
						getfiledat(&scfg,&f);
					else
						f.cdt=flength(fname);
					if(f.cdt>(user.cdt+user.freecdt)) {
						lprintf("%04d !%s has insufficient credit to download /%s/%s/%s (%lu credits)"
							,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname
							,scfg.dir[dir]->code
							,p
							,f.cdt);
						sockprintf(sock,"550 Insufficient credit (%lu required).",f.cdt);
						filepos=0;
						continue;
					}
				}

				if(strcspn(p,ILLEGAL_FILENAME_CHARS)!=strlen(p)) {
					success=FALSE;
					lprintf("%04d !%s illegal filename attempt: %s"
						,sock,user.alias,p);
					hacklog(&scfg, "FTP", user.alias, cmd, host_name, &ftp.client_addr);
				} else {
					if(fexist(fname)) {
						success=TRUE;
						if(!getsize && !getdate && !delecmd)
							lprintf("%04d %s downloading: %s (%ld bytes) in %s mode"
								,sock,user.alias,fname,flength(fname)
								,pasv_sock==INVALID_SOCKET ? "active":"passive");
					} 
				}
			}
			socket_debug[sock]|=SOCKET_DEBUG_DOWNLOAD;

			if(getsize && success) 
				sockprintf(sock,"213 %lu",flength(fname));
			else if(getdate && success) {
				t=fdate(fname);
				tm_p=gmtime(&t);
				if(tm_p==NULL)
					memset(&tm,0,sizeof(tm));
				else
					tm=*tm_p;
				sockprintf(sock,"213 %u%02u%02u%02u%02u%02u"
					,1900+tm.tm_year,tm.tm_mon+1,tm.tm_mday
					,tm.tm_hour,tm.tm_min,tm.tm_sec);
			} else if(delecmd && success) {
				if(remove(fname)!=0) {
					lprintf("%04d !ERROR %d deleting %s",sock,errno,fname);
					sockprintf(sock,"450 %s could not be deleted (error: %d)"
						,fname,errno);
				} else {
					lprintf("%04d %s deleted %s",sock,user.alias,fname);
					if(filedat) 
						removefiledat(&scfg,&f);
					sockprintf(sock,"250 %s deleted.",fname);
				}
			} else if(success) {
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,delfile,tmpfile
					,&lastactive,&user,dir,FALSE,credits,FALSE,NULL);
			}
			else {
				sockprintf(sock,"550 File not found: %s",p);
				lprintf("%04d !%s file (%s%s) not found for %.4s command"
					,sock,user.alias,vpath(lib,dir,str),p,cmd);
			}
			filepos=0;
			socket_debug[sock]&=~SOCKET_DEBUG_DOWNLOAD;
			continue;
		}

		if(!strnicmp(cmd, "DESC", 4)) {

			if(user.rest&FLAG('U')) {
				sockprintf(sock,"553 Insufficient access.");
				continue;
			}

			p=cmd+4;
			while(*p && *p<=' ') p++;

			if(*p==0) 
				sockprintf(sock,"501 No file description given.");
			else {
				sprintf(desc,"%.*s",(int)sizeof(desc)-1,p);
				sockprintf(sock,"200 File description set. Ready to STOR file.");
			}
			continue;
		}

		if(!strnicmp(cmd, "STOR ", 5)) {

			if(user.rest&FLAG('U')) {
				sockprintf(sock,"553 Insufficient access.");
				continue;
			}

			if(transfer_inprogress==TRUE) {
				lprintf("%04d !TRANSFER already in progress (%s)",sock,cmd);
				sockprintf(sock,"425 Transfer already in progress.");
				continue;
			}

			lib=curlib;
			dir=curdir;
			p=cmd+5;

			while(*p && *p<=' ') p++;

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				lib=-1;
				p++;
			}
			else if(!strncmp(p,"./",2))
				p+=2;
			/* Need to add support for uploading to aliased directories */
			if(lib<0 && (tp=strchr(p,'/'))!=NULL) {
				dir=-1;
				*tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) 
					lib=i;
				p=tp+1;
			}
			if(dir<0 && (tp=strchr(p,'/'))!=NULL) {
				*tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(i!=scfg.sysop_dir && i!=scfg.upload_dir 
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user))
						continue;
					if(!stricmp(scfg.dir[i]->code,p))
						break;
				}
				if(i<scfg.total_dirs) 
					dir=i;
				p=tp+1;
			}
			if(dir<0) {
				sprintf(str,"%s.rep",scfg.sys_id);
				if(!(startup->options&FTP_OPT_ALLOW_QWK)
					|| stricmp(p,str)) {
					lprintf("%04d !%s attempted to upload to invalid directory"
						,sock,user.alias);
					sockprintf(sock,"553 Invalid directory.");
					continue;
				}
				sprintf(fname,"%sfile/%04d.rep",scfg.data_dir,user.number);
				lprintf("%04d %s uploading: %s in %s mode"
					,sock,user.alias,fname
					,pasv_sock==INVALID_SOCKET ? "active":"passive");
			} else {
				if(!chk_ar(&scfg,scfg.dir[dir]->ul_ar,&user)) {
					lprintf("%04d !%s has insufficient access to upload to /%s/%s"
						,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname,scfg.dir[dir]->code);
					sockprintf(sock,"553 Insufficient access.");
					continue;
				}
				if(strcspn(p,ILLEGAL_FILENAME_CHARS)!=strlen(p)
					|| trashcan(&scfg,p,"file")) {
					lprintf("%04d !%s illegal filename attempt: %s"
						,sock,user.alias,p);
					sockprintf(sock,"553 Illegal filename attempt");
					hacklog(&scfg, "FTP", user.alias, cmd, host_name, &ftp.client_addr);
					continue;
				}
				sprintf(fname,"%s%s",scfg.dir[dir]->path,p);
				if(fexist(fname)  
					|| (startup->options&FTP_OPT_INDEX_FILE 
						&& !stricmp(p,startup->index_file_name))
					|| (startup->options&FTP_OPT_HTML_INDEX_FILE 
						&& !stricmp(p,startup->html_index_file))
					) {
					lprintf("%04d !%s attempted to overwrite existing file: %s"
						,sock,user.alias,fname);
					sockprintf(sock,"553 File already exists.");
					continue;
				}
				lprintf("%04d %s uploading: %s to %s (%s) in %s mode"
					,sock,user.alias
					,p						/* filename */
					,vpath(lib,dir,str)		/* virtual path */
					,scfg.dir[dir]->path	/* actual path */
					,pasv_sock==INVALID_SOCKET ? "active":"passive");
			}
			sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
			filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
				,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
				,&lastactive
				,&user
				,dir
				,TRUE	/* uploading */
				,TRUE	/* credits */
				,FALSE	/* append */
				,desc
				);
			filepos=0;
			continue;
		}

		if(!stricmp(cmd,"CDUP") || !stricmp(cmd,"XCUP")) {
			if(curdir<0)
				curlib=-1;
			else
				curdir=-1;
			sockprintf(sock,"200 CDUP command successful.");
			continue;
		}

		if(!strnicmp(cmd, "CWD ", 4) || !strnicmp(cmd,"XCWD ",5)) {
			p=cmd+4;
			while(*p && *p<=' ') p++;

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				curlib=-1;
				curdir=-1;
				p++;
			}
			/* Local File System? */
			if(sysop && !(startup->options&FTP_OPT_NO_LOCAL_FSYS) 
				&& !strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) {	
				p+=strlen(LOCAL_FSYS_DIR);
				if(!direxist(p)) {
					sockprintf(sock,"550 Directory does not exist.");
					lprintf("%04d !%s attempted to mount invalid directory: %s"
						,sock, user.alias, p);
					continue;
				}
				sprintf(local_dir,"%.*s",(int)sizeof(local_dir)-1,p);
				local_fsys=TRUE;
				sockprintf(sock,"250 CWD command successful (local file system mounted).");
				lprintf("%04d %s mounted local file system", sock, user.alias);
				continue;
			}
			success=FALSE;

			/* Directory Alias? */
			if(curlib<0 && alias(p,NULL,&user,&curdir)==TRUE) {
				if(curdir>=0)
					curlib=scfg.dir[curdir]->lib;
				success=TRUE;
			}

			orglib=curlib;
			orgdir=curdir;
			tp=0;
			if(!strncmp(p,"...",3)) {
				curlib=-1;
				curdir=-1;
				p+=3;
			}
			if(!strncmp(p,"./",2))
				p+=2;
			else if(!strncmp(p,"..",2)) {
				if(curdir<0)
					curlib=-1;
				else
					curdir=-1;
				p+=2;
			}
			if(*p==0)
				success=TRUE;
			else if(!strcmp(p,".")) 
				success=TRUE;
			if(!success  && (curlib<0 || *p=='/')) { /* Root dir */
				if(*p=='/') p++;
				tp=strchr(p,'/');
				if(tp) *tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) {
					curlib=i;
					success=TRUE;
				}
			}
			if((!success && curdir<0) || (success && tp && *(tp+1))) {
				if(tp)
					p=tp+1;
				tp=strchr(p,'/');
				if(tp) *tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=curlib)
						continue;
					if(i!=scfg.sysop_dir && i!=scfg.upload_dir
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user))
						continue;
					if(!stricmp(scfg.dir[i]->code,p))
						break;
				}
				if(i<scfg.total_dirs) {
					curdir=i;
					success=TRUE;
				} else
					success=FALSE;
			}

			if(success)
				sockprintf(sock,"250 CWD command successful.");
			else {
				sockprintf(sock,"550 %s: No such file or directory.",p);
				curlib=orglib;
				curdir=orgdir;
			}
			continue;
		}

		if(!stricmp(cmd, "PWD") || !stricmp(cmd,"XPWD")) {
			if(curlib<0)
				sockprintf(sock,"257 \"/\" is current directory.");
			else if(curdir<0)
				sockprintf(sock,"257 \"/%s\" is current directory."
					,scfg.lib[curlib]->sname);
			else
				sockprintf(sock,"257 \"/%s/%s\" is current directory."
					,scfg.lib[curlib]->sname,scfg.dir[curdir]->code);
			continue;
		}

		if(!strnicmp(cmd, "MKD", 3) || 
			!strnicmp(cmd,"XMKD",4) || 
			!strnicmp(cmd,"SITE EXEC",9)) 
			hacklog(&scfg, "FTP", user.alias, cmd, host_name, &ftp.client_addr);
		
		sockprintf(sock,"500 Syntax error: '%s'",cmd);
		lprintf("%04d !FTP: UNSUPPORTED COMMAND: '%s'",sock,cmd);
	} /* while(1) */

	if(transfer_inprogress==TRUE) {
		lprintf("%04d Waiting for transfer to complete...",sock);
		while(data_sock!=INVALID_SOCKET && transfer_inprogress==TRUE) {
			mswait(500);
			if(gettimeleft(&scfg,&user,logintime)<1) {
				lprintf("%04d Out of time, disconnecting",sock);
				sockprintf(sock,"421 Sorry, you've run out of time.");
				ftp_close_socket(&data_sock,__LINE__);
				transfer_aborted=TRUE;
			}
			if((time(NULL)-lastactive)>startup->max_inactivity) {
				lprintf("%04d Disconnecting due to to inactivity.",sock);
				sockprintf(sock,"421 Disconnecting due to inactivity (%u seconds)."
					,startup->max_inactivity);
				ftp_close_socket(&data_sock,__LINE__);
				transfer_aborted=TRUE;
			}
		}
		lprintf("%04d Done waiting for transfer to complete",sock);
	}

	/* Update User Statistics */
	if(user.number) {
		user.tlast=(ushort)(time(NULL)-logintime);
		putuserrec(&scfg,user.number,U_LASTON,8,ultoa(time(NULL),str,16));
		putuserrec(&scfg,user.number,U_TLAST,5,ultoa(user.tlast,str,10));
		adjustuserrec(&scfg,user.number,U_TIMEON,5,user.tlast);
	}

	if(user.number)
		lprintf("%04d %s logged off.",sock,user.alias);

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&FTP_OPT_MUTE)) 
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

#ifdef JAVASCRIPT
	lprintf("%04d JavaScript: Destroying context",sock);
	JS_DestroyContext(js_cx);	/* Free Context */
#endif

	status(STATUS_WFC);

	lprintf("%04d CTRL thread terminated", sock);
	active_clients--;
	update_clients();
	client_off(sock);

	socket_debug[sock]&=~SOCKET_DEBUG_CTRL;

	/* Free up resources here */
	ftp_close_socket(&sock,__LINE__);
	if(pasv_sock!=INVALID_SOCKET)
		ftp_close_socket(&pasv_sock,__LINE__);
	if(data_sock!=INVALID_SOCKET)
		ftp_close_socket(&data_sock,__LINE__);

	thread_down();
}

static void cleanup(int code)
{
	free_cfg(&scfg);

	if(server_socket!=INVALID_SOCKET)
		ftp_close_socket(&server_socket,__LINE__);
	server_socket=INVALID_SOCKET;

	update_clients();

#ifdef _WINSOCKAPI_
	if(WSAInitialized && WSACleanup()!=0) 
		lprintf("0000 !WSACleanup ERROR %d",ERROR_VALUE);
#endif

#ifdef _WIN32
	if(socket_mutex!=NULL) {
		CloseHandle(socket_mutex);
		socket_mutex=NULL;
	}
#endif

#ifdef JAVASCRIPT
	lprintf("0000 JavaScript: Destroying runtime");
	if(js_runtime!=NULL) {
		JS_DestroyRuntime(js_runtime);
		js_runtime=NULL;
	}
#endif

    lprintf("#### FTP Server thread terminated");
	status("Down");
	if(startup!=NULL && startup->terminated!=NULL)
		startup->terminated(code);
	thread_down();
}

char* DLLCALL ftp_ver(void)
{
	static char ver[256];
	char compiler[32];

	COMPILER_DESC(compiler);

	sprintf(ver,"%s v%s%s  "
		"Compiled %s %s with %s"
		,FTP_SERVER
		,FTP_VERSION
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,__DATE__, __TIME__, compiler);

	return(ver);
}

void DLLCALL ftp_server(void* arg)
{
	char			compiler[32];
	SOCKADDR_IN		server_addr;
	SOCKADDR_IN		client_addr;
	int				client_addr_len;
	SOCKET			client_socket;
	int				i;
	int				result;
	time_t			t;
	time_t			start;
	LINGER			linger;
	fd_set			socket_set;
	ftp_t*			ftp;
	struct timeval	tv;

	startup=(ftp_startup_t*)arg;

    if(startup==NULL) {
    	sbbs_beep(100,500);
    	fprintf(stderr, "No startup structure passed!\n");
    	return;
    }

	if(startup->size!=sizeof(ftp_startup_t)) {	/* verify size */
		sbbs_beep(100,500);
		sbbs_beep(300,500);
		sbbs_beep(100,500);
		fprintf(stderr, "Invalid startup structure!\n");
		return;
	}

	/* Setup intelligent defaults */
	if(startup->port==0)					startup->port=IPPORT_FTP;
	if(startup->qwk_timeout==0)				startup->qwk_timeout=600;		/* seconds */
	if(startup->max_inactivity==0)			startup->max_inactivity=300;	/* seconds */
	if(startup->index_file_name[0]==0)		strcpy(startup->index_file_name,"00index");
	if(startup->html_index_file[0]==0)		strcpy(startup->html_index_file,"00index.html");
	if(startup->html_index_script[0]==0)	strcpy(startup->html_index_script,"ftp-html.js");

	/*temporary*/
#ifdef JAVASCRIPT
	startup->options|=FTP_OPT_HTML_INDEX_FILE;
#endif

	thread_up();

	status("Initializing");

    memset(&scfg, 0, sizeof(scfg));

#ifdef __unix__		/* Ignore "Broken Pipe" signal */
	signal(SIGPIPE,SIG_IGN);
#endif

	lprintf("Synchronet FTP Server Version %s%s"
		,FTP_VERSION
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		);

	COMPILER_DESC(compiler);

	lprintf("Compiled %s %s with %s", __DATE__, __TIME__, compiler);

	srand(time(NULL));

	if(!(startup->options&FTP_OPT_LOCAL_TIMEZONE)) { 
		if(PUTENV("TZ=UCT0"))
			lprintf("!putenv() FAILED");
		tzset();

		if((t=checktime())!=0) {   /* Check binary time */
			lprintf("!TIME PROBLEM (%ld)",t);
			cleanup(1);
			return;
		}
	}

	if(!winsock_startup()) {
		cleanup(1);
		return;
	}

	t=time(NULL);
	lprintf("Initializing on %.24s with options: %lx"
		,ctime(&t),startup->options);

#ifdef _WIN32
    if((socket_mutex=CreateMutex(NULL,FALSE,NULL))==NULL) {
    	lprintf("!ERROR %d creating socket_mutex", GetLastError());
		cleanup(1);
        return;
    }
#endif

	/* Initial configuration and load from CNF files */
    sprintf(scfg.ctrl_dir, "%.*s",(int)sizeof(scfg.ctrl_dir)-1
    	,startup->ctrl_dir);
    lprintf("Loading configuration files from %s", scfg.ctrl_dir);
	scfg.size=sizeof(scfg);
	if(!load_cfg(&scfg, NULL, TRUE)) {
		lprintf("!Failed to load configuration files");
		cleanup(1);
		return;
	}

	/* Use DATA/TEMP for temp dir - should ch'd to be FTP/HOST specific */
	prep_dir(scfg.data_dir, scfg.temp_dir);

	if(!startup->max_clients) {
		startup->max_clients=scfg.sys_nodes;
		if(startup->max_clients<10)
			startup->max_clients=10;
	}
	lprintf("Maximum clients: %d",startup->max_clients);

	lprintf("Maximum inactivity: %d seconds",startup->max_inactivity);

	active_clients=0;
	update_clients();

	strlwr(scfg.sys_id); /* Use lower-case unix-looking System ID for group name */

	for(i=0;i<scfg.total_libs;i++) {
		strlwr(scfg.lib[i]->sname);
		dotname(scfg.lib[i]->sname,scfg.lib[i]->sname);
	}

	for(i=0;i<scfg.total_dirs;i++) 
		strlwr(scfg.dir[i]->code);

#ifdef JAVASCRIPT
	lprintf("JavaScript: Creating runtime: %lu bytes"
		,JAVASCRIPT_RUNTIME_MEMORY);

	if((js_runtime = JS_NewRuntime(JAVASCRIPT_RUNTIME_MEMORY))==NULL) {
		lprintf("!JS_NewRuntime failed");
		cleanup(1);
		return;
	}

	lprintf("JavaScript: Context stack: %lu bytes", JAVASCRIPT_CONTEXT_STACK);
#endif

    /* open a socket and wait for a client */

    if((server_socket=ftp_open_socket(SOCK_STREAM))==INVALID_SOCKET) {
		lprintf("!ERROR %d opening socket", ERROR_VALUE);
		cleanup(1);
		return;
	}

    lprintf("%04d FTP socket opened",server_socket);

#if 1
	linger.l_onoff=TRUE;
    linger.l_linger=5;	/* seconds */

	if((result=setsockopt(server_socket, SOL_SOCKET, SO_LINGER
    	,(char *)&linger, sizeof(linger)))!=0) {
		lprintf ("%04d !ERROR %d (%d) setting socket options."
			,server_socket, result, ERROR_VALUE);
		cleanup(1);
		return;
	}
#endif

	/*****************************/
	/* Listen for incoming calls */
	/*****************************/
    memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_addr.s_addr = htonl(startup->interface_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(startup->port);

    if((result=bind(server_socket, (struct sockaddr *) &server_addr
    	,sizeof(server_addr)))!=0) {
		lprintf("%04d !ERROR %d (%d) binding socket to port %d"
			,server_socket, result, ERROR_VALUE,startup->port);
		cleanup(1);
		return;
	}

    if((result=listen(server_socket, 1))!= 0) {
		lprintf("%04d !ERROR %d (%d) listening on socket"
			,server_socket, result, ERROR_VALUE);
		cleanup(1);
		return;
	}

	/* signal caller that we've started up successfully */
    if(startup->started!=NULL)
    	startup->started();

	lprintf("%04d FTP Server thread started on port %d",server_socket,startup->port);
	status(STATUS_WFC);

	while(server_socket!=INVALID_SOCKET) {

		/* now wait for connection */

		tv.tv_sec=2;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(server_socket,&socket_set);

		if((i=select(server_socket+1,&socket_set,NULL,NULL,&tv))<1) {
			if(i==0) {
				mswait(1);
				continue;
			}
			if(ERROR_VALUE==EINTR)
				lprintf("0000 FTP Server listening interrupted");
			else if(ERROR_VALUE == ENOTSOCK)
            	lprintf("0000 FTP Server sockets closed");
			else
				lprintf("0000 !ERROR %d selecting sockets",ERROR_VALUE);
			break;
		}

		client_addr_len = sizeof(client_addr);
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr
        	,&client_addr_len);

		if(client_socket == INVALID_SOCKET)
		{
			if(ERROR_VALUE == ENOTSOCK || ERROR_VALUE == EINTR)
            	lprintf("0000 FTP socket closed while listening");
            else
				lprintf("0000 !accept failed (ERROR %d)", ERROR_VALUE);
			continue;	/* Jun-08-2001 was break; */
		}
		if(startup->socket_open!=NULL)
			startup->socket_open(TRUE);
		sockets++;

		if(active_clients>=startup->max_clients) {
			lprintf("%04d !MAXMIMUM CLIENTS (%d) reached, access denied"
				,client_socket, startup->max_clients);
			sockprintf(client_socket,"421 Maximum active clients reached, please try again later.");
			mswait(3000);
			ftp_close_socket(&client_socket,__LINE__);
			continue;
		}

		if((ftp=malloc(sizeof(ftp_t)))==NULL) {
			lprintf("%04d !ERROR allocating %d bytes of memory for ftp_t"
				,client_socket,sizeof(ftp_t));
			sockprintf(client_socket,"421 System error, please try again later.");
			mswait(3000);
			ftp_close_socket(&client_socket,__LINE__);
			continue;
		}

		ftp->socket=client_socket;
		ftp->client_addr=client_addr;

		_beginthread (ctrl_thread, 0, ftp);
	}

	if(active_clients) {
		lprintf("0000 Waiting for %d active clients to disconnect...", active_clients);
		start=time(NULL);
		while(active_clients) {
			if(time(NULL)-start>TIMEOUT_THREAD_WAIT) {
				lprintf("0000 !TIMEOUT waiting for %d active clients ",active_clients);
				break;
			}
			mswait(100);
		}
	}

	cleanup(0);
}