/* js_console.cpp */

/* Synchronet JavaScript "Console" Object */

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

/*****************************/
/* Console Object Properites */
/*****************************/
enum {
	 CON_PROP_ONLINE
	,CON_PROP_STATUS
	,CON_PROP_LNCNTR 
	,CON_PROP_TOS
	,CON_PROP_ROWS
	,CON_PROP_AUTOTERM
	,CON_PROP_TIMEOUT			/* User inactivity timeout reference */
	,CON_PROP_TIMELEFT_WARN		/* low timeleft warning flag */
};

static JSBool js_console_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	ulong		val;
    jsint       tiny;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case CON_PROP_ONLINE:
			val=sbbs->online;
			break;
		case CON_PROP_STATUS:
			val=sbbs->console;
			break;
		case CON_PROP_LNCNTR:
			val=sbbs->lncntr;
			break;
		case CON_PROP_TOS:
			val=sbbs->tos;
			break;
		case CON_PROP_ROWS:
			val=sbbs->rows;
			break;
		case CON_PROP_AUTOTERM:
			val=sbbs->autoterm;
			break;
		case CON_PROP_TIMEOUT:
			val=sbbs->timeout;
			break;
		case CON_PROP_TIMELEFT_WARN:
			val=sbbs->timeleft_warn;
			break;
		default:
			return(JS_TRUE);
	}

	*vp = INT_TO_JSVAL(val);

	return(JS_TRUE);
}

static JSBool js_console_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	long		val;
    jsint       tiny;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	JS_ValueToInt32(cx, *vp, &val);

	switch(tiny) {
		case CON_PROP_ONLINE:
			sbbs->online=val;
			break;
		case CON_PROP_STATUS:
			sbbs->console=val;
			break;
		case CON_PROP_LNCNTR:
			sbbs->lncntr=val;
			break;
		case CON_PROP_TOS:
			sbbs->tos=val;
			break;
		case CON_PROP_ROWS:
			sbbs->rows=val;
			break;
		case CON_PROP_AUTOTERM:
			sbbs->autoterm=val;
			break;
		case CON_PROP_TIMEOUT:
			sbbs->timeout=val;
			break;
		case CON_PROP_TIMELEFT_WARN:
			sbbs->timeleft_warn=val;
			break;
		default:
			return(JS_TRUE);
	}

	return(JS_TRUE);
}

#define CON_PROP_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY

static struct JSPropertySpec js_console_properties[] = {
/*		 name				,tinyid					,flags			,getter,setter	*/

	{	"online"			,CON_PROP_ONLINE		,CON_PROP_FLAGS	,NULL,NULL},
	{	"status"			,CON_PROP_STATUS		,CON_PROP_FLAGS	,NULL,NULL},
	{	"line_counter"		,CON_PROP_LNCNTR 		,CON_PROP_FLAGS	,NULL,NULL},
	{	"top_of_screen"		,CON_PROP_TOS			,CON_PROP_FLAGS	,NULL,NULL},
	{	"rows"				,CON_PROP_ROWS			,CON_PROP_FLAGS	,NULL,NULL},
	{	"autoterm"			,CON_PROP_AUTOTERM		,CON_PROP_FLAGS	,NULL,NULL},
	{	"timeout"			,CON_PROP_TIMEOUT		,CON_PROP_FLAGS	,NULL,NULL},
	{	"timeleft_warning"	,CON_PROP_TIMELEFT_WARN	,CON_PROP_FLAGS	,NULL,NULL},
	{0}
};

/**************************/
/* Console Object Methods */
/**************************/

static JSBool
js_inkey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		key[2];
	long		mode=0;
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc && JSVAL_IS_INT(argv[0]))
		mode=JSVAL_TO_INT(argv[0]);
	key[0]=sbbs->inkey(mode);
	key[1]=0;

	js_str = JS_NewStringCopyZ(cx, key);
	*rval = STRING_TO_JSVAL(js_str);

    return(JS_TRUE);
}

static JSBool
js_getkey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		key[2];
	long		mode=0;
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc && JSVAL_IS_INT(argv[0]))
		mode=JSVAL_TO_INT(argv[0]);
	key[0]=sbbs->getkey(mode);
	key[1]=0;

	js_str = JS_NewStringCopyZ(cx, key);
	*rval = STRING_TO_JSVAL(js_str);

    return(JS_TRUE);
}

