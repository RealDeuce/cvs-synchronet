/* js_uifc.c */

/* Synchronet "uifc" (user interface) object */

/* $Id: js_uifc.c,v 1.27 2011/10/29 03:53:58 deuce Exp $ */

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

#ifndef JAVASCRIPT
#define JAVASCRIPT
#endif

#include "sbbs.h"
#include "uifc.h"
#include "ciolib.h"
#include "js_request.h"

/* Properties */
enum {
	 PROP_INITIALIZED	/* read-only */
	,PROP_MODE
	,PROP_CHANGES
	,PROP_SAVNUM
	,PROP_SCRN_LEN
    ,PROP_SCRN_WIDTH
	,PROP_ESC_DELAY
	,PROP_HELPBUF
	,PROP_HCOLOR
	,PROP_LCOLOR
	,PROP_BCOLOR
	,PROP_CCOLOR
	,PROP_LBCOLOR
	,PROP_LIST_HEIGHT
};

static JSBool js_get(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	jsval idval;
    jsint		tiny;
	uifcapi_t*	uifc;

	if((uifc=(uifcapi_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    JS_IdToValue(cx, id, &idval);
    tiny = JSVAL_TO_INT(idval);

	switch(tiny) {
		case PROP_INITIALIZED:
			*vp=BOOLEAN_TO_JSVAL(uifc->initialized);
			break;
		case PROP_MODE:
			*vp=UINT_TO_JSVAL(uifc->mode);
			break;
		case PROP_CHANGES:
			*vp=BOOLEAN_TO_JSVAL(uifc->changes);
			break;
		case PROP_SAVNUM:
			*vp=INT_TO_JSVAL(uifc->savnum);
			break;
		case PROP_SCRN_LEN:
			*vp=INT_TO_JSVAL(uifc->scrn_len);
			break;
		case PROP_SCRN_WIDTH:
			*vp=INT_TO_JSVAL(uifc->scrn_width);
			break;
		case PROP_ESC_DELAY:
			*vp=INT_TO_JSVAL(uifc->esc_delay);
			break;
		case PROP_HELPBUF:
			*vp=STRING_TO_JSVAL(JS_NewStringCopyZ(cx,uifc->helpbuf));
			break;
		case PROP_HCOLOR:
			*vp=INT_TO_JSVAL(uifc->hclr);
			break;
		case PROP_LCOLOR:
			*vp=INT_TO_JSVAL(uifc->lclr);
			break;
		case PROP_BCOLOR:
			*vp=INT_TO_JSVAL(uifc->bclr);
			break;
		case PROP_CCOLOR:
			*vp=INT_TO_JSVAL(uifc->cclr);
			break;
		case PROP_LBCOLOR:
			*vp=INT_TO_JSVAL(uifc->lbclr);
			break;
		case PROP_LIST_HEIGHT:
			*vp=INT_TO_JSVAL(uifc->list_height);
			break;
	}

	return(JS_TRUE);
}

static JSBool js_set(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
	jsval idval;
    jsint		tiny;
	int32		i=0;
	uifcapi_t*	uifc;

	if((uifc=(uifcapi_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    JS_IdToValue(cx, id, &idval);
    tiny = JSVAL_TO_INT(idval);

	if(tiny==PROP_CHANGES)
		return JS_ValueToBoolean(cx,*vp,&uifc->changes);
	else if(tiny==PROP_HELPBUF) {
		JSVALUE_TO_STRING(cx, *vp, uifc->helpbuf, NULL);
		return JS_TRUE;
	}

	if(!JS_ValueToInt32(cx, *vp, &i))
		return JS_FALSE;

	switch(tiny) {
		case PROP_CHANGES:
			uifc->changes=i;
			break;
		case PROP_MODE:
			uifc->mode=i;
			break;
		case PROP_SAVNUM:
			uifc->savnum=i;
			break;
		case PROP_SCRN_LEN:
			uifc->scrn_len=i;
			break;
		case PROP_SCRN_WIDTH:
			uifc->scrn_width=i;
			break;
		case PROP_ESC_DELAY:
			uifc->esc_delay=i;
			break;
		case PROP_LIST_HEIGHT:
			uifc->list_height=i;
			break;
		case PROP_HCOLOR:
			uifc->hclr=(char)i;
			break;
		case PROP_LCOLOR:
			uifc->lclr=(char)i;
			break;
		case PROP_BCOLOR:
			uifc->bclr=(char)i;
			break;
		case PROP_CCOLOR:
			uifc->cclr=(char)i;
			break;
		case PROP_LBCOLOR:
			uifc->lbclr=(char)i;
			break;
	}

	return(JS_TRUE);
}

static jsSyncPropertySpec js_properties[] = {
/*		 name,				tinyid,						flags,		ver	*/

	{	"initialized",		PROP_INITIALIZED,	JSPROP_ENUMERATE|JSPROP_READONLY, 314 },
	{	"mode",				PROP_MODE,			JSPROP_ENUMERATE,	314 },
	{	"changes",			PROP_CHANGES,		JSPROP_ENUMERATE,	314 },
	{	"save_num",			PROP_SAVNUM,		JSPROP_ENUMERATE,	314 },
	{	"screen_length",	PROP_SCRN_LEN,		JSPROP_ENUMERATE,	314 },
	{	"screen_width",		PROP_SCRN_WIDTH,	JSPROP_ENUMERATE,	314 },
	{	"list_height",		PROP_LIST_HEIGHT,	JSPROP_ENUMERATE,	314 },
	{	"esc_delay",		PROP_ESC_DELAY,		JSPROP_ENUMERATE,	314 },
	{	"help_text",		PROP_HELPBUF,		JSPROP_ENUMERATE,	314 },
	{	"background_color",	PROP_BCOLOR,		JSPROP_ENUMERATE,	314 },
	{	"frame_color",		PROP_HCOLOR,		JSPROP_ENUMERATE,	314 },
	{	"text_color",		PROP_LCOLOR,		JSPROP_ENUMERATE,	314 },
	{	"inverse_color",	PROP_CCOLOR,		JSPROP_ENUMERATE,	314 },
	{	"lightbar_color",	PROP_LBCOLOR,		JSPROP_ENUMERATE,	314 },
	{0}
};

/* Convenience functions */
static uifcapi_t* get_uifc(JSContext *cx, JSObject *obj)
{
	uifcapi_t* uifc;

	if((uifc=(uifcapi_t*)JS_GetPrivate(cx,obj))==NULL)
		return(NULL);

	if(!uifc->initialized) {
		JS_ReportError(cx,"UIFC not initialized");
		return(NULL);
	}

	return(uifc);
}

/* Methods */

static JSBool
js_uifc_init(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	int		ciolib_mode=CIOLIB_MODE_AUTO;
	char*	title="Synchronet";
	char*	mode;
	uifcapi_t* uifc;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_FALSE);

	if((uifc=(uifcapi_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc) {
		JSVALUE_TO_STRING(cx, argv[0], title, NULL);
		if(title==NULL)
			return(JS_FALSE);
	}

	if(argc>1) {
		JSVALUE_TO_STRING(cx, argv[1], mode, NULL);
		if(mode != NULL) {
			if(!stricmp(mode,"STDIO"))
				ciolib_mode=-1;
			else if(!stricmp(mode,"AUTO"))
				ciolib_mode=CIOLIB_MODE_AUTO;
			else if(!stricmp(mode,"X"))
				ciolib_mode=CIOLIB_MODE_X;
			else if(!stricmp(mode,"ANSI"))
				ciolib_mode=CIOLIB_MODE_ANSI;
			else if(!stricmp(mode,"CONIO"))
				ciolib_mode=CIOLIB_MODE_CONIO;
		}
	}

	rc=JS_SUSPENDREQUEST(cx);
	if(ciolib_mode==-1) {
		if(uifcinix(uifc)) {
			JS_RESUMEREQUEST(cx, rc);
			return(JS_TRUE);
		}
	} else {
		if(initciolib(ciolib_mode)) {
			JS_RESUMEREQUEST(cx, rc);
			return(JS_TRUE);
		}

		if(uifcini32(uifc)) {
			JS_RESUMEREQUEST(cx, rc);
			return(JS_TRUE);
		}
	}

	JS_SET_RVAL(cx, arglist, JSVAL_TRUE);
	uifc->scrn(title);
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

static JSBool
js_uifc_bail(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	uifcapi_t* uifc;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((uifc=get_uifc(cx,obj))==NULL)
		return(JS_FALSE);

	rc=JS_SUSPENDREQUEST(cx);
	uifc->bail();
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

static JSBool
js_uifc_msg(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	char*		str;
	uifcapi_t*	uifc;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((uifc=get_uifc(cx,obj))==NULL)
		return(JS_FALSE);

	JSVALUE_TO_STRING(cx, argv[0], str, NULL);
	if(str==NULL)
		return(JS_FALSE);

	rc=JS_SUSPENDREQUEST(cx);
	uifc->msg(str);
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

static JSBool
js_uifc_pop(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	char*		str=NULL;
	uifcapi_t*	uifc;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((uifc=get_uifc(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc)
		JSVALUE_TO_STRING(cx, argv[0], str, NULL);

	rc=JS_SUSPENDREQUEST(cx);
	uifc->pop(str);
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

static JSBool
js_uifc_input(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	char*		str;
	char*		org=NULL;
	char*		prompt=NULL;
	int32		maxlen=0;
	int32		left=0;
	int32		top=0;
	int32		mode=0;
	int32		kmode=0;
	uifcapi_t*	uifc;
	uintN		argn=0;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((uifc=get_uifc(cx,obj))==NULL)
		return(JS_FALSE);

	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&mode))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&left))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&top))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_STRING(argv[argn])) {
		JSVALUE_TO_STRING(cx, argv[argn++], prompt, NULL);
		if(prompt==NULL)
			return(JS_FALSE);
	}
	if(argn<argc && JSVAL_IS_STRING(argv[argn])) {
		JSVALUE_TO_STRING(cx, argv[argn++], org, NULL);
		if(org==NULL)
			return(JS_FALSE);
	}
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&maxlen))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&kmode))
		return(JS_FALSE);

	if(!maxlen)
		maxlen=40;

	if((str=(char*)alloca(maxlen+1))==NULL)
		return(JS_FALSE);

	memset(str,0,maxlen+1);

	if(org)
		strncpy(str,org,maxlen);

	rc=JS_SUSPENDREQUEST(cx);
	if(uifc->input(mode, left, top, prompt, str, maxlen, kmode)<0) {
		JS_RESUMEREQUEST(cx, rc);
		return(JS_TRUE);
	}
	JS_RESUMEREQUEST(cx, rc);

	JS_SET_RVAL(cx, arglist, STRING_TO_JSVAL(JS_NewStringCopyZ(cx,str)));

	return(JS_TRUE);
}

static JSBool
js_uifc_list(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	char*		title=NULL;
	int32		left=0;
	int32		top=0;
	int32		width=0;
	int32		dflt=0;
	int32		bar=0;
	int32		mode=0;
	JSObject*	objarg;
	uifcapi_t*	uifc;
	uintN		argn=0;
	jsval		val;
	jsuint      i;
	jsuint		numopts;
	str_list_t	opts=NULL;
	char		*opt;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((uifc=get_uifc(cx,obj))==NULL)
		return(JS_FALSE);

	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&mode))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&left))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&top))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&width))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&dflt))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_NUMBER(argv[argn]) 
		&& !JS_ValueToInt32(cx,argv[argn++],&bar))
		return(JS_FALSE);
	if(argn<argc && JSVAL_IS_STRING(argv[argn])) {
		JSVALUE_TO_STRING(cx, argv[argn++], title, NULL);
		if(title==NULL)
			return(JS_FALSE);
	}
	if(argn<argc && JSVAL_IS_OBJECT(argv[argn])) {
		if((objarg = JSVAL_TO_OBJECT(argv[argn++]))==NULL)
			return(JS_FALSE);
		if(JS_IsArrayObject(cx, objarg)) {
			if(!JS_GetArrayLength(cx, objarg, &numopts))
				return(JS_TRUE);
			opts=strListInit();
			for(i=0;i<numopts;i++) {
				if(!JS_GetElement(cx, objarg, i, &val))
					break;
				JSVALUE_TO_STRING(cx, val, opt, NULL);
				strListPush(&opts,opt);
			}
		}
	}

	rc=JS_SUSPENDREQUEST(cx);
    JS_SET_RVAL(cx, arglist, INT_TO_JSVAL(uifc->list(mode,left,top,width,(int*)&dflt,(int*)&bar,title,opts)));
	strListFree(&opts);
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

