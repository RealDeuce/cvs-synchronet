/* FTPSRVR.H */

/* Synchronet FTP server */

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

#ifndef _FTPSRVR_H_
#define _FTPSRVR_H_

#include "client.h"				/* client_t */

typedef struct {

	DWORD	size;				/* sizeof(ftp_startup_t) */
	WORD	port;
	WORD	max_clients;
	WORD	max_inactivity;
	WORD	qwk_timeout;
	WORD	reserved_word4;
	WORD	reserved_word3;
	WORD	reserved_word2;
	WORD	reserved_word1;
    DWORD   interface_addr;
    DWORD	options;			/* See FTP_OPT definitions */
    DWORD	reserved_dword8;
    DWORD	reserved_dword7;
    DWORD	reserved_dword6;
    DWORD	reserved_dword5;
    DWORD	reserved_dword4;
    DWORD	reserved_dword3;
    DWORD	reserved_dword2;
    DWORD	reserved_dword1;
	int 	(*lputs)(char*);
	void	(*status)(char*);
    void	(*started)(void);
    void	(*terminated)(int code);
    void	(*clients)(int active);
    void	(*thread_up)(BOOL up);
	void	(*socket_open)(BOOL open);
    void	(*client_on)(BOOL on, int sock, client_t*);
    void	(*reserved_fptr4)(void);
    void	(*reserved_fptr3)(void);
    void	(*reserved_fptr2)(void);
    void	(*reserved_fptr1)(void);
    char    ctrl_dir[128];
    char	index_file_name[64];
    char	html_index_file[64];
    char	html_index_script[64];
    char	reserved_path8[64];
    char	reserved_path7[128];
    char	reserved_path6[128];
    char	reserved_path5[128];
    char	reserved_path4[128];
    char	reserved_path3[128];
	char	answer_sound[128];
	char	hangup_sound[128];
    char	reserved_path2[128];
    char	reserved_path1[128];

} ftp_startup_t;

#define FTP_OPT_DEBUG_RX			(1<<0)
#define FTP_OPT_DEBUG_DATA			(1<<1)
#define FTP_OPT_INDEX_FILE			(1<<2)	/* Auto-generate ASCII Index files */
#define FTP_OPT_DEBUG_TX			(1<<3)
#define FTP_OPT_ALLOW_QWK			(1<<4)
#define FTP_OPT_NO_LOCAL_FSYS		(1<<5)
#define FTP_OPT_DIR_FILES			(1<<6)	/* Allow access to files in dir but not in database */
#define FTP_OPT_KEEP_TEMP_FILES		(1<<7)	/* Don't delete temp files (for debugging) */
#define FTP_OPT_HTML_INDEX_FILE		(1<<8)	/* Auto-generate HTML index files */
#define FTP_OPT_NO_HOST_LOOKUP		(1<<11)
#define FTP_OPT_LOCAL_TIMEZONE		(1<<30)	/* Don't force UCT/GMT */
#define FTP_OPT_MUTE				(1<<31)

#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#ifdef DLLCALL
#undef DLLCALL
#endif

#ifdef _WIN32
	#ifdef FTPSRVR_EXPORTS
		#define DLLEXPORT __declspec(dllexport)
	#else
		#define DLLEXPORT __declspec(dllimport)
	#endif
	#ifdef __BORLANDC__
		#define DLLCALL __stdcall
	#else
		#define DLLCALL
	#endif
#else
	#define DLLEXPORT
	#define DLLCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* arg is pointer to static ftp_startup_t */
DLLEXPORT void	DLLCALL ftp_server(void* arg);
DLLEXPORT void	DLLCALL ftp_terminate(void);
DLLEXPORT char*	DLLCALL ftp_ver(void);
#ifdef __cplusplus
}
#endif

#endif /* Don't add anything after this line */