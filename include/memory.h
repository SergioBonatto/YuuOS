#pragma once

#include "common.h"

paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);
uint32_t *create_user_pagetable(const void *image, size_t image_size);
