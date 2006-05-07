# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.12 2006/04/20 20:54:39 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST		= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)
XPDEV_LIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev$(LIBFILE)
XPDEV-MT_LIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev_mt$(LIBFILE)
XPDEV_SHLIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev$(SOFILE)
XPDEV-MT_SHLIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev_mt$(SOFILE)

all: lib mtlib

tests: $(MTOBJODIR) $(LIBODIR) $(EXEODIR) $(WRAPTEST)

dl: dl-lib dl-mtlib

lib:	$(OBJODIR) $(LIBODIR) $(XPDEV_LIB_BUILD)

dl-lib: $(OBJODIR) $(LIBODIR) $(XPDEV_SHLIB_BUILD)

mtlib:	$(MTOBJODIR) $(LIBODIR) $(XPDEV-MT_LIB_BUILD)

dl-mtlib: $(MTOBJODIR) $(LIBODIR) $(XPDEV-MT_SHLIB_BUILD)

$(WRAPTEST): $(XPDEV-MT_LIB_BUILD)
