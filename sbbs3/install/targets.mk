# targets.mk

# Make 'include file' defining targets for Synchronet SBBSINST project

# $Id$

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBSINST	=	$(EXEODIR)$(SLASH)sbbsinst$(EXEFILE) 

all:	$(EXEODIR) \
		$(LIBODIR) \
		$(SBBSINST)
