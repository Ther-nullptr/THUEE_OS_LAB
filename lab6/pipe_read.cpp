// write a function, the function will read n numbers from the pipe and output the square of each number
// n is defined in argv[1]

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    int n = atoi(argv[1]);
    for (int i = 0; i < n; i++)
    {
        int num;
        std::cin >> num;
        std::cout << num * num << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}