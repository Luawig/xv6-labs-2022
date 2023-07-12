#include "kernel/types.h"
#include "user/user.h"

int Read(int fd) {
    int num;
    int ret = read(fd, &num, sizeof(num));
    if (ret == 0) {
        return -1;
    }
    return num;
}

void loop(int in) {
    int prime = Read(in);
    if (prime == -1) {
        exit(0);
    }
    printf("prime %d\n", prime);
    int num = Read(in);
    if (num == -1) {
        exit(0);
    }

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        fprintf(2, "primes: pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "primes: fork failed\n");
        exit(1);
    } else if (pid == 0) {
        close(pipefd[1]);
        loop(pipefd[0]);
        close(pipefd[0]);
    } else {
        close(pipefd[0]);
        do {
            if (num % prime != 0) {
                write(pipefd[1], &num, sizeof(num));
            }
            num = Read(in);
        } while(num != -1);
        close(pipefd[1]);
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(2, "Usage: primes\n");
        exit(1);
    }

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        fprintf(2, "primes: pipe failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "primes: fork failed\n");
        exit(1);
    } else if (pid == 0) {
        close(pipefd[1]);
        loop(pipefd[0]);
        close(pipefd[0]);
    } else {
        close(pipefd[0]);
        for (int i = 2; i <= 35; i++) {
            write(pipefd[1], &i, sizeof(i));
        }
        close(pipefd[1]);
        wait(0);
    }

    exit(0);
}