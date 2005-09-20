# Makefile.gnu

#########################################################################
# Makefile for Synchronet Blackjack										#
# For use with GNU make and GNU C Compiler								#
# @format.tab-size 4, @format.use-tabs true								#
#																		#
# Linux: make -f Makefile.gnu											#
# Win32: make -f Makefile.gnu os=win32									#
# FreeBSD: make -f Makefile.gnu os=freebsd								#
#########################################################################

# $Id: GNUmakefile,v 1.1 2005/09/20 07:49:10 deuce Exp $

XSDK		=	../sdk
SRC_ROOT	=	../../src
include $(SRC_ROOT)/build/Common.gmake

vpath %.c $(XSDK)

CFLAGS	+=	-I$(XSDK) -DUSE_XPDEV $(XPDEV_CFLAGS)

ifeq ($(os),sunos)
 LDFLAGS	+=	-lsocket -lnsl -lrt
endif

$(DPOKER): $(OBJS)
   ifndef bcc
	@echo Linking $@
   endif
	$(QUIET)$(CC) $(MT_LDFLAGS) $(XPDEV-MT_LDFLAGS) $(LDFLAGS) $^ -o $@ $(XPDEV-MT_LIBS)

$(DPCLEAN): $(DPCLEAN_OBJS)
   ifndef bcc
	@echo Linking $@
   endif
	$(QUIET)$(CC) $(CFLAGS) $(MT_LDFLAGS) $(XPDEV-MT_LDFLAGS) $(LDFLAGS) $^ -o $@ $(XPDEV-MT_LIBS)
