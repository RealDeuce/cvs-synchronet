/* js_file.c */

/* Synchronet JavaScript "File" Object */

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
	char	name[MAX_PATH+1];
	char	mode[4];
	uchar	etx;
	FILE*	fp;
	BOOL	external;	/* externally created, don't close */
	BOOL	debug;

} private_t;


static void dbprintf(BOOL error, private_t* p, char* fmt, ...)
{
	va_list argptr;
	char sbuf[1024];

	if(p==NULL || (!p->debug && !error))
		return;

    va_start(argptr,fmt);
    vsprintf(sbuf,fmt,argptr);
    va_end(argptr);
	
	lprintf("%04u File %s%s",p->fp ? fileno(p->fp) : 0,error ? "ERROR: ":"",sbuf);
}

/* File Constructor (creates file descriptor) */

static JSBool
js_file_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString*	str;
	private_t*	p;

	if((str = JS_ValueToString(cx, argv[0]))==NULL) {
		dbprintf(TRUE, 0, "no filename specified");
		return(JS_FALSE);
	}

	*rval = JSVAL_VOID;

	if((p=(private_t*)calloc(1,sizeof(private_t)))==NULL) {
		dbprintf(TRUE, 0, "open_file malloc failed");
		return(JS_FALSE);
	}

	sprintf(p->name,"%.*s",sizeof(p->name)-1,JS_GetStringBytes(str));

	if(!JS_SetPrivate(cx, obj, p)) {
		dbprintf(TRUE, p, "JS_SetPrivate failed");
		return(JS_FALSE);
	}

	dbprintf(FALSE, p, "object constructed");
	return(JS_TRUE);
}

/* File Destructor */

static void js_finalize_file(JSContext *cx, JSObject *obj)
{
	private_t* p;
	
	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return;

	if(p->external==JS_FALSE && p->fp!=NULL)
		fclose(p->fp);

	dbprintf(FALSE, p, "closed: %s",p->name);

	free(p);

	JS_SetPrivate(cx, obj, NULL);
}


/* File Object Methods */

static JSBool
js_open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		mode="w+";	/* default mode */
	uintN		i;
	jsint		bufsize=2*1024;
	JSString*	str;
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp!=NULL)  
		return(JS_TRUE);
		

	for(i=0;i<argc;i++) {
		if(JSVAL_IS_STRING(argv[i])) {	/* mode */
			if((str = JS_ValueToString(cx, argv[i]))==NULL) {
				dbprintf(TRUE, 0, "invalid mode specified");
				return(JS_FALSE);
			}
			mode=JS_GetStringBytes(str);
		} else
			JS_ValueToInt32(cx,argv[i],&bufsize);
	}
	sprintf(p->mode,"%.*s",sizeof(p->mode)-1,mode);

	if((p->fp=fopen(p->name,p->mode))==NULL)
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	else {
		dbprintf(FALSE, p, "opened: %s",p->name);
		if(!bufsize)
			setvbuf(p->fp,NULL,_IONBF,0);	/* no buffering */
		else
			setvbuf(p->fp,NULL,_IOFBF,bufsize);
	}

	return(JS_TRUE);
}


static JSBool
js_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	if(p->fp==NULL)
		return(JS_TRUE);

	fclose(p->fp);

	dbprintf(FALSE, p, "closed");

	p->fp=NULL; 

	return(JS_TRUE);
}

static JSBool
js_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp;
	char*		buf;
	int			len=512;
	JSString*	str;
	private_t*	p;

	*rval = JSVAL_NULL;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(argc)
		len = JSVAL_TO_INT(argv[0]);

	if((buf=malloc(len+1))==NULL)
		return(JS_TRUE);

	len = fread(buf,1,len,p->fp);
	if(len<0) 
		len=0;
	buf[len]=0;

	if(p->etx) {
		cp=strchr(buf,p->etx);
		if(cp) *cp=0; 
	}

	str = JS_NewStringCopyZ(cx, buf);

	free(buf);

	*rval = STRING_TO_JSVAL(str);

	dbprintf(FALSE, p, "read %u bytes",len);
		
	return(JS_TRUE);
}

static JSBool
js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp;
	char*		buf;
	ulong		len=512;
	private_t*	p;

	*rval = JSVAL_NULL;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);
	
	if(argc)
		len = JSVAL_TO_INT(argv[0]);

	if((buf=malloc(len))==NULL)
		return(JS_TRUE);

	if(fgets(buf,len,p->fp)!=NULL) {
		truncsp(buf);
		if(p->etx) {
			cp=strchr(buf,p->etx);
			if(cp) *cp=0; 
		}
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,buf));
	}

	free(buf);

	return(JS_TRUE);
}

