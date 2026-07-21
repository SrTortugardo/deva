# Detectar arquitectura del host
HOST_ARCH := $(shell uname -m)

ifeq ($(HOST_ARCH),aarch64)
# en aarch64 necesitamos crossdev
CC := i686-elf-gcc
LD := i686-elf-ld
else
# en x86_64 usamos el compilador del sistema
CC := clang
LD := ld.lld
endif

AS := nasm

CFLAGS := \
	-m32 \
	-ffreestanding \
	-nostdlib \
	-nostdinc \
	-fno-pic \
	-fno-pie \
	-fno-stack-protector \
	-Ikernel/include

LDFLAGS := -m elf_i386 -T kernel/ld/kernel.ld

BUILD := build
ISO := iso

C_SOURCES := \
	kernel/arch/i686/hlt.c \
	kernel/arch/i686/idt.c \
	kernel/arch/i686/irq.c \
	kernel/arch/i686/isr.c \
	kernel/arch/i686/syscall.c \
	kernel/assets/font8x8.c \
	kernel/drivers/framebuffer/framebuffer.c \
	kernel/drivers/text/text.c \
	kernel/drivers/term/term.c \
	kernel/drivers/gdt/gdt.c \
	kernel/drivers/devafs/devafs.c \
	kernel/drivers/ata/ata.c \
	kernel/drivers/kbd/kbd.c \
	kernel/drivers/vfs/vfs.c \
	kernel/drivers/task/task.c \
	kernel/klib/string/string.c \
	kernel/kmain.c

ASM_SOURCES := \
	kernel/boot/boot_x86.asm \
	kernel/arch/i686/idt.asm \
	kernel/arch/i686/irq.asm \
	kernel/arch/i686/isr.asm \
	kernel/arch/i686/context_switch.asm

# objetos en build/kernel/:
#   C:   build/kernel/<basename>.o
#   ASM: build/kernel/<basename>.asm.o (para evitar colisiones si hay un c con el mismo nombgre)
C_OBJECTS := $(addprefix $(BUILD)/kernel/,$(notdir $(C_SOURCES:.c=.o)))
ASM_OBJECTS := $(addprefix $(BUILD)/kernel/,$(notdir $(ASM_SOURCES:.asm=.asm.o)))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.asm $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD)/deva.iso

$(BUILD)/deva.iso: $(BUILD)/kernel.elf
	@mkdir -p $(BUILD)
	cp $< $(ISO)/boot/kernel.bin
	grub-mkrescue -o $@ $(ISO)
	sh scripts/makefs.sh

$(BUILD)/kernel.elf: $(OBJECTS) kernel/ld/kernel.ld
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(C_OBJECTS): $(BUILD)/kernel/%.o : %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_OBJECTS): $(BUILD)/kernel/%.asm.o : %.asm
	@mkdir -p $(@D)
	$(AS) -f elf32 $< -o $@

run: all
	qemu-system-x86_64 \
		-hda build/disk.img \
		-cdrom build/deva.iso \
		-m 32

clean:
	rm -fr build/*
	rm -fr iso/boot/kernel.bin

.PHONY: all run clean
