/* lzh.h */

/* Synchronet LZH compression library */

/* $Id: lzh.h,v 1.6 2000/10/26 15:06:40 rswindell Exp $ */

/**************************************************************************** 
 * @format.tab-size 4		(Plain Text/Source Code File Header)			* 
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Rob Swindell's conversion of 1988 LZH (LHarc) encoding functions			* 
 * Based on Japanese version 29-NOV-1988									* 
 * LZSS coded by Haruhiko Okumura											* 
 * Adaptive Huffman Coding coded by Haruyasu Yoshizaki						* 
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

#ifdef EXPORT32
	#undef EXPORT32
#endif

#ifdef _WIN32
	#ifndef __FLAT__
		#define __FLAT__
	#endif
	#ifdef __BORLANDC__
		#define LZHCALL __stdcall
	#else
		#define LZHCALL
	#endif
	#ifdef LZHDLL
		#define EXPORT32 __declspec( dllexport )
	#else
		#define EXPORT32 __declspec( dllimport )
	#endif
#else	/* !_WIN32 */
	#define LZHCALL
	#define EXPORT32
#endif

#ifndef uchar
#define uchar unsigned char
#endif

#ifdef __cplusplus
extern "C" {
#endif
EXPORT32 long LZHCALL lzh_encode(uchar *inbuf, long inlen, uchar *outbuf);
EXPORT32 long LZHCALL lzh_decode(uchar *inbuf, long inlen, uchar *outbuf);
#ifdef __cplusplus
}
#endif

#ifdef __WATCOMC__	/* Use MSC standard (prepended underscore) */
#pragma aux lzh_encode			"_*"
#pragma aux lzh_decode          "_*"
#endif
