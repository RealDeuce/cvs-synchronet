/* genwrap.h */

/* General cross-platform development wrappers */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2002 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

#ifndef _GENWRAP_H
#define _GENWRAP_H

#include <stdio.h>		/* sprintf */
#include "gen_defs.h"	/* ulong */
#include "wrapdll.h"	/* DLLEXPORT and DLLCALL */

#if defined(__unix__)
	#include <unistd.h>	/* usleep */
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*********************/
/* Compiler-specific */
/*********************/

/* Compiler Description */
#if defined(__BORLANDC__)

	#define DESCRIBE_COMPILER(str) sprintf(str,"BCC %X.%02X" \
		,__BORLANDC__>>8,__BORLANDC__&0xff);	

#elif defined(_MSC_VER)

	#define DESCRIBE_COMPILER(str) sprintf(str,"MSC %u", _MSC_VER);

#elif defined(__GNUC__)

	#define DESCRIBE_COMPILER(str) sprintf(str,"GCC %u.%02u" \
		,__GNUC__,__GNUC_MINOR__);

#else /* Unknown compiler */

	#define DESCRIBE_COMPILER(str) strcpy(str,"UNKNOWN COMPILER");

#endif

/**********/
/* Macros */
/**********/

/* Target Platform Description */
#if defined(_WIN64)
	#define PLATFORM_DESC	"Win64"
#elif defined(_WIN32)
	#define PLATFORM_DESC	"Win32"
#elif defined(__OS2__)
	#define PLATFORM_DESC	"OS/2"
#elif defined(__MSDOS__)
	#define PLATFORM_DESC	"DOS"
#elif defined(__linux__)
	#define PLATFORM_DESC	"Linux"
#elif defined(__FreeBSD__)
	#define PLATFORM_DESC	"FreeBSD"
#elif defined(BSD)
	#define PLATFORM_DESC	"BSD"
#elif defined(__unix__)
	#define PLATFORM_DESC	"Unix"
#else
	#error "Need to describe target platform"
#endif

/*********************/
/* String Functionss */
/*********************/

#if defined(_MSC_VER) || defined(__MINGW32__)
	#define snprintf		_snprintf
	#define vsnprintf		_vsnprintf
#endif

#if !defined(_MSC_VER) && !defined(__BORLANDC__)
	DLLEXPORT char* DLLCALL ultoa(ulong, char*, int radix);
#endif

#if defined(__unix__)
	DLLEXPORT char*	DLLCALL strupr(char* str);
	DLLEXPORT char*	DLLCALL strlwr(char* str);
	DLLEXPORT char* DLLCALL	strrev(char* str);
	#if !defined(__BORLANDC__) && !defined(stricmp)
		#define stricmp(x,y)		strcasecmp(x,y)
		#define strnicmp(x,y,z)		strncasecmp(x,y,z)
	#endif
#endif

/****************************/
/* Common Utility Functions */
/****************************/

#if defined(_WIN32)

	#define SLEEP(x)		Sleep(x)
	#define BEEP(freq,dur)	Beep(freq,dur)

#elif defined(__OS2__)

	#define SLEEP(x)		DosSleep(x)
	#define BEEP(freq,dur)	DosBeep(freq,dur)

#elif defined(__unix__)

	#define SLEEP(x)		usleep(x*1000)
	#define BEEP(freq,dur)	unix_beep(freq,dur)
	DLLEXPORT void	DLLCALL	unix_beep(int freq, int dur);

#else	/* Unsupported OS */

	#error "Unsupported Target: Need some macros and/or function prototypes here."

#endif

DLLEXPORT int		DLLCALL	xp_random(int);
DLLEXPORT char*		DLLCALL os_version(char *str);

#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
