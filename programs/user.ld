ENTRY(_start)

SECTIONS
{
    /* Base clasica para ET_EXEC i386: por encima del kernel identity-mapped
     * (que ocupa los primeros 32 MiB) y por debajo del area privada de la
     * tarea en 0xA0000000. */
    . = 0x08048000;

    .text :
    {
        *(.text*)
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
