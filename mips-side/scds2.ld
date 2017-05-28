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
		*(.text .text.*)
		*(.gnu.linkonce.t.*)

		__text_end = ABSOLUTE(.); /* End of code */
	} > ram

	/**** Read-only data ****/
	__rodata_start = ABSOLUTE(.); /* Start of read-only data */

	.rodata BLOCK(8) :
	{
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.gnu.linkonce.r.*)))
	} > ram

	__rodata_end = ABSOLUTE(.);

	__data_start = ABSOLUTE(.); /* Start of initialised data */

	/**** Initialised data ****/
	.data BLOCK(8) :
	{
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.gnu.linkonce.d.*)))
	} > ram

	.eh_frame BLOCK(4) :
	{
		*(.eh_frame*)
	} > ram

	.gcc_except_table BLOCK(4) :
	{
		*(.gcc_except_table*)
	} > ram

	__CTOR_LIST__ = ABSOLUTE(.);

	.ctors BLOCK(4) :
	{
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
	} > ram

	__CTOR_END__ = ABSOLUTE(.);

	__DTOR_LIST__ = ABSOLUTE(.);

	.dtors BLOCK(4) :
	{
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
	} > ram

	__DTOR_END__ = ABSOLUTE(.);

	/* Make _gp able to refer to about 64 KiB of data, not just the 32 KiB
	 * after this. */
	_gp = ABSOLUTE(ALIGN(16) + 0x7FF0); /* Base of small data */

	.lit8 BLOCK(8) :
	{
		*(.lit8)
	} > ram

	.lit4 BLOCK(4) :
	{
		*(.lit4)
	} > ram

	.sdata BLOCK(8) :
	{
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.sdata)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.sdata.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.gnu.linkonce.s.*)))
		. = ALIGN(8);
	} > ram

	__data_end = ABSOLUTE(.); /* End of initialised data */

	/**** Uninitialised data ****/
	__bss_start = ABSOLUTE(.); /* Start of uninitialised data to be zeroed */

	.sbss :
	{
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.sbss)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.sbss.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.gnu.linkonce.sb.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.scommon)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.scommon.*)))
	}

	.bss :
	{
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.gnu.linkonce.b.*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))
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
