# PraxisOS

PraxisOS is a minimalist, monolithic operating system kernel designed for the RISC-V 32-bit architecture. This is a study project implementing concepts from [operating-system-in-1000-lines](https://operating-system-in-1000-lines.vercel.app/ja/).

## Architectural Deep Dive

PraxisOS is built on a classic monolithic kernel model. While minimalist, it implements several core operating system concepts to support a functional, albeit simple, user-space environment.

### Memory Management

The kernel uses the RISC-V Sv32 virtual memory scheme, which implements a two-level page table structure. The address space is partitioned between the kernel and a single user process.

-   **Address Space Layout:**
    -   **Kernel Space:** Loaded at the physical and virtual address `0x80200000`. The kernel's code, data, and a 64MB region for dynamic page allocation (`__free_ram`) are mapped with a 1:1 virtual-to-physical correspondence. This direct mapping simplifies the kernel's memory management logic.
    -   **User Space:** Each process is given a distinct address space starting at the virtual address `0x1000000`. This is defined in the `user.ld` linker script.

-   **Page Tables:**
    -   Each process has its own root page table (Level 1). Upon a context switch, the `satp` (Supervisor Address Translation and Protection) register is updated to point to the next process's root page table, effectively activating its address space.
    -   The `create_user_pagetable` function allocates a new page table for a process. It crucially maps the entire kernel space into the user process's address space, allowing the CPU to access kernel code and data when a trap occurs. It then maps the application's binary image and allocates pages for its stack.

-   **Physical Allocation:**
    -   Physical pages are allocated using `alloc_pages`, a simple bump allocator that carves out 4KB chunks from the `__free_ram` region. There is no corresponding deallocation mechanism, reflecting the system's simple lifecycle.

### Process Management and Scheduling

PraxisOS implements cooperative multitasking for a fixed number of processes (`PROCS_MAX`).

-   **Process Control Block:** The state of each process is maintained in a `struct process`, which contains its ID, state (`PROC_RUNNABLE`, `PROC_EXITED`, etc.), a pointer to its kernel stack, and a pointer to its root page table.

-   **Scheduling:**
    -   The scheduler is implemented in the `yield()` function and follows a basic round-robin policy. It iterates through the global `procs` array to find the next `PROC_RUNNABLE` process to execute.
    -   There is a designated `idle_proc` that runs when no other processes are runnable.

-   **Context Switching:**
    -   The `switch_context` function, written in assembly, performs the low-level machine state switch. It saves callee-saved registers to the current process's kernel stack and restores them for the next process.
    -   Upon a trap from user-space, the `sscratch` register is used to swap the user stack pointer with the process's kernel stack pointer, ensuring a safe and isolated execution environment for the trap handler.

### System Calls and Trap Handling

System calls provide the interface between user-space applications and kernel services.

-   **Trap Entry:** The `stvec` register is configured to point to `kernel_entry`, the unified assembly routine for handling all traps (interrupts, exceptions, and syscalls).
-   **Trap Handling:**
    1.  `kernel_entry` saves the complete user register context onto the process's kernel stack, creating a `struct trap_frame`.
    2.  It then calls `handle_trap`, a C function that reads the `scause` register to determine the cause of the trap.
    3.  For syscalls (`scause` = 8), `handle_trap` dispatches to `handle_syscall`.
-   **Syscall Dispatch:** `handle_syscall` uses the value in register `a3` as an index into `syscall_table`, an array of function pointers, to invoke the correct handler (e.g., `handle_putchar`, `handle_readfile`). Arguments are passed from user space in registers `a0-a2`.

### VirtIO Block Device Driver

A key feature of PraxisOS is its driver for the VirtIO block device, the standard para-virtualized I/O mechanism in QEMU.

-   **Initialization:** The `virtio_blk_init` function performs the standard VirtIO device initialization sequence, negotiating with the device and setting up the required virtqueue.
-   **Virtqueues:** Communication is managed via a single virtqueue, which consists of three main components:
    1.  **Descriptor Table:** An array of `struct virtq_desc` that describes the memory buffers (address, length, flags) the driver wants to share with the device.
    2.  **Available Ring:** The driver places the index of a descriptor chain into this ring to notify the device of a new request.
    3.  **Used Ring:** The device places the index of a completed descriptor chain here to signal completion to the driver.
-   **I/O Operations:**
    -   To perform a read or write, the driver constructs a `struct virtio_blk_req` header and assembles a three-descriptor chain: one for the request header, one for the data buffer, and one for the device to write a status byte.
    -   The driver "kicks" the device by writing to the `VIRTIO_REG_QUEUE_NOTIFY` register. It then busy-waits by polling the `used_index` of the virtqueue to detect when the operation has finished.

### In-Memory TAR Filesystem

The filesystem is not a traditional on-disk structure but rather an in-memory cache of files loaded from a TAR archive at boot.

-   **Initialization:** At startup, `fs_init` reads the first few sectors from the VirtIO block device. It parses this data as a USTAR-formatted TAR archive, extracting each file's name and content into the in-memory `files` array.
-   **File Operations:** The `readfile` and `writefile` syscalls operate directly on the data buffers in this array.
-   **Persistence:** When `writefile` is called, the `fs_flush` function is triggered. It reconstructs the entire TAR archive in a memory buffer from the current state of the `files` array and writes it back to the block device. This makes changes persistent for the duration of the QEMU session but is highly inefficient.

## Getting Started

Follow these instructions to set up the toolchain and run PraxisOS.

### Prerequisites

PraxisOS requires a specific toolchain for building the RISC-V 32-bit kernel and user-space applications. You will need:

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

    After running the script, QEMU will launch, and you will be greeted by the PraxisOS shell prompt:

    ```
    >
    ```

    Available commands:

    - `hello`: Prints a test message.
    - `readfile`: Reads and displays the content of `hello.txt` from the embedded disk image.
    - `writefile`: Writes data to `hello.txt` on the embedded disk.
    - `exit`: Terminates the shell and stops the OS.

    To exit QEMU, press `Ctrl+A` followed by `X`.
