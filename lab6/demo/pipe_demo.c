#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    int fd[2];
    pid_t pid;
    char send_msg[] = "Hello, world!\n";
    char buf[1024];

    // create pipe
    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    printf("fd[0] = %d\n", fd[0]);
    printf("fd[1] = %d\n", fd[1]);

    // fork child process
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        // child process: read from pipe
        close(fd[1]); // close write end of pipe

        while (read(fd[0], buf, sizeof(buf)) > 0)
        {
            printf("Received message: %s", buf);
            printf("child process PID: %d\n", getpid());
        }

        close(fd[0]);
    }
    else
    {
        // parent process: write to pipe
        close(fd[0]); // close read end of pipe
        write(fd[1], send_msg, sizeof(send_msg));
        printf("Sent message: %s", send_msg);
        printf("parent process PID: %d\n", getpid());
        close(fd[1]);
    }

    return 0;
}
