# targets.mk

# Make 'include file' defining targets for Synchronet project

# $Id: targets.mk,v 1.8 2003/04/02 02:02:33 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBS		= $(LIBODIR)$(SLASH)sbbs$(LIBFILE) 
FTPSRVR		= $(LIBODIR)$(SLASH)ftpsrvr$(LIBFILE)
WEBSRVR		= $(LIBODIR)$(SLASH)websrvr$(LIBFILE)
MAILSRVR	= $(LIBODIR)$(SLASH)mailsrvr$(LIBFILE)
SERVICES	= $(LIBODIR)$(SLASH)services$(LIBFILE)
SBBSCON		= $(EXEODIR)$(SLASH)sbbscon$(EXEFILE)
SBBSMONO	= $(EXEODIR)$(SLASH)sbbs$(EXEFILE)
NODE		= $(EXEODIR)$(SLASH)node$(EXEFILE)
BAJA		= $(EXEODIR)$(SLASH)baja$(EXEFILE)
FIXSMB		= $(EXEODIR)$(SLASH)fixsmb$(EXEFILE)
CHKSMB		= $(EXEODIR)$(SLASH)chksmb$(EXEFILE)
SMBUTIL		= $(EXEODIR)$(SLASH)smbutil$(EXEFILE)
SBBSECHO	= $(EXEODIR)$(SLASH)sbbsecho$(EXEFILE)
ECHOCFG		= $(EXEODIR)$(SLASH)echocfg$(EXEFILE)
ADDFILES	= $(EXEODIR)$(SLASH)addfiles$(EXEFILE)
FILELIST	= $(EXEODIR)$(SLASH)filelist$(EXEFILE)
MAKEUSER	= $(EXEODIR)$(SLASH)makeuser$(EXEFILE)
ANS2MSG		= $(EXEODIR)$(SLASH)ans2msg$(EXEFILE)
MSG2ANS		= $(EXEODIR)$(SLASH)msg2ans$(EXEFILE)

UTILS		= $(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA) $(NODE) \
		  $(SBBSECHO) $(ECHOCFG) $(ADDFILES) $(FILELIST) $(MAKEUSER) \
		  $(ANS2MSG) $(MSG2ANS)

all:	$(LIBODIR) $(EXEODIR) $(SBBSMONO) $(UTILS)

utils:	$(EXEODIR) $(UTILS)

dlls:	$(LIBODIR) \
		$(SBBS) $(FTPSRVR) $(MAILSRVR) $(SERVICES)

mono:	$(LIBODIR) $(EXEODIR) $(SBBSMONO)

clean:
	@$(DELETE) $(LIBODIR)$(SLASH)*
	@$(DELETE) $(EXEODIR)$(SLASH)*