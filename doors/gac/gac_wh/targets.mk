GAC_WH	=	$(EXEODIR)$(DIRSEP)gac_wh$(EXEFILE)

all: xpdev-mt ciolib-mt $(MTOBJODIR) $(EXEODIR) $(GAC_WH)

$(GAC_WH):	$(XPDEV-MT_LIB) $(CIOLIB-MT)

