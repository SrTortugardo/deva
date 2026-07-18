#!/bin/sh
set -e

if [ "$(uname -m)" = "aarch64" ]; then
    COMPILER=i686-elf-gcc
    LINKER=i686-elf-ld
else
    # x86_64, se puede compilar con las herramientas del host
    COMPILER=gcc
    LINKER=ld
fi

AS=nasm # no depende de la arquitectura
CFLAGS="-m32 -ffreestanding -nostdlib -Ikernel/include -nostdinc"
RUST_FLAGS="--target i686-unknown-linux-gnu -C panic=abort -O --emit=obj -o " # tengo planeado usar rust

# empezamos limpiando todo
sh scripts/clean.sh

# ASM
$AS -f elf32 kernel/boot/boot.asm -o build/kernel_asm.o

# ARCH
$COMPILER $CFLAGS -c kernel/arch/i686/hlt.c -o build/hlt.o

# ASSETS
$COMPILER $CFLAGS -c kernel/assets/font8x8.c -o build/font8x8.o

# DRIVERS
$COMPILER $CFLAGS -c kernel/drivers/framebuffer/framebuffer.c -o build/framebuffer.o
$COMPILER $CFLAGS -c kernel/drivers/text/text.c -o build/text.o
$COMPILER $CFLAGS -c kernel/drivers/term/term.c -o build/term.o

# KMAIN
$COMPILER $CFLAGS -c kernel/kmain.c -o build/kmain.o

# Linkear
$LINKER -m elf_i386 -T kernel/ld/kernel.ld -o build/kernel.elf build/*.o

cp build/kernel.elf iso/boot/kernel.bin
grub-mkrescue -o build/deva.iso iso
