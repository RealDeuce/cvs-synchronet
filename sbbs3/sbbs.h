/* sbbs.h */

/* Synchronet class (sbbs_t) definition and exported function prototypes */

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

#ifndef _SBBS_H
#define _SBBS_H

/****************************/
/* Standard library headers */
/****************************/

/***************/
/* OS-specific */
/***************/
#if defined(_WIN32)			/* Windows */

	#include <io.h>
	#include <share.h>
	#include <windows.h>
	#include <process.h>	/* _beginthread() prototype */
	#include <direct.h>		/* _mkdir() prototype */
	#include <mmsystem.h>	/* SND_ASYNC */

	#if defined(_DEBUG) && defined(_MSC_VER)
		#include <crtdbg.h> /* Windows debug macros and stuff */
	#endif

#elif defined(__unix__)		/* Unix-variant */

	#include <unistd.h>		/* close */

#endif

/******************/
/* ANSI C Library */
/******************/
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>			/* open */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef __FreeBSD__

	#include <malloc.h>

#endif

#include <sys/stat.h>

#ifdef JAVASCRIPT
	#ifdef __unix__
		#define XP_UNIX
	#else
		#define XP_PC
	#endif
	#define JS_THREADSAFE	/* required for multi-thread support */
	#include <jsapi.h>
	#include <jsprf.h>		/* JS-safe sprintf functions */

	#define	JAVASCRIPT_RUNTIME_MEMORY	(1*1024*1024)
	#define JAVASCRIPT_CONTEXT_STACK	(16*1024)
#endif

/***********************/
/* Synchronet-specific */
/***********************/
#include "sbbsinet.h"
#include "sbbswrap.h"
#include "smblib.h"
#include "smbwrap.h"
#include "ars_defs.h"
#include "scfgdefs.h"
#include "scfglib.h"
#include "userdat.h"
#include "riodefs.h"
#include "text.h"
#include "cmdshell.h"
#include "post.h"		/* post_t defintion */
#include "ringbuf.h"    /* RingBuf definition */
#include "client.h"		/* client_t definition */

/* Synchronet Node Instance class definition */
#ifdef __cplusplus
class sbbs_t
{

public:

	sbbs_t(ushort node_num, DWORD addr, char* host_name, SOCKET
		,scfg_t*, char* text[], client_t* client_info);
	~sbbs_t();

	bbs_startup_t*	startup;

	bool	init(void);
	bool	terminated;

	client_t client;
	SOCKET	client_socket;
	SOCKET	client_socket_dup;
	DWORD	client_addr;
	char	client_name[128];
	DWORD	local_addr;

	scfg_t	cfg;

	int		outchar_esc;		   // track ANSI escape seq output

	int 	rioctl(ushort action); // remote i/o control
	bool	rio_abortable;

    RingBuf	inbuf;
    RingBuf	outbuf;
	sem_t	output_sem;
	HANDLE	input_thread;
	pthread_mutex_t	input_thread_mutex;

	int 	outcom(uchar ch); 	   // send character
	int 	incom(void);		   // receive character

	void	spymsg(char *msg);		// send message to active spies

	void	putcom(char *str, int len=0);  // Send string
	void	hangup(void);		   // Hangup modem


    uchar	telnet_cmd[10];
    int		telnet_cmdlen;
	ulong	telnet_mode;
	uchar	telnet_last_rxch;

	bool	event_thread_running;
    bool	output_thread_running;
    bool	input_thread_running;

#ifdef JAVASCRIPT

	JSContext*	js_cx;
	JSObject*	js_glob;
	long		js_execfile(char *fname);
	bool		js_initcx(void);

#endif

	char 	menu_dir[128];	/* Over-ride default menu dir */
	char 	menu_file[128]; /* Over-ride menu file */

	user_t	useron; 		/* User currently online */
	node_t	thisnode;		/* Node information */
	smb_t	smb;			/* Currently open message base */
	char	rlogin_name[LEN_ALIAS+1];

	uint	temp_dirnum;

	FILE	*nodefile_fp,
			*node_ext_fp,
			*logfile_fp;

	int 	nodefile;		/* File handle for node.dab */
	int		node_ext;		/* File handle for node.exb */
	char 	cap_fname[41];	/* Capture filename - default is CAPTURE.TXT */
	FILE 	*capfile;		/* File string to use for capture file */
	int 	inputfile;		/* File handle to use for input */

