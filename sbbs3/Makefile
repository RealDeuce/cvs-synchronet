# Makefile

#########################################################################
# Makefile for Synchronet BBS 											#
# For use with Borland C++ Builder 5+ or Borland C++ 5.5 for Win32      #
# @format.tab-size 4													#
#																		#
# usage: make															#
#																		#
# Optional build targets: dlls, utils, mono, all (default)				#
#########################################################################

# $Id$

# Macros
DEBUG	=	1				# Comment out for release (non-debug) version
JS		=	1				# Comment out for non-JavaScript (v3.00) build
CC		=	bcc32
LD		=	ilink32
SLASH	=	\\
OFILE	=	obj
LIBFILE	=	.dll
EXEFILE	=	.exe
LIBODIR	=	bcc.win32.dll	# Library output directory
EXEODIR =	bcc.win32.exe	# Executable output directory
UIFC	=	..\uifc\		# Path to User Interfce library
XPDEV	=	..\xpdev\		# Path to Cross-platform wrappers
CFLAGS	=	-M -I$(XPDEV) -I$(UIFC)
LFLAGS  =	-m -s -c -Tpd -Gi -I$(LIBODIR)
DELETE	=	echo y | del 

# Optional compile flags (disable banner, warnings and such)
CFLAGS	=	$(CFLAGS) -q -d -H -X- -w-csu -w-pch -w-ccc -w-rch -w-par

# Debug or release build?
!ifdef DEBUG
CFLAGS	=	$(CFLAGS) -v -Od -D_DEBUG 
LFLAGS	=	$(LFLAGS) -v
LIBODIR	=	$(LIBODIR).debug
EXEODIR	=	$(EXEODIR).debug
!else
CFLAGS	=	$(CFLAGS) -g1
LIBODIR	=	$(LIBODIR).release
EXEODIR	=	$(EXEODIR).release
!endif

# JavaScript Support
!ifdef JS
CFLAGS	= 	$(CFLAGS) -DJAVASCRIPT -I../mozilla/js/src
LIBS	=	..\mozilla\js\src\Debug\js32omf.lib
!endif

# Cross platform/compiler definitions
!include targets.mk		# defines all targets
!include objects.mk		# defines $(OBJS)
!include headers.mk		# defines $(HEADERS)
!include sbbsdefs.mk	# defines $(SBBSDEFS)

SBBSLIB	=	$(LIBODIR)\sbbs.lib

.path.c = .;$(XPDEV)
.path.cpp = .;$(XPDEV)

# Implicit C Compile Rule for SBBS.DLL
.c.$(OFILE):
	@$(CC) $(CFLAGS) -WD -WM -n$(LIBODIR) -c $(SBBSDEFS) $<

# Implicit C++ Compile Rule for SBBS.DLL
.cpp.$(OFILE):
	@$(CC) $(CFLAGS) -WD -WM -n$(LIBODIR) -c $(SBBSDEFS) $<

# Create output directories if they don't exist
$(LIBODIR):
	@if not exist $(LIBODIR) mkdir $(LIBODIR)
$(EXEODIR):
	@if not exist $(EXEODIR) mkdir $(EXEODIR)

# Monolithic Synchronet executable Build Rule
$(SBBSMONO): sbbscon.c $(OBJS) $(LIBODIR)\ver.$(OFILE) $(LIBODIR)\ftpsrvr.$(OFILE) \
	$(LIBODIR)\mailsrvr.$(OFILE) $(LIBODIR)\mxlookup.$(OFILE) $(LIBODIR)\mime.$(OFILE) \
	$(LIBODIR)\services.$(OFILE)
	@$(CC) $(CFLAGS) -WM -e$(SBBSMONO) $** $(LIBS)

# SBBS DLL Link Rule
$(SBBS): $(OBJS) $(LIBODIR)\ver.$(OFILE)
    @echo Linking $< ...
	@$(LD) $(LFLAGS) c0d32.obj $(LIBS) $(OBJS) $(LIBODIR)\ver.$(OFILE), $*, $*, \
		import32.lib cw32mt.lib ws2_32.lib

# Mail Server DLL Link Rule
$(MAILSRVR): mailsrvr.c mxlookup.c mime.c crc32.c $(SBBSLIB)
    @echo Creating $@
	@$(CC) $(CFLAGS) -WD -WM -lGi -n$(LIBODIR) -DMAILSRVR_EXPORTS -DSMBDLL $** $(LIBS)

# FTP Server DLL Link Rule
$(FTPSRVR): ftpsrvr.c $(SBBSLIB)
    @echo Creating $@
	@$(CC) $(CFLAGS) -WD -WM -lGi -n$(LIBODIR) -DFTPSRVR_EXPORTS $** $(LIBS)

# Services DLL Link Rule
$(SERVICES): services.c $(SBBSLIB)
    @echo Creating $@
	@$(CC) $(CFLAGS) -WD -WM -lGi -n$(LIBODIR) -DSERVICES_EXPORTS $** $(LIBS)

# Synchronet Console Build Rule
$(SBBSCON): sbbscon.c $(SBBSLIB)
	@$(CC) $(CFLAGS) -n$(EXEODIR) $**

# Baja Utility
$(BAJA): baja.c ars.c crc32.c
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

# Node Utility
$(NODE): node.c 
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

SMBLIB = $(LIBODIR)\smblib.obj $(LIBODIR)\genwrap.obj $(LIBODIR)\filewrap.obj

# FIXSMB Utility
$(FIXSMB): fixsmb.c $(SMBLIB)
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

# CHKSMB Utility
$(CHKSMB): chksmb.c $(SMBLIB) $(XPDEV)dirwrap.c
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

# SMB Utility
$(SMBUTIL): smbutil.c smbtxt.c crc32.c lzh.c $(SMBLIB) $(XPDEV)dirwrap.c
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

# SBBSecho (FidoNet Packet Tosser)
$(SBBSECHO): sbbsecho.c rechocfg.c smbtxt.c crc32.c lzh.c $(SMBLIB) \
	$(LIBODIR)\ars.obj \
	$(LIBODIR)\nopen.obj \
	$(LIBODIR)\str_util.obj \
	$(LIBODIR)\date_str.obj \
	userdat.c \
	genwrap.c \
	dirwrap.c \
	$(LIBODIR)\load_cfg.obj \
	$(LIBODIR)\scfglib1.obj \
	$(LIBODIR)\scfglib2.obj
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 

# SBBSecho Configuration Program
$(ECHOCFG): echocfg.c rechocfg.c \
	$(UIFC)uifc.c \
	$(LIBODIR)\nopen.obj \
	$(LIBODIR)\str_util.obj
	@echo Creating $@
	@$(CC) $(CFLAGS) -n$(EXEODIR) $** 


!include depends.mk		# defines dependencies