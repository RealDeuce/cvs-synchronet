/* scfg.h */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2002 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <stdio.h>
#include <stdlib.h>

#include "uifc.h"
#include "sbbs.h"

/**********/
/* Macros */
/**********/

#define SETHELP(where)  uifc.sethelp(where)

/*************/
/* Constants */
/*************/

#define SUB_HDRMOD	(1L<<31)		/* Modified sub-board header info */

/************/
/* Typedefs */
/************/

/********************/
/* Global Variables */
/********************/
extern scfg_t cfg;
extern uifcapi_t uifc;
extern char item;
extern char **opt;
extern char tmp[256];
extern char error[256];
extern char *nulstr;
extern char *invalid_code,*num_flags;
extern int	backup_level;

/***********************/
/* Function Prototypes */
/***********************/

void bail(int code);
void errormsg(int line, char *source,  char action, char *object, ulong access);
int  save_changes(int mode);
void rerun_nodes();
void node_menu();
void node_cfg();
void results(int i);
void sys_cfg();
void net_cfg();
void msgs_cfg();
void sub_cfg(uint grpnum);
void xfer_cfg();
void libs_cfg();
void dir_cfg(uint libnum);
void xprogs_cfg();
void fevents_cfg();
void tevents_cfg();
void xtrn_cfg();
void swap_cfg();
void xtrnsec_cfg();
void page_cfg();
void xedit_cfg();
void txt_cfg();
void shell_cfg();
void init_mdms();
void mdm_cfg(int mdmnum);
int export_mdm(char *fname);
char code_ok(char *str);
int  bits(long l);
char oneflag(long l);
void getar(char *desc, char *ar);
char *ultoac(ulong l,char *str);

/* scfgnet.h */
faddr_t atofaddr(char *str);
