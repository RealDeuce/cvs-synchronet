/* qwk.h */

/* Synchronet QWK-related structures, constants, and prototypes */ 

/* $Id: qwk.h,v 1.1 2000/10/10 11:25:51 rswindell Exp $ */

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

typedef union {
	uchar	uc[10];
	ushort	ui[5];
	ulong	ul[2];
	float	f[2];
	double	d[1]; } converter;

#define LEN_QWKBUF	20000	/* 20k buffer for each message */
#define TAGLINE 	(1<<5)	/* Place tagline at end of qwk message */
#define TO_QNET 	(1<<6)	/* Sending to hub */
#define REP 		(1<<7)	/* It's a REP packet */
#define VIA 		(1<<8)	/* Include VIA */
#define TZ			(1<<9)	/* Include TZ */

float	ltomsbin(long val);
bool	route_circ(char *via, char *id);

