/* js_internal.c */

/* Synchronet "js" object, for internal JavaScript branch and GC control */

/* $Id$ */

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

#include "sbbs.h"

#include <jscntxt.h>	/* Needed for Context-private data structure */

enum {
	 PROP_TERMINATED
	,PROP_BRANCH_COUNTER
	,PROP_BRANCH_LIMIT
	,PROP_YIELD_INTERVAL
	,PROP_GC_INTERVAL
	,PROP_GC_ATTEMPTS
#ifdef jscntxt_h___
	,PROP_GC_COUNTER
	,PROP_GC_LASTBYTES
	,PROP_BYTES
	,PROP_MAXBYTES
#endif
};

static JSBool js_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint			tiny;
	js_branch_t*	branch;

	if((branch=(js_branch_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case PROP_TERMINATED:
			if(branch->terminated!=NULL)
				*vp=BOOLEAN_TO_JSVAL(*branch->terminated);
			break;
		case PROP_BRANCH_COUNTER:
			JS_NewNumberValue(cx,branch->counter,vp);
			break;
		case PROP_BRANCH_LIMIT:
			JS_NewNumberValue(cx,branch->limit,vp);
			break;
		case PROP_YIELD_INTERVAL:
			JS_NewNumberValue(cx,branch->yield_interval,vp);
			break;
		case PROP_GC_INTERVAL:
			JS_NewNumberValue(cx,branch->gc_interval,vp);
			break;
		case PROP_GC_ATTEMPTS:
			JS_NewNumberValue(cx,branch->gc_attempts,vp);
			break;
#ifdef jscntxt_h___
		case PROP_GC_COUNTER:
			JS_NewNumberValue(cx,cx->runtime->gcNumber,vp);
			break;
		case PROP_GC_LASTBYTES:
			JS_NewNumberValue(cx,cx->runtime->gcLastBytes,vp);
			break;
		case PROP_BYTES:
			JS_NewNumberValue(cx,cx->runtime->gcBytes,vp);
			break;
		case PROP_MAXBYTES:
			JS_NewNumberValue(cx,cx->runtime->gcMaxBytes,vp);
			break;
#endif
	}

	return(JS_TRUE);
}

static JSBool js_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint			tiny;
	js_branch_t*	branch;

	if((branch=(js_branch_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case PROP_TERMINATED:
			if(branch->terminated!=NULL)
				JS_ValueToBoolean(cx, *vp, branch->terminated);
			break;
		case PROP_BRANCH_COUNTER:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->counter);
			break;
		case PROP_BRANCH_LIMIT:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->limit);
			break;
		case PROP_GC_INTERVAL:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->gc_interval);
			break;
		case PROP_YIELD_INTERVAL:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->yield_interval);
			break;
#ifdef jscntxt_h___
		case PROP_MAXBYTES:
			JS_ValueToInt32(cx, *vp, (int32*)&cx->runtime->gcMaxBytes);
			break;
#endif
	}

	return(JS_TRUE);
}

#define RT_PROP_FLAGS	JSPROP_ENUMERATE|JSPROP_READONLY

static struct JSPropertySpec js_properties[] = {
/*		 name,				tinyid,						flags,				getter,	setter	*/

	{	"terminated",		PROP_TERMINATED,	JSPROP_ENUMERATE,	NULL,	NULL },
	{	"branch_counter",	PROP_BRANCH_COUNTER,JSPROP_ENUMERATE,	NULL,	NULL },
	{	"branch_limit",		PROP_BRANCH_LIMIT,	JSPROP_ENUMERATE,	NULL,	NULL },
	{	"yield_interval",	PROP_YIELD_INTERVAL,JSPROP_ENUMERATE,	NULL,	NULL },
	{	"gc_interval",		PROP_GC_INTERVAL,	JSPROP_ENUMERATE,	NULL,	NULL },
	{	"gc_attempts",		PROP_GC_ATTEMPTS,	RT_PROP_FLAGS,		NULL,	NULL },
#ifdef jscntxt_h___
	{	"gc_counter",		PROP_GC_COUNTER,	RT_PROP_FLAGS,		NULL,	NULL },
	{	"gc_last_bytes",	PROP_GC_LASTBYTES,	RT_PROP_FLAGS,		NULL,	NULL },
	{	"bytes",			PROP_BYTES,			RT_PROP_FLAGS,		NULL,	NULL },
	{	"max_bytes",		PROP_MAXBYTES,		JSPROP_ENUMERATE,	NULL,	NULL },
#endif
	{0}
};

