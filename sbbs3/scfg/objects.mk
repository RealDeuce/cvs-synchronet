# objects.mak

# Make 'include file' listing object files for Synchronet SCFG

# $Id: objects.mk,v 1.1 2002/04/12 06:22:46 rswindell Exp $

# LIBODIR, SBBSLIBODIR, SLASH, and OFILE must be pre-defined


OBJS	=	$(ODIR)$(SLASH)scfg.$(OFILE)\
			$(ODIR)$(SLASH)scfgxtrn.$(OFILE)\
			$(ODIR)$(SLASH)scfgmsg.$(OFILE)\
			$(ODIR)$(SLASH)scfgnet.$(OFILE)\
			$(ODIR)$(SLASH)scfgnode.$(OFILE)\
			$(ODIR)$(SLASH)scfgsub.$(OFILE)\
			$(ODIR)$(SLASH)scfgsys.$(OFILE)\
			$(ODIR)$(SLASH)scfgxfr1.$(OFILE)\
			$(ODIR)$(SLASH)scfgxfr2.$(OFILE)\
			$(ODIR)$(SLASH)scfgchat.$(OFILE)\
            $(ODIR)$(SLASH)scfgsave.$(OFILE)\
            $(ODIR)$(SLASH)scfglib1.$(OFILE)\
            $(ODIR)$(SLASH)smblib.$(OFILE)\
            $(ODIR)$(SLASH)scfglib2.$(OFILE)\
            $(ODIR)$(SLASH)ars.$(OFILE)\
            $(ODIR)$(SLASH)load_cfg.$(OFILE)\
            $(ODIR)$(SLASH)misc.$(OFILE)\
            $(ODIR)$(SLASH)crc32.$(OFILE)\
            $(ODIR)$(SLASH)userdat.$(OFILE)\
            $(ODIR)$(SLASH)date_str.$(OFILE)\
			$(ODIR)$(SLASH)scfgchat.$(OFILE)\
			$(ODIR)$(SLASH)genwrap.$(OFILE)\
			$(ODIR)$(SLASH)dirwrap.$(OFILE)\
			$(ODIR)$(SLASH)filewrap.$(OFILE)\
			$(ODIR)$(SLASH)uifcx.$(OFILE)

