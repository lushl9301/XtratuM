all: rsw

include ../../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk
#-include dep.mk

ifeq ($(XTRATUM_PATH)/user/bootloaders/rsw/.config, $(wildcard $(XTRATUM_PATH)/user/bootloaders/rsw/.config))
include $(XTRATUM_PATH)/user/bootloaders/rsw/.config
endif

RSW_PATH=$(XTRATUM_PATH)/user/bootloaders/rsw

RSW-common = common/rsw.c common/stdc.c

RSW-sparcv8-SRC=sparcv8/boot.S sparcv8/serial.c
RSW-x86-SRC=x86/boot.S x86/vga.c

RESIDENT_SW_SRC= $(RSW-common) $(RSW-$(ARCH)-SRC)

RESIDENT_SW := $(patsubst %.S,%.o, $(patsubst %.c,%.o,$(RESIDENT_SW_SRC)))
#RESIDENT_SW := $(patsubst %.S,%.32.o, $(patsubst %.c,%.o,$(RESIDENT_SW_SRC)))

SRCS=$(RESIDENT_SW_SRC)

TARGET_CFLAGS += -I$(RSW_PATH)/include --include config.h -DRSW
TARGET_ASFLAGS += -I$(RSW_PATH)/include --include config.h

KCONFIG_AUTOHEADER=$(RSW_PATH)/include/autoconf.h
$(KCONFIG_AUTOHEADER): silentoldconfig

$(ARCH)/rsw.lds: $(KCONFIG_AUTOHEADER) $(ARCH)/rsw.lds.in
	@$(TARGET_CC) $(TARGET_ASFLAGS) -x assembler-with-cpp -E -P $(ARCH)/rsw.lds.in -o $(ARCH)/rsw.lds

KCONFIG=$(RSW_PATH)/$(ARCH)/Kconfig

config:
	@$(XTRATUM_PATH)/scripts/kconfig/conf $(KCONFIG)

oldconfig:
	@$(XTRATUM_PATH)/scripts/kconfig/conf -o $(KCONFIG)

silentoldconfig:
	@mkdir -p $(RSW_PATH)/include/config
	@$(XTRATUM_PATH)/scripts/kconfig/conf -s $(KCONFIG)

menuconfig:
	@$(XTRATUM_PATH)/scripts/kconfig/mconf $(KCONFIG)

$(defconfig-targets):
	@$(XTRATUM_PATH)/scripts/kconfig/conf -D $(RSW_PATH)/$(ARCH)/$@ $(KCONFIG)

ifdef CONFIG_STAND_ALONE
rsw: $(ARCH)/rsw.lds $(RESIDENT_SW)
	$(TARGET_LD) $(TARGET_LDFLAGS_ARCH) -o rsw $(RESIDENT_SW) -T$(ARCH)/rsw.lds $(LIBGCC) -L$(LIBXM_PATH) -lxm
else
rsw: rsw_objs.o

rsw_objs.o: $(ARCH)/rsw.lds $(RESIDENT_SW)
	@$(TARGET_LD) $(TARGET_LDFLAGS) -r -o rsw_objs.o $(RESIDENT_SW) -L$(LIBXM_PATH) -lxm
endif

distclean: clean

clean:
	@$(RM) rsw *.o *~ $(ARCH)/*.o $(ARCH)/*~ $(ARCH)/rsw.lds common/*.o common/*~
