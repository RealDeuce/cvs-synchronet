/* js_socket.c */

/* Synchronet JavaScript "Socket" Object */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2001 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifdef JAVASCRIPT

typedef struct
{
	SOCKET	sock;
	BOOL	external;	/* externally created, don't close */
	BOOL	debug;
	BOOL	nonblocking;
	int		last_error;

} private_t;



static void dbprintf(BOOL error, private_t* p, char* fmt, ...)
{
	va_list argptr;
	char sbuf[1024];

	if(p==NULL || (!p->debug /*&& !error */))
		return;

    va_start(argptr,fmt);
    vsprintf(sbuf,fmt,argptr);
    va_end(argptr);
	
	lprintf("%04d Socket %s%s",p->sock,error ? "ERROR: ":"",sbuf);
}

/* Socket Constructor (creates socket descriptor) */

static JSBool
js_socket_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int		type=SOCK_STREAM;	/* default = TCP */
	private_t* p;

	if(argc)
		type=JSVAL_TO_INT(argv[0]);

	*rval = JSVAL_VOID;

	if((p=(private_t*)malloc(sizeof(private_t)))==NULL) {
		dbprintf(TRUE, 0, "open_socket malloc failed");
		return(JS_FALSE);
	}
	memset(p,0,sizeof(private_t));

	if((p->sock=open_socket(type))==INVALID_SOCKET) {
		dbprintf(TRUE, 0, "open_socket failed with error %d",ERROR_VALUE);
		return(JS_FALSE);
	}

	if(!JS_SetPrivate(cx, obj, p)) {
		dbprintf(TRUE, p, "JS_SetPrivate failed");
		return(JS_FALSE);
	}

	dbprintf(FALSE, p, "object constructed");
	return(JS_TRUE);
}

/* Socket Destructor */

static void js_finalize_socket(JSContext *cx, JSObject *obj)
{
	private_t* p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return;

	if(p->external==JS_FALSE && p->sock!=INVALID_SOCKET)
		close_socket(p->sock);

	dbprintf(FALSE, p, "closed/deleted");

	free(p);

	JS_SetPrivate(cx, obj, NULL);
}


/* Socket Object Methods */

static JSBool
js_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	if(p->sock==INVALID_SOCKET)
		return(JS_TRUE);

	close_socket(p->sock);

	p->last_error=ERROR_VALUE;

	dbprintf(FALSE, p, "closed");

	p->sock=INVALID_SOCKET; 

	return(JS_TRUE);
}

static JSBool
js_bind(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SOCKADDR_IN	addr;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);
	
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;

	if(argc)
		addr.sin_port = (ushort)JSVAL_TO_INT(argv[0]);

	if(bind(p->sock, (struct sockaddr *) &addr, sizeof (addr))!=0) {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "bind failed with error %d",ERROR_VALUE);
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	dbprintf(FALSE, p, "bound to port %u",addr.sin_port);
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return(JS_TRUE);
}

static JSBool
js_connect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ulong		ip_addr;
	ushort		port;
	JSString*	str;
	SOCKADDR_IN	addr;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc<2)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	dbprintf(FALSE, p, "resolving hostname: %s", JS_GetStringBytes(str));
	if((ip_addr=resolve_ip(JS_GetStringBytes(str)))==0) {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "resolve_ip failed with error %d",ERROR_VALUE);
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	port = (ushort)JSVAL_TO_INT(argv[1]);

	dbprintf(FALSE, p, "connecting to port %u at %s", port, JS_GetStringBytes(str));

	memset(&addr,0,sizeof(addr));
	addr.sin_addr.s_addr = ip_addr;
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	if(connect(p->sock, (struct sockaddr *)&addr, sizeof(addr))!=0) {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "connect failed with error %d",ERROR_VALUE);
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	dbprintf(FALSE, p, "connected to port %u at %s", port, JS_GetStringBytes(str));

	return(JS_TRUE);
}

