/* services.c */

/* Synchronet Services */

/* $Id: services.c,v 1.143 2003/10/09 02:06:44 rswindell Exp $ */

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

/* Platform-specific headers */
#ifdef _WIN32

	#include <io.h>			/* open/close */
	#include <share.h>		/* share open flags */
	#include <process.h>	/* _beginthread */
	#include <windows.h>	/* required for mmsystem.h */
	#include <mmsystem.h>	/* SND_ASYNC */

#endif

#ifdef __unix__
	#include <sys/param.h>	/* BSD? */
#endif

/* ANSI C Library headers */
#include <stdio.h>
#include <stdlib.h>			/* ltoa in GNU C lib */
#include <stdarg.h>			/* va_list */
#include <string.h>			/* strrchr */
#include <ctype.h>			/* isdigit */
#include <fcntl.h>			/* Open flags */
#include <errno.h>			/* errno */

/* Synchronet-specific headers */
#ifndef JAVASCRIPT
#define JAVASCRIPT	/* required to include JS API headers */
#endif
#include "sbbs.h"
#include "services.h"
#include "ident.h"	/* identify() */
#include "sbbs_ini.h"

/* Constants */

#define MAX_SERVICES			128
#define TIMEOUT_THREAD_WAIT		60		/* Seconds */
#define MAX_UDP_BUF_LEN			8192	/* 8K */
#define DEFAULT_LISTEN_BACKLOG	5

static services_startup_t* startup=NULL;
static scfg_t	scfg;
static DWORD	active_clients=0;
static DWORD	sockets=0;
static BOOL		terminated=FALSE;
static time_t	uptime=0;
static DWORD	served=0;
static char		revision[16];

typedef struct {
	/* These are sysop-configurable */
	WORD	port;
	char	protocol[34];
	char	cmd[128];
	DWORD	max_clients;
	DWORD	options;
	int		listen_backlog;
    DWORD	js_max_bytes;
	DWORD	js_cx_stack;
	DWORD	js_branch_limit;
	DWORD	js_yield_interval;
	DWORD	js_gc_interval;
	/* These are run-time state and stat vars */
	DWORD	clients;
	DWORD	served;
	SOCKET	socket;
	BOOL	running;
	BOOL	terminated;
} service_t;

static ini_bitdesc_t service_options[] = {

	{ BBS_OPT_NO_HOST_LOOKUP		,"NO_HOST_LOOKUP"		},
	{ BBS_OPT_GET_IDENT				,"GET_IDENT"			},
	{ BBS_OPT_NO_RECYCLE			,"NO_RECYCLE"			},
	{ BBS_OPT_MUTE					,"MUTE"					},
	{ SERVICE_OPT_UDP				,"UDP"					},
	{ SERVICE_OPT_STATIC			,"STATIC"				},
	{ SERVICE_OPT_STATIC_LOOP		,"LOOP"					},
	{ SERVICE_OPT_NATIVE			,"NATIVE"				},
	{ SERVICE_OPT_FULL_ACCEPT		,"FULL_ACCEPT"			},
	/* terminator */				
	{ -1							,NULL					}
};

typedef struct {
	SOCKET			socket;
	SOCKADDR_IN		addr;
	time_t			logintime;
	user_t			user;
	client_t*		client;
	service_t*		service;
	js_branch_t		branch;
	/* Initial UDP datagram */
	BYTE*			udp_buf;
	int				udp_len;
} service_client_t;

static service_t	*service=NULL;
static DWORD		services=0;

static int lprintf(char *fmt, ...)
{
	va_list argptr;
	char sbuf[1024];

    if(startup==NULL || startup->lputs==NULL)
        return(0);

#if defined(_WIN32)
	if(IsBadCodePtr((FARPROC)startup->lputs))
		return(0);
#endif

	va_start(argptr,fmt);
    vsnprintf(sbuf,sizeof(sbuf),fmt,argptr);
	sbuf[sizeof(sbuf)-1]=0;
    va_end(argptr);
    return(startup->lputs(startup->cbdata,sbuf));
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

static void update_clients(void)
{
	if(startup!=NULL && startup->clients!=NULL)
		startup->clients(startup->cbdata,active_clients);
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
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,TRUE,setuid);
}

static void thread_down(void)
{
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,FALSE,FALSE);
}

static SOCKET open_socket(int type)
{
	char	error[256];
	SOCKET	sock;

	sock=socket(AF_INET, type, IPPROTO_IP);
	if(sock!=INVALID_SOCKET && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(startup->cbdata,TRUE);
	if(sock!=INVALID_SOCKET) {
		sockets++;
		if(set_socket_options(&scfg, sock, error))
			lprintf("%04d !ERROR %s",sock, error);

#if 0 /*def _DEBUG */
		lprintf("%04d Socket opened (%d sockets in use)",sock,sockets);
#endif
	}
	return(sock);
}

static int close_socket(SOCKET sock)
{
	int		result;

	if(sock==INVALID_SOCKET)
		return(-1);

	shutdown(sock,SHUT_RDWR);	/* required on Unix */
	result=closesocket(sock);
	if(result==0 && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(startup->cbdata,FALSE);
	sockets--;
	if(result!=0)
		lprintf("%04d !ERROR %d closing socket",sock, ERROR_VALUE);
#if 0 /*def _DEBUG */
	else 
		lprintf("%04d Socket closed (%d sockets in use)",sock,sockets);
#endif

	return(result);
}

static void status(char* str)
{
	if(startup!=NULL && startup->status!=NULL)
	    startup->status(startup->cbdata,str);
}

static time_t checktime(void)
{
	struct tm tm;

    memset(&tm,0,sizeof(tm));
    tm.tm_year=94;
    tm.tm_mday=1;
    return(mktime(&tm)-0x2D24BD00L);
}

/* Global JavaScript Methods */

static JSBool
js_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		buf;
	int32		len=512;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&len);
	
	if((buf=malloc(len))==NULL)
		return(JS_TRUE);

	len=recv(client->socket,buf,len,0);

	if(len>0)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx,buf,len));

	free(buf);

	return(JS_TRUE);
}

static JSBool
js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		buf;
	int32		len=512;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&len);
	
	if((buf=malloc(len))==NULL)
		return(JS_TRUE);

	len=recv(client->socket,buf,len,0);	/* Need to switch to sockreadline */

	if(len>0)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx,buf,len));

	free(buf);

	return(JS_TRUE);
}

static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	uintN		i;
	char*		cp;
	JSString*	str;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	*rval = argv[0];

	for(i=0; i<argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
			continue;
		if((cp=JS_GetStringBytes(str))==NULL)
			continue;
		sendsocket(client->socket,cp,strlen(cp));
	}

	return(JS_TRUE);
}

static JSBool
js_writeln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);
	
	js_write(cx,obj,argc,argv,rval);

	cp="\r\n";
	sendsocket(client->socket,cp,2);

	return(JS_TRUE);
}

static JSBool
js_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		str[512];
    uintN		i;
    JSString*	js_str;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    if(startup==NULL || startup->lputs==NULL)
        return(JS_FALSE);

	str[0]=0;
    for(i=0;i<argc && strlen(str)<(sizeof(str)/2);i++) {
		if((js_str=JS_ValueToString(cx, argv[i]))==NULL)
		    return(JS_FALSE);
		strncat(str,JS_GetStringBytes(js_str),sizeof(str)/2);
		strcat(str," ");
	}

	if(service==NULL)
		lprintf("%04d %s",client->socket,str);
	else
		lprintf("%04d %s %s",client->socket,client->service->protocol,str);

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, str));

    return(JS_TRUE);
}

