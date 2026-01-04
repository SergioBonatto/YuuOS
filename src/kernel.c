#include "kernel.h"
#include "common.h"
#include "memory.h"
#include "process.h"
#include "context.h"



__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"                 // Set the stack pointer
        "j kernel_main\n"       				 // Jump to the kernel main function
        :
        : [stack_top] "r" (__stack_top)        // Pass the stack top address as %[stack_top]
    );
}

struct process procs[PROCS_MAX]; // ALL process control structures.
struct process *proc_a;
struct process *proc_b;
struct process *current_proc;
struct process *idle_proc;


struct sbiret sbi_call(
		long arg0,
		long arg1,
		long arg2,
		long arg3,
		long arg4,
		long arg5,
		long fid,
		long eid
		) {
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

	__asm__ __volatile__ ( "ecall"
			: "=r"(a0), "=r"(a1)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
			: "memory"
			);

	return (struct sbiret){.error = a0, .value = a1};
}


/* void *memset(void *buf, char c, size_t n) { */
/*     uint8_t *p = (uint8_t *) buf; */
/*     while (n--) */
/*         *p++ = c; */
/*     return buf; */
/* } */

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

// void yield(void) {
//     // search for a runnable process
//     struct process *next = idle_proc;
//     for (int i = 0; i < PROCS_MAX; i++) {
//         struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
//         if (proc->state == PROC_RUNNABLE && proc->pid > 0){
//             next = proc;
//             break;
//         }
//     }

//     // if there's no runnable process other than the current one
//     // return and continue processing
//     if (next == current_proc)
//         return;

//     __asm__ __volatile__(
//         "sfence.vma\n"
//         "csrw satp, %[satp]\n"
//         "sfence.vma\n"
//         "csrw sscratch, %[sscratch]\n"
//         :
//         :   [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
//             [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
//     );


//     // context switch
//     struct process *prev = current_proc;
//     current_proc = next;
//     switch_context(&prev->sp, &next->sp);
// }

struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];

void putchar(char ch){
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* console putchar */);
}

struct file *fs_lookup(const char *filename){
    for (int i = 0; i < FILES_MAX; i++){
        struct file *file = &files[i];
        if (!strcmp(file->name, filename))
            return file;
    }
    return NULL;
}

struct      virtio_virtq    *blk_request_vq;
struct      virtio_blk_req  *blk_req;
paddr_t     blk_req_paddr;
uint64_t    blk_capacity;

uint32_t virtio_reg_read32(unsigned offset){
    return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}

uint64_t virtio_reg_read64(unsigned offset){
    return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value){
    *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_write64(unsigned offset, uint64_t value){
    *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value){
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

// return whether there are requests being processed by the device
kbool virtq_is_busy(struct virtio_virtq *vq){
    return vq->last_used_index != *vq->used_index;
}

// Notifies the device that there is a new request
// `desc_index` is the index of the head descriptor of the new request
void virtq_kick(struct virtio_virtq *vq, int desc_index){
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    vq->avail.index++;
    __sync_synchronize();
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
    vq->last_used_index++;
}

// reads/writes from/to virtio-blk device
void read_write_disk(void *buf, unsigned sector, int is_write){
    if (sector >= blk_capacity / SECTOR_SIZE) {
        printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
                sector, blk_capacity / SECTOR_SIZE);
        return;
    }

    // construct the request according to the virtio-blk specification
    blk_req->sector = sector;
    blk_req->type   = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    if (is_write)
        memcpy(blk_req->data, buf, SECTOR_SIZE);

    // construct the virtqueue descriptors (using 3 descriptors)
    struct virtio_virtq *vq = blk_request_vq;
    vq->descs[0].addr       = blk_req_paddr;
    vq->descs[0].len        = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    vq->descs[0].flags      = VIRTQ_DESC_F_NEXT;
    vq->descs[0].next       = 1;

    vq->descs[1].addr       = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[1].len        = SECTOR_SIZE;
    vq->descs[1].flags      = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    vq->descs[1].next       = 2;

    vq->descs[2].addr       = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[2].len        = sizeof(uint8_t);
    vq->descs[2].flags      = VIRTQ_DESC_F_WRITE;

    // notify the device that there is a new request
    virtq_kick(vq, 0);

    // wait until the device finishes processing
    while (virtq_is_busy(vq))
        ;

    // virtio-blk: if a non-zero value is returned, it's an error
    if (blk_req->status!=0){
        printf("virtio: warn: failed to read/write sector=%d status=%d\n",
                sector, blk_req->status);
        return;
    }

    // for read operations, copy the data into the buffer
    if(!is_write)
        memcpy(buf, blk_req->data, SECTOR_SIZE);
}


void fs_flush(void){
    // copy all file contents into "disk" buffer
    memset(disk, 0, sizeof(disk));
    unsigned off = 0;
    for (int file_i = 0; file_i < FILES_MAX; file_i++){
        struct file *file = &files[file_i];
        if (!file->in_use)
            continue;

        struct tar_header *header = (struct tar_header *) &disk[off];
        memset(header, 0, sizeof(*header));
        strcpy(header->name, file->name);
        strcpy(header->mode, "000644");
        strcpy(header->magic, "ustar");
        strcpy(header->version, "00");
        header->type = '0';

        // turn the file size into an octal string
        int filesz = file->size;
        for (int i = sizeof(header->size); i > 0; i--){
            header->size[i - 1] = (filesz % 8) + '0';
            filesz /= 8;
        }

        // calculate the checksum
        int checksum = ' ' * sizeof(header->checksum);
        for (unsigned i = 0; i < sizeof(struct tar_header); i++)
            checksum += (unsigned char) disk[off + 1];

        for (int i = 5; i >= 0; i--){
            header->checksum[i] = (checksum % 8) + '0';
            checksum /= 8;
        }

        memcpy(header->data, file->data, file->size);
        off += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
    }

    // write disk buffer into the virtio-blk
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);
    printf("wrote %d bytes to disk \n", sizeof(disk));
}


