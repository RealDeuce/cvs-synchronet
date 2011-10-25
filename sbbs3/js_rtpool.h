/* $Id: js_rtpool.h,v 1.4 2010/04/02 23:56:25 deuce Exp $ */

#ifndef _JS_RTPOOL_H_
#define _JS_RTPOOL_H_

#ifdef __unix__
	#define XP_UNIX
#else
	#define XP_PC
	#define XP_WIN
#endif
#include <jsapi.h>

#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#ifdef DLLCALL
#undef DLLCALL
#endif
#ifdef _WIN32
	#ifdef SBBS_EXPORTS
		#define DLLEXPORT	__declspec(dllexport)
	#else
		#define DLLEXPORT	__declspec(dllimport)
	#endif
	#ifdef __BORLANDC__
		#define DLLCALL __stdcall
	#else
		#define DLLCALL
	#endif
#else	/* !_WIN32 */
	#define DLLEXPORT
	#define DLLCALL
#endif

#if defined(__cplusplus)
extern "C" {
#endif
DLLEXPORT JSRuntime * DLLCALL jsrt_GetNew(int maxbytes, unsigned long timeout, const char *filename, long line);
DLLEXPORT void DLLCALL jsrt_Release(JSRuntime *);
DLLEXPORT void DLLCALL jsrt_TriggerAll(void);
#if defined(__cplusplus)
}
#endif

#endif
