/* dirwrap.h */

/* Directory system-call wrappers */

/* $Id$ */

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

#ifndef _DIRWRAP_H
#define _DIRWRAP_H

#include "gen_defs.h"	/* ulong */

#if defined(DLLEXPORT)
	#undef DLLEXPORT
#endif
#if defined(DLLCALL)
	#undef DLLCALL
#endif

#if defined(_WIN32)
	#if defined(WRAPPER_DLL)
		#define DLLEXPORT	__declspec(dllexport)
	#else
		#define DLLEXPORT	__declspec(dllimport)
	#endif
	#if defined(__BORLANDC__)
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

/****************/
/* RTL-specific */
/****************/

#if defined(__unix__)

	#define ALLFILES "*"	/* matches all files in a directory */
	#include <glob.h>		/* POSIX.2 directory pattern matching function */
	#define MKDIR(dir)		mkdir(dir,0777)
	#define RMDIR(dir)		rmdir(dir)
	#define FULLPATH(a,r,l)	realpath(r,a)

#else	

	#define ALLFILES "*.*"	/* matches all files in a directory */
	#define MKDIR(dir)		_mkdir(dir)
	#define RMDIR(dir)		_rmdir(dir)
	#define FULLPATH(a,r,l)	_fullpath(a,r,l)

	/* glob-compatible findfirst/findnext wrapper */

	typedef struct 
	{
			size_t gl_pathc;			/* Count of paths matched so far */
			char **gl_pathv;			/* List of matched pathnames. */
			size_t gl_offs;				/* Slots to reserve in 'gl_pathv'. */
	} glob_t;

	/* Bits set in the FLAGS argument to `glob'.  */
	#define GLOB_ERR        (1 << 0)	/* Return on read errors.  */
	#define GLOB_MARK       (1 << 1)	/* Append a slash to each name.  */
	#define GLOB_NOSORT     (1 << 2)	/* Don't sort the names.  */
	#define GLOB_DOOFFS     (1 << 3)	/* Insert PGLOB->gl_offs NULLs.  */
	#define GLOB_NOCHECK    (1 << 4)	/* If nothing matches, return the pattern.  */
	#define GLOB_APPEND     (1 << 5)	/* Append to results of a previous call.  */
	#define GLOB_NOESCAPE   (1 << 6)	/* Backslashes don't quote metacharacters.  */
	#define GLOB_PERIOD     (1 << 7)	/* Leading `.' can be matched by metachars.  */
	#define GLOB_MAGCHAR    (1 << 8)	/* Set in gl_flags if any metachars seen.  */
	#define GLOB_ALTDIRFUNC (1 << 9)	/* Use gl_opendir et al functions.  */
	#define GLOB_BRACE      (1 << 10)	/* Expand "{a,b}" to "a" "b".  */
	#define GLOB_NOMAGIC    (1 << 11)	/* If no magic chars, return the pattern.  */
	#define GLOB_TILDE      (1 << 12)	/* Expand ~user and ~ to home directories. */
	#define GLOB_ONLYDIR    (1 << 13)	/* Match only directories.  */
	#define GLOB_TILDE_CHECK (1 << 14)	/* Like GLOB_TILDE but return an error
										  if the user name is not available.  */
	/* Error returns from `glob'.  */
	#define GLOB_NOSPACE    1       /* Ran out of memory.  */
	#define GLOB_ABORTED    2       /* Read error.  */
	#define GLOB_NOMATCH    3       /* No matches found.  */
	#define GLOB_NOSYS      4       /* Not implemented.  */

	DLLEXPORT int	DLLCALL	glob(const char *pattern, int flags, void* unused, glob_t*);
	DLLEXPORT void	DLLCALL globfree(glob_t*);

#endif

/*****************************/
/* POSIX Directory Functions */
/*****************************/
#if defined(_MSC_VER)
	/* dirent structure returned by readdir().
	 */
	struct dirent
	{	
		char        d_name[260];	/* filename */
	};

	/* DIR type returned by opendir().  The members of this structure
	 * must not be accessed by application programs.
	 */
	typedef struct
	{
		char				filespec[260];
		struct dirent		dirent;
		long				handle;
		struct _finddata_t	finddata;
		BOOL				end;		/* End of directory flag */
	} DIR;


	/* Prototypes.
	 */
	DIR            *	opendir  (const char *__dirname);
	struct dirent  *	readdir  (DIR *__dir);
	int                 closedir (DIR *__dir);
	void                rewinddir(DIR *__dir);
#else
	#include <dirent.h>	/* POSIX directory functions */
#endif


/**********/
/* Macros */
/**********/

/* POSIX readdir convenience macro */
#ifndef DIRENT
#define DIRENT struct dirent	
#endif

#if defined(__unix__)
#define BACKSLASH	'/'
#else /* MS-DOS based OS */
#define BACKSLASH	'\\'
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)

#define CHMOD(s,m)		_chmod(s,m)
#define PUTENV  		_putenv
#define GETCWD  		_getcwd

#elif defined(__BORLANDC__)

#define CHMOD(p,a)		_rtl_chmod(p,1,a) 	/* _chmod obsolete in 4.x */
#define PUTENV  		putenv
#define GETCWD  		getcwd

#else	/* ??? */

#define CHMOD(s,m)		chmod(s,m)
#define PUTENV  		putenv
#define GETCWD  		getcwd

#endif

/* General file system wrappers for all platforms and compilers */
DLLEXPORT BOOL		DLLCALL fexist(char *filespec);
DLLEXPORT long		DLLCALL flength(char *filename);
DLLEXPORT time_t	DLLCALL fdate(char *filename);
DLLEXPORT BOOL		DLLCALL	isdir(char *filename);
DLLEXPORT char*		DLLCALL getfname(char* path);
DLLEXPORT int		DLLCALL getfattr(char* filename);
DLLEXPORT ulong		DLLCALL getfreediskspace(char* path);

#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
