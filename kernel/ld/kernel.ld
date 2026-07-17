ENTRY(kernel_entry)

SECTIONS
{
    . = 0x100000;

    /* codigo */
    .text :
    {
        *(.multiboot)
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    /* ya no pienso usar programas asi, pero bueno, se queda */
    .programs :
    {
        _prog_start = .;
        *(.programs)
        _prog_end = .;
    }

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
}
