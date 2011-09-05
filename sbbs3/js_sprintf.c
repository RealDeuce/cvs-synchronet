/* js_sprintf.c */

/* Synchronet JavaScript "[s]printf" implementation */

/* $Id: js_sprintf.c,v 1.3 2006/12/21 21:27:19 deuce Exp $ */

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

#include "sbbs.h"
#include "xpprintf.h"	/* Hurrah for Deuce! */

char* DLLCALL
js_sprintf(JSContext *cx, uint argn, uintN argc, jsval *argv)
{
	char*		p;
    JSString*	str;

	if((p=js_ValueToStringBytes(cx, argv[argn++], NULL))==NULL)
		return(NULL);

	p=xp_asprintf_start(p);
    for(; argn<argc; argn++) {
		if(JSVAL_IS_DOUBLE(argv[argn]))
			p=xp_asprintf_next(p,XP_PRINTF_CONVERT|XP_PRINTF_TYPE_DOUBLE,*JSVAL_TO_DOUBLE(argv[argn]));
		else if(JSVAL_IS_INT(argv[argn]))
			p=xp_asprintf_next(p,XP_PRINTF_CONVERT|XP_PRINTF_TYPE_INT,JSVAL_TO_INT(argv[argn]));
		else if(JSVAL_IS_BOOLEAN(argv[argn]) && xp_printf_get_type(p)!=XP_PRINTF_TYPE_CHARP)
			p=xp_asprintf_next(p,XP_PRINTF_CONVERT|XP_PRINTF_TYPE_INT,JSVAL_TO_BOOLEAN(argv[argn]));
		else {
			if((str=JS_ValueToString(cx, argv[argn]))==NULL)
			    return(NULL);
			p=xp_asprintf_next(p,XP_PRINTF_CONVERT|XP_PRINTF_TYPE_CHARP,JS_GetStringBytes(str));
		}
	}

	return xp_asprintf_end(p, NULL);

}

void DLLCALL
js_sprintf_free(char* p)
{
	xp_asprintf_free(p);
}
