#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char *fifo = "/tmp/myfifo";

    // create the named pipe
    if (mkfifo(fifo, 0666) < 0) {
        perror("mkfifo");
        exit(1);
    }

    // open the named pipe for writing
    if ((fd = open(fifo, O_WRONLY)) < 0) {
        perror("open");
        exit(1);
    }

    // write data to the named pipe
    if (write(fd, "Hello, world!", 14) < 0) {
        perror("write");
        exit(1);
    }

    // close the named pipe
    close(fd);

    // remove the named pipe
    unlink(fifo);

    return 0;
}
