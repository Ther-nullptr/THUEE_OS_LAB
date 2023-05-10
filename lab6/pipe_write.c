#include <stdio.h>

int main(int argc, char *argv[])
{
    // open the file /dev/mypipe
    FILE *fp = fopen("/dev/mypipe", "w");
    int n = atoi(argv[1]);

    // write n numbers to the pipe
    for (int i = 0; i < n; i++)
    {
        fprintf(fp, "%d", i);
        printf("write %d\n", i);
    }
}