							/* Batch download queue */
	char 	**batdn_name;	/* Filenames */
	ushort	*batdn_alt; 	/* Alternate path */
	uint 	*batdn_dir, 	/* Directory for each file */
			 batdn_total;	/* Total files */
	long 	*batdn_offset;	/* Offset for data */
	ulong	*batdn_size;	/* Size of file in bytes */
	ulong	*batdn_cdt; 	/* Credit value of file */

							/* Batch upload queue */
	char 	**batup_desc,	/* Description for each file */
			**batup_name;	/* Filenames */
	long	*batup_misc;	/* Miscellaneous bits */
	ushort	*batup_alt; 	/* Alternate path */
	uint 	*batup_dir, 	/* Directory for each file */
			batup_total;	/* Total files */

	/*********************************/
	/* Color Configuration Variables */
	/*********************************/
	char 	*text[TOTAL_TEXT];			/* Text from ctrl\text.dat */
	char 	*text_sav[TOTAL_TEXT];		/* Text from ctrl\text.dat */
	char 	dszlog[127];	/* DSZLOG enviornment variable */
	int		keybuftop,keybufbot;	/* Keyboard input buffer pointers */
	char 	keybuf[KEY_BUFSIZE];	/* Keyboard input buffer */
	char *	connection;		/* Connection Description */
	ulong	cur_rate;		/* Current Connection (DCE) Rate */
	ulong	cur_cps;		/* Current Average Transfer CPS */
	ulong	dte_rate;		/* Current COM Port (DTE) Rate */
	time_t 	timeout;		/* User inactivity timeout reference */
	ulong 	timeleft_warn;	/* low timeleft warning flag */
	uchar 	curatr; 		/* Current Text Attributes Always */
	long 	lncntr; 		/* Line Counter - for PAUSE */
	long 	tos;			/* Top of Screen */
	long 	rows;			/* Current Rows for User */
	long 	autoterm;		/* Autodetected terminal type */
	char 	slbuf[SAVE_LINES][LINE_BUFSIZE+1]; /* Saved for redisplay */
	char 	slatr[SAVE_LINES];	/* Starting attribute of each line */
	char 	slcnt;			/* Number of lines currently saved */
	char 	lbuf[LINE_BUFSIZE+1];/* Temp storage for each line output */
	int		lbuflen;		/* Number of characters in line buffer */
	char 	latr;			/* Starting attribute of line buffer */
	ulong	console;		/* Defines current Console settings */
	char 	wordwrap[81];	/* Word wrap buffer */
	time_t	now,			/* Used to store current time in Unix format */
			answertime, 	/* Time call was answered */
			logontime,		/* Time user logged on */
			starttime,		/* Time stamp to use for time left calcs */
			ns_time,		/* File new-scan time */
			last_ns_time;	/* Most recent new-file-scan this call */
	uchar 	action;			/* Current action of user */
	long 	online; 		/* Remote/Local or not online */
	long 	sys_status; 	/* System Status */
	ushort 	*sub_cfg;		/* User configuration for this sub-board */
	ulong	*sub_ptr;		/* Highest read message */
	ulong	*sub_last;		/* last read message pointer */
	ushort	*sav_sub_cfg;	/* Save cfg and ptrs for subs */
	ulong	*sav_sub_ptr;	/* for fast pointer update */
	ulong	*sav_sub_last;	/* last read message pointer */

	ulong	logon_ulb,		/* Upload Bytes This Call */
			logon_dlb,		/* Download Bytes This Call */
			logon_uls,		/* Uploads This Call */
			logon_dls,		/* Downloads This Call */
			logon_posts,	/* Posts This Call */
			logon_emails,	/* Emails This Call */
			logon_fbacks;	/* Feedbacks This Call */
	uchar	logon_ml;		/* ML of the user upon logon */

	uint 	main_cmds;		/* Number of Main Commands this call */
	uint 	xfer_cmds;		/* Number of Xfer Commands this call */
	ulong	posts_read; 	/* Number of Posts read this call */
	char 	temp_uler[31];  /* User who uploaded the files to temp dir */
	char 	temp_file[41];	/* Origin of extracted temp files */
	long 	temp_cdt;		/* Credit value of file that was extracted */
	char 	autohang;		/* Used for auto-hangup after transfer */
	size_t 	logcol; 		/* Current column of log file */
	uint 	criterrs; 		/* Critical error counter */

