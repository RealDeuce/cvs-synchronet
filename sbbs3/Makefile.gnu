# Makefile.gnu

#########################################################################
# Makefile for Synchronet BBS 											#
# For use with GNU make and GNU C Compiler								#
# @format.tab-size 4													#
#########################################################################

# $Id: Makefile.gnu,v 1.5 2000/10/26 11:43:19 rswindell Exp $

# Macros
DEBUG	=	1		# Comment out for release (non-debug) version
CC		=	gcc
LD		=	dllwrap
SLASH	=	/
OFILE	=	o
ODIR	:=	gcc.win32.dll
LIBDIR	:=	/gcc/i386-mingw32/lib
CFLAGS	:=	-mno-cygwin
LFLAGS  :=	--target=i386-mingw32 -mno-cygwin

ifdef DEBUG
CFLAGS	:=	$(CFLAGS) -g -O0 -D_DEBUG 
ODIR	:=	$(ODIR).debug
else
LFLAGS	:=	$(LFLAGS) -S
ODIR	:=	$(ODIR).release
endif

SBBS	=	$(ODIR)/sbbs.dll

FTPSRVR	=	$(ODIR)/ftpsrvr.dll

MAILSRVR=	$(ODIR)/mailsrvr.dll

SBBSLIB	=	$(ODIR)/sbbs.a

ALL: $(ODIR) $(SBBS) $(FTPSRVR) $(MAILSRVR)

include objects.mak		# defines $(OBJS)
include headers.mak		# defines $(HEADERS)
include sbbsdefs.mak	# defines $(SBBSDEFS)
	
LIBS	=	$(LIBDIR)/libwsock32.a $(LIBDIR)/libwinmm.a

# Implicit C Compile Rule for SBBS.DLL
$(ODIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Implicit C++ Compile Rule for SBBS.DLL
$(ODIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Create output directory
$(ODIR):
	mkdir $(ODIR)

# SBBS DLL Link Rule
$(SBBS): $(OBJS) $(ODIR)/ver.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) --output-lib $(SBBSLIB)

# FTP Server DLL Link Rule
$(FTPSRVR): $(ODIR)/ftpsrvr.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) --output-lib $(ODIR)/ftpsrvr.a

# Mail Server DLL Link Rule
$(MAILSRVR): $(ODIR)/mailsrvr.o $(ODIR)/mxlookup.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) --output-lib $(ODIR)/mailsrvr.a

# Specifc Compile Rules
$(ODIR)/ftpsrvr.o: ftpsrvr.c ftpsrvr.h
	$(CC) $(CFLAGS) -c -DFTPSRVR_EXPORTS $< -o $@

$(ODIR)/mailsrvr.o: mailsrvr.c mailsrvr.h
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@

$(ODIR)/mxlookup.o: mxlookup.c
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@		

include depends.mak