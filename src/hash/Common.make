# $Id: Common.make,v 1.1 2019/06/29 01:14:55 rswindell Exp $
# Available Options:

HASH_LIB		=	$(HASH_SRC)$(DIRSEP)$(LIBODIR)$(DIRSEP)$(LIBPREFIX)hash$(LIBFILE)

HASH_CFLAGS	=	-I$(HASH_SRC)
HASH_LDFLAGS	=	-L$(HASH_SRC)$(DIRSEP)$(LIBODIR)
HASH_LIBS	=	$(UL_PRE)hash$(UL_SUF)
