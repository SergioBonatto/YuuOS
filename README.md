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

## Getting Started

Follow these instructions to set up the toolchain and run YuuOS.

### Prerequisites

YuuOS requires a specific toolchain for building the RISC-V 32-bit kernel and user-space applications. You will need:

- **Clang Compiler:** To compile C code for the `riscv32-unknown-elf` target.
- **LLD Linker:** The LLVM linker, used by Clang.
- **LLVM Objcopy:** To manipulate the compiled ELF binaries.
- **QEMU:** To emulate the RISC-V machine and run the OS.

Below are installation instructions for common operating systems.

#### macOS (with Homebrew)

```bash
brew install llvm qemu
```

The `llvm` package includes `clang`, `lld`, and `llvm-objcopy`. You may need to add LLVM's bin directory to your `PATH` for the tools to be found automatically.

```bash
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install clang lld llvm qemu-system-misc
```

These packages will provide the necessary `clang`, `lld`, `llvm-objcopy`, and `qemu-system-riscv32` tools.

### How to Use

Once the prerequisites are installed, you can build and run the operating system with a single script.

1.  **Execute the `run.sh` script:**

    ```bash
    ./run.sh
    ```

    This script handles everything:
    - Compiling the user-space shell.
    - Compiling the kernel.
    - Creating a `disk.tar` file system image from the contents of the `disk/` directory.
    - Launching the OS in QEMU.

2.  **Interact with the Shell:**

    After running the script, QEMU will launch, and you will be greeted by the YuuOS shell prompt:

    ```
    >
    ```

    The following commands are available:

    - `hello`: Prints a welcome message.
    - `readfile`: Reads the content of `hello.txt` from the disk and prints it.
    - `writefile`: Writes a predefined string to `hello.txt` on the disk.
    - `exit`: Terminates the shell process.

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
