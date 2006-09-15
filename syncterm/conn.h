/* $Id: conn.h,v 1.9 2006/08/30 20:45:24 deuce Exp $ */

#ifndef _CONN_H_
#define _CONN_H_

#include "sockwrap.h"

#include "bbslist.h"

extern SOCKET conn_socket;
extern char *conn_types[];
extern int conn_ports[];

enum {
	 CONN_TYPE_UNKNOWN
	,CONN_TYPE_RLOGIN
	,CONN_TYPE_TELNET
	,CONN_TYPE_RAW
#ifdef USE_CRYPTLIB
	,CONN_TYPE_SSH
#endif
	,CONN_TYPE_TERMINATOR
};

int conn_recv(char *buffer, size_t buflen, unsigned int timeout);
int conn_send(char *buffer, size_t buflen, unsigned int timeout);
int conn_connect(struct bbslist *bbs);
int conn_close(void);
BOOL conn_is_connected(void);
void conn_settype(int type);

#endif
