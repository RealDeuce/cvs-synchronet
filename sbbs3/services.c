/* services.c */

/* Synchronet Services */

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

	#include <io.h>			/* open/close */
	#include <share.h>		/* share open flags */
	#include <process.h>	/* _beginthread */
	#include <windows.h>	/* required for mmsystem.h */
	#include <mmsystem.h>	/* SND_ASYNC */

#elif defined(__unix__)

	#include <signal.h>		/* signal/SIGPIPE */

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

/* Constants */
#define SERVICES_VERSION		"1.00"

#define MAX_SERVICES			128
#define TIMEOUT_THREAD_WAIT		60		/* Seconds */

static services_startup_t* startup=NULL;
static scfg_t	scfg;
static int		active_clients=0;
static DWORD	sockets=0;
static BOOL		terminated=FALSE;
static time_t	uptime=0;

typedef struct {
	/* These are sysop-configurable */
	WORD	port;
	char	protocol[34];
	char	cmd[128];
	DWORD	max_clients;
	DWORD	options;
	/* These are run-time state and stat vars */
	DWORD	clients;
	SOCKET	socket;
} service_t;

typedef struct {
	SOCKET			socket;
	SOCKADDR_IN		addr;
	time_t			logintime;
	user_t			user;
	client_t*		client;
	service_t*		service;
	ulong			js_loop;
} service_client_t;

static service_t	*service;
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
    vsprintf(sbuf,fmt,argptr);
    va_end(argptr);
    return(startup->lputs(sbuf));
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
		startup->clients(active_clients);
}

static void client_on(SOCKET sock, client_t* client, BOOL update)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(TRUE,sock,client,update);
}

static void client_off(SOCKET sock)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(FALSE,sock,NULL,FALSE);
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

