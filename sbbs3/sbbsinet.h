/* sbbsinet.h */

/* Synchronet platform-specific Internet stuff */

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

#ifndef _SBBSINET_H
#define _SBBSINET_H

/***************/
/* OS-specific */
/***************/
#if defined _WIN32	|| defined __OS2__	/* Use WinSock */

#include <winsock.h>	/* socket/bind/etc. */

/* Let's agree on a standard WinSock symbol here, people */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_	
#endif

#elif defined __unix__	/* Unix-variant */

#include <netdb.h>		/* gethostbyname */
#ifdef __FreeBSD__
#include <sys/types.h>  /* For u_int32_t on FreeBSD */
#endif
#include <netinet/in.h>	/* IPPROTO_IP */
#include <sys/socket.h>	/* socket/bind/etc. */
#include <sys/ioctl.h>	/* FIONBIO */
#include <sys/time.h>	/* struct timeval */
#include <arpa/inet.h>	/* inet_ntoa */
#include <unistd.h>		/* close */

#endif

#include <errno.h>		/* errno */

/**********************************/
/* Socket Implementation-specific */
/**********************************/
#ifdef _WINSOCKAPI_

#undef  EINTR
#define EINTR			(WSAEINTR-WSABASEERR)
#undef  ENOTSOCK
#define ENOTSOCK		(WSAENOTSOCK-WSABASEERR)
#undef  EWOULDBLOCK
#define EWOULDBLOCK		(WSAEWOULDBLOCK-WSABASEERR)
#undef  ECONNRESET
#define ECONNRESET		(WSAECONNRESET-WSABASEERR)
#undef  ESHUTDOWN
#define ESHUTDOWN		(WSAESHUTDOWN-WSABASEERR)
#undef  ECONNABORTED
#define ECONNABORTED	(WSAECONNABORTED-WSABASEERR)

#define s_addr			S_un.S_addr

#define socklen_t		int

static  wsa_error;
#define ERROR_VALUE		((wsa_error=WSAGetLastError())>0 ? wsa_error-WSABASEERR : wsa_error)
#define sendsocket(s,b,l)	send(s,b,l,0)

#else	/* BSD sockets */

/* WinSock-isms */
#define HOSTENT			struct hostent
#define SOCKADDR_IN		struct sockaddr_in
#define LINGER			struct linger
#define SOCKET			int
#define SOCKET_ERROR	-1
#define INVALID_SOCKET  (SOCKET)(~0)
#define closesocket		close
#define ioctlsocket		ioctl
#define ERROR_VALUE		errno
#define sendsocket		write		// FreeBSD send() is broken

#endif	/* __unix__ */

#ifndef SHUT_RDWR
#define SHUT_RDWR		2	/* for shutdown() */
#endif

#ifndef IPPORT_FTP
#define IPPORT_FTP		21
#endif
#ifndef IPPORT_TELNET
#define IPPORT_TELNET	23
#endif
#ifndef IPPORT_SMTP
#define IPPORT_SMTP		25
#endif
#ifndef IPPORT_POP3
#define IPPORT_POP3		110
#endif

#endif	/* Don't add anything after this line */
