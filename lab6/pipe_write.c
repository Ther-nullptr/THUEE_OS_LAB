#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PIPE_BUFFER_SIZE 16

int main(int argc, char *argv[])
{
    char buffer[PIPE_BUFFER_SIZE];

    int fd = open("/dev/mypipe", O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("[ERROR] Fail to open pipe for writing data.\n");
        exit(1);
    }
    
    int n = atoi(argv[1]);

    // write n numbers to the pipe
    for (int i = 0; i < n; i++)
    {
        buffer[i] = i + 97;
        printf("write %c\n", i + 97);
    }
    
    write(fd, &buffer, strlen(buffer));
    close(fd);
    return 0;
}