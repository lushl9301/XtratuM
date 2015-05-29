all: ldr.bin

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/core/rules.mk
#-include dep.mk

SRCS_C-y = ldr.c stdc.c
SRCS_S-y = boot.S

LIBXM=../../user/libxm
TARGET_CFLAGS+=-I$(LIBXM)/include

SRCS = $(SRCS_S-y) $(SRCS_C-y)
OBJS += $(patsubst %.S, %.o, $(SRCS_S-y)) $(patsubst %.c, %.o, $(SRCS_C-y)) 

$(LIBXM)/libxm.a:
	$(MAKE) -s -C $(LIBXM)/../ links
	$(MAKE) -s -C $(LIBXM)

ldr.lds: $(LIBXM)/libxm.a ldr.$(ARCH).lds.in ../include/config.h
	@$(TARGET_CC) $(TARGET_ASFLAGS) -x assembler-with-cpp -E -P -I$(XM_CORE_PATH)/include ldr.$(ARCH).lds.in -o ldr.lds

ldr.elf: ldr.lds $(OBJS)
	@$(TARGET_LD) $(TARGET_LDFLAGS) -o ldr.elf -Tldr.lds $(OBJS) -L$(LIBXM) -lxm $(LIBGCC)

ldr.bin: ldr.elf
	@$(TARGET_OBJCOPY) -O binary ldr.elf ldr.bin
#	$(CC) $(filter-out -pg,$(CFLAGS)) -c gprof.c
