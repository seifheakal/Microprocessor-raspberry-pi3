SECTIONS
{
    . = 0x80000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    . = ALIGN(4096);
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    .cpu1Stack :
    {
        . = ALIGN(16);
        __stack_start  = .;
        . = . + 4096;
        __cpu1_stack  = .;
    }
    .cpu2Stack :
    {
        . = ALIGN(16);
        . = . + 4096;
        __cpu2_stack  = .;
    }
    .cpu3Stack :
    {
        . = ALIGN(16);
        . = . + 4096;
        __cpu3_stack  = .;
    }
    . = ALIGN(4096);
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;