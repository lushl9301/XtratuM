all: links user
include ../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk

XMUSR_SUBDIRS = libxm tools xal bootloaders/rsw

.PHONY: links clean_links

links:
	@if test -d $(LIBXM_PATH)/include/xm_inc; then \
		$(MAKE) -s -C . clean-silent;\
	fi
	@if ! test -d $(LIBXM_PATH)/include/arch; then \
		ln -sf $(LIBXM_PATH)/include/$(ARCH) $(LIBXM_PATH)/include/arch; \
	fi
	@if ! test -d $(LIBXM_PATH)/include/xm_inc; then \
		ln -sf $(XM_CORE_PATH)/include $(LIBXM_PATH)/include/xm_inc; \
	fi
	@if ! test -f $(LIBXM_PATH)/common/md5.c; then \
		ln -s $(XTRATUM_PATH)/core/klibc/md5.c $(LIBXM_PATH)/common/md5.c ; \
	fi

clean_links:
	@$(RM) -f $(LIBXM_PATH)/include/arch $(LIBXM_PATH)/include/xm_inc

user:
	@exec echo -e "\n> Building XM user";
	@for dir in $(XMUSR_SUBDIRS) ; do \
		echo "  - $$dir"; \
		$(MAKE) -C $$dir all ; \
	done
	@exec echo "> Done";

distclean: clean_links
	@exec echo -e "\n> Cleaning XM user";
	@for dir in $(XMUSR_SUBDIRS) ; do \
		echo "  - $$dir"; \
		$(MAKE) -C $$dir distclean ; \
	done
	@exec echo "> Done";
clean:
	@exec echo -e "> Cleaning XM user";
	@exec echo -e "  - Removing *.o *.a *~ files";
	$(MAKE) -s -C . clean-silent
	@exec echo -e "> Done";

clean-silent:
	@find -name "*~" -exec rm '{}' \;
	@find -name "*.o" -exec rm '{}' \;
	@find -name "*.a" -exec rm '{}' \;

