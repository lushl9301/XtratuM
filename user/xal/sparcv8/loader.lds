/*
 * $FILE$
 * 
 * ld script to build sparcv8
 * 
 * $VERSION$
 *
 * $AUTHOR$
 * 
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/* <track id="PARTITION_LINKER_SCRIPT"> */
/*OUTPUT_FORMAT("binary")*/
OUTPUT_FORMAT("elf32-sparc", "elf32-sparc", "elf32-sparc")
OUTPUT_ARCH(sparc)
ENTRY(start)

SECTIONS
{	
  _sguest = .;
  .text ALIGN (8): {
	. = ALIGN(4K);
	_sguest = .;
  	*(.text.init)
  	*(.text)
  }
  .rodata ALIGN (8) : {
	*(.rodata)
  	*(.rodata.*)
	*(.rodata.*.*)
  }
  
  .data ALIGN (8) : {
	_sdata = .;
	*(.data)
	_edata = .;
  }

  .bss ALIGN (8) : {
	*(.bss.noinit)
	_sbss = .;
	*(COMMON)
	*(.bss)
	_ebss = .;
  }
  
  _eguest = .;

  /DISCARD/ :
  {
	*(.note)
	*(.comment*)
  }
}
/* </track id="PARTITION_LINKER_SCRIPT"> */
