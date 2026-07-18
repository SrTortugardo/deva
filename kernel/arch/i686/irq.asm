BITS 32

extern irq_handler_c

section .text

global irq0_handler
irq0_handler:
    cli
    push byte 0
    push byte 32
    jmp irq_common_stub

global irq1_handler
irq1_handler:
    cli
    push byte 0
    push byte 33
    jmp irq_common_stub

irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax

    push esp
    call irq_handler_c
    add esp,4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp,8
    iret
