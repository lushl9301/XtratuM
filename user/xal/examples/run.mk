run.sparcv8:
	@$(MAKE) clean
	@$(MAKE)
	@tsim-leon3 -mmu -c tbatch
	
run.x86:
	@$(MAKE) clean
	@$(MAKE) resident_sw.iso
	@qemu -m 512 -cdrom resident_sw.iso -serial stdio -boot d

run: run.$(ARCH)
	
.PHONY: run run.$(ARCH)
