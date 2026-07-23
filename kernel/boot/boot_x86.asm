BITS 32

section .multiboot
align 4

MBALIGN  equ 1<<0
MEMINFO  equ 1<<1 ; Pedir memoria
VIDEO    equ 1<<2 ; Pedir modo grafico

FLAGS    equ MBALIGN | MEMINFO | VIDEO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

dd MAGIC
dd FLAGS
dd CHECKSUM

; campos no usados
dd 0
dd 0
dd 0
dd 0
dd 0

; Video : 800x600x32
dd 0
dd 800
dd 600
dd 32

; Pila

section .bss
align 16

stack_bottom:
    resb 32768      ; 32 KiB

stack_top:

; codigo
section .text
global kernel_entry
extern kmain

kernel_entry:

    cli

    ; Inicializar la pila
    mov esp, stack_top

    ; Alinear a 16 bytes
    and esp, -16

    ; aqui es pasar parametros a C
    ; EAX = magic
    ; EBX = multiboot_info_t*
    push ebx
    push eax

    call kmain

.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
