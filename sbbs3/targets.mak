# targets.mak

# Make 'include file' defining targets for Synchronet project

# $Id: targets.mak,v 1.11 2002/04/04 11:15:28 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBS	=	$(LIBODIR)$(SLASH)sbbs$(LIBFILE) 
FTPSRVR	=	$(LIBODIR)$(SLASH)ftpsrvr$(LIBFILE)
MAILSRVR=	$(LIBODIR)$(SLASH)mailsrvr$(LIBFILE)
SERVICES=	$(LIBODIR)$(SLASH)services$(LIBFILE)
SBBSCON	=	$(EXEODIR)$(SLASH)sbbscon$(EXEFILE)
SBBSMONO=	$(EXEODIR)$(SLASH)sbbs$(EXEFILE)
NODE	=	$(EXEODIR)$(SLASH)node$(EXEFILE)
BAJA	=	$(EXEODIR)$(SLASH)baja$(EXEFILE)
FIXSMB	=	$(EXEODIR)$(SLASH)fixsmb$(EXEFILE)
CHKSMB	=	$(EXEODIR)$(SLASH)chksmb$(EXEFILE)
SMBUTIL	=	$(EXEODIR)$(SLASH)smbutil$(EXEFILE)

all:	$(LIBODIR) $(EXEODIR) $(SBBSMONO) \
		$(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA) $(NODE)

utils:	$(EXEODIR) \
		$(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA) $(NODE)

dlls:	$(LIBODIR) \
		$(SBBS) $(FTPSRVR) $(MAILSRVR) $(SERVICES)

mono:	$(LIBODIR) $(EXEODIR) $(SBBSMONO)

clean:
	@$(DELETE) $(LIBODIR)$(SLASH)*
	@$(DELETE) $(EXEODIR)$(SLASH)*