/* Destructor */

static void 
js_finalize(JSContext *cx, JSObject *obj)
{
	uifcapi_t* p;

	if((p=(uifcapi_t*)JS_GetPrivate(cx,obj))==NULL)
		return;
	
	free(p);
	JS_SetPrivate(cx,obj,NULL);
}

static jsSyncMethodSpec js_functions[] = {
	{"init",            js_uifc_init,       1,	JSTYPE_BOOLEAN,	JSDOCSTR("string title")
	,JSDOCSTR("initialize")
	,314
	},		
	{"bail",			js_uifc_bail,		0,	JSTYPE_VOID,	JSDOCSTR("")
	,JSDOCSTR("uninitialize")
	,314
	},
	{"msg",				js_uifc_msg,		1,	JSTYPE_VOID,	JSDOCSTR("string text")
	,JSDOCSTR("print a message")
	,314
	},
	{"pop",				js_uifc_pop,		1,	JSTYPE_VOID,	JSDOCSTR("[string text]")
	,JSDOCSTR("popup (or down) a message")
	,314
	},
	{"input",			js_uifc_input,		0,	JSTYPE_STRING,	JSDOCSTR("[...]")
	,JSDOCSTR("prompt for a string input")
	,314
	},
	{"list",			js_uifc_list,		0,	JSTYPE_STRING,	JSDOCSTR("[...]")
	,JSDOCSTR("select from a list of options")
	,314
	},
	{0}
};

