# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef

partition0: partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 $(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef:custom_file.xef
	
custom_file.xef: custom_file.txt
	$(XEF) -m -o $@ $<

container.bin: $(PARTITIONS) xm_cf.xef.xmc custom_file.xef
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@

include ../run.mk
	