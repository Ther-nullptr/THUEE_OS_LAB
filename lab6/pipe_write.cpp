// write a fuction to output n numbers randomly, n is defined in argv[1]
// add some latency to the program, so that the output of this program will be slower than the output of pipe_read.c

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    int n = atoi(argv[1]);
    srand(time(NULL));
    for (int i = 0; i < n; i++)
    {
        int num = rand() % 100;
        std::cout << num << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}