.PHONY: xm showsizes scripts

all: xm

include xmconfig

ifndef XTRATUM_PATH
XTRATUM_PATH=.
-include path.mk

path.mk:
	@exec echo -e "\n# Automatically added by XM" > path.mk
	@exec echo -e "# Please don't modify" >> path.mk
	@exec echo -e "XTRATUM_PATH=`pwd`" >> path.mk
	@exec echo -e "export XTRATUM_PATH" >> path.mk
	@cat path.mk >> xmconfig
	@$(RM) -f path.mk
endif

include version
include config.mk

scripts: 
	@exec echo -e "\n> Building Kconfig:";
	@$(MAKE) -s -C scripts/kconfig conf mconf || exit 1

config oldconfig silentoldconfig menuconfig $(defconfig-targets): scripts
ifeq ($(MAKECMDGOALS),menuconfig)
	@exec echo -e "\nYou have to configure three different elements:"
	@exec echo -e "1.- The XtratuM itself."
	@exec echo -e "2.- The Resident Software, which is charge of loading the system from ROM -> RAM."
	@exec echo -e "3.- The XAL, a basic partition execution environment."
	@exec echo -en "\nPress 'Enter' to configure Xtratum (step 1)"
	@read dummy
	@$(MAKE) -s -C $(XTRATUM_PATH)/core $(MAKECMDGOALS);
	@exec echo -en "Press 'Enter' to configure Resident Sw (step 2)"
	@read dummy
	@$(MAKE) -s -C $(XTRATUM_PATH)/user/bootloaders/rsw $(MAKECMDGOALS)
	@exec echo -en "Press 'Enter' to configure XAL (step 3)"
	@read dummy
	@$(MAKE) -s -C $(XTRATUM_PATH)/user/xal $(MAKECMDGOALS)
	@exec echo -e "\nNext, you may run 'make'"
else
	@$(MAKE) -s -C $(XTRATUM_PATH)/core $(MAKECMDGOALS);
	@$(MAKE) -s -C $(XTRATUM_PATH)/user/bootloaders/rsw $(MAKECMDGOALS)
	@$(MAKE) -s -C $(XTRATUM_PATH)/user/xal $(MAKECMDGOALS)
endif

xm: $(XTRATUM_PATH)/core/include/autoconf.h
	@exec echo -en "\n> Configuring and building the \"XtratuM hypervisor\""
	@$(MAKE) -s -C core || exit 1
	@exec echo -en "\n> Configuring and building the \"User utilities\""
	@$(MAKE) -s -C user || exit 1


distclean: clean
	@make -s -C $(XTRATUM_PATH)/user distclean
	@make -C $(XTRATUM_PATH)/core clean
	@exec echo -e "> Cleaning up XM";
	@exec echo -e "  - Removing dep.mk Rules.mk files";
	@find -type f \( -name "dep.mk" -o -name dephost.mk \) -exec rm '{}' \;
	@find -type f \( -name ".config" -o -name .config.old \) -exec rm '{}' \;
	@find -type f -name "autoconf.h" -exec rm '{}' \;
	@find -type f -name ".menuconfig.log" -exec rm '{}' \;
	@find -type f \( -name "mconf" -o -name "conf" \) -exec rm '{}' \;

	@find -type f -name "partition?" -exec rm '{}' \;
	@find -type f -name "resident_sw" -exec rm '{}' \;
	@find -type f -name "*.c.xmc" -exec rm '{}' \;
	@find -type f -name "*.bin.xmc" -exec rm '{}' \;
	@find -type f -name "*.xef.xmc" -exec rm '{}' \;
	@find -type f -name "*.xef" -exec rm '{}' \;
	@find -type f -name "xm_cf" -exec rm '{}' \;

	@find -type f \( -name xmeformat -o -name xmbuildinfo -o -name xmpack -o -name xmcparser -o -name xmcpartcheck -o -name xmccheck \) -exec rm '{}' \;

	@find -type l -exec rm '{}' \;
	@$(RM) -rf $(XTRATUM_PATH)/core/include/config/*
	@$(RM) -rf $(XTRATUM_PATH)/user/xal/include/config/*
	@$(RM) -rf $(XTRATUM_PATH)/user/bootloaders/rsw/include/config/* $(XTRATUM_PATH)/user/bootloaders/rsw/$(ARCH)/rsw.lds
	@$(RM) -f $(XTRATUM_PATH)/xmconfig $(XTRATUM_PATH)/core/include/autoconf.h $(XTRATUM_PATH)/core/include/$(ARCH)/asm_offsets.h $(XTRATUM_PATH)/core/include/$(ARCH)/brksize.h $(XTRATUM_PATH)/core/include/$(ARCH)/ginfo.h $(XTRATUM_PATH)/scripts/lxdialog/lxdialog $(XTRATUM_PATH)/core/xm_core $(XTRATUM_PATH)/core/xm_core.bin $(XTRATUM_PATH)/core/xm_core.xef $(XTRATUM_PATH)/core/build.info $(XTRATUM_PATH)/core/kernel/$(ARCH)/xm.lds $(XTRATUM_PATH)/scripts/extractinfo user/bin/rswbuild $(XTRATUM_PATH)/core/module.lds $(XTRATUM_PATH)/core/module.lds.in
	@$(RM) -f $(XTRATUM_PATH)/core/Kconfig.ver $(XTRATUM_PATH)/core/include/comp.h
	@$(RM) $(DISTRORUN) $(DISTROTAR)
	@exec echo -e "> Done";

clean:
	@exec echo -e "> Cleaning XM";
	@exec echo -e "  - Removing *.o *.a *~ files";
	@find \( -name "*~" -o -name "*.o" -o -name "*.a" -o -name "*.xo" \) -exec rm '{}' \;
	@find -name "*.gcno" -exec rm '{}' \;
	@find -name "*.bin" -exec rm '{}' \;
	@$(RM) -f $(XTRATUM_PATH)/core/build.info $(XTRATUM_PATH)/user/tools/xmcparser/xmc.xsd $(XTRATUM_PATH)/$(DISTRO).run
	@exec echo -e "> Done";

DISTRO	= xtratum-$(XTRATUMVERSION)
DISTROTMP=/tmp/$(DISTRO)-$$PPID
DISTROTAR = $(DISTRO).tar.bz2
$(DISTROTAR): xm
	@$(RM) $(DISTROTAR)
	@make -s -C user/xal/examples -f Makefile clean
	@mkdir $(DISTROTMP) || exit 0
	@user/bin/xmdistro $(DISTROTMP)/$(DISTRO) $(DISTROTAR)
	@$(RM) -r $(DISTROTMP)

DISTRORUN = $(DISTRO).run
DISTROLABEL= "XtratuM binary distribution $(XTRATUMVERSION): "
$(DISTRORUN): $(DISTROTAR)
	@which makeself >/dev/null || (echo "Error: makeself program not found; install the makeself package" && exit -1)
	@mkdir $(DISTROTMP) || exit 0
	@tar xf $(DISTROTAR) -C $(DISTROTMP)
	@/bin/echo "> Generating self extracting binary distribution \"$(DISTRORUN)\""
	@makeself --bzip2 $(DISTROTMP)/$(DISTRO) $(DISTRORUN) $(DISTROLABEL) ./xtratum-installer > /dev/null 2>&1
	@#| tr "\n" "#" | sed -u "s/[^#]*#/./g; s/..././g"
	@$(RM) $(DISTROTAR)
	@$(RM) -r $(DISTROTMP)
	@/bin/echo -e "> Done\n"

distro-tar: $(DISTROTAR)
distro-run: $(DISTRORUN)
