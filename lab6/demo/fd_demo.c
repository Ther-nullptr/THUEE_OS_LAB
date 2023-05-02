#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    // open a file
    int fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    // print the file descriptor
    printf("fd = %d\n", fd);

    // close the file
    close(fd);

    // open a file
    fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);

    // print the file descriptor
    printf("fd = %d\n", fd);
}