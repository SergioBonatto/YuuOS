#pragma once

#include "common.h"

void boot(void);
void user_entry(void);
void kernel_entry(void);
void switch_context(uint32_t *prev_sp, uint32_t *next_sp);
