# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition1.xef partition2.xef partition3.xef 

partition1: supervisor.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 $(XMLCF))

partition2: partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 1 $(XMLCF))

partition3: partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 2 $(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition1.xef\
	-p 1:partition2.xef\
	-p 2:partition3.xef\

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) build $(PACK_ARGS) $@
	@exec echo -en "> Done [container]\n"
	
include ../run.mk
