#include "user.h"


struct command {
    const char *name;
    void (*fn)(void);
};

void cmd_hello(void){
    printf("Hello, my world!\n");
}

void cmd_readfile(void){
    char buf[128];
    int len = readfile("disk/hello.txt", buf, sizeof(buf));
    buf[len] = '\0';
    printf("%s\n", buf);
}

void cmd_writefile(void){
    writefile("disk/hello.txt", "hello from shell!\n", 19);
}

void cmd_uwufetch(void){
    printf(
        "     ⣤⢔⣒⠂⣀⣀⣤⣄⣀⠀⠀    root@YuuOS                 \n"
        "⣴⣿⠋⢠⣟⡼⣷⠼⣆⣼⢇⣿⣄⠱⣄     OS:        YuuOS 1.06.1    \n"
        "⠹⣿⡀⣆⠙⠢⠐⠉⠉⣴⣾⣽⢟⡰⠃     KERNEL:    RISC-V 32       \n"
        "⠀⠈⢿⣿⣦⠀⠤⢴⣿⠿⢋⣴⡏⠀      UPTIME:    since boot      \n"
        "⠀⠀⠀⢸⡙⠻⣿⣶⣦⣭⣉⠁⣿⠀⠀⠀    PACKAGES:  0               \n"
        "⠀⠀⠀⣷⠀⠈⠉⠉⠉⠉⠇⡟⠀⠀⠀     SHEL:      yuush           \n"
        "⠀⢀⠀⠀⣘⣦⣀⠀⠀⣀⡴⠊⠀⠀⠀⠀    UI:        N/A             \n"
        "⠈⠙⠛⠛⢻⣿⣿⣿⣿⠻⣧⡀                                   \n"
    );
}

void cmd_brapao(void){
    printf("achou esse easter egg, falta só o do site");
}

void cmd_exit(void){
    exit();
}


struct command commands[] = {
    { "hello", cmd_hello },
    { "uwufetch", cmd_uwufetch },
    { "exit", cmd_exit },
    { "readfile", cmd_readfile },
    { "writefile", cmd_writefile },
    { "brapao", cmd_brapao},
};

int main(void){
    while (1){
prompt:
        printf("> ");
        char cmdline[128];
        for (int i = 0;; i++){
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1){
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        int found = 0;

        for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++){
            if (strcmp(cmdline, commands[i].name) == 0){
                commands[i].fn();
                found = 1;
                break;
            }
        }

        if (!found)
            printf("unknown command: %s\n", cmdline);
    }
}
