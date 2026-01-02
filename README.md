# YuuOS

YuuOS is a minimalist, monolithic operating system kernel designed for the RISC-V 32-bit architecture. It is built as an educational project to demonstrate core operating system concepts. This project is an implementation based on the operating system described in [operating-system-in-1000-lines](https://operating-system-in-1000-lines.vercel.app/ja/).

## Features

- **Monolithic Kernel:** A simple, all-in-one kernel structure.
- **RISC-V 32-bit Support:** Specifically designed for the `rv32im` architecture.
- **Pre-emptive Multitasking:** Supports multiple processes with a simple round-robin scheduler.
- **Virtual Memory:** Implements demand paging for memory management.
- **System Calls:** Provides a basic set of system calls for user-space applications.
- **VirtIO Block Device Driver:** Includes a driver for `virtio-blk` devices.
- **Simple Filesystem:** A basic, tar-based filesystem for file management.
- **User-space Shell:** A simple shell provides a command-line interface for interacting with the OS.

## Components

- `kernel.c`, `kernel.h`: The core kernel code, including process management, memory management, and system call handling.
- `common.c`, `common.h`: Common utility functions.
- `user.c`, `user.h`: User-space library for system calls.
- `shell.c`: A simple command-line shell.
- `kernel.ld`, `user.ld`: Linker scripts for the kernel and user applications.
- `run.sh`: A script to build and run the OS using QEMU.

## Building and Running

To build and run YuuOS, you will need:

- `clang` targeting `riscv32-unknown-elf`
- `llvm-objcopy`
- `qemu-system-riscv32`

The `run.sh` script automates the process of compiling the kernel and shell, creating a disk image, and launching the OS in QEMU.

```bash
./run.sh
```

## Project Structure

```
.
├── common.c
├── common.h
├── disk/
│   ├── hello.txt
│   └── meow.txt
├── kernel.c
├── kernel.h
├── kernel.ld
├── lorem.txt
├── run.sh
├── shell.c
├── user.c
├── user.h
└── user.ld
```

- **`kernel.*`**: Files related to the kernel itself.
- **`user.*`**: Files related to user-space applications.
- **`shell.c`**: The user-space shell application.
- **`common.*`**: Common code shared between the kernel and user-space.
- **`disk/`**: Contains files that will be included in the initial disk image.
- **`run.sh`**: The main script to build and run the OS.