static SOCKET open_socket(int type)
{
	SOCKET sock;

	sock=socket(AF_INET, type, IPPROTO_IP);
	if(sock!=INVALID_SOCKET && startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(TRUE);
	if(sock!=INVALID_SOCKET) {
		sockets++;
#if 0 /*def _DEBUG */
		lprintf("%04d Socket opened (%d sockets in use)",sock,sockets);
#endif
	}
	return(sock);
}

static int close_socket(SOCKET sock)
{
	int		result;

	shutdown(sock,SHUT_RDWR);	/* required on Unix */
	result=closesocket(sock);
	if(/* result==0 && */ startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(FALSE);
	sockets--;
	if(result!=0) {
		if(ERROR_VALUE!=ENOTSOCK)
			lprintf("%04d !ERROR %d closing socket",sock, ERROR_VALUE);
	}
#if 0 /*def _DEBUG */
	else 
		lprintf("%04d Socket closed (%d sockets in use)",sock,sockets);
#endif

	return(result);
}

static void status(char* str)
{
	if(startup!=NULL && startup->status!=NULL)
	    startup->status(str);
}

static time_t checktime(void)
{
	struct tm tm;

    memset(&tm,0,sizeof(tm));
    tm.tm_year=94;
    tm.tm_mday=1;
    return(mktime(&tm)-0x2D24BD00L);
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

static u_long resolve_ip(char *addr)
{
	HOSTENT*	host;
	char*		p;

	for(p=addr;*p;p++)
		if(*p!='.' && !isdigit(*p))
			break;
	if(!(*p))
		return(inet_addr(addr));

	if ((host=gethostbyname(addr))==NULL) {
		lprintf("0000 !ERROR resolving hostname: %s",addr);
		return(0);
	}
	return(*((ulong*)host->h_addr_list[0]));
}

/* Global JavaScript Methods */
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

	lprintf("%04d %s %s",client->socket,client->service->protocol,str);

	*rval = JSVAL_VOID;
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
		inc_logons=JSVAL_TO_BOOLEAN(argv[2]);

	sprintf(user.note,"%.*s",sizeof(user.note)-1,client->client->addr);
	sprintf(user.comp,"%.*s",sizeof(user.comp)-1,client->client->host);
	sprintf(user.modem,"%.*s",sizeof(user.modem)-1,client->service->protocol);

	if(inc_logons) {
		user.logons++;
		user.ltoday++;
	}	

	putuserdat(&scfg,&user);

	/* user object */
	if(js_CreateUserObject(cx, obj, &scfg, "user", user.number)==NULL) 
		lprintf("%04d %s !JavaScript ERROR creating user object"
			,client->socket,client->service->protocol);

	/* file_area object */
	if(js_CreateFileAreaObject(cx, obj, &scfg, &user, "")==NULL) 
		lprintf("%04d %s !JavaScript ERROR creating file_area object"
			,client->socket,client->service->protocol);

	/* msg_area object */
	if(js_CreateMsgAreaObject(cx, obj, &scfg, &user, NULL)==NULL) 
		lprintf("%04d %s !JavaScript ERROR creating msg_area object"
			,client->socket,client->service->protocol);

	client->client->user=user.alias;
	client_on(client->socket,client->client,TRUE /* update */);

	memcpy(&client->user,&user,sizeof(user));

	client->logintime=time(NULL);

	lprintf("%04d %s Logging in %s"
		,client->socket,client->service->protocol,user.alias);

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
	{"log",				js_log,				1},		/* Log a string */
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

static JSClass js_server_class = {
        "Server",0, 
        JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub, 
        JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub 
}; 

static JSContext* 
js_initcx(JSRuntime* js_runtime, SOCKET sock, service_client_t* service_client, JSObject** glob)
{
	char		ver[256];
	JSContext*	js_cx;
	JSObject*	js_glob;
	JSObject*	server;
	jsval		val;
	BOOL		success=FALSE;

    if((js_cx = JS_NewContext(js_runtime, JAVASCRIPT_CONTEXT_STACK))==NULL)
		return(NULL);

    JS_SetErrorReporter(js_cx, js_ErrorReporter);

	do {

		JS_SetContextPrivate(js_cx, service_client);

		if((js_glob=js_CreateGlobalObject(js_cx, &scfg))==NULL) 
			break;

		if (!JS_DefineFunctions(js_cx, js_glob, js_global_functions))
			break;

		/* Client Object */
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

		if(js_CreateSystemObject(js_cx, js_glob, &scfg, uptime, startup->host_name)==NULL) 
			break;
		
		/* server object */
		if((server=JS_DefineObject(js_cx, js_glob, "server", &js_server_class
			,NULL,0))==NULL)
			break;

		sprintf(ver,"Synchronet Services v%s",SERVICES_VERSION);
		val = STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, ver));
		if(!JS_SetProperty(js_cx, server, "version", &val))
			break;

		val = STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx, services_ver()));
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

	client->js_loop++;

	/* Terminated? */ 
	if(terminated) {
		JS_ReportError(cx,"Terminated");
		client->js_loop=0;
		return(JS_FALSE);
	}
	/* Infinite loop? */
	if(client->js_loop>JAVASCRIPT_BRANCH_LIMIT) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",client->js_loop);
		client->js_loop=0;
		return(JS_FALSE);
	}
	/* Give up timeslices every once in a while */
	if(!(client->js_loop%JAVASCRIPT_YIELD_FREQUENCY))
		mswait(1);

    return(JS_TRUE);
}


