all: tools

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk

DIRS=\
	xmpack\
	xmcparser\
	xmbuildinfo\
	rswbuild\
	xef

tools:
	@for dir in $(DIRS) ; do \
		echo "  - tools/$$dir"; \
		$(MAKE) -C $$dir install > /dev/null ; \
	done

distclean: clean

clean:
	@find -name "*~" -exec rm '{}' \;
	@find -name "*.o" -exec rm '{}' \;
	@find -name "*.a" -exec rm '{}' \;
	@for dir in $(DIRS) ; do \
		$(MAKE) -C $$dir clean > /dev/null ; \
		$(MAKE) -C $$dir uninstall > /dev/null ; \
	done
