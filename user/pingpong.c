#include "kernel/types.h"
#include "user/user.h"

void pingpong() {
    char data[] = "a";
    int pipefd_p2c[2];
    int pipefd_c2p[2];
    if (pipe(pipefd_p2c) < 0 || pipe(pipefd_c2p) < 0) {
        fprintf(2, "pipe() failed\n");
        exit(1);
    }
    int child = fork();
    if (child < 0) {
        fprintf(2, "fork() failed\n");
        exit(1);
    } else if (child == 0) {
        // child
        close(pipefd_p2c[1]);
        close(pipefd_c2p[0]);
        char buf[1];
        read(pipefd_p2c[0], buf, 1);
        printf("%d: received ping\n", getpid());
        write(pipefd_c2p[1], buf, 1);
    } else {
        // parent
        close(pipefd_p2c[0]);
        close(pipefd_c2p[1]);
        write(pipefd_p2c[1], &data, 1);
        char buf[1];
        read(pipefd_c2p[0], buf, 1);
        printf("%d: received pong\n", getpid());
    }
}

int main(int argc, char *argv[]) {

    if (argc != 1) {
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    pingpong();
    
    exit(0);
}