	uint 	curgrp, 		/* Current group */
			*cursub,		/* Current sub-board for each group */
			curlib, 		/* Current library */
			*curdir;		/* Current directory for each library */
	uint 	*usrgrp,		/* Real group numbers */
			usrgrps;		/* Number groups this user has access to */
	uint 	*usrlib,		/* Real library numbers */
			usrlibs;		/* Number of libs this user can access */
	uint 	**usrsub,		/* Real sub numbers */
			*usrsubs;		/* Num of subs with access for each grp */
	uint 	**usrdir,		/* Real dir numbers */
			*usrdirs;		/* Num of dirs with access for each lib */
	uint	cursubnum;		/* For ARS */
	uint	curdirnum;		/* For ARS */
	ulong 	timeleft;		/* Number of seconds user has left online */
	char	sbbsnode[MAX_PATH];	/* Environment var to contain node dir path */
	char	sbbsnnum[MAX_PATH];	/* Environment var to contain node num */
	char	sbbsctrl[MAX_PATH];	/* Environmetn var to contain ctrl dir path */
	char 	*comspec;		/* Pointer to environment variable COMSPEC */
	ushort	altul;			/* Upload to alternate path flag */
	char 	cid[LEN_CID+1]; /* Caller ID (IP Address) of current caller */
	char 	*noaccess_str;	/* Why access was denied via ARS */
	long 	noaccess_val;	/* Value of parameter not met in ARS */
	int		errorlevel; 	/* Error level of external program */

	csi_t	main_csi;		/* Main Command Shell Image */

			/* Global command shell variables */
	uint	global_str_vars;
	char **	global_str_var;
	long *	global_str_var_name;
	uint	global_int_vars;
	long *	global_int_var;
	long *	global_int_var_name;
	char *	sysvar_p[MAX_SYSVARS];
	uint	sysvar_pi;
	long	sysvar_l[MAX_SYSVARS];
	uint	sysvar_li;

    /* ansi_term.cpp */
	char *	ansi(int atr);			/* Returns ansi escape sequence for atr */
    void	ansi_getxy(int* x, int* y);
	void	ansi_getlines(void);

			/* Command Shell Methods */
	int		exec(csi_t *csi);
	int		exec_function(csi_t *csi);
	int		exec_misc(csi_t *csi, char *path);
	int		exec_net(csi_t *csi);
	int		exec_msg(csi_t *csi);
	int		exec_file(csi_t *csi);
	long	exec_bin(char *mod, csi_t *csi);
	void	clearvars(csi_t *bin);
	void	freevars(csi_t *bin);
	char**	getstrvar(csi_t *bin, long name);
	long*	getintvar(csi_t *bin, long name);
	char*	copystrvar(csi_t *csi, char *p, char *str);
	void	skipto(csi_t *csi, uchar inst);
	bool	ftp_cmd(csi_t* csi, SOCKET ctrl_sock, char* cmdsrc, char* rsp);
	bool	ftp_put(csi_t* csi, SOCKET ctrl_sock, char* src, char* dest);
	bool	ftp_get(csi_t* csi, SOCKET ctrl_sock, char* src, char* dest, bool dir=false);
	SOCKET	ftp_data_sock(csi_t* csi, SOCKET ctrl_sock, SOCKADDR_IN*);


	void	reset_logon_vars(void);

	uint	finduser(char *str);

	int 	sub_op(uint subnum);
	ulong	getlastmsg(uint subnum, ulong *ptr, time_t *t);
	time_t	getmsgtime(uint subnum, ulong ptr);
	ulong	getmsgnum(uint subnum, time_t t);

	int		dir_op(uint dirnum);
	int		getuserxfers(int fromuser, int destuser, char *fname);
	void	getnodeext(uint number, char * str);
	void	putnodeext(uint number, char * str);
	void	getnodedat(uint number, node_t * node, bool lock);
	void	putnodedat(uint number, node_t * node);
	void	nodesync(void);
	user_t	nodesync_user;
	bool	nodesync_inside;

	void	getmsgptrs(void);
	void	putmsgptrs(void);
	void	getusrsubs(void);
	void	getusrdirs(void);
	uint	userdatdupe(uint usernumber, uint offset, uint datlen, char *dat
				,bool del);
	void	gettimeleft(void);
	bool	gettimeleft_inside;

