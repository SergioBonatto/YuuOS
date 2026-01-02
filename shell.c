#include "user.h"

void main(void){
    while (1){
prompt:
        kprintf("> ");
        char cmdline[128];
        for (int i = 0;; i++){
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1){
                kprintf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                kprintf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0)
            kprintf("hello my world!\n");
        else if (strcmp(cmdline, "brapao") == 0)
            kprintf("achou o easter egg, falta só o do site\n");
        else if (strcmp(cmdline, "exit") ==0)
            exit();
        else 
            kprintf("unknow command: %\n", cmdline);
    }
}