static JSBool
js_getstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		*p;
	long		mode=0;
	uintN		i;
	size_t		maxlen=0;
	sbbs_t*		sbbs;
    JSString*	js_str=NULL;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	for(i=0;i<argc;i++) {
		if(JSVAL_IS_INT(argv[i])) {
			if(!maxlen)
				maxlen=JSVAL_TO_INT(argv[i]);
			else
				mode=JSVAL_TO_INT(argv[i]);
			continue;
		}
		if(JSVAL_IS_STRING(argv[i])) {
			js_str = JS_ValueToString(cx, argv[i]);
			if (!js_str)
			    return(JS_FALSE);
		}
	}

	if(!maxlen) maxlen=128;

	if((p=(char *)calloc(1,maxlen+1))==NULL)
		return(JS_FALSE);

	if(js_str!=NULL)
		sprintf(p,"%.*s",maxlen,JS_GetStringBytes(js_str));

	sbbs->getstr(p,maxlen,mode);

	js_str = JS_NewStringCopyZ(cx, p);

	free(p);

	*rval = STRING_TO_JSVAL(js_str);

    return(JS_TRUE);
}

static JSBool
js_getnum(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ulong		maxnum=~0;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc && JSVAL_IS_INT(argv[0]))
		maxnum=JSVAL_TO_INT(argv[0]);

	*rval = INT_TO_JSVAL(sbbs->getnum(maxnum));

    return(JS_TRUE);
}

static JSBool
js_getkeys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		key[2];
	uintN		i;
	long		val;
	ulong		maxnum=~0;
	sbbs_t*		sbbs;
    JSString*	js_str=NULL;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	for(i=0;i<argc;i++) {
		if(JSVAL_IS_INT(argv[i])) {
			maxnum=JSVAL_TO_INT(argv[i]);
			continue;
		}
		if(JSVAL_IS_STRING(argv[i])) {
			js_str = JS_ValueToString(cx, argv[i]);
		}
	}
	if (!js_str)
		return(JS_FALSE);

	val=sbbs->getkeys(JS_GetStringBytes(js_str),maxnum);

	if(val==-1) {			// abort
		*rval = INT_TO_JSVAL(0);
	} else if(val<0) {		// number
		val&=~0x80000000;
		*rval = INT_TO_JSVAL(val);
	} else {				// key
		key[0]=(uchar)val;
		key[1]=0;
		js_str = JS_NewStringCopyZ(cx, key);
		*rval = STRING_TO_JSVAL(js_str);
	}

    return(JS_TRUE);
}

static JSBool
js_gettemplate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		str[128];
	long		mode=0;
	uintN		i;
	sbbs_t*		sbbs;
    JSString*	js_str=NULL;
    JSString*	js_fmt=NULL;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	for(i=0;i<argc;i++) {
		if(JSVAL_IS_STRING(argv[i])) {
			if(js_fmt==NULL)
				js_fmt = JS_ValueToString(cx, argv[i]);
			else
				js_str = JS_ValueToString(cx, argv[i]);
		} else if (JSVAL_IS_INT(argv[i]))
			mode=JSVAL_TO_INT(argv[i]);
	}

	if(js_fmt==NULL)
		return(JS_FALSE);

	if(js_str==NULL)
		str[0]=0;
	else
		sprintf(str, "%.*s", sizeof(str)-1, JS_GetStringBytes(js_str));

	sbbs->gettmplt(str,JS_GetStringBytes(js_fmt),mode);
	js_str = JS_NewStringCopyZ(cx, str);
	*rval = STRING_TO_JSVAL(js_str);

    return(JS_TRUE);
}

static JSBool
js_ungetstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);
	
	if((js_str=JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);

	p=JS_GetStringBytes(js_str);

	while(p && *p)
		sbbs->ungetkey(*(p++));
	
    return(JS_TRUE);
}

static JSBool
js_yesno(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);
	
	if((js_str=JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);

	*rval = BOOLEAN_TO_JSVAL(sbbs->yesno(JS_GetStringBytes(js_str)));

    return(JS_TRUE);
}

static JSBool
js_noyes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);
	
	if((js_str=JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);

	*rval = BOOLEAN_TO_JSVAL(sbbs->noyes(JS_GetStringBytes(js_str)));

    return(JS_TRUE);
}

static JSBool
js_mnemonics(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if((js_str=JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);
	
	sbbs->mnemonics(JS_GetStringBytes(js_str));

    return(JS_TRUE);
}

static JSBool
js_clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->CLS;

    return(JS_TRUE);
}

