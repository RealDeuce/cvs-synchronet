# Makefile.gnu

#########################################################################
# Makefile for Synchronet BBS List										#
# For use with GNU make and GNU C Compiler								#
# @format.tab-size 4, @format.use-tabs true								#
#																		#
# Linux: make -f Makefile.gnu											#
# Win32: make -f Makefile.gnu os=win32									#
#########################################################################

# $Id: Makefile.gnu,v 1.1 2000/12/02 01:58:51 rswindell Exp $

# Macros
CC		=	gcc
LD		=	ld

ifeq ($(os),win32)	# Windows

EXEFILE	=	.exe
LIBDIR	:=	/gcc/i386-mingw32/lib
CFLAGS	:=	-mno-cygwin
LFLAGS  :=	--target=i386-mingw32 -mno-cygwin
DELETE	=	echo y | del 
LIBS	=	$(LIBDIR)/libwsock32.a

else	# Linux

EXEFILE	=	
LIBODIR	:=	gcc.linux.lib
EXEODIR	:=	gcc.linux.exe
LIBDIR	:=	/usr/lib
CFLAGS	:=	
LFLAGS  :=	
DELETE	=	rm -f -v
LIBS	=	$(LIBDIR)/libpthread.a

endif

CFLAGS	:=	$(CFLAGS) -I../sdk -I/src/sbbs3

SBL: sbl$(EXEFILE)

sbl$(EXEFILE) : sbl.c ../sdk/xsdk.c ../sdk/xsdkvars.c /src/sbbs3/smbwrap.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)