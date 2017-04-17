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
	/**** Code ****/
	.text 0x80002000 :
	{
		__text_start = ABSOLUTE(.); /* Start of code */

		KEEP(*scds2_start.o(.text*))
		*(.text*)
		*(.gnu.linkonce.*)
		*(__libc*)

		__text_end = ABSOLUTE(.); /* End of code */
	} > ram

	/**** Read-only data ****/

	.rodata BLOCK(8) :
	{
		__rodata_start = ABSOLUTE(.); /* Start of read-only data */

		*(.rodata*)

		__rodata_end = ABSOLUTE(.);
	} > ram

	/**** Initialised data ****/
	.data BLOCK(8) :
	{
		__data_start = ABSOLUTE(.); /* Start of initialised data */

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

		. = ALIGN(8);
		/* Make _gp able to refer to about 64 KiB of data, not just the 32 KiB
		 * after this. */
		_gp = ABSOLUTE(ALIGN(16) + 0x7FF0); /* Base of small data */

		*(.lit8)
		*(.lit4)
		*(.sdata)

		. = ALIGN(8);
		__data_end = ABSOLUTE(.); /* End of initialised data */
	} > ram

	/**** Uninitialised data ****/
	__bss_start = ABSOLUTE(.); /* Start of uninitialised data to be zeroed */
	.sbss :
	{
		*(.sbss)
		*(.scommon)
	}

	.bss :
	{
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(4);
	__bss_end = ABSOLUTE(.); /* End of uninitialised data to be zeroed */

	.noinit (NOLOAD) : /* Completely uninitialised data */
	{
		*(.noinit)
	}

	/* Align the heap and stack to 8 bytes */
	. = ALIGN(8);
	__all_end = ABSOLUTE(.); /* Where the heap and stack may start */

	.gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
	.gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }

	PROVIDE(__stack = __all_end + 0x100000);
	/* After the stack, there may be a 16-byte MIPS argument area. Start the
	 * heap after that. */
	PROVIDE(__heap_start = __stack + 0x10);
	PROVIDE(__heap_end = 0x81FFFFF0);

	/DISCARD/ :
	{
		*(.reginfo)
	}

	_end = ABSOLUTE(.);
}
