all: xmbuildinfo

INSTALL=xmbuildinfo

include ../../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk
include $(XTRATUM_PATH)/user/install.mk

xmbuildinfo: xmbuildinfo.in
	# replace by s/.../$(\1)/
	cp $^ $@
	sed -i "s|\$$TARGET_CC |$(TARGET_CC) |g" $@
	sed -i 's|\$$TARGET_CFLAGS |$(TARGET_CFLAGS) |g' $@
	sed -i "s|\$$TARGET_CFLAGS_ARCH |$(TARGET_CFLAGS_ARCH) |g" $@
	sed -i "s|\$$TARGET_LD |$(TARGET_LD) |g" $@
	sed -i "s|\$$TARGET_LDFLAGS_ARCH |$(TARGET_LDFLAGS_ARCH) |g" $@
	sed -i "s|\$$TARGET_AS |$(TARGET_AS) |g" $@
	sed -i "s|\$$TARGET_OBJCOPY |$(TARGET_OBJCOPY) |g" $@

	sed -i "s|\$$XM_VERSION|$(XM_VERSION)|g" $@
	sed -i "s|\$$XM_SUBVERSION|$(XM_SUBVERSION)|g" $@
	sed -i "s|\$$XM_REVISION|$(XM_REVISION)|g" $@
