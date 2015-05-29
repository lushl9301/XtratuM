INSTALL=xmeformat
all: i

include ../../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk
include $(XTRATUM_PATH)/user/install.mk

LIBXMHOST=$(XTRATUM_PATH)/user/libxm/libxmhost.a

$(LIBXMHOST):
	@$(MAKE) -C $(XTRATUM_PATH)/user/libxm libxmhost.a

-include dephost.mk

XEF_SRCS=xmeformat.c
XEF_OBJS := $(patsubst %.c,%.host.o,$(XEF_SRCS))

HOST_CFLAGS += --include xm_inc/config.h --include xm_inc/arch/arch_types.h -I./

INSTALL=xmeformat

i: $(INSTALL)

xmeformat: $(LIBHOSTXEF) $(XEF_OBJS) $(LIBXMHOST)
	@$(HOST_CC) $(HOST_CFLAGS) -o $@ $(XEF_OBJS) -L$(LIBXM_PATH) -lxmhost

SRCS=$(XEF_SRCS)

clean:
	@$(RM) -f *.o *~ ${INSTALL} $(ARCH)/*~ $(ARCH)/*.o dephost.mk ${INSTALL}.tex ${INSTALL}.3

DATE=$(shell LANG=C date "+%B %Y")

xmeformat.3: xmeformat.pod
	pod2man  --section=3  --center="XtratuM tools" --release=XtratuM-$(XTRATUMVERSION)  --date="${DATE}" $< $@

xmeformat.tex: xmeformat.pod
	pod2latex -h1level 3  $< $@
