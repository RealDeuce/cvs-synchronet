/* gen_defs.h */

/* Synchronet general constant and macro definitions */

/* $Id: gen_defs.h,v 1.7 2000/11/02 05:02:26 rswindell Exp $ */

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

#ifndef _GEN_DEFS_H
#define _GEN_DEFS_H

									/* Control characters */
#define STX 	0x02				/* Start of text			^B	*/
#define ETX 	0x03				/* End of text				^C	*/
#define BEL		0x07				/* Bell/beep				^G	*/
#define FF		0x0c				/* Form feed				^L	*/
#define ESC 	0x1b				/* Escape					^[	*/
#define DEL		0x7f				/* Delete					^BS	*/
#define BS		'\b'				/* Back space				^H	*/
#define TAB 	'\t'				/* Horizontal tabulation	^I	*/
#define LF		'\n'				/* Line feed				^J	*/
#define CR		'\r'				/* Carriage return			^M	*/
#define SP		' '					/* Space						*/

#ifndef MAX_PATH
	#ifdef MAXPATHLEN
		#define MAX_PATH MAXPATHLEN	/* clib.h */
	#elif defined _MAX_PATH
		#define MAX_PATH _MAX_PATH
	#else
		#define MAX_PATH 260		
	#endif
#endif

/* Unsigned type short-hands	*/
#ifndef uchar
#define uchar	unsigned char
#endif
#ifdef __GLIBC__
	#include <sys/types.h>
#else
	#ifndef ushort
	#define ushort  unsigned short
	#define uint    unsigned int
	#define ulong   unsigned long
	#endif
#endif

/* Windows Types */
#ifndef BYTE
#define BYTE	uchar
#endif
#ifndef WORD
#define WORD	ushort
#endif
#ifndef DWORD
#define DWORD	ulong
#endif
#ifndef BOOL
#define BOOL	int
#endif
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif
#ifndef HANDLE
#define HANDLE	void*
#endif

/* Open flags */
#ifdef _WIN32

#ifndef O_DENYNONE
#define O_DENYNONE	OF_SHARE_DENY_NONE
#define O_DENYALL	OF_SHARE_EXCLUSIVE
#endif

#elif defined(__unix__)

#include <fcntl.h>

#define O_DENYNONE	0
#define O_DENYALL	0
#define O_BINARY	0
#define SH_DENYNO	2          // open() will *not* block
#define SH_DENYRW	F_WRLCK	   // blocks on read/write
#define SH_DENYRD   F_RDLCK	   // blocks on read
#define SH_DENYWR   F_WRLCK    // blocks on write (and read)

#endif

#ifdef _MSC_VER			/* Visual C++ */

#define S_IWRITE	_S_IWRITE

#endif

/****************************************************************************/
/* MALLOC/FREE Macros for various compilers and environments				*/
/* MALLOC is used for allocations of 64k or less							*/
/* FREE is used to free buffers allocated with MALLOC						*/
/* LMALLOC is used for allocations of possibly larger than 64k				*/
/* LFREE is used to free buffers allocated with LMALLOC 					*/
/* REALLOC is used to re-size a previously MALLOCed or LMALLOCed buffer 	*/
/* FAR16 is used to create a far (32-bit) pointer in 16-bit compilers		*/
/* HUGE16 is used to create a huge (32-bit) pointer in 16-bit compilers 	*/
/****************************************************************************/
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	#define HUGE16 huge
	#define FAR16 far
	#if defined(__TURBOC__)
		#define REALLOC(x,y) farrealloc(x,y)
		#define LMALLOC(x) farmalloc(x)
		#define MALLOC(x) farmalloc(x)
		#define LFREE(x) farfree(x)
		#define FREE(x) farfree(x)
	#elif defined(__WATCOMC__)
		#define REALLOC realloc
		#define LMALLOC(x) halloc(x,1)	/* far heap, but slow */
		#define MALLOC malloc			/* far heap, but 64k max */
		#define LFREE hfree
		#define FREE free
	#else	/* Other 16-bit Compiler */
		#define REALLOC realloc
		#define LMALLOC malloc
		#define MALLOC malloc
		#define LFREE free
		#define FREE free
	#endif
#else		/* 32-bit Compiler or Small Memory Model */
	#define HUGE16
	#define FAR16
	#define REALLOC realloc
	#define LMALLOC malloc
	#define MALLOC malloc
	#define LFREE free
	#define FREE free
#endif


#endif /* Don't add anything after this #endif statement */
