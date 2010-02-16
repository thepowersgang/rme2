/*
 * AcessMicro Kernel
 * Linker Script
 */

ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	. = 0x100000;
	.multiboot : AT(ADDR(.multiboot)) {
		*(.multiboot)
	}
	
	.text ALIGN(0x1000): AT(ADDR(.text)) {
		*(.text)
	}
	
	.rodata ALIGN(0x1000): AT(ADDR(.rodata)) {
		*(.rodata)
		*(.rdata)
	}
	
	/*
	.debug_abbrev : { *(.debug_abbrev) }
	.debug_info : { *(.debug_info) }
	.debug_line : { *(.debug_line) }
	.debug_loc : { *(.debug_loc) }
	.debug_pubnames : { *(.debug_pubnames) }
	.debug_aranges : { *(.debug_aranges) }
	.debug_ranges : { *(.debug_ranges) }
	.debug_str : { *(.debug_str) }
	.debug_frame : { *(.debug_frame) }
	*/
	
	.padata ALIGN (0x1000) : AT(ADDR(.padata)) {
		*(.padata)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data)) {
		*(.data)
	}

	.bss : AT(ADDR(.bss)) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
	gKernelEnd = (. + 0xFFF)&0xFFFFF000;
}
