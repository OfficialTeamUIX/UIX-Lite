/* The patch code: one section added after the last retail section. */
ENTRY(patch_start)
SECTIONS
{
	. = 0x001fe000;
	.text   : { *(EXCLUDE_FILE (*ftp_tune.o) .text*) }
	.rodata : { *(EXCLUDE_FILE (*ftp_tune.o) .rodata*) }
	.data   : {
		*(EXCLUDE_FILE (*ftp_tune.o) .data*)
		*(EXCLUDE_FILE (*ftp_tune.o) .bss*)
		*(EXCLUDE_FILE (*ftp_tune.o) COMMON)
	}
	/* Keep optional helpers after the established payload so they cannot move
	   the working hook functions and global data. */
	.ftp_tune : {
		*ftp_tune.o(.text*)
		*ftp_tune.o(.rodata*)
		*ftp_tune.o(.data*)
		*ftp_tune.o(.bss*)
		*ftp_tune.o(COMMON)
	}
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.llvm_addrsig) }
}
