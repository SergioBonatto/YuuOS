#include "kernel.h"
#include "common.h"
#include "memory.h"

paddr_t alloc_pages(uint32_t n){
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

void map_page(
        uint32_t    *table1,
        uint32_t    vaddr,
        paddr_t     paddr,
        uint32_t    flags
        ) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0){
        // create the 1st level page table if it doesn't exist
        paddr_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    // set the 2nd level page table entry to map the phisical page
    uint32_t vpn0       = (vaddr >> 12) & 0x3ff;
    uint32_t *table0    = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0]        = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

// map user pages
uint32_t *create_user_pagetable(const void *image, size_t image_size){
    uint32_t *page_table = (uint32_t *)alloc_pages(1);

    for (
        paddr_t paddr = (paddr_t)__kernel_base;
        paddr < (paddr_t)__free_ram_end;
        paddr += PAGE_SIZE
    ) {
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }
    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

    for (
        uint32_t off = 0;
        off < image_size;
        off += PAGE_SIZE
        ){
            paddr_t page        = alloc_pages(1);
            size_t remaining    = image_size - off;
            size_t copy_size    = (PAGE_SIZE <= remaining) ? PAGE_SIZE : remaining;
            memcpy((void *)page, image + off, copy_size);
            map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
        }

        return page_table;
}
