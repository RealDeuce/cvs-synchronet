/* js_branch.c */

/* Synchronet "branch" object, for JavaScript BranchCallback control */

/* $Id: js_branch.c,v 1.1 2003/07/08 00:24:57 rswindell Exp $ */

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

enum {
	 BRANCH_PROP_COUNTER
	,BRANCH_PROP_LIMIT
	,BRANCH_PROP_YIELD_FREQ
	,BRANCH_PROP_GC_FREQ
};

static JSBool js_branch_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint			tiny;
	js_branch_t*	branch;

	if((branch=(js_branch_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case BRANCH_PROP_COUNTER:
			JS_NewNumberValue(cx,branch->counter,vp);
			break;
		case BRANCH_PROP_LIMIT:
			JS_NewNumberValue(cx,branch->limit,vp);
			break;
		case BRANCH_PROP_GC_FREQ:
			JS_NewNumberValue(cx,branch->gc_freq,vp);
			break;
		case BRANCH_PROP_YIELD_FREQ:
			JS_NewNumberValue(cx,branch->yield_freq,vp);
			break;
	}

	return(JS_TRUE);
}

static JSBool js_branch_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint			tiny;
	js_branch_t*	branch;

	if((branch=(js_branch_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case BRANCH_PROP_COUNTER:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->counter);
			break;
		case BRANCH_PROP_LIMIT:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->limit);
			break;
		case BRANCH_PROP_GC_FREQ:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->gc_freq);
			break;
		case BRANCH_PROP_YIELD_FREQ:
			JS_ValueToInt32(cx, *vp, (int32*)&branch->yield_freq);
			break;
	}

	return(TRUE);
}


static struct JSPropertySpec js_branch_properties[] = {
/*		 name,			tinyid,				flags,				getter,	setter	*/

	{	"counter",		BRANCH_PROP_COUNTER,	JSPROP_ENUMERATE,	NULL,	NULL },
	{	"limit",		BRANCH_PROP_LIMIT,		JSPROP_ENUMERATE,	NULL,	NULL },
	{	"gc_freq",		BRANCH_PROP_GC_FREQ,	JSPROP_ENUMERATE,	NULL,	NULL },
	{	"yield_freq",	BRANCH_PROP_YIELD_FREQ,	JSPROP_ENUMERATE,	NULL,	NULL },
	{0}
};

#ifdef _DEBUG
static char* branch_prop_desc[] = {
	 "counter incremented for each branch"
	,"maximum number of branches, used for infinite-loop detection (0=disabled)"
	,"frequency of periodic garbage collection (0=disabled)"
	,"frequency of periodic time-slice yields (0=disabled)"
	,NULL
};
#endif


static JSClass js_branch_class = {
     "Branch"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_branch_get			/* getProperty	*/
	,js_branch_set			/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

JSObject* DLLCALL js_CreateBranchObject(JSContext* cx, JSObject* parent, js_branch_t* branch)
{
	JSObject*	obj;

	if((obj = JS_DefineObject(cx, parent, "branch", &js_branch_class, NULL
		,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
		return(NULL);

	if(!JS_SetPrivate(cx, obj, branch))	/* Store a pointer to js_branch_t */
		return(NULL);

	if(!JS_DefineProperties(cx, obj, js_branch_properties))	/* expose them */
		return(NULL);

#ifdef _DEBUG
	js_DescribeObject(cx,obj,"JavaScript execution branch control object");
	js_CreateArrayOfStrings(cx, obj, "_property_desc_list", branch_prop_desc, JSPROP_READONLY);
#endif

	return(obj);
}
