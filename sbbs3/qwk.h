/* qwk.h */

/* Synchronet QWK-related structures, constants, and prototypes */ 

/* $Id: qwk.h,v 1.3 2002/08/06 02:42:55 rswindell Exp $ */

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

#define QWK_NEWLINE		'\xe3'	/* QWK line terminator */
#define QWK_BLOCK_LEN	128

/* QWK mode bits */
#define TAGLINE 	(1<<5)	/* Place tagline at end of qwk message */
#define TO_QNET 	(1<<6)	/* Sending to hub */
#define REP 		(1<<7)	/* It's a REP packet */
#define VIA 		(1<<8)	/* Include VIA */
#define TZ			(1<<9)	/* Include TZ */
#define MSGID		(1<<10)	/* Include MSGID and REPLY */

float	ltomsbin(long val);
bool	route_circ(char *via, char *id);

