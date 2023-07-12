#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXLEN 512

int getstr(char *buf) {
    int flag = -1;
    char ch;
    while (read(0, &ch, 1) > 0) {
        flag = 0;
        if (ch == '\n') {
            *buf = '\0';
            return 1;
        }
        if (ch == ' ') {
            *buf = '\0';
            return 0;
        }
        *buf++ = ch;
    }
    return flag;
}

int getargs(char *args[]) {
    int ret;
    while (1) {
        char *buf = (char *) malloc(MAXLEN);
        if ((ret = getstr(buf)) == 1) {
            *args++ = buf;
            *args = "\0";
            break;
        } else if (ret == 0){
            *args++ = buf;
        } else {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(2, "Usage: xargs <command> <args...>\n");
        exit(1);
    }

    char *args[MAXARG];
    memmove(args, argv, argc * sizeof (char *));
    while (getargs(args + argc) != 1) {
        int pid = fork();
        if (pid < 0) {
            fprintf(2, "xargs: fork failed\n");
            exit(1);
        } else if (pid == 0) {
            exec(args[1], args + 1);
        } else {
            wait(0);
        }
    }

    exit(0);
}