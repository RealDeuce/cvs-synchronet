# To build the .dll files, use "make [OPTIONS] DLLIBS=1 dl"

SRC_ROOT	=	..
!include $(SRC_ROOT)/build/Common.bmake

#The following is necessary only when DLL-exporting wrapper functions
!ifdef DLLIBS
CFLAGS	=	$(CFLAGS) -DWRAPPER_EXPORTS=1
!endif

MT_CFLAGS = $(MT_CFLAGS) -DLINK_LIST_THREADSAFE
MKSHLIB =       $(CC) -WD

$(WRAPTEST): $(XPDEV-MT_LIB_BUILD) $(EXEODIR)
	@echo Linking $<
	$(QUIET)$(CC) $(LDFLAGS) $(MT_LDFLAGS) $(XPDEV-MT_CFLAGS) $(XPDEV-MT_LDFLAGS) -e$@ $(XPDEV-MT_LIBS)

$(XPDEV_LIB_BUILD): $(OBJS)
	@echo Creating $< ...
	-$(QUIET)$(DELETE) $@
	&$(QUIET)tlib $@ +$**

$(XPDEV_SHLIB_BUILD): $(OBJS)
	@echo Linking $@
	$(QUIET)$(MKSHLIB) $(MT_LDFLAGS) -lGi -e$@ $(LDFLAGS) $(SHLIBOPTS) @&&|
	$**
	cw32.lib
|

$(XPDEV-MT_LIB_BUILD): $(MTOBJS)
	@echo Creating $< ...
	-$(QUIET)$(DELETE) $@
	&$(QUIET)tlib $@ +$**

$(XPDEV-MT_SHLIB_BUILD): $(MTOBJS)
	@echo Linking $@
	$(QUIET)$(MKSHLIB) $(MT_LDFLAGS) -lGi -e$@ $(LDFLAGS) $(SHLIBOPTS) @&&|
	$**
	cw32mt.lib
|