static JSBool
js_send(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp;
	int			len;
	JSString*	str;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!argc)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	cp=JS_GetStringBytes(str);
	len=strlen(cp);

	if(sendsocket(p->sock,cp,len)==len) {
		dbprintf(FALSE, p, "sent %u bytes",len);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	} else {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "send of %u bytes failed",len);
	}
		
	return(JS_TRUE);
}

static JSBool
js_sendfile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		fname;
	char*		buf;
	long		len;
	int			file;
	JSString*	str;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!argc)
		return(JS_FALSE);

	if((str = JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);
	if((fname=JS_GetStringBytes(str))==NULL)
		return(JS_FALSE);

	if((file=nopen(fname,O_RDONLY|O_BINARY))==-1)
		return(JS_TRUE);

	len=filelength(file);
	if((buf=malloc(len))==NULL) {
		close(file);
		return(JS_TRUE);
	}
	if(read(file,buf,len)!=len) {
		close(file);
		return(JS_TRUE);
	}
	close(file);

	if(sendsocket(p->sock,buf,len)==len) {
		dbprintf(FALSE, p, "sent %u bytes",len);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	} else {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "send of %u bytes failed",len);
	}
	free(buf);

	return(JS_TRUE);
}

static JSBool
js_recv(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		buf[513];
	int			len;
	JSString*	str;

	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc)
		len = JSVAL_TO_INT(argv[0]);
	else
		len = sizeof(buf)-1;

	len = len > sizeof(buf)-1 ? sizeof(buf)-1 : len;

	len = recv(p->sock,buf,len,0);
	if(len<0) {
		p->last_error=ERROR_VALUE;
		*rval = JSVAL_NULL;
		return(JS_TRUE);
	}
	buf[len]=0;

	str = JS_NewStringCopyZ(cx, buf);

	*rval = STRING_TO_JSVAL(str);

	dbprintf(FALSE, p, "received %u bytes",len);
		
	return(JS_TRUE);
}

static JSBool
js_peek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		buf[513];
	int			len;
	JSString*	str;

	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc)
		len = JSVAL_TO_INT(argv[0]);
	else
		len = sizeof(buf)-1;

	len = len > sizeof(buf)-1 ? sizeof(buf)-1 : len;

	len = recv(p->sock,buf,len,MSG_PEEK);
	if(len<0) {
		p->last_error=ERROR_VALUE;	
		*rval = JSVAL_NULL;
		return(JS_TRUE);
	}
	buf[len]=0;

	str = JS_NewStringCopyZ(cx, buf);

	*rval = STRING_TO_JSVAL(str);

	dbprintf(FALSE, p, "received %u bytes",len);
		
	return(JS_TRUE);
}

static JSBool
js_recvline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		ch;
	char		buf[513];
	int			i;
	int			len;
	BOOL		rd;
	time_t		start;
	time_t		timeout=30;	/* seconds */
	JSString*	str;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc)
		len = JSVAL_TO_INT(argv[0]);
	else
		len = sizeof(buf)-1;

	len = len > sizeof(buf)-1 ? sizeof(buf)-1 : len;

	if(argc>1)
		timeout = JSVAL_TO_INT(argv[1]);

	start=time(NULL);
	for(i=0;i<len;) {

		if(!socket_check(p->sock,&rd)) {
			p->last_error=ERROR_VALUE;
			break;		/* disconnected */
		}

		if(!rd) {
			if(time(NULL)-start>timeout) {
				dbprintf(FALSE, p, "recvline timeout");
				*rval = JSVAL_NULL;
				return(JS_TRUE);	/* time-out */
			}
			mswait(1);
			continue;	/* no data */
		}

		if(recv(p->sock, &ch, 1, 0)!=1) {
			p->last_error=ERROR_VALUE;
			break;
		}

		if(ch=='\n' && i>=1) 
			break;

		buf[i++]=ch;
	}
	if(i>0 && buf[i-1]=='\r')
		buf[i-1]=0;
	else
		buf[i]=0;

	str = JS_NewStringCopyZ(cx, buf);

	*rval = STRING_TO_JSVAL(str);

	dbprintf(FALSE, p, "received %u bytes",strlen(buf));
		
	return(JS_TRUE);
}