static JSBool
js_clearline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->clearline();

    return(JS_TRUE);
}

static JSBool
js_crlf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->outchar(CR);
	sbbs->outchar(LF);

    return(JS_TRUE);
}

static JSBool
js_attr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->attr(JSVAL_TO_INT(argv[0]));

    return(JS_TRUE);
}

static JSBool
js_pause(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->pause();

    return(JS_TRUE);
}

static JSBool
js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	sbbs->bputs(JS_GetStringBytes(str));

    return(JS_TRUE);
}

static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	sbbs->rputs(JS_GetStringBytes(str));

    return(JS_TRUE);
}

static JSBool
js_putmsg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	long		mode=0;
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	if(argc>1 && JSVAL_IS_INT(argv[1]))
		mode=JSVAL_TO_INT(argv[1]);

	sbbs->putmsg(JS_GetStringBytes(str),mode);

    return(JS_TRUE);
}

static JSBool
js_printfile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	long		mode=0;
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	if(argc>1 && JSVAL_IS_INT(argv[1]))
		mode=JSVAL_TO_INT(argv[1]);

	sbbs->printfile(JS_GetStringBytes(str),mode);

    return(JS_TRUE);
}

static JSBool
js_printtail(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			lines=0;
	long		mode=0;
	uintN		i;
	sbbs_t*		sbbs;
    JSString*	js_str=NULL;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	for(i=0;i<argc;i++) {
		if(JSVAL_IS_INT(argv[i])) {
			if(!lines)
				lines=JSVAL_TO_INT(argv[i]);
			else
				mode=JSVAL_TO_INT(argv[i]);
		} else if(JSVAL_IS_STRING(argv[i]))
			js_str = JS_ValueToString(cx, argv[i]);
	}

	if(js_str==NULL)
		return(JS_FALSE);

	if(!lines) 
		lines=5;

	sbbs->printtail(JS_GetStringBytes(js_str),lines,mode);

    return(JS_TRUE);
}

static JSBool
js_menu(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	sbbs->menu(JS_GetStringBytes(str));

    return(JS_TRUE);
}

static JSBool
js_uselect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	uintN		i;
	int			num=0;
	char*		title;
	char*		item;
	char*		ar_str;
	uchar*		ar;
	sbbs_t*		sbbs;
    JSString*	js_str;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);
	
	if(!argc) {
		*rval = INT_TO_JSVAL(sbbs->uselect(0,0,NULL,NULL,NULL));
		return(JS_TRUE);
	}
	
	for(i=0;i<argc;i++) {
		if(!JSVAL_IS_STRING(argv[i])) {
			num = JSVAL_TO_INT(argv[i]);
			continue;
		}
		if((js_str=JS_ValueToString(cx, argv[i]))==NULL)
			return(JS_FALSE);

		if(title==NULL) 
			title=JS_GetStringBytes(js_str);
		else if(item==NULL)
			item=JS_GetStringBytes(js_str);
		else {
			ar_str=JS_GetStringBytes(js_str);
			ar=arstr(NULL,ar_str,&sbbs->cfg);
		}
	}

	*rval = INT_TO_JSVAL(sbbs->uselect(1, num, title, item, ar));

    return(JS_TRUE);
}

static JSBool
js_center(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString*	str;
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	str = JS_ValueToString(cx, argv[0]);
	if (!str)
		return(JS_FALSE);

	sbbs->center(JS_GetStringBytes(str));

    return(JS_TRUE);
}

static JSBool
js_saveline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->slatr[sbbs->slcnt]=sbbs->latr; 
	sprintf(sbbs->slbuf[sbbs->slcnt<SAVE_LINES ? sbbs->slcnt++ : sbbs->slcnt] 
			,"%.*s",sbbs->lbuflen,sbbs->lbuf); 
	sbbs->lbuflen=0; 

    return(JS_TRUE);
}

static JSBool
js_restoreline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->lbuflen=0; 
	sbbs->attr(sbbs->slatr[--sbbs->slcnt]);
	sbbs->rputs(sbbs->slbuf[sbbs->slcnt]); 
	sbbs->curatr=LIGHTGRAY;

    return(JS_TRUE);
}

static JSBool
js_ansi_save(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->ANSI_SAVE();

    return(JS_TRUE);
}