static void js_service_thread(void* arg)
{
	char*					p;
	char*					host_name;
	HOSTENT*				host;
	SOCKET					socket;
	client_t				client;
	service_t*				service;
	service_client_t		service_client;
	/* JavaScript-specific */
	char					spath[MAX_PATH];
	char*					args=NULL;
	int						argc=0;
	JSString*				arg_str;
	JSObject*				argv;
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

	thread_up();

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
		&& !(startup->options&BBS_OPT_NO_HOST_LOOKUP))
		lprintf("%04d %s client name: %s", socket, service->protocol, host_name);

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
	sprintf(client.addr,"%.*s",(int)sizeof(client.addr)-1,inet_ntoa(service_client.addr.sin_addr));
	sprintf(client.host,"%.*s",(int)sizeof(client.host)-1,host_name);
	client.port=ntohs(service_client.addr.sin_port);
	client.protocol=service->protocol;
	client.user="<unknown>";
	service_client.client=&client;

	active_clients++;
	update_clients();

	/* Initialize client display */
	client_on(socket,&client,FALSE /* update */);

	if((js_runtime=JS_NewRuntime(startup->js_max_bytes))==NULL
		|| (js_cx=js_initcx(js_runtime,socket,&service_client,&js_glob))==NULL) {
		lprintf("%04d !%s ERROR initializing JavaScript context"
			,socket,service->protocol);
		client_off(socket);
		close_socket(socket);
		active_clients--;
		update_clients();
		if(service->clients)
			service->clients--;
		thread_down();
		return;
	}

	/* RUN SCRIPT */
	sprintf(spath,"%s%s",scfg.exec_dir,service->cmd);
	p=spath;
	while(*p && *p>' ') p++;
	if(*p) {
		*p=0;	/* truncate arguments */
		args=p+1;
	}

	argv=JS_NewArrayObject(js_cx, 0, NULL);

	if(args!=NULL && argv!=NULL) {
		while(*args) {
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
	}
	JS_DefineProperty(js_cx, js_glob, "argv", OBJECT_TO_JSVAL(argv)
		,NULL,NULL,JSPROP_READONLY);
	JS_DefineProperty(js_cx, js_glob, "argc", INT_TO_JSVAL(argc)
		,NULL,NULL,JSPROP_READONLY);

	val = BOOLEAN_TO_JSVAL(JS_FALSE);
	JS_SetProperty(js_cx, js_glob, "logged_in", &val);

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
	active_clients--;
	update_clients();

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&BBS_OPT_MUTE)
		&& !(service->options&BBS_OPT_MUTE))
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	thread_down();
	lprintf("%04d %s JavaScript service thread terminated (%u clients remain, %u total)"
		, socket, service->protocol, service->clients, active_clients);

	client_off(socket);
	close_socket(socket);
}

static void native_service_thread(void* arg)
{
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

	thread_up();

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
		&& !(startup->options&BBS_OPT_NO_HOST_LOOKUP))
		lprintf("%04d %s client name: %s", socket, service->protocol, host_name);

	if(trashcan(&scfg,host_name,"host")) {
		lprintf("%04d !%s CLIENT BLOCKED in host.can: %s"
			,socket, service->protocol, host_name);
		close_socket(socket);
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
	sprintf(client.addr,"%.*s",(int)sizeof(client.addr)-1,inet_ntoa(service_client.addr.sin_addr));
	sprintf(client.host,"%.*s",(int)sizeof(client.host)-1,host_name);
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

	active_clients++;
	update_clients();

	/* Initialize client display */
	client_on(socket,&client,FALSE /* update */);

	/* RUN SCRIPT */
	if(!strchr(service->cmd,BACKSLASH))
		sprintf(cmd,"%s%s",scfg.exec_dir,service->cmd);
	else
		strcpy(cmd,service->cmd);
	sprintf(fullcmd,cmd,socket_dup);

	system(fullcmd);

	if(service->clients)
		service->clients--;
	active_clients--;
	update_clients();

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&BBS_OPT_MUTE)
		&& !(service->options&BBS_OPT_MUTE))
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	thread_down();
	lprintf("%04d %s service thread terminated (%u clients remain, %u total)"
		,socket, service->protocol, service->clients, active_clients);

	client_off(socket);
	close_socket(socket);
	closesocket(socket_dup);	/* close duplicate handle */
}


void DLLCALL services_terminate(void)
{
	terminated=TRUE;
}

#define NEXT_FIELD(p)	while(*p && *p>' ') p++; while(*p && *p<=' ') p++;

static service_t* read_services_cfg(char* services_cfg, DWORD* services)
{
	char*	p;
	char*	tp;
	char	line[1024];
	FILE*	fp;
	service_t*	service=NULL;

	if((fp=fopen(services_cfg,"r"))==NULL) {
		lprintf("!ERROR %d opening %s",errno,services_cfg);
		return(NULL);
	}

	for((*services)=0;!feof(fp) && (*services)<MAX_SERVICES;) {
		if(!fgets(line,sizeof(line)-1,fp))
			break;
		p=line;
		while(*p && *p<=' ') p++;
		if(*p==0 || *p==';')	/* ignore blank lines or comments */
			continue;
		
		if((service=(service_t*)realloc(service,sizeof(service_t)*((*services)+1)))==NULL) {
			lprintf("!MALLOC FAILURE");
			return(FALSE);
		}
		memset(&service[*services],0,sizeof(service_t));
		service[*services].socket=INVALID_SOCKET;

		tp=p; 
		while(*tp && *tp>' ') tp++; 
		*tp=0;
		sprintf(service[*services].protocol,"%.*s",sizeof(service[0].protocol),p);
		p=tp+1;
		while(*p && *p<=' ') p++;
		service[*services].port=atoi(p);
		NEXT_FIELD(p);
		service[*services].max_clients=strtol(p,NULL,10);
		NEXT_FIELD(p);
		service[*services].options=strtol(p,NULL,16);
		NEXT_FIELD(p);

		sprintf(service[*services].cmd,"%.*s",sizeof(service[0].cmd),p);
		truncsp(service[*services].cmd);

		(*services)++;
	}
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
    lprintf("#### Services thread terminated");
	status("Down");
	if(startup!=NULL && startup->terminated!=NULL)
		startup->terminated(code);
}