static JSBool
js_login(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	JSBool		inc_logons=JS_FALSE;
	user_t		user;
	jsval		val;
	JSString*	js_str;
	service_client_t* client;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	/* User name */
	if((js_str=JS_ValueToString(cx, argv[0]))==NULL) 
		return(JS_FALSE);

	if((p=JS_GetStringBytes(js_str))==NULL) 
		return(JS_FALSE);

	if(isdigit(*p))
		user.number=atoi(p);
	else
		user.number=matchuser(&scfg,p,FALSE);

	if(getuserdat(&scfg,&user)!=0) {
		lprintf("%04d %s !USER NOT FOUND: '%s'"
			,client->socket,client->service->protocol,p);
		return(JS_TRUE);
	}

	if(user.misc&(DELETED|INACTIVE)) {
		lprintf("%04d %s !DELETED OR INACTIVE USER #%d: %s"
			,client->socket,client->service->protocol,user.number,p);
		return(JS_TRUE);
	}

	/* Password */
	if(user.pass[0]) {
		if((js_str=JS_ValueToString(cx, argv[1]))==NULL) 
			return(JS_FALSE);

		if((p=JS_GetStringBytes(js_str))==NULL) 
			return(JS_FALSE);

		if(stricmp(user.pass,p)) { /* Wrong password */
			lprintf("%04d %s !INVALID PASSWORD ATTEMPT FOR USER: %s"
				,client->socket,client->service->protocol,user.alias);
			return(JS_TRUE);
		}
	}

	if(argc>2)
		JS_ValueToBoolean(cx,argv[2],&inc_logons);

	if(client->client!=NULL) {
		SAFECOPY(user.note,client->client->addr);
		SAFECOPY(user.comp,client->client->host);
		SAFECOPY(user.modem,client->service->protocol);
	}

	if(inc_logons) {
		user.logons++;
		user.ltoday++;
	}	

	putuserdat(&scfg,&user);

	/* user-specific objects */
	if(!js_CreateUserObjects(cx, obj, &scfg, &user, NULL, NULL)) 
		lprintf("%04d %s !JavaScript ERROR creating user objects"
			,client->socket,client->service->protocol);

	memcpy(&client->user,&user,sizeof(user));

	if(client->client!=NULL) {
		client->client->user=client->user.alias;
		client_on(client->socket,client->client,TRUE /* update */);
	}

	client->logintime=time(NULL);

	lprintf("%04d %s Logging in %s"
		,client->socket,client->service->protocol,client->user.alias);

	val = BOOLEAN_TO_JSVAL(JS_TRUE);
	JS_SetProperty(cx, obj, "logged_in", &val);

	*rval=BOOLEAN_TO_JSVAL(JS_TRUE);

	return(JS_TRUE);
}

static JSBool
js_logout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval val;
	service_client_t* client;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(client->user.number<1)	/* Not logged in */
		return(JS_TRUE);

	logoutuserdat(&scfg,&client->user,time(NULL),client->logintime);

	lprintf("%04d %s Logging out %s"
		,client->socket,client->service->protocol,client->user.alias);

	memset(&client->user,0,sizeof(client->user));

	val = BOOLEAN_TO_JSVAL(JS_FALSE);
	JS_SetProperty(cx, obj, "logged_in", &val);

	*rval=BOOLEAN_TO_JSVAL(JS_TRUE);

	return(JS_TRUE);
}

static JSFunctionSpec js_global_functions[] = {
	{"read",			js_read,			0},		/* read from client socket */
	{"readln",			js_readln,			0},		/* read line from client socket */
	{"write",			js_write,			0},		/* write to client socket */
	{"writeln",			js_writeln,			0},		/* write line to client socket */
	{"print",			js_writeln,			0},		/* write line to client socket */
	{"log",				js_log,				0},		/* Log a string */
 	{"login",			js_login,			2},		/* Login specified username and password */
	{"logout",			js_logout,			0},		/* Logout user */
    {0}
};

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	char*	prot="???";
	SOCKET	sock=0;
	char*	warning;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))!=NULL) {
		prot=client->service->protocol;
		sock=client->socket;
	}

	if(report==NULL) {
		lprintf("%04d %s !JavaScript: %s", sock,prot,message);
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

	lprintf("%04d %s !JavaScript %s%s%s: %s",sock,prot,warning,file,line,message);
}

/* Server Object Properites */
enum {
	 SERVER_PROP_TERMINATED
	,SERVER_PROP_CLIENTS
};


static JSBool js_server_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case SERVER_PROP_TERMINATED:
#if 0
			lprintf("%s client->service->terminated=%d"
				,client->service->protocol, client->service->terminated);
#endif
			*vp = BOOLEAN_TO_JSVAL(client->service->terminated);
			break;
		case SERVER_PROP_CLIENTS:
			*vp = INT_TO_JSVAL(active_clients);
			break;
	}

	return(JS_TRUE);
}

static JSBool js_server_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case SERVER_PROP_CLIENTS:
			JS_ValueToInt32(cx,*vp,(int32*)&active_clients);
			update_clients();
			break;
	}

	return(JS_TRUE);
}

#define SERVER_PROP_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY

static struct JSPropertySpec js_server_properties[] = {
/*		 name				,tinyid					,flags,				getter,	setter	*/

	{	"terminated"		,SERVER_PROP_TERMINATED	,SERVER_PROP_FLAGS,	NULL,NULL},
	{	"clients"			,SERVER_PROP_CLIENTS	,JSPROP_ENUMERATE,	NULL,NULL},
	{0}
};

static JSClass js_server_class = {
         "Server"			/* name			*/
		,0					/* flags		*/
        ,JS_PropertyStub	/* addProperty	*/
		,JS_PropertyStub	/* delProperty	*/
		,js_server_get		/* getProperty	*/
		,js_server_set		/* setProperty	*/
		,JS_EnumerateStub	/* enumerate	*/
		,JS_ResolveStub		/* resolve		*/
		,JS_ConvertStub		/* convert		*/
		,JS_FinalizeStub	/* finalize		*/
}; 

/* Server Methods */