static JSBool
js_getsockopt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			lvl;
	int			opt;
	int			len;
	int			val;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	lvl = JSVAL_TO_INT(argv[0]);
	opt = JSVAL_TO_INT(argv[1]);
	len = sizeof(val);

	if(getsockopt(p->sock,lvl,opt,(char*)&val,&len)==0) {
		dbprintf(FALSE, p, "option %d (level: %d) = %d",opt,lvl,val);
		*rval = INT_TO_JSVAL(val);
	} else {
		p->last_error=ERROR_VALUE;
		dbprintf(TRUE, p, "error %d getting option %d (level: %d)"
			,ERROR_VALUE,opt,lvl);
		*rval = INT_TO_JSVAL(-1);
	}

	return(JS_TRUE);
}


static JSBool
js_setsockopt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			lvl;
	int			opt;
	int			val;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	lvl = JSVAL_TO_INT(argv[0]);
	opt = JSVAL_TO_INT(argv[1]);
	val = JSVAL_TO_INT(argv[2]);

	*rval = BOOLEAN_TO_JSVAL(setsockopt(p->sock,lvl,opt,(char*)&val,sizeof(val))==0);
	p->last_error=ERROR_VALUE;

	return(JS_TRUE);
}

static JSBool
js_ioctlsocket(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	long		cmd;
	ulong		arg=0;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	cmd = JSVAL_TO_INT(argv[0]);
	if(argc>1)
		arg = JSVAL_TO_INT(argv[1]);

	if(ioctlsocket(p->sock,cmd,&arg)==0)
		*rval = INT_TO_JSVAL(arg);
	else
		*rval = INT_TO_JSVAL(-1);

	p->last_error=ERROR_VALUE;

	return(JS_TRUE);
}

/* Socket Object Properites */
enum {
	 SOCK_PROP_LAST_ERROR
	,SOCK_PROP_IS_CONNECTED
	,SOCK_PROP_DATA_WAITING
	,SOCK_PROP_NREAD
	,SOCK_PROP_DEBUG
	,SOCK_PROP_DESCRIPTOR
	,SOCK_PROP_NONBLOCKING
};

static JSBool js_socket_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	dbprintf(FALSE, p, "setting property %d",tiny);

	switch(tiny) {
		case SOCK_PROP_DEBUG:
			p->debug = JSVAL_TO_BOOLEAN(*vp);
			break;
		case SOCK_PROP_DESCRIPTOR:
			p->sock = JSVAL_TO_INT(*vp);
			break;
		case SOCK_PROP_LAST_ERROR:
			p->last_error = JSVAL_TO_INT(*vp);
			break;
		case SOCK_PROP_NONBLOCKING:
			p->nonblocking = JSVAL_TO_BOOLEAN(*vp);
			ioctlsocket(p->sock,FIONBIO,(ulong*)&(p->nonblocking));
			break;
	}

	return(JS_TRUE);
}

static JSBool js_socket_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	ulong		cnt;
	BOOL		rd;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

#if 0 /* just too much */
	dbprintf(FALSE, sock, "getting property %d",tiny);
#endif

	switch(tiny) {
		case SOCK_PROP_LAST_ERROR:
			*vp = INT_TO_JSVAL(p->last_error);
			break;
		case SOCK_PROP_IS_CONNECTED:
			*vp = BOOLEAN_TO_JSVAL(socket_check(p->sock,NULL));
			break;
		case SOCK_PROP_DATA_WAITING:
			socket_check(p->sock,&rd);
			*vp = BOOLEAN_TO_JSVAL(rd);
			break;
		case SOCK_PROP_NREAD:
			cnt=0;
			if(ioctlsocket(p->sock, FIONREAD, &cnt)==0) 
				*vp = INT_TO_JSVAL(cnt);
			else
				*vp = INT_TO_JSVAL(0);
			break;
		case SOCK_PROP_DEBUG:
			*vp = INT_TO_JSVAL(p->debug);
			break;
		case SOCK_PROP_DESCRIPTOR:
			*vp = INT_TO_JSVAL(p->sock);
			break;
		case SOCK_PROP_NONBLOCKING:
			*vp = BOOLEAN_TO_JSVAL(p->nonblocking);
			break;
	}

	return(TRUE);
}

