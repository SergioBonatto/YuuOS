# YuuOS

YuuOS is a minimalist, monolithic operating system kernel designed for the RISC-V 32-bit architecture. This is a study project implementing concepts from [operating-system-in-1000-lines](https://operating-system-in-1000-lines.vercel.app/ja/).

## Features

- **Monolithic Kernel:** A simple, all-in-one kernel structure.
- **RISC-V 32-bit Support:** Specifically designed for the `rv32im` architecture.
- **Pre-emptive Multitasking:** Fixed context switching between up to 8 processes with timer-based preemption.
- **Virtual Memory:** Two-level hierarchical page table with Sv32 MMU support (simplified, not full demand paging).
- **System Calls:** Implements 5 basic syscalls (`SYS_PUTCHAR`, `SYS_GETCHAR`, `SYS_EXIT`, `SYS_READFILE`, `SYS_WRITEFILE`).
- **VirtIO Block Device Driver:** Minimal virtio-blk driver for block device I/O.
- **Simplified Filesystem:** TAR USTAR format embedded in disk image at boot; not a dynamic filesystem implementation.
- **User-space Shell:** Basic interactive shell with hardcoded commands (`hello`, `readfile`, `writefile`, `exit`).

## Components

- `kernel.c`, `kernel.h`: Core kernel code with process management, memory management (page tables), syscall dispatch, timer interrupt handling, and virtio-blk driver.
- `common.c`, `common.h`: Shared utility functions (`kmemcpy`, `kmemset`, `strcmp`, `kprintf`) used by both kernel and user-space.
- `user.c`, `user.h`: User-space library providing syscall wrappers and process entry point.
- `shell.c`: Simple shell application demonstrating user-space execution and file I/O.
- `kernel.ld`, `user.ld`: Linker scripts defining memory layout (kernel at 0x80200000, user processes at 0x1000000).
- `run.sh`: Build script handling compilation, binary conversion, TAR disk image creation, and QEMU invocation.

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

The `llvm` package includes `clang`, `lld`, and `llvm-objcopy`. Since `run.sh` invokes these tools by name, you must add LLVM's bin directory to your `PATH`:

```bash
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

Add this to your shell profile (`.zshrc`, `.bash_profile`) to persist across sessions.

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

    Available commands:

    - `hello`: Prints a test message.
    - `readfile`: Reads and displays the content of `hello.txt` from the embedded disk image.
    - `writefile`: Writes data to `hello.txt` on the embedded disk.
    - `exit`: Terminates the shell and stops the OS.

    To exit QEMU, press `Ctrl+A` followed by `X`.

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
- **`disk/`**: Contains files that will be included in the initial disk image (embedded as TAR USTAR format).
- **`run.sh`**: The main script to build and run the OS.

## Technical Details

### Memory Layout

- **Kernel Space:** 0x80200000 - 0x804FFFFF (3MB kernel image + 64MB free RAM for page allocation)
- **User Space:** 0x1000000 - 0x17FFFFF (per-process, 8MB reserved)
- **Process Stack:** 8KB per process (grows downward)
- **Physical Pages:** 4KB page size; 2-level hierarchical page tables (Sv32)

### Process Management

- **Max Processes:** 8 (PROCS_MAX)
- **Scheduling:** Timer-driven context switching at fixed intervals; processes run in predefined order.
- **Context Switch:** Assembly routine saves/restores all registers via process stack.

### System Calls

Syscalls use the ECALL instruction with arguments in registers (a0-a3):

| Syscall | Code | Arguments | Returns |
|---------|------|-----------|---------|
| SYS_PUTCHAR | 1 | ch (a0) | 0 |
| SYS_GETCHAR | 2 | none | character |
| SYS_EXIT | 3 | none | never returns |
| SYS_READFILE | 4 | filename (a0), buf (a1), len (a2) | bytes read |
| SYS_WRITEFILE | 5 | filename (a0), buf (a1), len (a2) | bytes written |

### Filesystem

The filesystem is not a traditional filesystem implementation. Instead:
- Files are stored in TAR USTAR format in a flat disk image.
- The image is loaded into the kernel's in-memory file table at boot.
- Maximum 2 files (FILES_MAX) can be tracked simultaneously.
- Write operations modify the in-memory copy; changes are lost after shutdown.

### Build Artifacts

- `shell.elf`: User-space application (uncompressed ELF)
- `shell.bin`: Binary-only user image (objcopy -O binary)
- `shell.bin.o`: Shell binary embedded as object file (linked into kernel)
- `kernel.elf`: Final kernel executable
- `disk.tar`: TAR archive of disk/ directory contents
- `*.map`: Linker maps showing symbol addresses
- `qemu.log`: QEMU debug log