static JSBool
js_client_add(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	client_t	client;
	void*		vp;
	SOCKET		sock=INVALID_SOCKET;
	socklen_t	addr_len;
	SOCKADDR_IN	addr;
	service_client_t* service_client;

	active_clients++, update_clients();

	if((service_client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	memset(&client,0,sizeof(client));
	client.size=sizeof(client);
	client.protocol=service_client->service->protocol;
	client.time=time(NULL);
	client.user="<unknown>";
	SAFECOPY(client.host,client.user);

	if(!JSVAL_IS_OBJECT(argv[0])) /* no socket supplied? */
		return(JS_TRUE);

	if((vp=JS_GetPrivate(cx,JSVAL_TO_OBJECT(argv[0])))!=NULL)
		sock=*(SOCKET*)vp;

	addr_len = sizeof(addr);
	if(getpeername(sock, (struct sockaddr *)&addr, &addr_len)==0) {
		SAFECOPY(client.addr,inet_ntoa(addr.sin_addr));
		client.port=ntohs(addr.sin_port);
	}

	if(argc>1)
		client.user=JS_GetStringBytes(JS_ValueToString(cx,argv[1]));

	if(argc>2)
		SAFECOPY(client.host,JS_GetStringBytes(JS_ValueToString(cx,argv[2])));

	client_on(sock, &client, /* update? */ FALSE);

	lprintf("client_add(%04u,%s,%s)",sock,client.user,client.host);
	return(JS_TRUE);
}

static JSBool
js_client_update(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	client_t	client;
	void*		vp;
	SOCKET		sock=INVALID_SOCKET;
	socklen_t	addr_len;
	SOCKADDR_IN	addr;
	service_client_t* service_client;

	if((service_client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	memset(&client,0,sizeof(client));
	client.size=sizeof(client);
	client.protocol=service_client->service->protocol;
	client.user="<unknown>";
	SAFECOPY(client.host,client.user);

	if(!JSVAL_IS_OBJECT(argv[0])) /* no socket supplied? */
		return(JS_TRUE);

	if((vp=JS_GetPrivate(cx,JSVAL_TO_OBJECT(argv[0])))!=NULL)
		sock=*(SOCKET*)vp;

	addr_len = sizeof(addr);
	if(getpeername(sock, (struct sockaddr *)&addr, &addr_len)==0) {
		SAFECOPY(client.addr,inet_ntoa(addr.sin_addr));
		client.port=ntohs(addr.sin_port);
	}

	if(argc>1)
		client.user=JS_GetStringBytes(JS_ValueToString(cx,argv[1]));

	if(argc>2)
		SAFECOPY(client.host,JS_GetStringBytes(JS_ValueToString(cx,argv[2])));

	client_on(sock, &client, /* update? */ TRUE);

	lprintf("client_update(%04u,%s,%s)",sock,client.user,client.host);
	return(JS_TRUE);
}


static JSBool
js_client_remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	void* vp;

	if(active_clients)
		active_clients--, update_clients();

	if(!JSVAL_IS_OBJECT(argv[0])) /* no socket supplied? */
		return(JS_TRUE);

	if((vp=JS_GetPrivate(cx,JSVAL_TO_OBJECT(argv[0])))!=NULL)
		client_off(*(SOCKET*)vp);
	
	lprintf("client_remove(%04u)",*(SOCKET*)vp);

	return(JS_TRUE);
}

static JSContext* 
js_initcx(JSRuntime* js_runtime, SOCKET sock, service_client_t* service_client, JSObject** glob)
{
	char		ver[256];
	JSContext*	js_cx;
	JSObject*	js_glob;
	JSObject*	server;
	JSString*	js_str;
	jsval		val;
	BOOL		success=FALSE;

    if((js_cx = JS_NewContext(js_runtime, service_client->service->js_cx_stack))==NULL)
		return(NULL);

    JS_SetErrorReporter(js_cx, js_ErrorReporter);

	do {

		JS_SetContextPrivate(js_cx, service_client);

		if((js_glob=js_CreateGlobalObject(js_cx, &scfg, NULL))==NULL) 
			break;

		if (!JS_DefineFunctions(js_cx, js_glob, js_global_functions))
			break;

		/* Internal JS Object */
		if(js_CreateInternalJsObject(js_cx, js_glob, &service_client->branch)==NULL)
			break;

		/* Client Object */
		if(service_client->client!=NULL)
			if(js_CreateClientObject(js_cx, js_glob, "client", service_client->client, sock)==NULL)
				break;

		/* User Class */
		if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
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

		/* user-specific objects */
		if(!js_CreateUserObjects(js_cx, js_glob, &scfg, NULL, NULL, NULL)) 
			break;

		if(js_CreateSystemObject(js_cx, js_glob, &scfg, uptime, startup->host_name)==NULL) 
			break;
		
		/* server object */
		if((server=JS_DefineObject(js_cx, js_glob, "server", &js_server_class
			,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
			break;

		if(!JS_DefineProperties(js_cx, server, js_server_properties))
			break;

		if(service_client->client==NULL)	/* static service */
			if(js_CreateSocketObject(js_cx, server, "socket", service_client->socket)==NULL)
				break;

		JS_DefineFunction(js_cx, server, "client_add"	, js_client_add,	1, 0);
		JS_DefineFunction(js_cx, server, "client_update", js_client_update,	1, 0);
		JS_DefineFunction(js_cx, server, "client_remove", js_client_remove, 1, 0);

		sprintf(ver,"Synchronet Services %s",revision);
		if((js_str=JS_NewStringCopyZ(js_cx, ver))==NULL)
			break;
		val = STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(js_cx, server, "version", &val))
			break;

		if((js_str=JS_NewStringCopyZ(js_cx, services_ver()))==NULL)
			break;
		val = STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(js_cx, server, "version_detail", &val))
			break;

		if(glob!=NULL)
			*glob=js_glob;

		success=TRUE;

	} while(0);


	if(!success) {
		JS_DestroyContext(js_cx);
		return(NULL);
	}

	return(js_cx);
}

static JSBool
js_BranchCallback(JSContext *cx, JSScript *script)
{
	service_client_t* client;

	if((client=(service_client_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	client->branch.counter++;

	/* Terminated? */ 
	if(terminated && !(client->service->options&SERVICE_OPT_STATIC)) {
		JS_ReportError(cx,"Terminated");
		client->branch.counter=0;
		return(JS_FALSE);
	}
	/* Infinite loop? */
	if(client->branch.limit && client->branch.counter > client->branch.limit) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",client->branch.counter);
		client->branch.counter=0;
		return(JS_FALSE);
	}
	/* Give up timeslices every once in a while */
	if(client->branch.yield_interval && (client->branch.counter%client->branch.yield_interval)==0)
		YIELD();

	if(client->branch.gc_interval && (client->branch.counter%client->branch.gc_interval)==0)
		JS_MaybeGC(cx), client->branch.gc_attempts++;

    return(JS_TRUE);
}

static void js_init_args(JSContext* js_cx, JSObject* js_obj, const char* cmdline)
{
	char					argbuf[MAX_PATH+1];
	char*					p;
	char*					args;
	int						argc=0;
	JSString*				arg_str;
	JSObject*				argv;
	jsval					val;

	argv=JS_NewArrayObject(js_cx, 0, NULL);
	JS_DefineProperty(js_cx, js_obj, "argv", OBJECT_TO_JSVAL(argv)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	p=(char*)cmdline;
	while(*p && *p>' ') p++;	/* find end of filename */
	while(*p && *p<=' ') p++;	/* find first arg */
	SAFECOPY(argbuf,p);

	args=argbuf;
	while(*args && argv!=NULL) {
		p=strchr(args,' ');
		if(p!=NULL)
			*p=0;
		while(*args && *args<=' ') args++; /* Skip spaces */
		arg_str = JS_NewStringCopyZ(js_cx, args);
		if(arg_str==NULL)
			break;
		val=STRING_TO_JSVAL(arg_str);
		if(!JS_SetElement(js_cx, argv, argc, &val))
			break;
		argc++;
		if(p==NULL)	/* last arg */
			break;
		args+=(strlen(args)+1);
	}
	JS_DefineProperty(js_cx, js_obj, "argc", INT_TO_JSVAL(argc)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);
}

static void js_service_thread(void* arg)
{
	int						i;
	char*					host_name;
	HOSTENT*				host;
	SOCKET					socket;
	client_t				client;
	service_t*				service;
	service_client_t		service_client;
	/* JavaScript-specific */
	char					spath[MAX_PATH+1];
	char					fname[MAX_PATH+1];
	JSString*				datagram;
	JSObject*				js_glob;
	JSScript*				js_script;
	JSRuntime*				js_runtime;
	JSContext*				js_cx;
	jsval					val;
	jsval					rval;

	// Copy service_client arg
	service_client=*(service_client_t*)arg;
	// Free original
	free(arg);

	socket=service_client.socket;
	service=service_client.service;

	lprintf("%04d %s JavaScript service thread started", socket, service->protocol);

	thread_up(TRUE /* setuid */);

	/* Host name lookup and filtering */
	if(service->options&BBS_OPT_NO_HOST_LOOKUP 
		|| startup->options&BBS_OPT_NO_HOST_LOOKUP)
		host=NULL;
	else
		host=gethostbyaddr((char *)&service_client.addr.sin_addr
			,sizeof(service_client.addr.sin_addr),AF_INET);

	if(host!=NULL && host->h_name!=NULL)
		host_name=host->h_name;
	else
		host_name="<no name>";

	if(!(service->options&BBS_OPT_NO_HOST_LOOKUP)
		&& !(startup->options&BBS_OPT_NO_HOST_LOOKUP)) {
		lprintf("%04d %s Hostname: %s"
			,socket, service->protocol, host_name);
		for(i=0;host!=NULL && host->h_aliases!=NULL 
			&& host->h_aliases[i]!=NULL;i++)
			lprintf("%04d %s HostAlias: %s"
				,socket, service->protocol, host->h_aliases[i]);
	}

	if(trashcan(&scfg,host_name,"host")) {
		lprintf("%04d !%s CLIENT BLOCKED in host.can: %s"
			,socket, service->protocol, host_name);
		close_socket(socket);
		if(service->clients)
			service->clients--;
		thread_down();
		return;
	}


#if 0	/* Need to export from SBBS.DLL */
	identity=NULL;
	if(service->options&BBS_OPT_GET_IDENT 
		&& startup->options&BBS_OPT_GET_IDENT) {
		identify(&service_client.addr, service->port, str, sizeof(str)-1);
		identity=strrchr(str,':');
		if(identity!=NULL) {
			identity++;	/* skip colon */
			while(*identity && *identity<=SP) /* point to user name */
				identity++;
			lprintf("%04d Identity: %s",socket, identity);
		}
	}
#endif

	client.size=sizeof(client);
	client.time=time(NULL);
	SAFECOPY(client.addr,inet_ntoa(service_client.addr.sin_addr));
	SAFECOPY(client.host,host_name);
	client.port=ntohs(service_client.addr.sin_port);
	client.protocol=service->protocol;
	client.user="<unknown>";
	service_client.client=&client;

	active_clients++, update_clients();

	/* Initialize client display */
	client_on(socket,&client,FALSE /* update */);

	if((js_runtime=JS_NewRuntime(service->js_max_bytes))==NULL
		|| (js_cx=js_initcx(js_runtime,socket,&service_client,&js_glob))==NULL) {
		lprintf("%04d !%s ERROR initializing JavaScript context"
			,socket,service->protocol);
		client_off(socket);
		close_socket(socket);
		if(active_clients)
			active_clients--, update_clients();
		if(service->clients)
			service->clients--;
		thread_down();
		return;
	}

	/* RUN SCRIPT */
	SAFECOPY(fname,service->cmd);
	truncstr(fname," ");
	sprintf(spath,"%s%s",scfg.mods_dir,fname);
	if(scfg.mods_dir[0]==0 || !fexist(spath))
		sprintf(spath,"%s%s",scfg.exec_dir,fname);

	js_init_args(js_cx, js_glob, service->cmd);

	val = BOOLEAN_TO_JSVAL(JS_FALSE);
	JS_SetProperty(js_cx, js_glob, "logged_in", &val);

	if(service->options&SERVICE_OPT_UDP 
		&& service_client.udp_buf != NULL
		&& service_client.udp_len > 0) {
		datagram = JS_NewStringCopyN(js_cx, service_client.udp_buf, service_client.udp_len);
		if(datagram==NULL)
			val=JSVAL_VOID;
		else
			val = STRING_TO_JSVAL(datagram);
	} else
		val = JSVAL_VOID;
	JS_SetProperty(js_cx, js_glob, "datagram", &val);
	FREE_AND_NULL(service_client.udp_buf);

	JS_ClearPendingException(js_cx);

	js_script=JS_CompileFile(js_cx, js_glob, spath);

	if(js_script==NULL) 
		lprintf("%04d !JavaScript FAILED to compile script (%s)",socket,spath);
	else  {
		JS_SetBranchCallback(js_cx, js_BranchCallback);
		JS_ExecuteScript(js_cx, js_glob, js_script, &rval);
		JS_DestroyScript(js_cx, js_script);
	}
	JS_DestroyContext(js_cx);	/* Free Context */

	JS_DestroyRuntime(js_runtime);

	if(service_client.user.number) {
		lprintf("%04d %s Logging out %s"
			,socket, service->protocol, service_client.user.alias);
		logoutuserdat(&scfg,&service_client.user,time(NULL),service_client.logintime);
	}

	if(service->clients)
		service->clients--;
	if(active_clients)
		active_clients--, update_clients();

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&BBS_OPT_MUTE)
		&& !(service->options&BBS_OPT_MUTE))
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	thread_down();
	lprintf("%04d %s JavaScript service thread terminated (%u clients remain, %d total, %lu served)"
		, socket, service->protocol, service->clients, active_clients, service->served);

	client_off(socket);
	close_socket(socket);
}

static void js_static_service_thread(void* arg)
{
	char					spath[MAX_PATH+1];
	char					fname[MAX_PATH+1];
	service_t*				service;
	service_client_t		service_client;
	SOCKET					socket;
	/* JavaScript-specific */
	JSObject*				js_glob;
	JSScript*				js_script;
	JSRuntime*				js_runtime;
	JSContext*				js_cx;
	jsval					val;
	jsval					rval;

	// Copy service_client arg
	service=(service_t*)arg;

	service->running=TRUE;
	socket = service->socket;

	lprintf("%04d %s static JavaScript service thread started", service->socket, service->protocol);

	thread_up(TRUE /* setuid */);

	memset(&service_client,0,sizeof(service_client));
	service_client.socket = service->socket;
	service_client.service = service;
	service_client.branch.limit = service->js_branch_limit;
	service_client.branch.gc_interval = service->js_gc_interval;
	service_client.branch.yield_interval = service->js_yield_interval;
	service_client.branch.terminated = &service->terminated;

	if((js_runtime=JS_NewRuntime(service->js_max_bytes))==NULL) {
		lprintf("%04d !%s ERROR initializing JavaScript runtime"
			,service->socket,service->protocol);
		close_socket(service->socket);
		service->socket=INVALID_SOCKET;
		thread_down();
		return;
	}

	SAFECOPY(fname,service->cmd);
	truncstr(fname," ");
	sprintf(spath,"%s%s",scfg.mods_dir,fname);
	if(scfg.mods_dir[0]==0 || !fexist(spath))
		sprintf(spath,"%s%s",scfg.exec_dir,fname);

	do {
		if((js_cx=js_initcx(js_runtime,service->socket,&service_client,&js_glob))==NULL) {
			lprintf("%04d !%s ERROR initializing JavaScript context"
				,service->socket,service->protocol);
			break;
		}

		js_init_args(js_cx, js_glob, service->cmd);

		val = BOOLEAN_TO_JSVAL(JS_FALSE);
		JS_SetProperty(js_cx, js_glob, "logged_in", &val);

		JS_SetBranchCallback(js_cx, js_BranchCallback);
	
		if((js_script=JS_CompileFile(js_cx, js_glob, spath))==NULL)  {
			lprintf("%04d !JavaScript FAILED to compile script (%s)",service->socket,spath);
			break;
		}

		JS_ExecuteScript(js_cx, js_glob, js_script, &rval);
		JS_DestroyScript(js_cx, js_script);

		JS_DestroyContext(js_cx);	/* Free Context */
		js_cx=NULL;
	} while(!service->terminated && service->options&SERVICE_OPT_STATIC_LOOP);

	if(js_cx!=NULL)
		JS_DestroyContext(js_cx);	/* Free Context */

	JS_DestroyRuntime(js_runtime);

	thread_down();
	lprintf("%04d %s static JavaScript service thread terminated (%lu clients served)"
		,socket, service->protocol, service->served);

	close_socket(service->socket);
	service->socket=INVALID_SOCKET;

	service->running=FALSE;
}

static void native_static_service_thread(void* arg)
{
	char					cmd[MAX_PATH];
	char					fullcmd[MAX_PATH*2];
	SOCKET					socket;
	SOCKET					socket_dup;
	service_t*				service;

	service = (service_t*)arg;

	service->running=TRUE;
	socket = service->socket;

	lprintf("%04d %s static service thread started", socket, service->protocol);

	thread_up(TRUE /* setuid */);

#ifdef _WIN32
	if(!DuplicateHandle(GetCurrentProcess(),
		(HANDLE)socket,
		GetCurrentProcess(),
		(HANDLE*)&socket_dup,
		0,
		TRUE, // Inheritable
		DUPLICATE_SAME_ACCESS)) {
		lprintf("%04d !%s ERROR %d duplicating socket descriptor"
			,socket,service->protocol,GetLastError());
		close_socket(service->socket);
		service->socket=INVALID_SOCKET;
		thread_down();
		return;
	}
#else
	socket_dup = dup(service->socket);
#endif

	/* RUN SCRIPT */
	if(strpbrk(service->cmd,"/\\")==NULL)
		sprintf(cmd,"%s%s",scfg.exec_dir,service->cmd);
	else
		strcpy(cmd,service->cmd);
	sprintf(fullcmd,cmd,socket_dup);
	
	do {
		system(fullcmd);
	} while(!service->terminated && service->options&SERVICE_OPT_STATIC_LOOP);

	thread_down();
	lprintf("%04d %s static service thread terminated (%lu clients served)"
		,socket, service->protocol, service->served);

	close_socket(service->socket);
	service->socket=INVALID_SOCKET;
	closesocket(socket_dup);	/* close duplicate handle */

	service->running=FALSE;
}

static void native_service_thread(void* arg)
{
	int						i;
	char					cmd[MAX_PATH];
	char					fullcmd[MAX_PATH*2];
	char*					host_name;
	HOSTENT*				host;
	SOCKET					socket;
	SOCKET					socket_dup;
	client_t				client;
	service_t*				service;
	service_client_t		service_client=*(service_client_t*)arg;

	free(arg);

	socket=service_client.socket;
	service=service_client.service;

	lprintf("%04d %s service thread started", socket, service->protocol);

	thread_up(TRUE /* setuid */);

	/* Host name lookup and filtering */
	if(service->options&BBS_OPT_NO_HOST_LOOKUP 
		|| startup->options&BBS_OPT_NO_HOST_LOOKUP)
		host=NULL;
	else
		host=gethostbyaddr((char *)&service_client.addr.sin_addr
			,sizeof(service_client.addr.sin_addr),AF_INET);

	if(host!=NULL && host->h_name!=NULL)
		host_name=host->h_name;
	else
		host_name="<no name>";

	if(!(service->options&BBS_OPT_NO_HOST_LOOKUP)
		&& !(startup->options&BBS_OPT_NO_HOST_LOOKUP)) {
		lprintf("%04d %s Hostname: %s"
			,socket, service->protocol, host_name);
		for(i=0;host!=NULL && host->h_aliases!=NULL 
			&& host->h_aliases[i]!=NULL;i++)
			lprintf("%04d %s HostAlias: %s"
				,socket, service->protocol, host->h_aliases[i]);
	}

	if(trashcan(&scfg,host_name,"host")) {
		lprintf("%04d !%s CLIENT BLOCKED in host.can: %s"
			,socket, service->protocol, host_name);
		close_socket(socket);
		if(service->clients)
			service->clients--;
		thread_down();
		return;
	}


#if 0	/* Need to export from SBBS.DLL */
	identity=NULL;
	if(service->options&BBS_OPT_GET_IDENT 
		&& startup->options&BBS_OPT_GET_IDENT) {
		identify(&service_client.addr, service->port, str, sizeof(str)-1);
		identity=strrchr(str,':');
		if(identity!=NULL) {
			identity++;	/* skip colon */
			while(*identity && *identity<=SP) /* point to user name */
				identity++;
			lprintf("%04d Identity: %s",socket, identity);
		}
	}
#endif

	client.size=sizeof(client);
	client.time=time(NULL);
	SAFECOPY(client.addr,inet_ntoa(service_client.addr.sin_addr));
	SAFECOPY(client.host,host_name);
	client.port=ntohs(service_client.addr.sin_port);
	client.protocol=service->protocol;
	client.user="<unknown>";

#ifdef _WIN32
	if(!DuplicateHandle(GetCurrentProcess(),
		(HANDLE)socket,
		GetCurrentProcess(),
		(HANDLE*)&socket_dup,
		0,
		TRUE, // Inheritable
		DUPLICATE_SAME_ACCESS)) {
		lprintf("%04d !%s ERROR %d duplicating socket descriptor"
			,socket,service->protocol,GetLastError());
		close_socket(socket);
		thread_down();
		return;
	}
#else
	socket_dup = dup(socket);
#endif

	active_clients++, update_clients();

	/* Initialize client display */
	client_on(socket,&client,FALSE /* update */);

	/* RUN SCRIPT */
	if(strpbrk(service->cmd,"/\\")==NULL)
		sprintf(cmd,"%s%s",scfg.exec_dir,service->cmd);
	else
		strcpy(cmd,service->cmd);
	sprintf(fullcmd,cmd,socket_dup);

	system(fullcmd);

	if(service->clients)
		service->clients--;
	if(active_clients)
		active_clients--, update_clients();

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&BBS_OPT_MUTE)
		&& !(service->options&BBS_OPT_MUTE))
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	thread_down();
	lprintf("%04d %s service thread terminated (%u clients remain, %d total, %lu served)"
		,socket, service->protocol, service->clients, active_clients, service->served);

	client_off(socket);
	close_socket(socket);
	closesocket(socket_dup);	/* close duplicate handle */
}


void DLLCALL services_terminate(void)
{
	DWORD i;

	terminated=TRUE;
	for(i=0;i<services;i++) {
		if(service[i].socket==INVALID_SOCKET)
			continue;
		if(service[i].options&SERVICE_OPT_STATIC)
			continue;
		close_socket(service[i].socket);
		service[i].socket=INVALID_SOCKET;
	}
}

#define NEXT_FIELD(p)	FIND_WHITESPACE(p); SKIP_WHITESPACE(p)

static service_t* read_services_cfg(service_t* service, char* services_cfg, DWORD* services)
{
	char*	p;
	char*	tp;
	char	line[1024];
	FILE*	fp;
	service_t*	np;
	service_t*	serv;
	
	if((fp=fopen(services_cfg,"r"))==NULL)
		return(service);

	lprintf("Reading %s",services_cfg);
	for((*services)=0;!feof(fp) && (*services)<MAX_SERVICES;) {
		if(!fgets(line,sizeof(line),fp))
			break;
		p=line;
		SKIP_WHITESPACE(p);
		if(*p==0 || *p==';')	/* ignore blank lines or comments */
			continue;
		
		if((np=(service_t*)realloc(service,sizeof(service_t)*((*services)+1)))==NULL) {
			lprintf("!MALLOC FAILURE");
			return(service);
		}
		service=np;
		serv=&service[*services];
		memset(serv,0,sizeof(service_t));
		serv->socket=INVALID_SOCKET;
		serv->listen_backlog=DEFAULT_LISTEN_BACKLOG;

		/* These are not configurable per service when using services.cfg */
		serv->js_max_bytes=startup->js_max_bytes;
		serv->js_cx_stack=startup->js_cx_stack;
		serv->js_branch_limit=startup->js_branch_limit;
		serv->js_gc_interval=startup->js_gc_interval;
		serv->js_yield_interval=startup->js_yield_interval;

		tp=p; 
		FIND_WHITESPACE(tp);
		*tp=0;
		SAFECOPY(serv->protocol,p);
		p=tp+1;
		SKIP_WHITESPACE(p);
		serv->port=atoi(p);
		NEXT_FIELD(p);
		serv->max_clients=strtol(p,NULL,10);
		NEXT_FIELD(p);
		serv->options=strtol(p,NULL,16);
		NEXT_FIELD(p);

		SAFECOPY(serv->cmd,p);
		truncsp(serv->cmd);

		(*services)++;
	}
	fclose(fp);

	return(service);
}

static service_t* read_services_ini(service_t* service, char* services_ini, DWORD* services)
{
	uint		i,j;
	FILE*		fp;
	char		cmd[INI_MAX_VALUE_LEN];
	char		host[INI_MAX_VALUE_LEN];
	char**		sec_list;
	service_t*	np;
	service_t	serv;

	if((fp=fopen(services_ini,"r"))==NULL)
		return(service);

	lprintf("Reading %s",services_ini);
	sec_list = iniGetSectionList(fp,"");
    for(i=0; sec_list!=NULL && sec_list[i]!=NULL; i++) {
		memset(&serv,0,sizeof(service_t));
		SAFECOPY(serv.protocol,sec_list[i]);
		serv.socket=INVALID_SOCKET;
		serv.port=iniGetShortInt(fp,sec_list[i],"Port",0);
		serv.max_clients=iniGetInteger(fp,sec_list[i],"MaxClients",0);
		serv.listen_backlog=iniGetInteger(fp,sec_list[i],"ListenBacklog",DEFAULT_LISTEN_BACKLOG);
		serv.options=iniGetBitField(fp,sec_list[i],"Options",service_options,0);
		SAFECOPY(serv.cmd,iniGetString(fp,sec_list[i],"Command","",cmd));

		/* JavaScript operating parameters */
		serv.js_max_bytes=iniGetInteger(fp,sec_list[i]		,strJavaScriptMaxBytes		,startup->js_max_bytes);
		serv.js_cx_stack=iniGetInteger(fp,sec_list[i]		,strJavaScriptContextStack	,startup->js_cx_stack);
		serv.js_branch_limit=iniGetInteger(fp,sec_list[i]	,strJavaScriptBranchLimit	,startup->js_branch_limit);
		serv.js_gc_interval=iniGetInteger(fp,sec_list[i]	,strJavaScriptGcInterval	,startup->js_gc_interval);
		serv.js_yield_interval=iniGetInteger(fp,sec_list[i]	,strJavaScriptYieldInterval	,startup->js_yield_interval);

		for(j=0;j<*services;j++)
			if(service[j].port==serv.port && service[j].options==serv.options)
				break;
		if(j<*services)	{ /* ignore duplicate services */
			lprintf("Ignoring duplicate service: %s",sec_list[i]);
			continue;
		}

		if(stricmp(iniGetString(fp,sec_list[i],"Host",startup->host_name,host), startup->host_name)) {
			lprintf("Ignoring service (%s) for host: %s", sec_list[i], host);
			continue;
		}
		if(stricmp(iniGetString(fp,sec_list[i],"NotHost","",host), startup->host_name)==0) {
			lprintf("Ignoring service (%s) for host: %s", sec_list[i], host);
			continue;
		}

		if((np=(service_t*)realloc(service,sizeof(service_t)*((*services)+1)))==NULL) {
			fclose(fp);
			lprintf("!MALLOC FAILURE");
			return(service);
		}
		service=np;
		service[*services]=serv;
		(*services)++;
	}
	iniFreeStringList(sec_list);

	fclose(fp);

	return(service);
}


static void cleanup(int code)
{
	free_cfg(&scfg);

	update_clients();

#ifdef _WINSOCKAPI_	
	if(WSAInitialized && WSACleanup()!=0) 
		lprintf("0000 !WSACleanup ERROR %d",ERROR_VALUE);
#endif

	thread_down();
    lprintf("#### Services thread terminated (%lu clients served)",served);
	status("Down");
	if(startup!=NULL && startup->terminated!=NULL)
		startup->terminated(startup->cbdata,code);
}

const char* DLLCALL services_ver(void)
{
	static char ver[256];
	char compiler[32];

	DESCRIBE_COMPILER(compiler);

	sscanf("$Revision: 1.143 $", "%*s %s", revision);

	sprintf(ver,"Synchronet Services %s%s  "
		"Compiled %s %s with %s"
		,revision
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,__DATE__, __TIME__, compiler
		);

	return(ver);
}

void DLLCALL services_thread(void* arg)
{
	char			path[MAX_PATH+1];
	char			error[256];
	char			host_ip[32];
	char			compiler[32];
	char			str[128];
	SOCKADDR_IN		addr;
	SOCKADDR_IN		client_addr;
	socklen_t		client_addr_len;
	SOCKET			socket;
	SOCKET			client_socket;
	BYTE*			udp_buf = NULL;
	int				udp_len;
	int				i;
	int				result;
	int				optval;
	ulong			total_clients;
	ulong			total_running;
	time_t			t;
	time_t			initialized=0;
	fd_set			socket_set;
	SOCKET			high_socket;
	ulong			total_sockets;
	struct timeval	tv;
	service_client_t* client;

	services_ver();

	startup=(services_startup_t*)arg;

    if(startup==NULL) {
    	sbbs_beep(100,500);
    	fprintf(stderr, "No startup structure passed!\n");
    	return;
    }

	if(startup->size!=sizeof(services_startup_t)) {	/* verify size */
		sbbs_beep(100,500);
		sbbs_beep(300,500);
		sbbs_beep(100,500);
		fprintf(stderr, "Invalid startup structure!\n");
		return;
	}

#ifdef _THREAD_SUID_BROKEN
	startup->seteuid(TRUE);
#endif

	/* Setup intelligent defaults */
	if(startup->sem_chk_freq==0)			startup->sem_chk_freq=5;
	if(startup->js_max_bytes==0)			startup->js_max_bytes=JAVASCRIPT_MAX_BYTES;
	if(startup->js_cx_stack==0)				startup->js_cx_stack=JAVASCRIPT_CONTEXT_STACK;

	uptime=0;
	served=0;
	startup->recycle_now=FALSE;
	do {

		thread_up(FALSE /* setuid */);

		status("Initializing");

		memset(&scfg, 0, sizeof(scfg));

		lprintf("Synchronet Services Revision %s%s"
			,revision
#ifdef _DEBUG
			," Debug"
#else
			,""
#endif
			);

		DESCRIBE_COMPILER(compiler);

		lprintf("Compiled %s %s with %s", __DATE__, __TIME__, compiler);

		srand(time(NULL));	/* Seed random number generator */
		sbbs_random(10);	/* Throw away first number */

		if(!winsock_startup()) {
			cleanup(1);
			return;
		}

		t=time(NULL);
		lprintf("Initializing on %.24s with options: %lx"
			,CTIME_R(&t,str),startup->options);

		/* Initial configuration and load from CNF files */
		SAFECOPY(scfg.ctrl_dir, startup->ctrl_dir);
		lprintf("Loading configuration files from %s", scfg.ctrl_dir);
		scfg.size=sizeof(scfg);
		SAFECOPY(error,UNKNOWN_LOAD_ERROR);
		if(!load_cfg(&scfg, NULL, TRUE, error)) {
			lprintf("!ERROR %s",error);
			lprintf("!Failed to load configuration files");
			cleanup(1);
			return;
		}

		if(startup->host_name[0]==0)
			SAFECOPY(startup->host_name,scfg.sys_inetaddr);

		if(!(scfg.sys_misc&SM_LOCAL_TZ) && !(startup->options&BBS_OPT_LOCAL_TIMEZONE)) {
			if(putenv("TZ=UTC0"))
				lprintf("!putenv() FAILED");
			tzset();

			if((t=checktime())!=0) {   /* Check binary time */
				lprintf("!TIME PROBLEM (%ld)",t);
				cleanup(1);
				return;
			}
		}

		if(uptime==0)
			uptime=time(NULL);	/* this must be done *after* setting the timezone */

		active_clients=0, update_clients();

		if(startup->cfg_file[0]==0)			
			sprintf(startup->cfg_file,"%sservices.cfg",scfg.ctrl_dir);
		service=read_services_cfg(service, startup->cfg_file, &services);

		if(startup->ini_file[0]==0)			
			sprintf(startup->ini_file,"%sservices.ini",scfg.ctrl_dir);
		service=read_services_ini(service, startup->ini_file, &services);

		if(service==NULL) {
			lprintf("!Failure reading configuration file (%s or %s)"
				,startup->cfg_file,startup->ini_file);
			cleanup(1);
			return;
		}


		/* Open and Bind Listening Sockets */
		total_sockets=0;
		for(i=0;i<(int)services;i++) {

			service[i].socket=INVALID_SOCKET;

			if((socket = open_socket(
				(service[i].options&SERVICE_OPT_UDP) ? SOCK_DGRAM : SOCK_STREAM))
				==INVALID_SOCKET) {
				lprintf("!ERROR %d opening %s socket"
					,ERROR_VALUE, service[i].protocol);
				cleanup(1);
				return;
			}

			if(service[i].options&SERVICE_OPT_UDP) {
				/* We need to set the REUSE ADDRESS socket option */
				optval=TRUE;
				if(setsockopt(socket,SOL_SOCKET,SO_REUSEADDR
					,(char*)&optval,sizeof(optval))!=0) {
					lprintf("%04d !ERROR %d setting %s socket option"
						,socket, ERROR_VALUE, service[i].protocol);
					close_socket(socket);
					continue;
				}
			   #ifdef BSD
				if(setsockopt(socket,SOL_SOCKET,SO_REUSEPORT
					,(char*)&optval,sizeof(optval))!=0) {
					lprintf("%04d !ERROR %d setting %s socket option"
						,socket, ERROR_VALUE, service[i].protocol);
					close_socket(socket);
					continue;
				}
			   #endif
			}
			memset(&addr, 0, sizeof(addr));

			addr.sin_addr.s_addr = htonl(startup->interface_addr);
			addr.sin_family = AF_INET;
			addr.sin_port   = htons(service[i].port);

			if(startup->seteuid!=NULL)
				startup->seteuid(FALSE);
			result=bind(socket, (struct sockaddr *) &addr, sizeof(addr));
			if(startup->seteuid!=NULL)
				startup->seteuid(TRUE);
			if(result!=0) {
				lprintf("%04d !ERROR %d binding %s socket to port %u"
					,socket, ERROR_VALUE, service[i].protocol, service[i].port);
				lprintf("%04d %s",socket,BIND_FAILURE_HELP);
				close_socket(socket);
				continue;
			}

			lprintf("%04d %s socket bound to %s port %u"
				,socket, service[i].protocol
				,service[i].options&SERVICE_OPT_UDP ? "UDP" : "TCP"
				,service[i].port);

			if(!(service[i].options&SERVICE_OPT_UDP)) {
				if(listen(socket,service[i].listen_backlog)!=0) {
					lprintf("%04d !ERROR %d listening on %s socket"
						,socket, ERROR_VALUE, service[i].protocol);
					close_socket(socket);
					continue;
				}
			}
			service[i].socket=socket;
			total_sockets++;
		}

		if(!total_sockets) {
			lprintf("0000 !No service sockets bound");
			cleanup(1);
			return;
		}

		/* Setup static service threads */
		for(i=0;i<(int)services;i++) {
			if(!(service[i].options&SERVICE_OPT_STATIC))
				continue;
			if(service[i].socket==INVALID_SOCKET)	/* bind failure? */
				continue;

			/* start thread here */
			if(service[i].options&SERVICE_OPT_NATIVE)	/* Native */
				_beginthread(native_static_service_thread, 0, &service[i]);
			else										/* JavaScript */
				_beginthread(js_static_service_thread, 0, &service[i]);
		}

		status("Listening");

		if(initialized==0) {
			initialized=time(NULL);
			sprintf(path,"%sservices.rec",scfg.ctrl_dir);
			t=fdate(path);
			if(t!=-1 && t>initialized)
				initialized=t;
		}
			
		terminated=FALSE;

		/* signal caller that we've started up successfully */
		if(startup->started!=NULL)
    		startup->started(startup->cbdata);

		/* Main Server Loop */
		while(!terminated) {

			total_clients=0;
			for(i=0;i<(int)services;i++) 
				total_clients+=service[i].clients;

			if(!(startup->options&BBS_OPT_NO_RECYCLE)) {
				sprintf(path,"%sservices.rec",scfg.ctrl_dir);
				t=fdate(path);
				if(!total_clients && t!=-1 && t>initialized) {
					lprintf("0000 Recycle semaphore file (%s) detected",path);
					initialized=t;
					break;
				}
				if(startup->recycle_sem!=NULL && sem_trywait(&startup->recycle_sem)==0)
					startup->recycle_now=TRUE;
				if(!total_clients && startup->recycle_now==TRUE) {
					lprintf("0000 Recycle semaphore signaled");
					startup->recycle_now=FALSE;
					break;
				}
			}

			/* Setup select() parms */
			FD_ZERO(&socket_set);	
			high_socket=0;
			for(i=0;i<(int)services;i++) {
				if(service[i].options&SERVICE_OPT_STATIC)
					continue;
				if(service[i].socket==INVALID_SOCKET)
					continue;
				if(!(service[i].options&SERVICE_OPT_FULL_ACCEPT)
					&& service[i].max_clients && service[i].clients >= service[i].max_clients)
					continue;
				FD_SET(service[i].socket,&socket_set);
				if(service[i].socket>high_socket)
					high_socket=service[i].socket;
			}
			if(high_socket==0) {	/* No dynamic services? */
				YIELD();
				continue;
			}
			tv.tv_sec=startup->sem_chk_freq;
			tv.tv_usec=0;
			if((result=select(high_socket+1,&socket_set,NULL,NULL,&tv))<1) {
				if(result==0)
					continue;

				if(ERROR_VALUE==EINTR)
					lprintf("0000 Services listening interrupted");
				else if(ERROR_VALUE == ENOTSOCK)
            		lprintf("0000 Services sockets closed");
				else
					lprintf("0000 !ERROR %d selecting sockets",ERROR_VALUE);
				break;
			}

			/* Determine who services this socket */
			for(i=0;i<(int)services;i++) {

				if(service[i].socket==INVALID_SOCKET)
					continue;

				if(!FD_ISSET(service[i].socket,&socket_set))
					continue;

				client_addr_len = sizeof(client_addr);

				udp_len=0;

				if(service[i].options&SERVICE_OPT_UDP) {
					/* UDP */
					if((udp_buf = (BYTE*)calloc(1, MAX_UDP_BUF_LEN)) == NULL) {
						lprintf("%04d %s !ERROR %d allocating UDP buffer"
							,service[i].socket, service[i].protocol, errno);
						continue;
					}

					udp_len = recvfrom(service[i].socket
						,udp_buf, MAX_UDP_BUF_LEN, 0 /* flags */
						,(struct sockaddr *)&client_addr, &client_addr_len);
					if(udp_len<1) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d recvfrom failed"
							,service[i].socket, service[i].protocol, ERROR_VALUE);
						continue;
					}

					if((client_socket = open_socket(SOCK_DGRAM))
						==INVALID_SOCKET) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d opening socket"
							,service[i].socket, service[i].protocol, ERROR_VALUE);
						continue;
					}

					lprintf("%04d %s created client socket: %d"
						,service[i].socket, service[i].protocol, client_socket);

					/* We need to set the REUSE ADDRESS socket option */
					optval=TRUE;
					if(setsockopt(client_socket,SOL_SOCKET,SO_REUSEADDR
						,(char*)&optval,sizeof(optval))!=0) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d setting socket option"
							,client_socket, service[i].protocol, ERROR_VALUE);
						close_socket(client_socket);
						continue;
					}
				   #ifdef BSD
					if(setsockopt(client_socket,SOL_SOCKET,SO_REUSEPORT
						,(char*)&optval,sizeof(optval))!=0) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d setting socket option"
							,client_socket, service[i].protocol, ERROR_VALUE);
						close_socket(client_socket);
						continue;
					}
				   #endif

					memset(&addr, 0, sizeof(addr));
					addr.sin_addr.s_addr = htonl(startup->interface_addr);
					addr.sin_family = AF_INET;
					addr.sin_port   = htons(service[i].port);

					result=bind(client_socket, (struct sockaddr *) &addr, sizeof(addr));
					if(result==SOCKET_ERROR) {
						/* Failed to re-bind to same port number, use user port */
						lprintf("%04d %s ERROR %d re-binding socket to port %u failed, "
							"using user port"
							,client_socket, service[i].protocol, ERROR_VALUE, service[i].port);
						addr.sin_port=0;
						result=bind(client_socket, (struct sockaddr *) &addr, sizeof(addr));
					}
					if(result!=0) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d re-binding socket to port %u"
							,client_socket, service[i].protocol, ERROR_VALUE, service[i].port);
						close_socket(client_socket);
						continue;
					}

					/* Set client address as default addres for send/recv */
					if(connect(client_socket
						,(struct sockaddr *)&client_addr, client_addr_len)!=0) {
						FREE_AND_NULL(udp_buf);
						lprintf("%04d %s !ERROR %d connect failed"
							,client_socket, service[i].protocol, ERROR_VALUE);
						close_socket(client_socket);
						continue;
					}

				} else { 
					/* TCP */
					if((client_socket=accept(service[i].socket
						,(struct sockaddr *)&client_addr, &client_addr_len))==INVALID_SOCKET) {
						if(ERROR_VALUE == ENOTSOCK || ERROR_VALUE == EINVAL)
            				lprintf("%04d %s socket closed while listening"
								,service[i].socket, service[i].protocol);
						else
							lprintf("%04d %s !ERROR %d accept failed" 
								,service[i].socket, service[i].protocol, ERROR_VALUE);
						break;
					}
					sockets++;
#if 0 /*def _DEBUG */
					lprintf("%04d Socket opened (%d sockets in use)",client_socket,sockets);
#endif
					if(startup->socket_open!=NULL)	/* Callback, increments socket counter */
						startup->socket_open(startup->cbdata,TRUE);	
				}
				SAFECOPY(host_ip,inet_ntoa(client_addr.sin_addr));

				if(trashcan(&scfg,host_ip,"ip-silent")) {
					FREE_AND_NULL(udp_buf);
					close_socket(client_socket);
					continue;
				}

				lprintf("%04d %s connection accepted from: %s port %u"
					,client_socket
					,service[i].protocol, host_ip, ntohs(client_addr.sin_port));

				if(service[i].max_clients && service[i].clients+1>service[i].max_clients) {
					lprintf("%04d !%s MAXMIMUM CLIENTS (%u) reached, access denied"
						,client_socket, service[i].protocol, service[i].max_clients);
					mswait(3000);
					close_socket(client_socket);
					continue;
				}

