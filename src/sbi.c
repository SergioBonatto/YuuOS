#include "sbi.h"
#include "common.h"
#include "kernel.h"

void putchar(char ch){
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* console putchar */);
}

int getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}