static JSBool
js_readbin(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	BYTE		b;
	WORD		w;
	DWORD		l;
	size_t		size=sizeof(DWORD);
	private_t*	p;

	*rval = JSVAL_TO_INT(-1);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(argc) 
		size = JSVAL_TO_INT(argv[0]);

	switch(size) {
		case sizeof(BYTE):
			if(fread(&b,1,size,p->fp)==size)
				*rval = INT_TO_JSVAL(b);
			break;
		case sizeof(WORD):
			if(fread(&w,1,size,p->fp)==size)
				*rval = INT_TO_JSVAL(w);
			break;
		case sizeof(DWORD):
			if(fread(&l,1,size,p->fp)==size)
				*rval = INT_TO_JSVAL(l);
			break;
	}
		
	return(JS_TRUE);
}

static JSBool
js_readall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsint       len=0;
    jsval       line;
    JSObject*	array;
	private_t*	p;

	*rval = JSVAL_NULL;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

    array = JS_NewArrayObject(cx, 0, NULL);

    while(!feof(p->fp)) {
		js_readln(cx, obj, 0, NULL, &line); 
		if(line==JSVAL_NULL)
			break;
        if(!JS_SetElement(cx, array, len++, &line))
			break;
	}
    *rval = OBJECT_TO_JSVAL(array);

    return(JS_TRUE);
}

static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp;
	size_t		len;	/* string length */
	size_t		tlen;	/* total length to write (may be greater than len) */
	JSString*	str;
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(!argc)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	cp = JS_GetStringBytes(str);
	len = strlen(cp);
	tlen = len;
	if(argc>1) {
		tlen=JSVAL_TO_INT(argv[1]);
		if(len>tlen)
			len=tlen;
	}
	if(fwrite(cp,1,len,p->fp)==len) {
		if(tlen>len) {
			len=tlen-len;
			if((cp=malloc(len))==NULL) {
				dbprintf(TRUE, p, "malloc failure of %u bytes", len);
				*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
				return(JS_TRUE);
			}
			memset(cp,p->etx,len);
			fwrite(cp,1,len,p->fp);
			free(cp);
		}
		dbprintf(FALSE, p, "wrote %u bytes",tlen);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	} else 
		dbprintf(TRUE, p, "write of %u bytes failed",len);
		
	return(JS_TRUE);
}

static JSBool
js_writeln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		cp="";
	JSString*	str;
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(argc) {
		if((str = JS_ValueToString(cx, argv[0]))==NULL)
			return(JS_FALSE);
		cp = JS_GetStringBytes(str);
	}

	if(fprintf(p->fp,"%s\n",cp)!=0)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return(JS_TRUE);
}

static JSBool
js_writebin(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	BYTE		b;
	WORD		w;
	DWORD		l;
	size_t		wr=0;
	size_t		size=sizeof(DWORD);
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(argc>1) 
		size = JSVAL_TO_INT(argv[1]);

	switch(size) {
		case sizeof(BYTE):
			b = (BYTE)JSVAL_TO_INT(argv[0]);
			wr=fwrite(&b,1,size,p->fp);
			break;
		case sizeof(WORD):
			w = (WORD)JSVAL_TO_INT(argv[0]);
			wr=fwrite(&w,1,size,p->fp);
			break;
		case sizeof(DWORD):
			l = JSVAL_TO_INT(argv[0]);
			wr=fwrite(&l,1,size,p->fp);
			break;
		default:	
			/* unknown size */
			dbprintf(TRUE, p, "unsupported binary write size: %d",size);
			break;
	}
	if(wr==size)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		
	return(JS_TRUE);
}

static JSBool
js_writeall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint      i;
    jsuint      limit;
    JSObject*	array;
    jsval       elemval;
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

    if(!JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[0])))
		return(JS_TRUE);

    array = JSVAL_TO_OBJECT(argv[0]);

    JS_GetArrayLength(cx, array, &limit);

    *rval = BOOLEAN_TO_JSVAL(JS_TRUE);

    for(i=0;i<limit;i++) {
        if(!JS_GetElement(cx, array, i, &elemval))
			break;
        js_writeln(cx, obj, 1, &elemval, rval);
		if(*rval!=BOOLEAN_TO_JSVAL(JS_TRUE))
			break;
    }

    return(JS_TRUE);
}

static JSBool
js_lock(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(lock(fileno(p->fp),JSVAL_TO_INT(argv[0]),JSVAL_TO_INT(argv[1]))==0)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return(JS_TRUE);
}

