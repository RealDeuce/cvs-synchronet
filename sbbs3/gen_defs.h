/* gen_defs.h */

/* Synchronet general constant and macro definitions */

/* $Id: gen_defs.h,v 1.1 2000/10/10 11:24:42 rswindell Exp $ */

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
#define BS		0x08				/* Back space				^H	*/
#define TAB 	0x09				/* Horizontal tabulation	^I	*/
#define LF		0x0a				/* Line feed				^J	*/
#define FF		0x0c				/* Form feed				^L	*/
#define CR		0x0d				/* Carriage return			^M	*/
#define ESC 	0x1b				/* Escape					^[	*/
#define SP		0x20				/* Space						*/

									/* Unsigned type short-hands	*/
#define uchar	unsigned char
#define ushort  unsigned short
#define uint    unsigned int
#define ulong   unsigned long

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
