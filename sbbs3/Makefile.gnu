# Makefile.gnu

#########################################################################
# Makefile for Synchronet BBS 											#
# For use with GNU make and GNU C Compiler								#
# @format.tab-size 4, @format.use-tabs true								#
#																		#
# Linux: make -f Makefile.gnu											#
# Win32: make -f Makefile.gnu os=win32									#
# FreeBSD: make -f Makefile.gnu os=freebsd								#
#																		#
# Optional build targets: dlls, utils, mono, all (default)				#
#########################################################################

# $Id: Makefile.gnu,v 1.33 2002/01/24 12:08:47 rswindell Exp $

# Macros
DEBUG	=	1		# Comment out for release (non-debug) version
CC		=	gcc
SLASH	=	/
OFILE	=	o

ifeq ($(os),win32)	# Windows

LD		=	dllwrap
LIBFILE	=	.dll
EXEFILE	=	.exe
LIBODIR	:=	gcc.win32.dll
EXEODIR	:=	gcc.win32.exe
LIBDIR	:=	/gcc/i386-mingw32/lib
CFLAGS	:=	-mno-cygwin
LFLAGS  :=	--target=i386-mingw32 -mno-cygwin
DELETE	=	echo y | del 
OUTLIB	=	--output-lib
LIBS	=	$(LIBDIR)/libwsock32.a $(LIBDIR)/libwinmm.a

else	# Unix (begin)

LD		=	ld
LIBFILE	=	.a
EXEFILE	=	

ifeq ($(os),freebsd)	# FreeBSD
LIBODIR	:=	gcc.freebsd.lib
EXEODIR	:=	gcc.freebsd.exe
else                    # Linux
LIBODIR	:=	gcc.linux.lib
EXEODIR	:=	gcc.linux.exe
endif

LIBDIR	:=	/usr/lib
LFLAGS  :=	
DELETE	=	rm -f -v
OUTLIB	=	-o

CFLAGS	:=	-DJAVASCRIPT -I../mozilla/js/src

ifeq ($(os),freebsd)	# FreeBSD
CFLAGS	:=	$(CFLAGS) -pthread -D_THREAD_SAFE
LIBS	:=	-pthread
else			# Linux / Other UNIX
LIBS	:=	$(LIBDIR)/libpthread.a 
endif

endif   # Unix (end)

# Math library needed
LIBS	:=	$(LIBS) -lm

ifdef DEBUG
CFLAGS	:=	$(CFLAGS) -g -O0 -D_DEBUG 
LIBODIR	:=	$(LIBODIR).debug
EXEODIR	:=	$(EXEODIR).debug
ifeq ($(os),freebsd)	# FreeBSD
LIBS	:=	$(LIBS) ../mozilla/js/src/FreeBSD4.3-RELEASE_DBG.OBJ/libjs.a
else			# Linux
LIBS	:=	$(LIBS) ../mozilla/js/src/Linux_All_DBG.OBJ/libjs.a
endif
else # RELEASE
LFLAGS	:=	$(LFLAGS) -S
LIBODIR	:=	$(LIBODIR).release
EXEODIR	:=	$(EXEODIR).release
ifeq ($(os),freebsd)	# FreeBSD
LIBS	:=	$(LIBS) ../mozilla/js/src/FreeBSD4.3-RELEASE_OPT.OBJ/libjs.a
else
LIBS	:=	$(LIBS) ../mozilla/js/src/Linux_All_OPT.OBJ/libjs.a
endif
endif

include targets.mak		# defines all targets
include objects.mak		# defines $(OBJS)
include headers.mak		# defines $(HEADERS)
include sbbsdefs.mak	# defines $(SBBSDEFS)

SBBSLIB	=	$(LIBODIR)/sbbs.a
	

# Implicit C Compile Rule for SBBS
$(LIBODIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Implicit C++ Compile Rule for SBBS
$(LIBODIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Create output directories
$(LIBODIR):
	mkdir $(LIBODIR)

$(EXEODIR):
	mkdir $(EXEODIR)

# Monolithic Synchronet executable Build Rule
$(SBBSMONO): sbbscon.c conwrap.c $(OBJS) $(LIBODIR)/ver.o $(LIBODIR)/ftpsrvr.o $(LIBODIR)/mailsrvr.o $(LIBODIR)/mxlookup.o $(LIBODIR)/mime.o $(LIBODIR)/services.o
#	$(CC) $(CFLAGS) -o $(SBBSMONO) $^ $(LIBS)
	$(CC) -o $(SBBSMONO) $^ $(LIBS)

# Synchronet BBS library Link Rule
$(SBBS): $(OBJS) $(LIBODIR)/ver.o
	$(LD) $(LFLAGS) -o $(SBBS) $^ $(LIBS) $(OUTLIB) $(SBBSLIB)

# FTP Server Link Rule
$(FTPSRVR): $(LIBODIR)/ftpsrvr.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(OUTLIB) $(LIBODIR)/ftpsrvr.a

# Mail Server Link Rule
$(MAILSRVR): $(LIBODIR)/mailsrvr.o $(LIBODIR)/mxlookup.o $(LIBODIR)/mime.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(OUTLIB) $(LIBODIR)/mailsrvr.a

# Synchronet Console Build Rule
$(SBBSCON): sbbscon.c $(SBBSLIB)
	$(CC) $(CFLAGS) -o $@ $^

# Specifc Compile Rules
$(LIBODIR)/ftpsrvr.o: ftpsrvr.c ftpsrvr.h
	$(CC) $(CFLAGS) -c -DFTPSRVR_EXPORTS $< -o $@

$(LIBODIR)/mailsrvr.o: mailsrvr.c mailsrvr.h
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@

$(LIBODIR)/mxlookup.o: mxlookup.c
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@		

$(LIBODIR)/mime.o: mime.c
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@		

$(LIBODIR)/services.o: services.c services.h
	$(CC) $(CFLAGS) -c -DSERVICES_EXPORTS $< -o $@

# Baja Utility
$(BAJA): baja.c ars.c smbwrap.c crc32.c
	$(CC) $(CFLAGS) -o $@ $^

# Node Utility
$(NODE): node.c smbwrap.c
	$(CC) $(CFLAGS) -o $@ $^

# FIXSMB Utility
$(FIXSMB): fixsmb.c smblib.c smbwrap.c
	$(CC) $(CFLAGS) -o $@ $^

# CHKSMB Utility
$(CHKSMB): chksmb.c smblib.c smbwrap.c conwrap.c
	$(CC) $(CFLAGS) -o $@ $^

# SMB Utility
$(SMBUTIL): smbutil.c smblib.c smbwrap.c conwrap.c smbtxt.c crc32.c lzh.c 
	$(CC) $(CFLAGS) -o $@ $^


include depends.mak