	/* str.cpp */
	char *	timestr(time_t* intime);  /* ASCII representation of time_t */
    char	timestr_output[60];
	void	userlist(char subonly);
	size_t	gettmplt(char *outstr, char *tmplt, long mode);
	void	sif(char *fname, char *answers, long len);	/* Synchronet Interface File */
	void	sof(char *fname, char *answers, long len);
	void	create_sif_dat(char *siffile, char *datfile);
	void	read_sif_dat(char *siffile, char *datfile);
	void	printnodedat(uint number, node_t* node);
	bool	inputnstime(time_t *dt);
	bool	chkpass(char *pass, user_t* user, bool unique);
	char *	cmdstr(char *instr, char *fpath, char *fspec, char *outstr);
	char	cmdstr_output[512];

	void	subinfo(uint subnum);
	void	dirinfo(uint dirnum);
	bool	trashcan(char *insearch, char *name);

	/* writemsg.cpp */
	void	automsg(void);
	bool	writemsg(char *str, char *top, char *title, long mode, int subnum
				,char *dest);
	char	putmsg(char HUGE16 *str, long mode);
	bool	msgabort(void);
	bool	email(int usernumber, char *top, char *title, long mode);
	void	forwardmail(smbmsg_t* msg, int usernum);
	bool	postmsg(uint subnum, smbmsg_t* msg, long wm_mode);
	void	removeline(char *str, char *str2, char num, char skip);
	ulong	msgeditor(char *buf, char *top, char *title);
	void	editfile(char *path);
	void	getsmsg(int usernumber);
	void	getnmsg(void);
	void	putnmsg(int num, char *strin);
	int		loadmsg(smbmsg_t *msg, ulong number);
	ushort	chmsgattr(ushort attr);
	void	show_msgattr(ushort attr);
	void	show_msghdr(smbmsg_t* msg);
	void	show_msg(smbmsg_t* msg, long mode);
	void	msgtotxt(smbmsg_t* msg, char *str, int header, int tails);
	void	quotemsg(smbmsg_t* msg, int tails);
	void	putmsg_fp(FILE *fp, long length, long mode);
	void	editmsg(smbmsg_t* msg, uint subnum);
	void	editor_inf(int xeditnum,char *dest, char *title, long mode
				,uint subnum);
	void	copyfattach(uint to, uint from, char *title);
	bool	movemsg(smbmsg_t* msg, uint subnum);

	/* mail.cpp */
	int		delmail(uint usernumber,int which);
	void	delfattach(uint to, char *title);
	void	telluser(smbmsg_t* msg);
	void	delallmail(uint usernumber);

	/* getmsg.cpp */
	post_t* loadposts(long *posts, uint subnum, ulong ptr, long mode);

	/* readmail.cpp */
	void	readmail(uint usernumber, int sent);

	/* bulkmail.cpp */
	void	bulkmail(uchar *ar);
	int		bulkmailhdr(uint usernum, smbmsg_t* msg, ushort msgattr, ulong offset
				,ulong length, char *title);

	/* con_out.cpp */
	int		bputs(char *str);				/* BBS puts function */
	int		rputs(char *str);				/* BBS raw puts function */
	int		bprintf(char *fmt, ...);		/* BBS printf function */
	int		rprintf(char *fmt, ...);		/* BBS raw printf function */
	void	outchar(char ch);				/* Output a char - check echo and emu.  */
	void	center(char *str);

	/* getstr.cpp */
	size_t	getstr(char *str, size_t length, long mode);
	long	getnum(ulong max);

	/* getkey.cpp */
	char	getkey(long mode); 		/* Waits for a key hit local or remote  */
	long	getkeys(char *str, ulong max);
	void	ungetkey(char ch);		/* Places 'ch' into the input buffer    */
	char	inkey(long mode);		/* Returns key if one has been hit		*/
	char	question[128];
	bool	yesno(char *str);
	bool	noyes(char *str);
	void	pause(void);
	char *	mnestr;
	void	mnemonics(char *str);

	/* prntfile.cpp */
	void	printfile(char *str, long mode);
	void	printtail(char *str, int lines, long mode);
	void	menu(char *code);

	int		uselect(int add, uint n, char *title, char *item, uchar *ar);
	uint	uselect_total, uselect_num[500];

