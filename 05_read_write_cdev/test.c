#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv){
    int fd;
    char c;
    if (argc < 2) {
        printf("I need the file to open as an argument.\n");
        return 0;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Error opening file");
        return fd;
    }
    while (read(fd, &c, 1) > 0) 
        putchar(c);
    close(fd);
    return 0;
}