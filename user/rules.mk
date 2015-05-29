XM_CORE_PATH=$(XTRATUM_PATH)/core
LIBXM_PATH=$(XTRATUM_PATH)/user/libxm

check_gcc = $(shell if $(TARGET_CC) $(1) -S -o /dev/null -xc /dev/null > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi)
check_hgcc = $(shell if $(HOST_CC) $(1) -S -o /dev/null -xc /dev/null > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi)

HOST_CFLAGS = -Wall -O2 -D$(ARCH) -I$(LIBXM_PATH)/include -DHOST -D"__XM_INCFLD(_fld)=<xm_inc/_fld>" -fno-strict-aliasing
HOST_LDFLAGS =

TARGET_CFLAGS = -Wall -O2 -nostdlib -nostdinc -D$(ARCH) -fno-strict-aliasing -fomit-frame-pointer -D"__XM_INCFLD(_fld)=<xm_inc/_fld>"
TARGET_CFLAGS += -I$(LIBXM_PATH)/include --include xm_inc/config.h --include xm_inc/arch/arch_types.h

# disable pointer signedness warnings in gcc 4.0
TARGET_CFLAGS += $(call check_gcc,-Wno-pointer-sign,)
# disable stack protector in gcc 4.1
TARGET_CFLAGS += $(call check_gcc,-fno-stack-protector,)
# disable warning: variable set but not used
TARGET_CFLAGS += $(call check_gcc,-Wno-unused-but-set-variable,)
HOST_CFLAGS += $(call check_hgcc,-Wno-unused-but-set-variable,)

ifeq ($(ARCH), x86)
TARGET_CFLAGS += $(TARGET_CFLAGS_ARCH)
HOST_CFLAGS += $(HOST_CFLAGS_ARCH)
HOST_ASFLAGS += $(HOST_CFLAGS_ARCH)
endif

TARGET_ASFLAGS = -Wall -O2 -D__ASSEMBLY__ -fno-builtin -D$(ARCH) -D"__XM_INCFLD(_fld)=<xm_inc/_fld>"
TARGET_ASFLAGS += -I$(LIBXM_PATH)/include -nostdlib -nostdinc --include xm_inc/config.h
TARGET_ASFLAGS += $(TARGET_ASFLAGS_ARCH)
LIBGCC=`$(TARGET_CC) -print-libgcc-file-name $(TARGET_CFLAGS_ARCH)`

TARGET_LDFLAGS = $(TARGET_LDFLAGS_ARCH)


%.host.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

%.host.o: %.S
	$(HOST_CC) $(HOST_ASFLAGS) -o $@ -c $<

%.o: %.c
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@

%.o: %.S
	$(TARGET_CC) $(TARGET_ASFLAGS) -o $@ -c $<

ifdef CONFIG_DEBUG
TARGET_CFLAGS+=-g -D_DEBUG_
HOST_CFLAGS+=-g -D_DEBUG_
else
TARGET_CFLAGS+=-fomit-frame-pointer
endif

.PHONY: $(clean-targets) $(config-targets)
dep.mk: $(SRCS)
# don't generate deps  when cleaning
ifeq ($(findstring $(MAKECMDGOALS), $(clean-targets) $(config-targets) ),)
	@for file in $(SRCS) ; do \
		$(TARGET_CC) $(TARGET_CFLAGS) -M $$file ; \
	done > dep.mk

#	@for file in $(SRCS) ; do \
		$(TARGET_CC) $(TARGET_CFLAGS) -M $$file | sed -e "s/.*:/`dirname $$file`\/&/" ; \
	done > dep.mk
endif

dephost.mk: $(HOST_SRCS)
# don't generate deps  when cleaning
ifeq ($(findstring $(MAKECMDGOALS), $(clean-targets) $(config-targets) ),)
	@for file in $(HOST_SRCS) ; do \
		$(HOST_CC) $(HOST_CFLAGS) -M $$file | sed -e "s/\(.*\).o:/`dirname $$file`\/\1.host.o:/" ; \
	done > dephost.mk
endif