	void	riosync(char abortable);
	bool	validattr(char a);
	int		stripattr(char *str);
	void	redrwstr(char *strin, int i, int l, long mode);
	void	attr(int atr);				/* Change local and remote text attributes */
	void	ctrl_a(char x);			/* Peforms the Ctrl-Ax attribute changes */
	void	clearline(void);

	/* atcodes.cpp */
	int		atcodes(char *code);
	int		syncatcodes(char *sp, int len);

	/* getnode.cpp */
	int		whos_online(bool listself);/* Lists active nodes, returns active nodes */

	/* logonoff.cpp */
	bool	answer();
	int		login(char *str, char *pw);
	bool	logon(void);
	void	logout(void);
	void	logoff(void);
	void	newuser(void);					/* Get new user							*/
	void	backout(void);

	/* readmsgs.cpp */
	int		scanposts(uint subnum, long mode, char *find);	/* Scan sub-board */
	int		searchsub(uint subnum, char *search);	/* Search for string on sub */
	int		searchsub_toyou(uint subnum);
	int		text_sec(void);						/* Text sections */
	void	listmsgs(int subnum, post_t * post, long i, long posts);
	void	msghdr(smbmsg_t* msg);
	int		searchposts(uint subnum, post_t * post, long start, long msgs
				,char *search);
	void	showposts_toyou(post_t * post, ulong start, long posts);

	/* chat.cpp */
	void	chatsection(void);
	void	multinodechat(int channel=1);
	void	nodepage(void);
	void	nodemsg(void);
	int		nodemsg_inside;
	uchar	lastnodemsg;	/* Number of node last message was sent to */
	char	lastnodemsguser[LEN_ALIAS+1];
	void	guruchat(char *line, char *guru, int gurunum);
	bool	guruexp(char **ptrptr, char *line);
	void	localguru(char *guru, int gurunum);
	bool	sysop_page(void);
	bool	guru_page(void);
	void	privchat(bool local=false);
	bool	chan_access(uint cnum);
	int		getnodetopage(int all, int telegram);

	/* main.cpp */
	void	printstatslog(uint node);
	ulong	logonstats(void);
	void	logoffstats(void);

	/* misc.cpp */
	int		nopen(char *str, int access);
	void	errormsg(int line, char *file, char action, char *object
				,ulong access, char *extinfo=NULL);
	int		mv(char *src, char *dest, char copy); /* fast file move/copy function */
	bool	chksyspass(void);
	bool	chk_ar(uchar * str, user_t * user); /* checks access requirements */
	bool	ar_exp(uchar ** ptrptr, user_t * user);

	/* upload.cpp */
	bool	uploadfile(file_t* f);
	char	sbbsfilename[128],sbbsfiledesc[128]; /* env vars */
	void	upload(uint dirnum);
    char	upload_lastdesc[LEN_FDESC+1];
	void	update_uldate(file_t* f);
	bool	bulkupload(uint dirnum);

	/* download.cpp */
	void	downloadfile(file_t* f);
	void	notdownloaded(ulong size, time_t start, time_t end);
	int		protocol(char *cmdline, int cd);
	void	seqwait(uint devnum);
	void	autohangup(void);
	bool	checkprotlog(file_t* f);

	/* file.cpp */
	void	fileinfo(file_t* f);
	void	openfile(file_t* f);
	void	closefile(file_t* f);
	bool	removefcdt(file_t* f);
	bool	movefile(file_t* f, int newdir);
	char *	getfilespec(char *str);
	uint	delfiles(char *path, char *spec);
	bool	checkfname(char *fname);
	bool	addtobatdl(file_t* f);

	/* listfile.cpp */
	bool	listfile(char *fname, char HUGE16 *buf, uint dirnum
				,char *search, char letter, ulong datoffset);
	int		listfiles(uint dirnum, char *filespec, int tofile, long mode);
	int		listfileinfo(uint dirnum, char *filespec, long mode);
	void	listfiletofile(char *fname, char HUGE16 *buf, uint dirnum, int file);
	int		batchflagprompt(uint dirnum, file_t bf[], uint total, long totalfiles);

	/* bat_xfer.cpp */
	void	batchmenu(void);
	void	batch_create_list(void);
	void	batch_add_list(char *list);
	bool	create_batchup_lst(void);
	bool	create_batchdn_lst(void);
	bool	create_bimodem_pth(void);
	void	batch_upload(void);
	void	batch_download(int xfrprot);
	void	start_batch_download(void);

