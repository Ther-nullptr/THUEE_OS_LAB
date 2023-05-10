#include <stdio.h>

int main(int argc, char *argv[])
{
    // open the file /dev/mypipe
    FILE *fp = fopen("/dev/mypipe", "r");
    int n = atoi(argv[1]);

    // read n numbers from the pipe
    for (int i = 0; i < n; i++)
    {
        int num;
        fscanf(fp, "%d", &num);
        printf("read %d\n", num);
    }
}