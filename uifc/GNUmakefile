# smblib/Makefile

#########################################################################
# Makefile for Synchronet Message Base Library (SMBLIB)					#
# For use with Borland C++ Builder 5+ or Borland C++ 5.5 for Win32      #
# @format.tab-size 4													#
#																		#
# usage: make															#
#########################################################################

# $Id$

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version

SRC_ROOT = ..
# Cross platform/compiler definitions
include $(SRC_ROOT)/build/Common.gmake	# defines clean and output directory rules

CFLAGS += -DWRAPPER_IMPORTS -I$(XPDEV_SRC) $(CIOLIB-MT_CFLAGS)

# UIFC Library Link Rule
$(UIFCLIB_BUILD): $(OBJS)
	@echo Creating $@ ...
	$(QUIET)ar rc $@ $(OBJS)
	$(QUIET)ranlib $@

# MT-UIFC Library Link Rule
$(UIFCLIB-MT_BUILD): $(MT_OBJS)
	@echo Creating $@ ...
	$(QUIET)ar rc $@ $(MT_OBJS)
	$(QUIET)ranlib $@

$(UIFCTEST): $(MTOBJODIR)$(DIRSEP)uifctest$(OFILE)
	@echo Creating $@ ...
	$(QUIET)$(CC) $(MT_LDFLAGS) $(UIFC-MT_LDFLAGS) $(XPDEV-MT_LDFLAGS) $(CIOLIB-MT_LDFLAGS) $(LDFLAGS) -o $@ $(MTOBJODIR)$(DIRSEP)uifctest$(OFILE) $(UIFC-MT_LIBS) $(CIOLIB-MT_LIBS) $(XPDEV-MT_LIBS)
