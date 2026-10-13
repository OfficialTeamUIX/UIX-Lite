ENTRY(patch_xonline_createfile)
SECTIONS
{
    . = 0x00158d00;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.llvm_addrsig) }
}
