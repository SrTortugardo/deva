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

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
}
