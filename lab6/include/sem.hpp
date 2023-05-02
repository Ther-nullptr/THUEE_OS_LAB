#include <mutex>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <condition_variable>

class Semaphore
{
public:
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    ~Semaphore() = default;

    Semaphore(int initCnt, int maxCnt) : cnt(initCnt), max(maxCnt)
    {
        if (initCnt < 0 || maxCnt <= 0 || initCnt > maxCnt)
        {
            throw std::invalid_argument{"Invalid argument!"};
        }
    }

    void Down()
    {
        std::unique_lock<std::mutex> lg{mtx};
        cv.wait(lg, [this]
                { return cnt != 0; });
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
        cv.notify_one();
    }

private:
    int cnt;
    int max;
    std::condition_variable cv;
    mutable std::mutex mtx;
};