void handle_exit(struct trap_frame *f){
    printf("process %d exited\n", current_proc->pid);
    current_proc->state = PROC_EXITED;
    yield();
    PANIC("unreachable");
};

void handle_getchar(struct trap_frame *f){
    while (1){
        long ch = getchar();
        if (ch >= 0) {
            f->a0 = ch;
            return;
        }
        yield();
    }
}

void handle_putchar(struct trap_frame *f){
    putchar(f->a0);
}

void handle_readfile_writefile(struct trap_frame *f){
    const char *filename    = (const char *) f->a0;
    char *buf               = (char *) f->a1;
    int len                 = f->a2;
    struct file *file       = fs_lookup(filename);
    if (!file) {
        printf("file not found: %s\n", filename);
        f->a0 = -1;
    }
    if (len > (int) sizeof(file->data))
        len = file->size;
    if (f->a3 == SYS_WRITEFILE) {
        memcpy(file->data, buf, len);
        file->size = len;
        fs_flush();
    } else {
        memcpy(buf, file->data, len);
    }
    f->a0 = len;
}

void handle_error (struct trap_frame *f){
    PANIC("unexpected suscall a3=%x\n", f->a3);
}

struct virtio_virtq *virtq_init(unsigned index) {
    // Allocate a region for the virtqueue
    paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
    vq->queue_index = index;
    vq->used_index  = (volatile uint16_t *) &vq->used.index;

    // select the queue: write the virtqueue index (first queue is 0)
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);

    // specify the queue size: write the # of decription we'll use
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);

    // write the physical page from frame number (not physical address!) of the queue
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr / PAGE_SIZE);

    return vq;
}


void virtio_blk_init(void){
    if(virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
        PANIC("virtio: invalid magic value");
    if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
        PANIC("virtio: invalid version");
    if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
        PANIC("virtio: invalid device id");

    // 1. Reset the devide
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);

    // 2. Set the ACKNOWLEDGE status bit: We found the device.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);

    // 3. Set the DRIVER status bit: we know how to use the devices
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);

    // 4. set our page size: we use 4KB pages
    // this defines PFN (page frame number) calculation
    virtio_reg_write32(VIRTIO_REG_PAGE_SIZE, PAGE_SIZE);

    // 5. Initialize a queue for disk read/write requests
    blk_request_vq = virtq_init(0);

    // 6. Set the DRIVER_OK status bit: we can now use the device!
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

    // get disk capacity
    blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
    printf("virtio-blk: capacity is %d bytes\n", (int)blk_capacity);

    // Allocate a region to store requests to the device
    blk_req_paddr   = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE );
    blk_req         = (struct virtio_blk_req *) blk_req_paddr;
}

int oct2int(char *oct, int len){
    int dec = 0;
    for (int i = 0; i < len; i++){
        if(oct[i] < '0' || oct[i] > '7')
            break;

        dec = dec * 8 + (oct[i] - '0');
    }
    return dec;
}

void fs_init(void){
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);

    unsigned off = 0;
    for (int i = 0; i < FILES_MAX; i++){
        struct tar_header *header = (struct tar_header *) &disk[off];
        if (header->name[0] == '\0')
            break;

        if  (strcmp(header->magic, "ustar") != 0)
            PANIC("invalid tar header: magic=\"%s\"", header->size);

        int filesz          = oct2int(header->size, sizeof(header->size));
        struct file *file   = &files[i];
        file->in_use        = true;
        strcpy(file->name, header->name);
        memcpy(file->data, header->data, filesz);
        file->size          = filesz;
        printf("file: %s, size:%d\n", file->name, file->size);

        off += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
    }
}

typedef void (*syscall_handler)(struct trap_frame *f);

static syscall_handler syscall_table[SYS_MAX] = {
    [SYS_EXIT]      = handle_exit,
    [SYS_GETCHAR]   = handle_getchar,
    [SYS_PUTCHAR]   = handle_putchar,
    [SYS_READFILE]  = handle_readfile_writefile,
    [SYS_WRITEFILE] = handle_readfile_writefile,
};


void handle_syscall(struct trap_frame *f){
    if (f->a3 < SYS_MAX && syscall_table[f->a3]){
        syscall_table[f->a3](f);
    } else {
        PANIC("unexpected syscall a3=%x\n", f->a3);
    }
}

void handle_trap(struct trap_frame *f) {
    uint32_t scause     = READ_CSR(scause);
    uint32_t stval      = READ_CSR(stval);
    uint32_t user_pc    = READ_CSR(sepc);
    if (scause == SCAUSE_ECALL){
        handle_syscall(f);
        user_pc += 4;
    } else {
        PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }
    WRITE_CSR(sepc, user_pc);
}


void delay(void){
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

void proc_a_entry(void){
    printf("starting process A\n");
    while(1){
        putchar('A');
        yield();
    }
}


void proc_b_entry(void){
    printf("starting process B\n");
    while(1){
        putchar('B');
        yield();
    }
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    printf("\n\n");

    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    virtio_blk_init();
    fs_init();

    char buf[SECTOR_SIZE];
    read_write_disk(buf, 0, false /* read from disk */);
    printf("first sector: %s\n", buf);

    strcpy(buf, "hello from kernel!!!\n");
    read_write_disk(buf, 0, true /* write to the disk */);

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = 0; //idle
    current_proc = idle_proc;

    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

    yield();

	PANIC("unreachable here!");
}
