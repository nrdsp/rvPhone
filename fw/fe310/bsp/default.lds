OUTPUT_ARCH("riscv")

ENTRY(_enter)

MEMORY
{
	flash (rxai!w) : ORIGIN = 0x20000000, LENGTH = 0x6a120
	itim (wx!rai) : ORIGIN = 0x8000000, LENGTH = 0x2000
	ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 0x4000
}

PHDRS
{
	flash PT_LOAD;
	ram PT_LOAD;
	ram_init PT_LOAD;
	itim PT_LOAD;
	itim_init PT_LOAD;
}

SECTIONS
{

	__stack_size = 0x1000;
	PROVIDE(__metal_boot_hart = 0);
	PROVIDE(__metal_chicken_bit = 0);

	.init : {
		KEEP (*(.text.metal.init.enter))
		KEEP (*(.text.metal.init.*))
		KEEP (*(SORT_NONE(.init)))
		KEEP (*(.text.libgloss.start))
	} >flash AT>flash :flash

	.fini : {
		KEEP (*(SORT_NONE(.fini)))
	} >flash AT>flash :flash

	.text : {
		*(.text.unlikely .text.unlikely.*)
		*(.text.startup .text.startup.*)
		*(.text .text.*)
		*(.gnu.linkonce.t.*)
	} >flash AT>flash :flash

	PROVIDE (__etext = .);
	PROVIDE (_etext = .);
	PROVIDE (etext = .);

	.rodata : {
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)
		. = ALIGN(8);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
	} >flash AT>flash :flash

	. = ALIGN(8);

	.preinit_array : {
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >flash AT>flash :flash

	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >flash AT>flash :flash

	.fini_array : {
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >flash AT>flash :flash

	.ctors : {
		KEEP (*crtbegin.o(.ctors))
		KEEP (*crtbegin?.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
	} >flash AT>flash :flash

	.dtors : {
		KEEP (*crtbegin.o(.dtors))
		KEEP (*crtbegin?.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
	} >flash AT>flash :flash

	.itim : ALIGN(8) {
		*(.itim .itim.*)
	} >itim AT>flash :itim_init

	PROVIDE( metal_segment_itim_source_start = LOADADDR(.itim) );
	PROVIDE( metal_segment_itim_target_start = ADDR(.itim) );
	PROVIDE( metal_segment_itim_target_end = ADDR(.itim) + SIZEOF(.itim) );

	.data : ALIGN(8) {
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
		*(.sdata .sdata.* .sdata2.*)
		*(.gnu.linkonce.s.*)
	} >itim AT>flash :itim_init
/*	} >ram AT>flash :ram_init	*/

	PROVIDE( metal_segment_data_source_start = LOADADDR(.data) );
	PROVIDE( metal_segment_data_target_start = ADDR(.data) );
	PROVIDE( metal_segment_data_target_end = ADDR(.data) + SIZEOF(.data) );

	.bss : ALIGN(8) {
		*(.ibss .ibss.*)
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
	} >itim AT>ram :itim
/*	} >ram AT>ram :ram	*/

	PROVIDE( metal_segment_bss_target_start = ADDR(.bss) );
	PROVIDE( metal_segment_bss_target_end = ADDR(.bss) + SIZEOF(.bss) );

	__heap_size = 0x4000 - __stack_size;
/*	__heap_size = 0x1000;	*/
	.heap : {
		PROVIDE( metal_segment_heap_target_start = . );
		. += __heap_size;
		PROVIDE( metal_segment_heap_target_end = . );
		PROVIDE( _heap_end = . );
	} >ram AT>ram :ram

	PROVIDE(__stack_size = __stack_size);

	.stack : {
		PROVIDE(metal_segment_stack_begin = .);
		. += __stack_size;
		PROVIDE(metal_segment_stack_end = .);
		PROVIDE( _sp = . );
	} >ram AT>ram :ram

}

