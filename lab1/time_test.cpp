#include <bits/stdc++.h>

int main()
{
    // initialize the time stamp, use std::chrono, with microseconds
    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Start time: " << std::chrono::duration_cast<std::chrono::microseconds>(start_time.time_since_epoch()).count() << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "End time: " << std::chrono::duration_cast<std::chrono::microseconds>(end_time.time_since_epoch()).count() << std::endl;
    // get the time diff
    auto time_diff = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Time diff: " << time_diff << std::endl;
}