static JSBool js_uifc_resolve(JSContext *cx, JSObject *obj, jsid id)
{
	char*			name=NULL;

	if(id != JSID_VOID && id != JSID_EMPTY) {
		jsval idval;
		
		JS_IdToValue(cx, id, &idval);
		if(JSVAL_IS_STRING(idval))
			JSSTRING_TO_STRING(cx, JSVAL_TO_STRING(idval), name, NULL);
	}

	return(js_SyncResolve(cx, obj, name, js_properties, js_functions, NULL, 0));
}

static JSBool js_uifc_enumerate(JSContext *cx, JSObject *obj)
{
	return(js_uifc_resolve(cx, obj, JSID_VOID));
}

static JSClass js_uifc_class = {
     "UIFC"					/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_get					/* getProperty	*/
	,js_set					/* setProperty	*/
	,js_uifc_enumerate		/* enumerate	*/
	,js_uifc_resolve		/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,js_finalize			/* finalize		*/
};

JSObject* js_CreateUifcObject(JSContext* cx, JSObject* parent)
{
	JSObject*	obj;
	uifcapi_t*	api;

	if((obj = JS_DefineObject(cx, parent, "uifc", &js_uifc_class, NULL
		,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
		return(NULL);

	if((api=(uifcapi_t*)malloc(sizeof(uifcapi_t)))==NULL)
		return(NULL);

	memset(api,0,sizeof(uifcapi_t));
	api->size=sizeof(uifcapi_t);
	api->esc_delay=25;

	if(!JS_SetPrivate(cx, obj, api))	/* Store a pointer to uifcapi_t */
		return(NULL);

	return(obj);
}

