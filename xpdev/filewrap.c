/* filewrap.c */

/* File-related system-call wrappers */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2003 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/* OS-specific */
#if defined(__unix__)

#include <stdarg.h>		/* va_list */
#include <string.h>     /* strlen() */
#include <unistd.h>     /* getpid() */
#include <fcntl.h>      /* fcntl() file/record locking */
#include <sys/file.h>	/* L_SET for Solaris */
#include <errno.h>
#include <sys/param.h>	/* BSD */

#endif

/* ANSI */
#include <sys/types.h>	/* _dev_t */
#include <sys/stat.h>	/* struct stat */

#include "filewrap.h"	/* Verify prototypes */

/****************************************************************************/
/* Returns the modification time of the file in 'fd'						*/
/****************************************************************************/
time_t DLLCALL filetime(int fd)
{
	struct stat st;

	if(fstat(fd, &st)!=0)
		return(-1);

	return(st.st_mtime);
}

#if defined(__unix__) && !defined(__BORLANDC__)

/****************************************************************************/
/* Returns the length of the file in 'fd'									*/
/****************************************************************************/
long DLLCALL filelength(int fd)
{
	struct stat st;

	if(fstat(fd, &st)!=0)
		return(-1L);

	return(st.st_size);
}

/* Sets a lock on a portion of a file */
int DLLCALL lock(int fd, long pos, long len)
{
	#if defined(F_SANERDLCKNO) || !defined(BSD)
 		struct flock alock;

	#ifndef F_SANEWRLCKNO
		int	flags;
		if((flags=fcntl(fd,F_GETFL))==-1)
			return -1;

		if(flags==O_RDONLY)
			alock.l_type = F_RDLCK; /* set read lock to prevent writes */
		else
			alock.l_type = F_WRLCK; /* set write lock to prevent all access */
	#else
		alock.l_type = F_SANEWRLCKNO;
	#endif
		alock.l_whence = L_SET;		/* SEEK_SET */
		alock.l_start = pos;
		alock.l_len = (int)len;

		if(fcntl(fd, F_SETLK, &alock)==-1 && errno != EINVAL)
			return(-1);
	#endif

	#if !defined(F_SANEWRLCKNO) && !defined(__QNX__) && !defined(__solaris__)
		/* use flock (doesn't work over NFS) */
		if(flock(fd,LOCK_EX|LOCK_NB)!=0 && errno != EOPNOTSUPP)
			return(-1);
	#endif

		return(0);
}

/* Removes a lock from a file record */
int DLLCALL unlock(int fd, long pos, long len)
{

#if defined(F_SANEUNLCK) || !defined(BSD)
	struct flock alock;
#ifdef F_SANEUNLCK
	alock.l_type = F_SANEUNLCK;   /* remove the lock */
#else
	alock.l_type = F_UNLCK;   /* remove the lock */
#endif
	alock.l_whence = L_SET;
	alock.l_start = pos;
	alock.l_len = (int)len;
	if(fcntl(fd, F_SETLK, &alock)==-1 && errno != EINVAL)
		return(-1);
#endif

#if !defined(F_SANEUNLCK) && !defined(__QNX__) && !defined(__solaris__)
	/* use flock (doesn't work over NFS) */
	if(flock(fd,LOCK_UN|LOCK_NB)!=0 && errno != EOPNOTSUPP)
		return(-1);
#endif

	return(0);
}

/* Opens a file in specified sharing (file-locking) mode */
#if !defined(__QNX__)
int DLLCALL sopen(const char *fn, int access, int share, ...)
{
	int fd;
	int pmode=S_IREAD;
#ifndef F_SANEWRLCKNO
	int	flock_op=LOCK_NB;	/* non-blocking */
#endif
#if defined(F_SANEWRLCKNO) || !defined(BSD)
	struct flock alock;
#endif
    va_list ap;

    if(access&O_CREAT) {
        va_start(ap,share);
        pmode = va_arg(ap,unsigned int);
        va_end(ap);
    }

	if ((fd = open(fn, access, pmode)) < 0)
		return -1;

	if (share == SH_DENYNO) /* no lock needed */
		return fd;
#if defined(F_SANEWRLCKNO) || !defined(BSD)
	/* use fcntl (doesn't work correctly with threads) */
	alock.l_type = share;
	alock.l_whence = L_SET;
	alock.l_start = 0;
	alock.l_len = 0;       /* lock to EOF */

	if(fcntl(fd, F_SETLK, &alock)==-1 && errno != EINVAL) {	/* EINVAL means the file does not support locking */
		close(fd);
		return -1;
	}
#endif

#if !defined(F_SANEWRLCKNO) && !defined(__QNX__) && !defined(__solaris__)
	/* use flock (doesn't work over NFS) */
	if(share==SH_DENYRW)
		flock_op|=LOCK_EX;
	else   /* SH_DENYWR */
		flock_op|=LOCK_SH;
	if(flock(fd,flock_op)!=0 && errno != EOPNOTSUPP) { /* That object doesn't do locks */
		if(errno==EWOULDBLOCK) 
			errno=EAGAIN;
		close(fd);
		return(-1);
	}
#endif

	return fd;
}
#endif /* !QNX */

#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__DMC__)

#include <io.h>				/* tell */
#include <stdio.h>			/* SEEK_SET */
#include <sys/locking.h>	/* _locking */

/* Fix MinGW locking.h typo */
#if defined LK_UNLOCK && !defined LK_UNLCK
	#define LK_UNLCK LK_UNLOCK
#endif

int DLLCALL lock(int file, long offset, long size) 
{
	int	i;
	long	pos;
   
	pos=tell(file);
	if(offset!=pos)
		lseek(file, offset, SEEK_SET);
	i=_locking(file,LK_NBLCK,size);
	if(offset!=pos)
		lseek(file, pos, SEEK_SET);
	return(i);
}

int DLLCALL unlock(int file, long offset, long size)
{
	int	i;
	long	pos;
   
	pos=tell(file);
	if(offset!=pos)
		lseek(file, offset, SEEK_SET);
	i=_locking(file,LK_UNLCK,size);
	if(offset!=pos)
		lseek(file, pos, SEEK_SET);
	return(i);
}

#endif	/* !Unix && (MSVC || MinGW) */

#ifdef __unix__
FILE *_fsopen(char *pszFilename, char *pszMode, int shmode)
{
	int file;
	int Mode=0;
	char *p;
	
	for(p=pszMode;*p;p++)  {
		switch (*p)  {
			case 'r':
				Mode |= 1;
				break;
			case 'w':
				Mode |= 2;
				break;
			case 'a':
				Mode |= 4;
				break;
			case '+':
				Mode |= 8;
				break;
			case 'b':
			case 't':
				break;
			default:
				errno=EINVAL;
			return(NULL);
		}
	}
	switch(Mode)  {
		case 1:
			Mode=O_RDONLY;
			break;
		case 2:
			Mode=O_WRONLY|O_CREAT;
			break;
		case 4:
			Mode=O_APPEND|O_WRONLY|O_CREAT;
			break;
		case 9:
			Mode=O_RDWR;
			break;
		case 10:
			Mode=O_RDWR|O_CREAT;
			break;
		case 12:
			Mode=O_RDWR|O_APPEND|O_CREAT;
			break;
		default:
			errno=EINVAL;
			return(NULL);
	}
	if(Mode&O_CREAT)
		file=sopen(pszFilename,Mode,shmode,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	else
		file=sopen(pszFilename,Mode,shmode);
	if(file==-1)
		return(NULL);
	return(fdopen(file,pszMode));
}
#endif
