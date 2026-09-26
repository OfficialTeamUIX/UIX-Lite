/* The patch code: one section added after the last retail section. */
ENTRY(patch_start)
SECTIONS
{
	. = 0x001fe000;
	.text   : { *(.text*) }
	.rodata : { *(.rodata*) }
	.data   : { *(.data*) *(.bss*) *(COMMON) }
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.llvm_addrsig) }
}