#ifdef _WIN32
				if(startup->answer_sound[0] && !(startup->options&BBS_OPT_MUTE)
					&& !(service[i].options&BBS_OPT_MUTE))
					PlaySound(startup->answer_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

				if(trashcan(&scfg,host_ip,"ip")) {
					FREE_AND_NULL(udp_buf);
					lprintf("%04d !%s CLIENT BLOCKED in ip.can: %s"
						,client_socket, service[i].protocol, host_ip);
					mswait(3000);
					close_socket(client_socket);
					continue;
				}

				if((client=malloc(sizeof(service_client_t)))==NULL) {
					FREE_AND_NULL(udp_buf);
					lprintf("%04d !%s ERROR allocating %u bytes of memory for service_client"
						,client_socket, service[i].protocol, sizeof(service_client_t));
					mswait(3000);
					close_socket(client_socket);
					continue;
				}

				memset(client,0,sizeof(service_client_t));
				client->socket=client_socket;
				client->addr=client_addr;
				client->service=&service[i];
				client->service->clients++;		/* this should be mutually exclusive */
				client->udp_buf=udp_buf;
				client->udp_len=udp_len;
				client->branch.limit			= service[i].js_branch_limit;
				client->branch.gc_interval		= service[i].js_gc_interval;
				client->branch.yield_interval	= service[i].js_yield_interval;
				client->branch.terminated		= &client->service->terminated;

				udp_buf = NULL;

				if(service[i].options&SERVICE_OPT_NATIVE)	/* Native */
					_beginthread(native_service_thread, 0, client);
				else										/* JavaScript */
					_beginthread(js_service_thread, 0, client);
				service[i].served++;
				served++;
			}
		}

		/* Close Service Sockets */
		lprintf("0000 Closing service sockets");
		for(i=0;i<(int)services;i++) {
			service[i].terminated=TRUE;
			if(service[i].socket==INVALID_SOCKET)
				continue;
			if(service[i].options&SERVICE_OPT_STATIC)
				continue;
			close_socket(service[i].socket);
			service[i].socket=INVALID_SOCKET;
		}

		/* Wait for Dynamic Service Threads to terminate */
		total_clients=0;
		for(i=0;i<(int)services;i++) 
			total_clients+=service[i].clients;
		if(total_clients) {
			lprintf("0000 Waiting for %d clients to disconnect",total_clients);
			while(1) {
				total_clients=0;
				for(i=0;i<(int)services;i++) 
					total_clients+=service[i].clients;
				if(!total_clients)
					break;
				mswait(500);
			}
			lprintf("0000 Done waiting");
		}

		/* Wait for Static Service Threads to terminate */
		total_running=0;
		for(i=0;i<(int)services;i++) 
			total_running+=service[i].running;
		if(total_running) {
			lprintf("0000 Waiting for %d static services to terminate",total_running);
			while(1) {
				total_running=0;
				for(i=0;i<(int)services;i++) 
					total_running+=service[i].running;
				if(!total_running)
					break;
				mswait(500);
			}
			lprintf("0000 Done waiting");
		}

		/* Free Service Data */
		services=0;
		free(service);
		service=NULL;

		cleanup(0);
		if(!terminated) {
			lprintf("Recycling server...");
			mswait(2000);
		}

	} while(!terminated);
}
