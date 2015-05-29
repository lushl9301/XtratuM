#SRCS := $(sort $(wildcard *.c))
#OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
#LOBJS := $(OBJS)

all: objs mods

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/core/rules.mk
-include dep.mk

srcs-y := kdevice.c

ifdef CONFIG_SPARCv8
srcs-$(CONFIG_DEV_UART) += leon_uart.c
ifdef CONFIG_EXT_SYNC_MPT_IO
srcs-$(CONFIG_PLAN_EXTSYNC) += spartan6_fpga.c
endif
endif #CONFIG_SPARCv8

ifdef CONFIG_x86
srcs-$(CONFIG_DEV_UART) += pc_uart.c
srcs-$(CONFIG_DEV_VGA) += pc_vga.c
endif

srcs-$(CONFIG_DEV_TTNOC) += ttnocports.c
srcs-$(CONFIG_DEV_TTNOC) += ttnoc/txrx.c ttnoc/lowlevel_io.c ttnoc/port.c ttnoc/ttportcfg.c ttnoc/sync.c ttnoc/regfile.c
srcs-$(CONFIG_DEV_MEMBLOCK) += memblock.c

SRCS := $(srcs-y) $(srcs-m)
OBJS := $(patsubst %.c,%.o,$(srcs-y))
LOBJS := $(OBJS)

OBJS-M := $(patsubst %.c,%.xo,$(srcs-m))

objs: $(OBJS)
	$(TARGET_LD) $(TARGET_LDFLAGS) -r -o drivers.o $(LOBJS)

mods: $(OBJS-M)
