#  OpenDoors 6.23
#  (C} Copyright 1991 - 1997 by Brian Pirie. All Rights Reserved.
#
#  Oct-2001 door32.sys/socket modifications by Rob Swindell (www.synchro.net}
#
#
#         File: Win32.mak
#
#  Description: Makefile used to build the Win32 OpenDoors libraries from
#               the sources. Usage is described below.
#
#    Revisions: Date          Ver   Who  Change
#               ---------------------------------------------------------------
#               Aug 09, 2003  6.23  SH   *nix port
#
###############################################################################
#
# USAGE INFORMATION
#
###############################################################################
#
# Command Line:   make -fWin32.mak
#                     or
#                 nmake /f Win32.mak
#
###############################################################################
#
# CONFIGURATION
#
# Customize this section of the makefile to provide the relevant information
# for your compiler, assembler (if any} and build environment.
#
###############################################################################
# Compiler executable file name. Use:
#
#                  tcc - For Borland Turbo C and Turbo C++
#                  bcc - For Borland C++
#                   cl - For Microsoft compilers
#
CC	:=	gcc
#
#------------------------------------------------------------------------------
#
# Linker executable file name. Use:
#
#                tlink - For Borland compilers
#                 link - For Microsoft compilers
#
# Get OS name
OS      :=      $(shell uname)
os	:=	$(shell echo $(OS) | tr '[A-Z]' '[a-z]' | tr ' ' '_')

LD	:=	gcc

ifdef DEBUG
 CFLAGS	+=	-g
 BUILDTYPE	:=	debug
else
 BUILDTYPE	:=	release
endif
#
#------------------------------------------------------------------------------
#
# Compiler command-line flags.
#
CFLAGS	+=	-O2 -L. -I../xpdev
ifeq ($(OS),Darwin)
 CFLAGS		+=	-D__unix__
 LDFLAGS	+=	$(CFLAGS) -dynamiclib -single_module
else
 LDFLAGS	+=	$(CFLAGS) -shared
endif
# /MTd /Zi - for debug
#
#------------------------------------------------------------------------------
#
# Link flags.
#
LDFLAGS	+=	-L../xpdev/$(LD).$(os).lib.$(BUILDTYPE)
#
#------------------------------------------------------------------------------
#
# Output directories. customize for your own preferences. Note that trailing
# backslash (\} characters are required.
#
SHLIB		:=	.so
STATICLIB	:=	.a
OBJFILE 	:=	.o
ifdef PROFILE
	CFLAGS	+=	-pg
	SHLIB	:=	_p${SHLIB}
	STATICLIB	:=	_p.a
endif
#
###############################################################################
#
# DEPENDENCIES
#
# You won't normally have to change anything after this point in this makefile.
#
###############################################################################
#
# Define primary target.
#
all: ${LIBDIR}libODoors${SHLIB} ${LIBDIR}libODoors${STATICLIB} ex_chat ex_diag ex_hello ex_music ex_ski ex_vote
#
#------------------------------------------------------------------------------
#
# Name of all headers.
#
HEADERS= ${HEADERDIR}ODCom.h\
         ${HEADERDIR}ODCore.h\
         ${HEADERDIR}ODGen.h\
         ${HEADERDIR}ODInEx.h\
         ${HEADERDIR}ODInQue.h\
         ${HEADERDIR}ODKrnl.h\
         ${HEADERDIR}ODPlat.h\
         ${HEADERDIR}ODRes.h\
         ${HEADERDIR}ODScrn.h\
         ${HEADERDIR}ODStat.h\
         ${HEADERDIR}ODSwap.h\
         ${HEADERDIR}ODTypes.h\
         ${HEADERDIR}ODUtil.h\
         ${HEADERDIR}OpenDoor.h
#
#------------------------------------------------------------------------------
#
# Build DLL from objects.
#
OBJECTS := ${OBJDIR}ODAuto${OBJFILE}\
         ${OBJDIR}ODBlock${OBJFILE}\
         ${OBJDIR}ODCFile${OBJFILE}\
         ${OBJDIR}ODCmdLn${OBJFILE}\
         ${OBJDIR}ODCom${OBJFILE}\
         ${OBJDIR}ODCore${OBJFILE}\
         ${OBJDIR}ODDrBox${OBJFILE}\
         ${OBJDIR}ODEdit${OBJFILE}\
         ${OBJDIR}ODEdStr${OBJFILE}\
         ${OBJDIR}ODEmu${OBJFILE}\
         ${OBJDIR}ODGetIn${OBJFILE}\
         ${OBJDIR}ODGraph${OBJFILE}\
         ${OBJDIR}ODInEx1${OBJFILE}\
         ${OBJDIR}ODInEx2${OBJFILE}\
         ${OBJDIR}ODInQue${OBJFILE}\
         ${OBJDIR}ODKrnl${OBJFILE}\
         ${OBJDIR}ODList${OBJFILE}\
         ${OBJDIR}ODLog${OBJFILE}\
         ${OBJDIR}ODMulti${OBJFILE}\
         ${OBJDIR}ODPlat${OBJFILE}\
         ${OBJDIR}ODPCB${OBJFILE}\
         ${OBJDIR}ODPopup${OBJFILE}\
         ${OBJDIR}ODPrntf${OBJFILE}\
         ${OBJDIR}ODRA${OBJFILE}\
         ${OBJDIR}ODScrn${OBJFILE}\
         ${OBJDIR}ODSpawn${OBJFILE}\
         ${OBJDIR}ODStand${OBJFILE}\
         ${OBJDIR}ODStat${OBJFILE}\
         ${OBJDIR}ODStr${OBJFILE}\
         ${OBJDIR}ODUtil${OBJFILE}\
         ${OBJDIR}ODWCat${OBJFILE}\
	 ${OBJDIR}ODWin${OBJFILE}
#         ${OBJDIR}ODoor.res
#         ${OBJDIR}odsys${OBJFILE}\	this file is missing

${LIBDIR}libODoors${SHLIB} : ${OBJECTS}
	$(LD) $(LDFLAGS) -o ${LIBDIR}libODoors${SHLIB}.6.2 ${OBJECTS}
	ln -fs ${LIBDIR}libODoors${SHLIB}.6.2 ${LIBDIR}libODoors${SHLIB}

${LIBDIR}libODoors${STATICLIB} : ${OBJECTS}
	ar -r ${LIBDIR}libODoors${STATICLIB} ${OBJECTS}
	ranlib ${LIBDIR}libODoors${STATICLIB}
	
ex_chat: ex_chat.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(CFLAGS) ex_chat.c -o ex_chat -lODoors

ex_diag: ex_diag.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(CFLAGS) ex_diag.c -o ex_diag -lODoors

ex_hello: ex_hello.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(CFLAGS) ex_hello.c -o ex_hello -lODoors

ex_music: ex_music.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(CFLAGS) ex_music.c -o ex_music -lODoors

ex_ski: ex_ski.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(LDFLAGS) ex_ski.c -o ex_ski -lODoors -lxpdev

ex_vote: ex_vote.c ${LIBDIR}libODoors${SHLIB}
	$(CC) $(CFLAGS) ex_vote.c ../xpdev/filewrap.c -o ex_vote -lODoors -DMULTINODE_AWARE

#
#------------------------------------------------------------------------------