#ifdef _DEBUG
static char* prop_desc[] = {
	 "termination has been requested (stop execution as soon as possible)"
	,"number of branch operations performed in this runtime"
	,"maximum number of branches, used for infinite-loop detection (0=disabled)"
	,"interval of periodic time-slice yields (lower number=higher frequency, 0=disabled)"
	,"interval of periodic garbage collection attempts (lower number=higher frequency, 0=disabled)"
	,"number of garbage collections attempted in this runtime - <small>READ ONLY</small>"
#ifdef jscntxt_h___
	,"number of garbage collections performed in this runtime - <small>READ ONLY</small>"
	,"number of heap bytes in use after last garbage collection - <small>READ ONLY</small>"
	,"number of heap bytes currently in use - <small>READ ONLY</small>"
	,"maximum number of bytes available for heap"
#endif
	,NULL
};
#endif

static JSBool
js_BranchCallback(JSContext *cx, JSScript *script)
{
	js_branch_t*	branch;

	if((branch=(js_branch_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	branch->counter++;

	/* Infinite loop? */
	if(branch->limit && branch->counter > branch->limit) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",branch->counter);
		branch->counter=0;
		return(JS_FALSE);
	}
	/* Give up timeslices every once in a while */
	if(branch->yield_interval && (branch->counter%branch->yield_interval)==0)
		YIELD();

	if(branch->gc_interval && (branch->counter%branch->gc_interval)==0)
		JS_MaybeGC(cx), branch->gc_attempts++;

	if(branch->terminated!=NULL && *branch->terminated)
		return(JS_FALSE);

    return(JS_TRUE);
}

/* Execute a string in its own context (away from Synchronet objects) */
static JSBool
js_eval(JSContext *parent_cx, JSObject *parent_obj, uintN argc, jsval *argv, jsval *rval)
{
	char*			buf;
    JSScript*		script;
	JSContext*		cx;
	JSObject*		obj;
	js_branch_t*	branch;
	JSErrorReporter	reporter;

	*rval=JSVAL_VOID;

	if(argc<1)
		return(JS_TRUE);

	if((branch=(js_branch_t*)JS_GetPrivate(parent_cx, parent_obj))==NULL)
		return(JS_FALSE);

	if((buf=JS_GetStringBytes(JS_ValueToString(parent_cx, argv[0])))==NULL)
		return(JS_FALSE);

	if((cx=JS_NewContext(JS_GetRuntime(parent_cx),JAVASCRIPT_CONTEXT_STACK))==NULL)
		return(JS_FALSE);

	/* Use the error reporter from the parent context */
#ifdef jscntxt_h___
	reporter=parent_cx->errorReporter;
#else
	JS_SetErrorReporter(parent_cx,(reporter=JS_SetErrorReporter(parent_cx,NULL)));
#endif
	JS_SetErrorReporter(cx,reporter);

#ifdef jscntxt_h___
	JS_SetBranchCallback(cx, parent_cx->branchCallback);
#else
	JS_SetContextPrivate(cx,branch);
	JS_SetBranchCallback(cx, js_BranchCallback);
#endif

	if((obj=JS_NewObject(cx, NULL, NULL, NULL))==NULL
		|| !JS_InitStandardClasses(cx,obj)) {
		JS_DestroyContext(cx);
		return(JS_FALSE);
	}

	if((script=JS_CompileScript(cx, obj, buf, strlen(buf), NULL, 0))!=NULL) {
		branch->counter=0;	/* Reset loop counter */
		JS_ExecuteScript(cx, obj, script, rval);
		JS_DestroyScript(cx, script);
	}

	JS_DestroyContext(cx);

    return(JS_TRUE);
}


static JSClass js_internal_class = {
     "JsInternal"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_get					/* getProperty	*/
	,js_set					/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

static jsMethodSpec js_functions[] = {
	{"eval",            js_eval,            0,	JSTYPE_STRING,	JSDOCSTR("string script")
	,JSDOCSTR("evaluate a JavaScript string in its own (secure) context, returning the result")
	},		
	{0}
};

JSObject* DLLCALL js_CreateInternalJsObject(JSContext* cx, JSObject* parent, js_branch_t* branch)
{
	JSObject*	obj;

	if((obj = JS_DefineObject(cx, parent, "js", &js_internal_class, NULL
		,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
		return(NULL);

	if(!JS_SetPrivate(cx, obj, branch))	/* Store a pointer to js_branch_t */
		return(NULL);

	if(!JS_DefineProperties(cx, obj, js_properties))	/* expose them */
		return(NULL);

	if (!js_DefineMethods(cx, obj, js_functions, /* append? */ FALSE)) 
		return(NULL);

#ifdef _DEBUG
	js_DescribeObject(cx,obj,"JavaScript execution and garbage collection control object");
	js_CreateArrayOfStrings(cx, obj, "_property_desc_list", prop_desc, JSPROP_READONLY);
#endif

	return(obj);
}