const char* DLLCALL services_ver(void)
{
	static char ver[256];
	char compiler[32];

	COMPILER_DESC(compiler);

	sprintf(ver,"Synchronet Services v%s%s  "
		"Compiled %s %s with %s"
		,SERVICES_VERSION
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
	char			cmd[128];
	SOCKADDR_IN		addr;
	SOCKADDR_IN		client_addr;
	int				client_addr_len;
	SOCKET			socket;
	SOCKET			client_socket;
	int				i;
	int				result;
	ulong			total_clients;
	time_t			t;
	time_t			initialized=0;
	fd_set			socket_set;
	SOCKET			high_socket;
	ulong			total_sockets;
	struct timeval	tv;
	service_client_t* client;

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

	/* Setup intelligent defaults */
	if(startup->js_max_bytes==0)			startup->js_max_bytes=JAVASCRIPT_MAX_BYTES;

	do {

		thread_up();

		status("Initializing");

		memset(&scfg, 0, sizeof(scfg));

#ifdef __unix__		/* Ignore "Broken Pipe" signal */
		signal(SIGPIPE,SIG_IGN);
#endif

		lprintf("Synchronet Services Version %s%s"
			,SERVICES_VERSION
#ifdef _DEBUG
			," Debug"
#else
			,""
#endif
			);

		COMPILER_DESC(compiler);

		lprintf("Compiled %s %s with %s", __DATE__, __TIME__, compiler);

		srand(time(NULL));	/* Seed random number generator */
		sbbs_random(10);	/* Throw away first number */

		if(!winsock_startup()) {
			cleanup(1);
			return;
		}

		t=time(NULL);
		lprintf("Initializing on %.24s with options: %lx"
			,ctime(&t),startup->options);

		/* Initial configuration and load from CNF files */
		sprintf(scfg.ctrl_dir, "%.*s", (int)sizeof(scfg.ctrl_dir)-1
    		,startup->ctrl_dir);
		lprintf("Loading configuration files from %s", scfg.ctrl_dir);
		scfg.size=sizeof(scfg);
		if(!load_cfg(&scfg, NULL, TRUE, error)) {
			lprintf("!ERROR %s",error);
			lprintf("!Failed to load configuration files");
			cleanup(1);
			return;
		}

		if(startup->host_name[0]==0)
			sprintf(startup->host_name,"%.*s",sizeof(startup->host_name),scfg.sys_inetaddr);

		if(!(scfg.sys_misc&SM_LOCAL_TZ) && !(startup->options&BBS_OPT_LOCAL_TIMEZONE)) {
			if(PUTENV("TZ=UTC0"))
				lprintf("!putenv() FAILED");
			tzset();

			if((t=checktime())!=0) {   /* Check binary time */
				lprintf("!TIME PROBLEM (%ld)",t);
				cleanup(1);
				return;
			}
		}

		if(uptime==0)
			uptime=time(NULL);

		active_clients=0;
		update_clients();

		if(startup->services_cfg[0]==0)			
			sprintf(startup->services_cfg,"%sservices.cfg",scfg.ctrl_dir);
		if((service=read_services_cfg(startup->services_cfg, &services))==NULL) {
			lprintf("!Failure reading configuration file");
			cleanup(1);
			return;
		}

		/* Open and Bind Listening Sockets */
		total_sockets=0;
		for(i=0;i<(int)services;i++) {

			service[i].socket=INVALID_SOCKET;

			if((socket = open_socket(SOCK_STREAM))==INVALID_SOCKET) {
				lprintf("!ERROR %d opening socket", ERROR_VALUE);
				cleanup(1);
				return;
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

			lprintf("%04d %s socket bound to port %u"
				,socket, service[i].protocol, service[i].port);

			if(listen(socket,10)!=0) {
				lprintf("%04d !ERROR %d listening on %s socket"
					,socket, ERROR_VALUE, service[i].protocol);
				close_socket(socket);
				continue;
			}
			service[i].socket=socket;
			total_sockets++;
		}

		if(!total_sockets) {
			lprintf("0000 !No service sockets bound");
			cleanup(1);
			return;
		}

		/* signal caller that we've started up successfully */
		if(startup->started!=NULL)
    		startup->started();

		status("Listening");

		if(initialized==0) {
			initialized=time(NULL);
			sprintf(path,"%sservices.rec",scfg.ctrl_dir);
			t=fdate(path);
			if(t!=-1 && t>initialized)
				initialized=t;
		}
			
		terminated=FALSE;

		/* Main Server Loop */
		while(!terminated) {

			total_clients=0;
			for(i=0;i<(int)services;i++) 
				total_clients+=service[i].clients;

			sprintf(path,"%sservices.rec",scfg.ctrl_dir);
			t=fdate(path);
			if(!total_clients && t!=-1 && t>initialized) {
				lprintf("0000 Recycle semaphore file (%s) detected",path);
				initialized=t;
				break;
			}
			if(!total_clients && startup->recycle_now==TRUE) {
				lprintf("0000 Recycle semaphore signaled");
				startup->recycle_now=FALSE;
				break;
			}

			/* Setup select() parms */
			FD_ZERO(&socket_set);	
			high_socket=0;
			for(i=0;i<(int)services;i++) {
				if(service[i].socket==INVALID_SOCKET)
					continue;
				FD_SET(service[i].socket,&socket_set);
				if(service[i].socket>high_socket)
					high_socket=service[i].socket;
			}
			tv.tv_sec=2;
			tv.tv_usec=0;
			if((result=select(high_socket+1,&socket_set,NULL,NULL,&tv))<1) {
				if(result==0) {
					mswait(1);
					continue;
				}

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
				if(!FD_ISSET(service[i].socket,&socket_set))
					continue;

				client_addr_len = sizeof(client_addr);
				if((client_socket=accept(service[i].socket,
					(struct sockaddr *)&client_addr,&client_addr_len))==INVALID_SOCKET) {
					if(ERROR_VALUE == ENOTSOCK)
            			lprintf("%04d %s socket closed while listening"
							,service[i].socket, service[i].protocol);
					else
						lprintf("%04d %s !ERROR %d accept failed", 
							service[i].socket, service[i].protocol, ERROR_VALUE);
					break;
				}
				strcpy(host_ip,inet_ntoa(client_addr.sin_addr));

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
					lprintf("%04d !%s CLIENT BLOCKED in ip.can: %s"
						,client_socket, service[i].protocol, host_ip);
					mswait(3000);
					close_socket(client_socket);
					continue;
				}

				if((client=malloc(sizeof(service_client_t)))==NULL) {
					lprintf("%04d !%s ERROR allocating %u bytes of memory for service_client"
						,client_socket, service[i].protocol, sizeof(service_client_t));
					mswait(3000);
					close_socket(client_socket);
					continue;
				}

				if(startup->socket_open!=NULL)
					startup->socket_open(TRUE);	/* Callback */

				memset(client,0,sizeof(service_client_t));
				client->socket=client_socket;
				client->addr=client_addr;
				client->service=&service[i];
				client->service->clients++;		/* this should be mutually exclusive */

				sprintf(cmd,"%.*s",sizeof(cmd)-1,service[i].cmd);
				strlwr(cmd);
				if(strstr(cmd,".js"))	/* JavaScript */
					_beginthread(js_service_thread, 0, client);
				else					/* Native */
					_beginthread(native_service_thread, 0, client);
			}
		}

		/* Close Service Sockets */
		for(i=0;i<(int)services;i++) {
			if(service[i].socket!=INVALID_SOCKET)
				close_socket(service[i].socket);
			service[i].socket=INVALID_SOCKET;
		}

		/* Wait for Service Threads to terminate */
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

		/* Free Service Data */
		services=0;
		free(service);
		service=NULL;

		cleanup(0);
		if(!terminated)
			lprintf("Recycling server...");

	} while(!terminated);
}
