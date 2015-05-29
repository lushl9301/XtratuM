all: bpatch

INSTALL=bpatch

include ../../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk
include $(XTRATUM_PATH)/user/install.mk

-include dephost.mk

SRCS=bpatch.c
OBJS := $(patsubst %.c,%.host.o,$(SRCS))

bpatch: $(OBJS)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $<
clean:
	@$(RM) -f *.o *~ bpatch dephost.mk dep.mk
