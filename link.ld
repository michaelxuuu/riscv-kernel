
OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

MEMORY
{   
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
  .text : {
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(_text_end = .);
  } >ram

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } >ram

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*)
    . = ALIGN(16);
    *(.data .data.*)
  } >ram

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*)
  } >ram

  . = ALIGN(0x1000);
  PROVIDE(_bss_end = .);
  
	PROVIDE(_ram_end = ORIGIN(ram) + LENGTH(ram));
}