static JSBool
js_ansi_restore(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->ANSI_RESTORE();

    return(JS_TRUE);
}

static JSBool
js_ansi_gotoxy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if(argc<2)
		return(JS_FALSE);

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->GOTOXY(JSVAL_TO_INT(argv[0]),JSVAL_TO_INT(argv[1]));

    return(JS_TRUE);
}

static JSBool
js_ansi_up(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc)
		sbbs->rprintf("\x1b[%dA",JSVAL_TO_INT(argv[0]));
	else
		sbbs->rputs("\x1b[A");

    return(JS_TRUE);
}

static JSBool
js_ansi_down(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc)
		sbbs->rprintf("\x1b[%dB",JSVAL_TO_INT(argv[0]));
	else
		sbbs->rputs("\x1b[B");

    return(JS_TRUE);
}

static JSBool
js_ansi_right(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc)
		sbbs->rprintf("\x1b[%dC",JSVAL_TO_INT(argv[0]));
	else
		sbbs->rputs("\x1b[C");

    return(JS_TRUE);
}

static JSBool
js_ansi_left(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(argc)
		sbbs->rprintf("\x1b[%dD",JSVAL_TO_INT(argv[0]));
	else
		sbbs->rputs("\x1b[D");

    return(JS_TRUE);
}

static JSBool
js_ansi_getlines(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	sbbs_t*		sbbs;

	if((sbbs=(sbbs_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	sbbs->ansi_getlines();

    return(JS_TRUE);
}

static JSFunctionSpec js_console_functions[] = {
	{"inkey",			js_inkey,			0},		// get key - no wait 
	{"getkey",			js_getkey,			0},		// get key - with wait 
	{"getstr",			js_getstr,			0},		// get string 
	{"getnum",			js_getnum,			0},		// get number 
	{"getkeys",			js_getkeys,			1},		// get one of a list of keys 
	{"gettemplate",		js_gettemplate,		1},		// get a string based on template 
	{"ungetstr",		js_ungetstr,		1},		// put a string in the keyboard buffer 
	{"yesno",			js_yesno,			1},		// Y/n question 
	{"noyes",			js_noyes,			1},		// N/y question 
	{"mnemonics",		js_mnemonics,		1},		// mnemonics input 
	{"clear",           js_clear,			0},		// clear screen 
	{"clearline",       js_clearline,		0},		// clear current line 
	{"crlf",            js_crlf,			0},		// output cr/lf 
	{"attr",			js_attr,			1},		// set current text attribute 
	{"pause",			js_pause,			0},		// pause 
	{"print",			js_print,			1},		// display a string (supports ^A and @-codes) 
	{"write",			js_write,			1},		// display a raw string 
	{"putmsg",			js_putmsg,			1},		// display message text (^A, @-codes, etc) with mode 
	{"center",			js_center,			1},		// display a string centered on the screen 
	{"printfile",		js_printfile,		1},		// print a file, optional mode
	{"printtail",		js_printtail,		2},		// print last x lines of file, optional mode
	{"menu",			js_menu,			1},		// print menu (auto-extension) 
	{"uselect",			js_uselect,			0},		// user selection menu
	{"saveline",		js_saveline,		0},		// save last output line 
	{"restoreline",		js_restoreline,		0},		// restore last output line 
	{"ansi_pushxy",		js_ansi_save,		0},
	{"ansi_popxy",		js_ansi_restore,	0},
	{"ansi_gotoxy",		js_ansi_gotoxy,		2},
	{"ansi_up",			js_ansi_up,			0},
	{"ansi_down",		js_ansi_down,		0},
	{"ansi_right",		js_ansi_right,		0},
	{"ansi_left",		js_ansi_left,		0},
	{"ansi_getlines",	js_ansi_getlines,	0},
	{0}
};


static JSClass js_console_class = {
     "Console"				/* name			*/
    ,0						/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_console_get		/* getProperty	*/
	,js_console_set		/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

JSObject* js_CreateConsoleObject(JSContext* cx, JSObject* parent)
{
	JSObject* obj;

	obj = JS_DefineObject(cx, parent, "console", &js_console_class, NULL, 0);

	if(obj==NULL)
		return(NULL);

	if(!JS_DefineProperties(cx, obj, js_console_properties))
		return(NULL);

	if (!JS_DefineFunctions(cx, obj, js_console_functions)) 
		return(NULL);

	return(obj);
}

#endif	/* JAVSCRIPT */