static JSBool
js_unlock(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		return(JS_TRUE);

	if(unlock(fileno(p->fp),JSVAL_TO_INT(argv[0]),JSVAL_TO_INT(argv[1]))==0)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return(JS_TRUE);
}

static JSBool
js_delete(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp!=NULL) {	/* close it if it's open */
		fclose(p->fp);
		p->fp=NULL;
	}

	*rval = BOOLEAN_TO_JSVAL(remove(p->name)==0);

	return(JS_TRUE);
}

static JSBool
js_flush(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	else 
		*rval = BOOLEAN_TO_JSVAL(fflush(p->fp)==0);

	return(JS_TRUE);
}

static JSBool
js_clear_error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(p->fp==NULL)
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	else  {
		clearerr(p->fp);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return(JS_TRUE);
}


/* File Object Properites */
enum {
	 FILE_PROP_NAME		
	,FILE_PROP_MODE
	,FILE_PROP_ETX
	,FILE_PROP_EXISTS	
	,FILE_PROP_DATE		
	,FILE_PROP_IS_OPEN	
	,FILE_PROP_EOF		
	,FILE_PROP_ERROR	
	,FILE_PROP_DESCRIPTOR
	,FILE_PROP_DEBUG	
	,FILE_PROP_POSITION	
	,FILE_PROP_LENGTH	
	,FILE_PROP_ATTRIBUTES
};

static JSBool js_file_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	dbprintf(FALSE, p, "setting property %d",tiny);

	switch(tiny) {
		case FILE_PROP_DEBUG:
			p->debug = JSVAL_TO_BOOLEAN(*vp);
			break;
		case FILE_PROP_POSITION:
			if(p->fp!=NULL)
				fseek(p->fp,JSVAL_TO_INT(*vp),SEEK_SET);
			break;
		case FILE_PROP_LENGTH:
			if(p->fp!=NULL)
				chsize(fileno(p->fp),JSVAL_TO_INT(*vp));
			break;
		case FILE_PROP_ATTRIBUTES:
			CHMOD(p->name,JSVAL_TO_INT(*vp));
			break;
		case FILE_PROP_ETX:
			p->etx = (uchar)JSVAL_TO_INT(*vp);
			break;
	}

	return(JS_TRUE);
}

static JSBool js_file_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	private_t*	p;

	if((p=(private_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

#if 0 /* just too much */
	dbprintf(FALSE, sock, "getting property %d",tiny);
#endif

	switch(tiny) {
		case FILE_PROP_NAME:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, p->name));
			break;
		case FILE_PROP_MODE:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, p->mode));
			break;
		case FILE_PROP_EXISTS:
			if(p->fp)	/* open? */
				*vp = BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				*vp = BOOLEAN_TO_JSVAL(fexist(p->name));
			break;
		case FILE_PROP_DATE:
			*vp = INT_TO_JSVAL(fdate(p->name));
			break;
		case FILE_PROP_IS_OPEN:
			*vp = BOOLEAN_TO_JSVAL(p->fp!=NULL);
			break;
		case FILE_PROP_EOF:
			if(p->fp)
				*vp = BOOLEAN_TO_JSVAL(feof(p->fp)!=0);
			else
				*vp = BOOLEAN_TO_JSVAL(JS_TRUE);
			break;
		case FILE_PROP_ERROR:
			if(p->fp)
				*vp = INT_TO_JSVAL(ferror(p->fp));
			else
				*vp = INT_TO_JSVAL(0);
			break;
		case FILE_PROP_POSITION:
			if(p->fp)
				*vp = INT_TO_JSVAL(ftell(p->fp));
			else
				*vp = INT_TO_JSVAL(-1);
			break;
		case FILE_PROP_LENGTH:
			if(p->fp)	/* open? */
				*vp = INT_TO_JSVAL(filelength(fileno(p->fp)));
			else
				*vp = INT_TO_JSVAL(flength(p->name));
			break;
		case FILE_PROP_ATTRIBUTES:
			*vp = INT_TO_JSVAL(getfattr(p->name));
			break;
		case FILE_PROP_DEBUG:
			*vp = INT_TO_JSVAL(p->debug);
			break;
		case FILE_PROP_DESCRIPTOR:
			if(p->fp)
				*vp = INT_TO_JSVAL(fileno(p->fp));
			else
				*vp = INT_TO_JSVAL(-1);
			break;
		case FILE_PROP_ETX:
			*vp = INT_TO_JSVAL(p->etx);
			break;
	}

	return(TRUE);
}

