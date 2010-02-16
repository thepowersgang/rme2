;
;
;

[section .multiboot]
    MULTIBOOT_PAGE_ALIGN	equ 1<<0
    MULTIBOOT_MEMORY_INFO	equ 1<<1
    MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
    MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
mboot:
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
	dd mboot	;Location of Multiboot Header
	
[section .text]
[global start]
[extern main]
start:
	
	; Set up stack
	mov esp, _stack
	
	call main
	
	cli
.hlt:
	hlt
	jmp .hlt

[section .bss]
	resb	0x2000
_stack:
