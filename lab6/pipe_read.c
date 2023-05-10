#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd = open("/dev/mypipe", O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("[ERROR] Fail to open pipe for reading data.\n");
        exit(1);
    }
    
    int n = atoi(argv[1]);
    
    char *buffer;
    buffer = (char *)malloc(n * sizeof(char));
    read(fd, buffer, n);
    
    for (int i = 0; i < n; i++)
    {
        printf("read %c\n", buffer[i]);
    }
    
    close(fd);
    free(buffer);
    return 0;
}