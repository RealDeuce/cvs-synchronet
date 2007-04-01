/* $Id: telnet_io.h,v 1.4 2005/06/13 00:28:15 rswindell Exp $ */

#ifndef _TELNET_IO_H_
#define _TELNET_IO_H_

extern uchar	telnet_local_option[0x100];
extern uchar	telnet_remote_option[0x100];

BYTE* telnet_interpret(BYTE* inbuf, int inlen, BYTE* outbuf, int *outlen);
BYTE* telnet_expand(BYTE* inbuf, size_t inlen, BYTE* outbuf, size_t *newlen);

#endif
