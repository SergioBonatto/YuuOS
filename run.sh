#!/bin/bash
set -xue

QEMU=qemu-system-riscv32
OBJCOPY=llvm-objcopy
CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -march=rv32im -mabi=ilp32 -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib -Iinclude"

# Build shell
$CC $CFLAGS -Iinclude -Wl,-Tsrc/user.ld -Wl,-Map=build/shell.map -o build/shell.elf src/shell.c src/user.c src/common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary build/shell.elf build/shell.bin

# Create shell.bin.o from shell.bin
(cd build; $OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o)

# Build kernel
$CC $CFLAGS -Wl,-Tsrc/kernel.ld -Wl,-Map=build/kernel.map -o build/kernel.elf \
      src/kernel.c src/common.c src/context.c src/process.c src/memory.c build/shell.bin.o

# Run QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot  \
    -d unimp,guest_errors,int,cpu_reset -D qemu.log                         \
    -drive id=disk0,file=disk.tar,format=raw,if=none                        \
    -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0             \
    -kernel build/kernel.elf