	/* tmp_xfer.cpp */
	void	temp_xfer(void);
	void	extract(uint dirnum);
	char *	temp_cmd(void);					/* Returns temp file command line */
	ulong	create_filelist(char *name, long mode);

	/* viewfile.cpp */
	int		viewfile(file_t* f, int ext);
	void	viewfiles(uint dirnum, char *fspec);
	void	viewfilecontents(file_t* f);

	/* sortdir.cpp */
	void	resort(uint dirnum);

	/* xtrn.cpp */
	int		external(char* cmdline, long mode, char* startup_dir=NULL);

	/* xtrn_sec.cpp */
	int		xtrn_sec(void);					/* The external program section  */
	void	xtrndat(char* name, char* dropdir, uchar type, ulong tleft
				,ulong misc);
	bool	exec_xtrn(uint xtrnnum);			/* Executes online external program */
	bool	user_event(user_event_t);			/* Executes user event(s) */
	char	xtrn_access(uint xnum);			/* Does useron have access to xtrn? */
	void	moduserdat(uint xtrnnum);

	/* logio.cpp */
	void	logentry(char *code,char *entry);
	void	log(char *str);				/* Writes 'str' to node log */
	void	logch(char ch, bool comma);	/* Writes 'ch' to node log */
	void	logline(char *code,char *str); /* Writes 'str' on it's own line in log */
	void	logofflist(void);              /* List of users logon activity */
	void	errorlog(char *text);			/* Logs errors to ERROR.LOG and NODE.LOG */
	bool	errorlog_inside;
	bool	errormsg_inside;

	#if DEBUG
	void	dlog(int line, char *file, char *text);	/* Debug log file */
	#endif

	/* qwk.cpp */
	bool	qwklogon;
	time_t	qwkmail_time;
	void	qwk_sec(void);
	int		qwk_route(char *inaddr, char *fulladdr);
	void	update_qwkroute(char *via);
	void	qwk_success(ulong msgcnt, char bi, char prepack);
	void	qwksetptr(uint subnum, char *buf, int reset);
	void	qwkcfgline(char *buf,uint subnum);

	/* pack_qwk.cpp */
	bool	pack_qwk(char *packet, ulong *msgcnt, bool prepack);

	/* un_qwk.cpp */
	bool	unpack_qwk(char *packet,uint hubnum);

	/* pack_rep.cpp */
	bool	pack_rep(uint hubnum);

	/* un_rep.cpp */
	bool	unpack_rep(char* repfile=NULL);

	/* msgtoqwk.cpp */
	ulong	msgtoqwk(smbmsg_t* msg, FILE *qwk_fp, long mode, int subnum, int conf);

	/* qwktomsg.cpp */
	bool	qwktomsg(FILE *qwk_fp, char *hdrblk, char fromhub, uint subnum
				,uint touser);

	/* fido.cpp */
	bool	netmail(char *into, char *subj, long mode);
	void	qwktonetmail(FILE *rep, char *block, char *into, uchar fromhub);
	bool	lookup_netuser(char *into);

	bool	inetmail(char *into, char *subj, long mode);
	bool	qnetmail(char *into, char *subj, long mode);

	/* useredit.cpp */
	void	useredit(int usernumber);
	int		searchup(char *search,int usernum);
	int		searchdn(char *search,int usernum);
	void	maindflts(user_t* user);
	void	purgeuser(int usernumber);

	/* ver.cpp */
	void	ver(void);

	/* scansubs.cpp */
	void	scansubs(long mode);
	void	scanallsubs(long mode);
	void	new_scan_cfg(ulong misc);
	void	new_scan_ptr_cfg(void);

	/* scandirs.cpp */
	void	scanalldirs(long mode);
	void	scandirs(long mode);

	#define nosound()
	#define checkline()

	void	catsyslog(int crash);

	/* telgate.cpp */
	void	telnet_gate(char* addr, ulong mode);	// See TG_* for mode bits

};

#endif /* __cplusplus */

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

