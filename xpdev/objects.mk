# libobjs.mk

# Make 'include file' listing object files for xpdev "wrappers"

# $Id$

# OBJODIR, SLASH, and OFILE must be pre-defined

OBJS	= \
	$(OBJODIR)$(DIRSEP)conwrap$(OFILE) \
	$(OBJODIR)$(DIRSEP)dat_file$(OFILE) \
	$(OBJODIR)$(DIRSEP)dirwrap$(OFILE) \
	$(OBJODIR)$(DIRSEP)filewrap$(OFILE) \
	$(OBJODIR)$(DIRSEP)genwrap$(OFILE) \
	$(OBJODIR)$(DIRSEP)ini_file$(OFILE) \
	$(OBJODIR)$(DIRSEP)link_list$(OFILE) \
	$(OBJODIR)$(DIRSEP)sockwrap$(OFILE) \
	$(OBJODIR)$(DIRSEP)str_list$(OFILE)

MTOBJS	= \
	$(MTOBJODIR)$(DIRSEP)conwrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)dat_file$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)dirwrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)filewrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)genwrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)ini_file$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)link_list$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)msg_queue$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)semwrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)sockwrap$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)str_list$(OFILE) \
	$(MTOBJODIR)$(DIRSEP)threadwrap$(OFILE)
