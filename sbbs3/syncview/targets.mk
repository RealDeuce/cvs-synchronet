SYNCVIEW	=	$(EXEODIR)$(DIRSEP)syncview$(EXEFILE)

all: xpdev-mt ciolib-mt $(MTOBJODIR) $(EXEODIR) $(SYNCVIEW)

$(SYNCVIEW):	$(XPDEV-MT_LIB) $(CIOLIB-MT)
