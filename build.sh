#!/bin/sh
set -e

if [ "$(uname -m)" = "aarch64" ]; then
    COMPILER=i686-elf-gcc
    LINKER=i686-elf-ld
    AR=i686-elf-ar
else
    # x86_64, se puede compilar con las herramientas del host
    COMPILER=clang
    LINKER=ld.lld
    AR=llvm-ar
fi

AS=nasm # no depende de la arquitectura
# empezamos limpiando todo
sh scripts/clean.sh

kernel() {
    CFLAGS="-m32 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie -fno-stack-protector -Ikernel/include"

    mkdir -p build/kernel/
    # ASM
    $AS -f elf32 kernel/boot/boot_x86.asm -o build/kernel/kernel_asm.o
    $AS -f elf32 kernel/arch/i686/idt.asm -o build/kernel/idt_s.o
    $AS -f elf32 kernel/arch/i686/irq.asm -o build/kernel/irq_s.o
    $AS -f elf32 kernel/arch/i686/isr.asm -o build/kernel/isr_s.o
    $AS -f elf32 kernel/arch/i686/context_switch.asm -o build/kernel/cw_s.o

    # ARCH
    $COMPILER $CFLAGS -c kernel/arch/i686/hlt.c -o build/kernel/hlt.o
    $COMPILER $CFLAGS -c kernel/arch/i686/idt.c -o build/kernel/idt.o
    $COMPILER $CFLAGS -c kernel/arch/i686/irq.c -o build/kernel/irq.o
    $COMPILER $CFLAGS -c kernel/arch/i686/isr.c -o build/kernel/isr.o
    $COMPILER $CFLAGS -c kernel/arch/i686/syscall.c -o build/kernel/syscall.o

    # ASSETS
    $COMPILER $CFLAGS -c kernel/assets/font8x8.c -o build/kernel/font8x8.o

    # DRIVERS
    $COMPILER $CFLAGS -c kernel/drivers/drivers.c -o build/kernel/drivers.o
    $COMPILER $CFLAGS -c kernel/drivers/framebuffer/framebuffer.c -o build/kernel/framebuffer.o
    $COMPILER $CFLAGS -c kernel/drivers/text/text.c -o build/kernel/text.o
    $COMPILER $CFLAGS -c kernel/drivers/term/term.c -o build/kernel/term.o
    $COMPILER $CFLAGS -c kernel/drivers/gdt/gdt.c -o build/kernel/gdt.o
    $COMPILER $CFLAGS -c kernel/drivers/elf/elf.c -o build/kernel/elf.o
    $COMPILER $CFLAGS -c kernel/drivers/paging/paging.c -o build/kernel/paging.o
    $COMPILER $CFLAGS -c kernel/drivers/task/task.c -o build/kernel/task.o
    $COMPILER $CFLAGS -c kernel/drivers/fat32/fat32.c -o build/kernel/fat32.o
    $COMPILER $CFLAGS -c kernel/drivers/vfs/vfs.c -o build/kernel/vfs.o
    $COMPILER $CFLAGS -c kernel/drivers/ata/ata.c -o build/kernel/ata.o
    $COMPILER $CFLAGS -c kernel/drivers/mm/mm.c -o build/kernel/mm.o
    $COMPILER $CFLAGS -c kernel/drivers/kbd/kbd.c -o build/kernel/kbd.o

    # KLIB
    $COMPILER $CFLAGS -c kernel/klib/string/string.c -o build/kernel/string.o

    # KMAIN
    $COMPILER $CFLAGS -c kernel/kmain.c -o build/kernel/kmain.o

    # Linkear
    $LINKER -m elf_i386 -T kernel/ld/kernel.ld -o build/kernel.elf build/kernel/*.o

    cp build/kernel.elf iso/boot/kernel.bin
    grub-mkrescue -o build/deva.iso iso
}

libc() {
    mkdir -p build/libc/
    CFLAGS="-m32 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie -fno-stack-protector -Ilibc/"

    # Compilar todos los .c del libc a objetos separados.
    $COMPILER $CFLAGS -c libc/syscall.c -o build/libc/syscall.o
    $COMPILER $CFLAGS -c libc/io.c     -o build/libc/io.o
    $COMPILER $CFLAGS -c libc/string.c -o build/libc/string.o
    $COMPILER $CFLAGS -c libc/proc.c -o build/libc/proc.o
    $COMPILER $CFLAGS -c libc/vfs.c -o build/libc/vfs.o
    $COMPILER $CFLAGS -c libc/mem.c -o build/libc/mem.o

    # Empaquetar en libc.a (archivo estatico). -rcs: replace, create, write index.
    $AR rcs build/libc/libc.a build/libc/*
}

programs() {
    mkdir -p build/program disk/bin
    CFLAGS="-m32 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie -fno-stack-protector -Ilibc/"
    LDFLAGS="-m elf_i386 -T programs/user.ld -nostdlib -static"

    # Cada programa se linkea independientemente con el libc.a
    $COMPILER $CFLAGS -c programs/hello.c   -o build/program/hello.o
    $COMPILER $CFLAGS -c programs/drawbmp.c -o build/program/drawbmp.o
    $COMPILER $CFLAGS -c programs/cat.c     -o build/program/cat.o
    $COMPILER $CFLAGS -c programs/ls.c      -o build/program/ls.o
    $COMPILER $CFLAGS -c programs/echo.c    -o build/program/echo.o
    $COMPILER $CFLAGS -c programs/cp.c      -o build/program/cp.o
    $COMPILER $CFLAGS -c programs/rm.c      -o build/program/rm.o
    $COMPILER $CFLAGS -c programs/mv.c      -o build/program/mv.o
    $COMPILER $CFLAGS -c programs/write.c   -o build/program/write.o
    $COMPILER $CFLAGS -c programs/sh.c      -o build/program/sh.o
    $COMPILER $CFLAGS -c programs/clear.c   -o build/program/clear.o

    $LINKER $LDFLAGS -o disk/bin/hello      build/program/hello.o      build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/drawbmp    build/program/drawbmp.o    build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/cat        build/program/cat.o        build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/ls         build/program/ls.o         build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/echo       build/program/echo.o       build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/cp         build/program/cp.o         build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/rm         build/program/rm.o         build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/mv         build/program/mv.o         build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/write      build/program/write.o      build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/sh         build/program/sh.o         build/libc/libc.a
    $LINKER $LDFLAGS -o disk/bin/clear      build/program/clear.o      build/libc/libc.a
}

disk() {
    dd if=/dev/zero of=build/disk.img bs=1M count=5 2>/dev/null
    mkfs.fat -F32 build/disk.img >/dev/null 2>&1
    mcopy -s -i build/disk.img disk/* ::/
}


kernel
libc
programs
disk

echo "build OK"