#define FILE_PROP_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY

static struct JSPropertySpec js_file_properties[] = {
/*		 name				,tinyid					,flags,				getter,	setter	*/
	{	"name"				,FILE_PROP_NAME			,FILE_PROP_FLAGS,	NULL,NULL},
	{	"mode"				,FILE_PROP_MODE			,FILE_PROP_FLAGS,	NULL,NULL},
	{	"exists"			,FILE_PROP_EXISTS		,FILE_PROP_FLAGS,	NULL,NULL},
	{	"date"				,FILE_PROP_DATE			,FILE_PROP_FLAGS,	NULL,NULL},
	{	"is_open"			,FILE_PROP_IS_OPEN		,FILE_PROP_FLAGS,	NULL,NULL},
	{	"eof"				,FILE_PROP_EOF			,FILE_PROP_FLAGS,	NULL,NULL},
	{	"error"				,FILE_PROP_ERROR		,FILE_PROP_FLAGS,	NULL,NULL},
	{	"descriptor"		,FILE_PROP_DESCRIPTOR	,FILE_PROP_FLAGS,	NULL,NULL},
	/* writeable */
	{	"etx"				,FILE_PROP_ETX			,JSPROP_ENUMERATE,  NULL,NULL},
	{	"debug"				,FILE_PROP_DEBUG		,JSPROP_ENUMERATE,	NULL,NULL},
	{	"position"			,FILE_PROP_POSITION		,JSPROP_ENUMERATE,	NULL,NULL},
	{	"length"			,FILE_PROP_LENGTH		,JSPROP_ENUMERATE,	NULL,NULL},
	{	"attributes"		,FILE_PROP_ATTRIBUTES	,JSPROP_ENUMERATE,	NULL,NULL},
	{0}
};

static JSClass js_file_class = {
     "File"					/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_file_get			/* getProperty	*/
	,js_file_set			/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,js_finalize_file		/* finalize		*/
};

static JSFunctionSpec js_file_functions[] = {
	{"open",			js_open,			1},		/* open file (w/mode) [buffered] */
	{"close",			js_close,			0},		/* close file */
	{"delete",			js_delete,			0},		/* delete the file */
	{"remove",			js_delete,			0},		/* delete the file */
	{"clear_error",		js_clear_error,		0},		/* clear error */
	{"clearError",		js_clear_error,		0},		/* clear error */
	{"flush",			js_flush,			0},		/* flush buffers */
	{"lock",			js_lock,			2},		/* lock offset, length */
	{"unlock",			js_unlock,			2},		/* unlock offset, length */
	{"read",			js_read,			0},		/* read a string [length] */
	{"readln",			js_readln,			0},		/* read a \n terminated string	*/
	{"readBin",			js_readbin,			0},		/* read an integer (length) */
	{"readAll",			js_readall,			0},		/* read all lines into an array */
	{"write",			js_write,			1},		/* write a string [length] */
	{"writeln",			js_writeln,			0},		/* write a \n terminated string */
	{"writeBin",		js_writebin,		1},		/* read an integer (length) */
	{"writeAll",		js_writeall,		0},		/* write array of strings to file */
	{0}
};


JSObject* DLLCALL js_CreateFileClass(JSContext* cx, JSObject* parent)
{
	JSObject*	sockobj;

	sockobj = JS_InitClass(cx, parent, NULL
		,&js_file_class
		,js_file_constructor
		,0	/* number of constructor args */
		,js_file_properties
		,js_file_functions
		,NULL,NULL);

	return(sockobj);
}

JSObject* DLLCALL js_CreateFileObject(JSContext* cx, JSObject* parent, char *name, FILE* fp)
{
	JSObject* obj;
	private_t*	p;

	obj = JS_DefineObject(cx, parent, name, &js_file_class, NULL, 0);

	if(obj==NULL)
		return(NULL);

	if(!JS_DefineProperties(cx, obj, js_file_properties))
		return(NULL);

	if (!JS_DefineFunctions(cx, obj, js_file_functions)) 
		return(NULL);

	if((p=(private_t*)calloc(1,sizeof(private_t)))==NULL)
		return(NULL);

	p->fp=fp;
	p->debug=JS_FALSE;
	p->external=JS_TRUE;

	if(!JS_SetPrivate(cx, obj, p)) {
		dbprintf(TRUE, p, "JS_SetPrivate failed");
		return(NULL);
	}

	dbprintf(FALSE, p, "object created");

	return(obj);
}


#endif	/* JAVSCRIPT */
