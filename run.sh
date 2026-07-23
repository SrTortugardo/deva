#!/bin/sh
qemu-system-x86_64 -boot order=d -hda build/disk.img -cdrom build/deva.iso -m 512
