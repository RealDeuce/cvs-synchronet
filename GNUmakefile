# Global GNU makefile for Synchronet
#
# Usage:
# ------
# [g]make install [variable=value]...
#
# variables:
# ----------
# DEBUG = Set to force a debug build
# RELEASE = Set to force a release build
# INSTALL = Set to CLASSIC (All in one sbbs dir) or UNIX (use /etc, /sbin...)
# SYMLINK = Don't copy binaries, rather create symlinks in $(SBBSDIR)/exec
# SBBSDIR = Directory to do CLASSIC install to
# PREFIX = Set to the UNIX base directory to install to
# bcc = Set to use Borland compiler
# os = Set to the OS name (Not required)
# SBBSUSER = Owner for the installed files
# SBBSGROUP = Group for the installed files
# NOCVS	= do not do CVS update
# JSLIB = Full path and filename to JavaScript library.
# CVSTAG = CVS tag to pull

ifndef DEBUG
 ifndef RELEASE
  DEBUG	:=	1
 endif
endif

ifdef SYMLINK
 INSBIN	:=	ln -sf
else
 INSBIN	:=	cp
endif

ifdef bcc
 CCPRE	:=	bcc
 MKFLAGS	+=	bcc=1
else
 CCPRE	:=	gcc
endif

INSTALL	?=	CLASSIC		# Can be CLASSIC or UNIX
CVSTAG	?=	HEAD		# CVS tag to pull... HEAD means current.

SBBSUSER	?= $(USER)
SBBSGROUP	?= $(GROUP)
SBBSCHOWN	:= $(SBBSUSER):$(SBBSGROUP)

ifeq ($(INSTALL),UNIX)
 PREFIX	?=	/usr/local
 MKFLAGS	+=	PREFIX=$(PREFIX)
else	# Classic Install
 SBBSDIR	?=	$(shell pwd)
endif

# Get OS
ifndef os
 os             =       $(shell uname)
endif
os      :=      $(shell echo $(os) | tr '[A-Z]' '[a-z]' | tr ' ' '_')

machine         :=      $(shell if uname -m | egrep -v "(i[3456789]|x)86" > /dev/null; then uname -m | tr "[A-Z]" "[a-z]" | tr " " "_" ; fi)
ifeq ($(machine),)
 machine        :=      $(os)
else
 machine        :=      $(os).$(machine)
endif

MKFLAGS	+=	os=$(os)

ifdef DEBUG
 SUFFIX  :=  debug
 MKFLAGS	+=	DEBUG=1
else
 SUFFIX  :=  release
 MKFLAGS	+=	RELEASE=1
endif

ifdef JSLIB
 MKFLAGS	+=	JSLIB=$(JSLIB)
endif

all: binaries baja externals $(SBBSDIR)/docs

binaries:	sbbs3 scfg umonitor uedit

externals:	sbj sbl

sbbs3:	$(SBBSDIR)/src/sbbs3 $(SBBSDIR)/src/uifc $(SBBSDIR)/src/xpdev \
	$(SBBSDIR)/include \
	$(SBBSDIR)/lib/mozilla/js/$(machine).$(SUFFIX) \
	$(SBBSDIR)/lib/mozilla/nspr/$(machine).$(SUFFIX) \
	$(SBBSDIR)/lib/fltk/$(machine)
	$(MAKE) -C $(SBBSDIR)/src/sbbs3 $(MKFLAGS)

scfg:	$(SBBSDIR)/src/sbbs3 $(SBBSDIR)/src/uifc $(SBBSDIR)/src/xpdev \
	$(SBBSDIR)/include \
	$(SBBSDIR)/lib/fltk/$(machine)
	$(MAKE) -C $(SBBSDIR)/src/sbbs3/scfg $(MKFLAGS)

umonitor:	$(SBBSDIR)/src/sbbs3 \
	$(SBBSDIR)/src/uifc $(SBBSDIR)/src/xpdev \
	$(SBBSDIR)/include \
	$(SBBSDIR)/lib/fltk/$(machine)
	$(MAKE) -C $(SBBSDIR)/src/sbbs3/umonitor $(MKFLAGS)

uedit:	$(SBBSDIR)/src/sbbs3 \
	$(SBBSDIR)/src/uifc $(SBBSDIR)/src/xpdev \
	$(SBBSDIR)/include \
	$(SBBSDIR)/lib/fltk/$(machine)

	$(MAKE) -C $(SBBSDIR)/src/sbbs3/uedit $(MKFLAGS)

