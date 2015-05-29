
$(if $(XAL_PATH),, \
	$(warning "The configuration variable XAL_PATH is not set,") \
	$(error "check the \"common/mkconfig.dist\" file (see README)."))

ifneq ($(XAL_PATH)/common/config.mk, $(wildcard $(XAL_PATH)/common/config.mk))
XAL_PATH=../..
XTRATUM_PATH=../../../..
else
is_installed := 1
include $(XAL_PATH)/common/config.mk
endif


# early detect of misconfiguration and missing variables
$(if $(XTRATUM_PATH),, \
	$(warning "The configuration variable XTRATUM_PATH is not set,") \
	$(error "check the \"common/mkconfig.dist\" file (see README)."))

#INCLUDES OF XTRATUM CONFIGURATION
include $(XTRATUM_PATH)/xmconfig
include $(XTRATUM_PATH)/version
ifdef is_installed
include $(XTRATUM_PATH)/lib/rules.mk
else
include $(XTRATUM_PATH)/user/rules.mk
endif

#PATHS
ifdef is_installed
XMBIN_PATH=$(XTRATUM_PATH)/bin
XMCORE_PATH=$(XTRATUM_PATH)/lib
else
XMBIN_PATH=$(XTRATUM_PATH)/user/bin
XMCORE_PATH=$(XTRATUM_PATH)/core
endif
XALLIB_PATH=$(XAL_PATH)/lib
XALBIN_PATH=$(XAL_PATH)/bin

# APPLICATIONS
XMCPARSER=$(XMBIN_PATH)/xmcparser
XMPACK=$(XMBIN_PATH)/xmpack
RSWBUILD=$(XMBIN_PATH)/rswbuild
XEF=$(XMBIN_PATH)/xmeformat build
GRUBISO=$(XMBIN_PATH)/grub_iso
XPATHSTART=$(XALBIN_PATH)/xpathstart

# XM CORE
XMCORE_ELF=$(XMCORE_PATH)/xm_core
XMCORE_BIN=$(XMCORE_PATH)/xm_core.bin
XMCORE=$(XMCORE_PATH)/xm_core.xef

#LIBRARIES
LIB_XM=-lxm
LIB_XAL=-lxal

#FLAGS
TARGET_CFLAGS += -I$(XAL_PATH)/include -fno-builtin

ifneq ($(EXTERNAL_LDFLAGS),y)
TARGET_LDFLAGS += -u start -u xmImageHdr -T$(XALLIB_PATH)/loader.lds\
	-L$(LIBXM_PATH) -L$(XALLIB_PATH)\
	--start-group $(LIBGCC) $(LIB_XM) $(LIB_XAL) --end-group
endif

# ADDRESS OF EACH PARTITION
# function usage: $(call xpathstart,partitionid,xmlfile)
# xpathstart = $(shell $(XPATHSTART) $(1) $(2))
xpathstart = $(shell $(XAL_PATH)/bin/xpath -c -f $(2) '/xm:SystemDescription/xm:PartitionTable/xm:Partition['$(1)']/xm:PhysicalMemoryAreas/xm:Area[1]/@start')


%.xef:  %
	$(XEF) $< -c -o $@

%.xef.xmc: %.bin.xmc
	$(XEF) -m $< -c -o $@

xm_cf.bin.xmc: xm_cf.$(ARCH).xml # $(XMLCF)
	$(XMCPARSER) -o $@ $^

xm_cf.c.xmc: xm_cf.$(ARCH).xml # $(XMLCF)
	$(XMCPARSER) -c -o $@ $^


resident_sw: container.bin
	$(RSWBUILD) $^ $@
	
resident_sw.iso: resident_sw
	$(GRUBISO) $@ $^

distclean: clean
	@$(RM) *~

clean:
	@$(RM) $(PARTITIONS) $(patsubst %.bin,%, $(PARTITIONS)) $(patsubst %.xef,%, $(PARTITIONS))
	@$(RM) container.bin resident_sw xm_cf xm_cf.bin xm_cf.*.xmc
	@$(RM) *.o *.*.xmc dep.mk
	@$(RM) *.iso
