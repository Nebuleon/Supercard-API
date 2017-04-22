OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)

/**** Start point ****/
ENTRY(_start)

MEMORY
{
	ram (wx) : ORIGIN = 0x80000000, LENGTH = 32M
}

SECTIONS
{
	/**** Uninitialised data ****/
	.bss 0x81FF0000 (NOLOAD) :
	{
		__bss_start = ABSOLUTE(.); /* Start of uninitialised data to be zeroed */
		*(.sbss)
		*(.scommon)
		*(.bss)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = ABSOLUTE(.); /* End of uninitialised data to be zeroed */
	}

	.noinit 0x81FF0000 + SIZEOF(.bss) (NOLOAD) : /* Completely uninitialised data */
	{
		*(.noinit)
	}

	/**** Code ****/
	.text 0x81000000 : AT(0x80002000)
	{
		__text_start = ABSOLUTE(.); /* Start of code */

		KEEP(*start.o(.text*))
		*(.text*)
		*(.gnu.linkonce.*)
		*(__libc*)

		__text_end = ABSOLUTE(.); /* End of code */
	} > ram

	__data_start = ABSOLUTE(.); /* Start of data */

	/**** Data ****/
	.data :
	{
		*(.rodata*)

		*(.data*)
		*(.eh_frame*)
		*(.gcc_except_table*)

		. = ALIGN(4);
		__CTOR_LIST__ = ABSOLUTE(.);
		KEEP(*(SORT(.ctors*)))
		__CTOR_END__ = ABSOLUTE(.);
		__DTOR_LIST__ = ABSOLUTE(.);
		KEEP(*(SORT(.dtors*)))
		__DTOR_END__ = ABSOLUTE(.);

		. = ALIGN(4);

		*(.lit8)
		*(.lit4)
		*(.sdata)

		. = ALIGN(4);
	} > ram

	. = ALIGN(4);
	/* This must imperatively be after the ALIGN directive, which allows other
	 * sections to be placed between __data_end and packed_data automatically
	 * by the linker if they are not explicitly kept.
	 *
	 * The packer will append the packed executable to the concatenated
	 * sections. This tracks its location so that the code can access it. */
	packed_data = ABSOLUTE(.);

	.gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
	.gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }

	. = ADDR(.noinit) + SIZEOF(.noinit);
	/* Align the stack and heap to 8 bytes */
	PROVIDE(__stack = ALIGN(8) + 4096);
	/* After the stack, there may be a 16-byte MIPS argument area. Start the
	 * heap after that. */
	PROVIDE(__heap_start = __stack + 0x10);
	PROVIDE(__heap_end = 0x81FFFFF0);

	/DISCARD/ :
	{
		*(.reginfo)
		*(.MIPS.abiflags)
	}
}
