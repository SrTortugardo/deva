BITS 32

section .text

; void context_switch(uint32_t *old_esp, uint32_t new_esp)
;
; Guarda el contexto de la tarea actual en su pila, almacena el puntero
; en *old_esp y cambia a la pila new_esp para reanudar esa tarea.
global context_switch
context_switch:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ebp, esp ; EBP = pila actual ya con el frame guardado

    ; Argumentos via stack: [ebp+52] = old_esp_ptr, [ebp+56] = new_esp
    mov eax, [ebp + 52]
    mov [eax], ebp        ; *old_esp = ESP actual

    mov ecx, [ebp + 56]
    mov esp, ecx          ; Cambiar a la nueva pila

    ; Restaurar contexto de la nueva tarea
    pop gs
    pop fs
    pop es
    pop ds
    popa

    ; Las tareas nuevas arrancan con IF=0 (entraron via interrupt gate y
    ; popa no restaura EFLAGS). sti inhibe IRQs hasta despues de la
    ; siguiente instruccion, asi que sti; ret es seguro: el ret salta al
    ; entry con IF=1. Para tareas ya en ejecucion es un no-op (el iret
    ; final restaurara EFLAGS de todos modos).
    sti
    ret