#ifdef __cplusplus
extern "C" {
#endif

	/* main.cpp */
	DLLEXPORT BOOL		DLLCALL getstats(scfg_t* cfg, char node, stats_t* stats);

	/* mail.cpp */
	DLLEXPORT int		DLLCALL getmail(scfg_t* cfg, int usernumber, BOOL sent);
	DLLEXPORT mail_t *	DLLCALL loadmail(smb_t* smb, ulong* msgs, uint usernumber
										,int which, long mode);
	DLLEXPORT void		DLLCALL freemail(mail_t* mail);

	/* filedat.c */
	DLLEXPORT BOOL		DLLCALL getfileixb(scfg_t* cfg, file_t* f);
	DLLEXPORT BOOL		DLLCALL getfiledat(scfg_t* cfg, file_t* f);
	DLLEXPORT BOOL		DLLCALL putfiledat(scfg_t* cfg, file_t* f);
	DLLEXPORT void		DLLCALL putextdesc(scfg_t* cfg, uint dirnum, ulong datoffset, char *ext);
	DLLEXPORT void		DLLCALL getextdesc(scfg_t* cfg, uint dirnum, ulong datoffset, char *ext);

	DLLEXPORT BOOL		DLLCALL removefiledat(scfg_t* cfg, file_t* f);
	DLLEXPORT BOOL		DLLCALL addfiledat(scfg_t* cfg, file_t* f);
	DLLEXPORT BOOL		DLLCALL findfile(scfg_t* cfg, uint dirnum, char *filename);
	DLLEXPORT char *	DLLCALL getfname(char *path);
	DLLEXPORT char *	DLLCALL padfname(char *filename, char *str);
	DLLEXPORT char *	DLLCALL unpadfname(char *filename, char *str);
	DLLEXPORT BOOL		DLLCALL rmuserxfers(scfg_t* cfg, int fromuser, int destuser, char *fname);

	DLLEXPORT ulong		DLLCALL	getposts(scfg_t* cfg, uint subnum);
	DLLEXPORT long		DLLCALL getfiles(scfg_t* cfg, uint dirnum);

	/* str.cpp */
	DLLEXPORT char *	DLLCALL ascii_str(uchar* str);
	DLLEXPORT ushort	DLLCALL crc16(char *str);
	DLLEXPORT BOOL		DLLCALL trashcan(scfg_t* cfg, char *insearch, char *name);
	DLLEXPORT int		DLLCALL putsmsg(scfg_t* cfg, int usernumber, char *strin);

	/* misc.c */
	DLLEXPORT char *	DLLCALL zonestr(short zone);
	DLLEXPORT char *	DLLCALL strip_ctrl(char *str);
	DLLEXPORT char *	DLLCALL strip_exascii(char *str);
	DLLEXPORT char *	DLLCALL prep_file_desc(char *str);

	/* load_cfg.c */
	DLLEXPORT BOOL		DLLCALL load_cfg(scfg_t* cfg, char* text[], BOOL prep);
	DLLEXPORT void		DLLCALL free_cfg(scfg_t* cfg);
	DLLEXPORT void		DLLCALL free_text(char* text[]);

	/* save_cfg.c */
	DLLEXPORT BOOL		DLLCALL save_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_node_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_main_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_msgs_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_file_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_chat_cfg(scfg_t* cfg, int backup_level);
	DLLEXPORT BOOL		DLLCALL write_xtrn_cfg(scfg_t* cfg, int backup_level);

	/* scfglib1.c */
	DLLEXPORT char *	DLLCALL prep_dir(char* base, char* dir);

	/* date_str.c */

	/* ASCII date (MM/DD/YY) to unix conversion */
	DLLEXPORT time_t	DLLCALL dstrtounix(scfg_t*, char *str);	
	 /* Unix time to ASCII date */
	DLLEXPORT char *	DLLCALL unixtodstr(scfg_t*, time_t, char *str);
	/* seconds to HH:MM:SS */
	DLLEXPORT char *	DLLCALL sectostr(uint sec, char *str);		

	/* logfile.cpp */
	DLLEXPORT BOOL		DLLCALL hacklog(scfg_t* cfg, char* prot, char* user, char* text, 
										char* host, SOCKADDR_IN* addr);
	DLLEXPORT BOOL		DLLCALL spamlog(scfg_t* cfg, char* reason, char* host, char* ip_addr);

	DLLEXPORT char *	DLLCALL remove_ctrl_a(char* instr, char* outstr);

#ifdef JAVASCRIPT

	/* js_global.c */
	DLLEXPORT JSObject* DLLCALL js_CreateGlobalObject(JSContext* cx, scfg_t* cfg);

	/* js_system.c */
	DLLEXPORT JSObject* DLLCALL js_CreateSystemObject(JSContext* cx, JSObject* parent, scfg_t* cfg);

	/* js_client.c */
	DLLEXPORT JSObject* DLLCALL js_CreateClientObject(JSContext* cx, JSObject* parent
													,char* name, client_t* client, SOCKET sock);
	/* js_user.c */
	DLLEXPORT JSObject*	DLLCALL js_CreateUserClass(JSContext* cx, JSObject* parent, scfg_t* cfg);
	DLLEXPORT JSObject* DLLCALL js_CreateUserObject(JSContext* cx, JSObject* parent, scfg_t* cfg
													,char* name, uint usernumber);

	/* js_file_area.c */
	DLLEXPORT JSObject* DLLCALL js_CreateFileAreaObject(JSContext* cx, JSObject* parent, scfg_t* cfg 
													,user_t* user, char* html_index_file);
	/* js_socket.c */
	DLLEXPORT JSObject* DLLCALL js_CreateSocketClass(JSContext* cx, JSObject* parent);
	DLLEXPORT JSObject* DLLCALL js_CreateSocketObject(JSContext* cx, JSObject* parent
													,char *name, SOCKET sock);

	/* js_file.c */
	DLLEXPORT JSObject* DLLCALL js_CreateFileClass(JSContext* cx, JSObject* parent);

	/* js_console.cpp */
	JSObject* js_CreateConsoleObject(JSContext* cx, JSObject* parent);

	/* js_bbs.cpp */
	JSObject* js_CreateBbsObject(JSContext* cx, JSObject* parent);

#endif

#ifdef SBBS /* These aren't exported */

	/* main.c */
	SOCKET	open_socket(int type);
	int		close_socket(SOCKET);
	BOOL	socket_check(SOCKET sock, BOOL *rd);
	u_long	resolve_ip(char *addr);


	/* misc.c */
	int		nopen(char *str, int access);
	FILE *	fnopen(int *file, char *str, int access);
	int		bstrlen(char *str);
	char *	ultoac(ulong l,char *str);
	void	truncsp(char *str);		/* Truncates white spaces off end of str */
	void	backslash(char *str);
	void	backslashcolon(char *str);
	ulong	crc32(char *buf, ulong len);
	void	ucrc16(uchar ch, ushort *rcrc);
	int		pstrcmp(char **str1, char **str2);  /* Compares pointers to pointers */
	int		strsame(char *str1, char *str2);	/* Compares number of same chars */
	ulong	ahtoul(char *str);	/* Converts ASCII hex to ulong */
	char *	hexplus(uint num, char *str); 	/* Hex plus for 3 digits up to 9000 */
	uint	hptoi(char *str);

	char *	readtext(long *line, FILE *stream);
	BOOL 	md(char *path);

	int 	eprintf(char *fmt, ...);	/* event log */
	int 	lprintf(char *fmt, ...);	/* telnet log */
	int 	lputs(char *);				/* telnet log */

	/* qwk.cpp */
	void	remove_re(char *str);

	/* ver.cpp */
	char*	os_version(char *str);
	char*	socklib_version(char* str);

	/* sortdir.cpp */
	int		fnamecmp_a(char **str1, char **str2);	 /* for use with resort() */
	int		fnamecmp_d(char **str1, char **str2);
	int		fdatecmp_a(uchar **buf1, uchar **buf2);
	int		fdatecmp_d(uchar **buf1, uchar **buf2);

	BOOL	filematch(char *filename, char *filespec);

	/* chat.cpp */
	void	packchatpass(char *pass, node_t* node);
	char *	unpackchatpass(char *pass, node_t* node);

#endif /* SBBS */

extern const char* wday[];	/* abbreviated weekday names */
extern const char* mon[];	/* abbreviated month names */
extern char lastuseron[LEN_ALIAS+1];  /* Name of user last online */

#ifdef __cplusplus
}
#endif

/* Global data */

#if defined(__FLAT__) || defined(_WIN32)

#define lread(f,b,l) read(f,b,l)
#define lfread(b,l,f) fread(b,l,f)
#define lwrite(f,b,l) write(f,b,l)
#define lfwrite(b,l,f) fwrite(b,l,f)

#define lkbrd(x)	0

#endif

#endif	/* Don't add anything after this line */