#define SOCK_PROP_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY

static struct JSPropertySpec js_socket_properties[] = {
/*		 name				,tinyid					,flags,				getter,	setter	*/

	{	"last_error"		,SOCK_PROP_LAST_ERROR	,SOCK_PROP_FLAGS,	NULL,NULL},
	{	"is_connected"		,SOCK_PROP_IS_CONNECTED	,SOCK_PROP_FLAGS,	NULL,NULL},
	{	"data_waiting"		,SOCK_PROP_DATA_WAITING	,SOCK_PROP_FLAGS,	NULL,NULL},
	{	"nread"				,SOCK_PROP_NREAD		,SOCK_PROP_FLAGS,	NULL,NULL},
	{	"debug"				,SOCK_PROP_DEBUG		,JSPROP_ENUMERATE,	NULL,NULL},
	{	"descriptor"		,SOCK_PROP_DESCRIPTOR	,JSPROP_ENUMERATE,	NULL,NULL},
	{	"nonblocking"		,SOCK_PROP_NONBLOCKING	,JSPROP_ENUMERATE,	NULL,NULL},
	{0}
};

static JSClass js_socket_class = {
     "Socket"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_socket_get			/* getProperty	*/
	,js_socket_set			/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,js_finalize_socket		/* finalize		*/
};

static JSFunctionSpec js_socket_functions[] = {
	{"close",			js_close,			0},		/* close socket */
	{"bind",			js_bind,			0},		/* bind to a port */
	{"connect",         js_connect,         2},		/* connect to an IP address and port */
	{"send",			js_send,			1},		/* send a string */
	{"sendfile",		js_sendfile,		1},		/* send a file */
	{"write",			js_send,			1},		/* send a string */
	{"recv",			js_recv,			0},		/* receive a string */
	{"read",			js_recv,			0},		/* receive a string */
	{"peek",			js_peek,			0},		/* receive a string, leave in buffer */
	{"recvline",		js_recvline,		0},		/* receive a \n terminated string	*/
	{"readline",		js_recvline,		0},		/* receive a \n terminated string	*/
	{"readln",			js_recvline,		0},		/* receive a \n terminated string	*/
	{"getoption",		js_getsockopt,		2},		/* getsockopt(level,opt)			*/
	{"setoption",		js_setsockopt,		3},		/* setsockopt(level,opt,val)		*/
	{"ioctl",			js_ioctlsocket,		1},		/* ioctl(cmd,arg)					*/
	{0}
};


JSObject* DLLCALL js_CreateSocketClass(JSContext* cx, JSObject* parent)
{
	JSObject*	sockobj;

	sockobj = JS_InitClass(cx, parent, NULL
		,&js_socket_class
		,js_socket_constructor
		,0	/* number of constructor args */
		,js_socket_properties
		,js_socket_functions
		,NULL,NULL);

	return(sockobj);
}

JSObject* DLLCALL js_CreateSocketObject(JSContext* cx, JSObject* parent, char *name, SOCKET sock)
{
	JSObject* obj;
	private_t*	p;

	obj = JS_DefineObject(cx, parent, name, &js_socket_class, NULL, 0);

	if(obj==NULL)
		return(NULL);

	if(!JS_DefineProperties(cx, obj, js_socket_properties))
		return(NULL);

	if (!JS_DefineFunctions(cx, obj, js_socket_functions)) 
		return(NULL);

	if((p=(private_t*)malloc(sizeof(private_t)))==NULL)
		return(NULL);
	memset(p,0,sizeof(private_t));

	p->sock=sock;
	p->external=JS_TRUE;

	if(!JS_SetPrivate(cx, obj, p)) {
		dbprintf(TRUE, p, "JS_SetPrivate failed");
		return(NULL);
	}

	dbprintf(FALSE, p, "object created");

	return(obj);
}


#endif	/* JAVSCRIPT */
