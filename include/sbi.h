#pragma once
#include "common.h"

struct sbiret sbi_call(
    long arg0,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long fid,
    long eid
);

__attribute__((noreturn)) void exit(void);
void putchar(char ch);

int getchar(void);