baja:	$(SBBSDIR)/exec binaries
	$(MAKE) -C $(SBBSDIR)/exec $(MKFLAGS) BAJAPATH=$(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/baja

sbj:	$(SBBSDIR)/xtrn
	$(MAKE) -C $(SBBSDIR)/xtrn/sbj $(MKFLAGS)

sbl:	$(SBBSDIR)/xtrn
	$(MAKE) -C $(SBBSDIR)/xtrn/sbl $(MKFLAGS) SBBS_SRC=$(SBBSDIR)/src/sbbs3 XPDEV=$(SBBSDIR)/src/xpdev

node_dirs:	$(SBBSDIR)/node1 $(SBBSDIR)/node2 $(SBBSDIR)/node3 $(SBBSDIR)/node4

install: all $(SBBSDIR)/ctrl $(SBBSDIR)/text node_dirs
ifeq ($(INSTALL),UNIX)
	@echo ERROR: UNIX Install type not yet supported.
	fail
else
	@echo Installing to $(SBBSDIR)
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/baja $(SBBSDIR)/exec/baja
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/node $(SBBSDIR)/exec/node
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/chksmb $(SBBSDIR)/exec/chksmb
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/fixsmb $(SBBSDIR)/exec/fixsmb
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/addfiles $(SBBSDIR)/exec/addfiles
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/makeuser $(SBBSDIR)/exec/makeuser
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/smbutil $(SBBSDIR)/exec/smbutil
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/sbbs $(SBBSDIR)/exec/sbbs
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/sbbsmono $(SBBSDIR)/exec/sbbs
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/sbbsecho $(SBBSDIR)/exec/sbbsecho
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/echocfg $(SBBSDIR)/exec/echocfg
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).exe.$(SUFFIX)/filelist $(SBBSDIR)/exec/filelist
	$(INSBIN) $(SBBSDIR)/src/sbbs3/scfg/$(CCPRE).$(machine).$(SUFFIX)/scfg $(SBBSDIR)/exec/scfg
	$(INSBIN) $(SBBSDIR)/src/sbbs3/scfg/$(CCPRE).$(machine).$(SUFFIX)/scfghelp.ixb $(SBBSDIR)/exec/scfghelp.ixb
	$(INSBIN) $(SBBSDIR)/src/sbbs3/scfg/$(CCPRE).$(machine).$(SUFFIX)/scfghelp.dat $(SBBSDIR)/exec/scfghelp.dat
	$(INSBIN) $(SBBSDIR)/src/sbbs3/umonitor/$(CCPRE).$(machine).$(SUFFIX)/umonitor $(SBBSDIR)/exec/umonitor
	$(INSBIN) $(SBBSDIR)/src/sbbs3/uedit/$(CCPRE).$(machine).$(SUFFIX)/uedit $(SBBSDIR)/exec/uedit
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).lib.$(SUFFIX)/libsbbs.so $(SBBSDIR)/exec/libsbbs.so
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).lib.$(SUFFIX)/libftpsrvr.so $(SBBSDIR)/exec/libftpsrvr.so
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).lib.$(SUFFIX)/libmailsrvr.so $(SBBSDIR)/exec/libmailsrvr.so
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).lib.$(SUFFIX)/libservices.so $(SBBSDIR)/exec/libservices.so
	$(INSBIN) $(SBBSDIR)/src/sbbs3/$(CCPRE).$(machine).lib.$(SUFFIX)/libwebsrvr.so $(SBBSDIR)/exec/libwebsrvr.so
# kludge... must fix this to allow moz JS libs and such.  ToDo
	$(INSBIN) $(SBBSDIR)/lib/mozilla/*/$(machine).$(SUFFIX)/*.so $(SBBSDIR)/exec/
	-chown -R $(SBBSCHOWN) $(SBBSDIR)
	-chown -h $(SBBSCHOWN) $(SBBSDIR)/exec/*
endif

# CVS checkout command-line
CVS_CO = @cd $(SBBSDIR); cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs co

$(SBBSDIR)/ctrl: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) ctrl
endif

$(SBBSDIR)/text: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) text
endif

$(SBBSDIR)/docs: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) docs
endif

$(SBBSDIR)/exec: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) exec
endif

$(SBBSDIR)/node1: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) node1
endif

$(SBBSDIR)/node2: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) node2
endif

$(SBBSDIR)/node3: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) node3
endif

$(SBBSDIR)/node4: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) node4
endif

$(SBBSDIR)/xtrn: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) -N xtrn
endif

$(SBBSDIR)/src/sbbs3: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG)  src/sbbs3
endif

$(SBBSDIR)/src/uifc: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG)  src/uifc
endif

$(SBBSDIR)/src/xpdev: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG)  src/xpdev
endif

$(SBBSDIR)/include: cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG)  include
endif

$(SBBSDIR)/lib/mozilla/js/$(machine).$(SUFFIX): cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) lib/mozilla/js/$(machine).$(SUFFIX)
endif

$(SBBSDIR)/lib/mozilla/nspr/$(machine).$(SUFFIX): cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) lib/mozilla/nspr/$(machine).$(SUFFIX)
endif

$(SBBSDIR)/lib/fltk/$(machine): cvslogin
ifndef NOCVS
	$(CVS_CO) -r $(CVSTAG) lib/fltk/$(machine)
endif

cvslogin: $(SBBSDIR)
ifndef NOCVS
	@echo Press \<ENTER\> when prompted for password
	@cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login
endif

$(SBBSDIR):
	@[ ! -e $(SBBSDIR) ] && mkdir $(SBBSDIR);

