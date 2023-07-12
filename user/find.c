#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *filename) {
    struct stat st;
    struct dirent de;
    char buf[512], *p;
    int fd = open(path, 0);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        exit(1);
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(1);
    }
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
        printf("find: path too long\n");
        close(fd);
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        switch(st.type) {
            case T_DEVICE:
            case T_FILE:
                if (strcmp(de.name, filename) == 0) {
                    printf("%s\n", buf);
                }
                break;
            case T_DIR:
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }
                find(buf, filename);
                break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(2, "Usage: find <dir> <filename>\n");
        exit(1);
    }

    int fd = open(argv[1], 0);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", argv[1]);
        exit(1);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", argv[1]);
        close(fd);
        exit(1);
    }
    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not a directory\n", argv[1]);
        close(fd);
        exit(1);
    }
    close(fd);

    find(argv[1], argv[2]);

    exit(0);
}