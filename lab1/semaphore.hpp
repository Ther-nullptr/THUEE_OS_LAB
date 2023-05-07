#include <mutex>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <condition_variable>
#include <cassert>

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

class Semaphore
{
public:
    Semaphore &operator=(const Semaphore &) = delete;
    ~Semaphore() = default;

    Semaphore(int init_count, int max_count) : cnt(init_count), max(max_count)
    {
        assert(init_count <= max_count && init_count >= 0 && max_count >= 0);
    }

    Semaphore(const Semaphore &sem) : cnt(sem.cnt), max(sem.max)
    {
        assert(cnt <= max && cnt >= 0 && max >= 0);
    }

    void Down()
    {
        std::unique_lock<std::mutex> lg{mtx};
        while (cnt == 0) 
        {
            cv.wait(lg);
        }
        --cnt;
    }

    void Up()
    {
        std::unique_lock<std::mutex> lg{mtx};
        if (cnt == max)
        {
            throw std::overflow_error{"The semaphore is full!"};
        }
        ++cnt;
        cv.notify_one(); // notify the first thread waiting on the condition variable to get the lock
    }

private:
    int cnt;
    int max;
    std::condition_variable cv;
    mutable std::mutex mtx;
};

#endif // !SEMAPHORE_HPP