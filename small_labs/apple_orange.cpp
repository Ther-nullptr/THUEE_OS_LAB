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

class Problem
{
public:
    void father()
    {
        while(1)
        {
            empty.Down(); // let the empty position of the table be occupied
            mutex.Down(); // let only one thread to access the plate
            std::cout << "[father] put an apple on the plate" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
            apple.Up(); // put an apple on the plate
            mutex.Up(); // let other threads to access the plate
        }
    }

    void mother()
    {
        while(1)
        {
            empty.Down(); // let the empty position of the table be occupied
            mutex.Down(); // let only one thread to access the plate
            std::cout << "[mother] put an orange on the plate" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
            orange.Up(); // put an orange on the plate
            mutex.Up(); // let other threads to access the plate
        }
    }

    void son()
    {
        while(1)
        {
            orange.Down(); // let the orange on the plate be eaten
            mutex.Down(); // let only one thread to access the plate
            std::cout << "[son] eat an orange" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
            empty.Up(); // generate an empty position
            mutex.Up(); // let other threads to access the plate
        }
    }

    void daughter()
    {
        while(1)
        {
            apple.Down(); // let the apple on the plate be eaten
            mutex.Down(); // let only one thread to access the plate
            std::cout << "[daughter] eat an apple" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
            empty.Up(); // generate an empty position
            mutex.Up(); // let other threads to access the plate
        }
    }

private:
    Semaphore apple{0, 1};
    Semaphore orange{0, 1};
    Semaphore empty{1, 1};
    Semaphore mutex{1, 1};
};

int main()
{
    Problem p;
    std::thread father{&Problem::father, &p};
    std::thread mother{&Problem::mother, &p};
    std::thread son{&Problem::son, &p};
    std::thread daughter{&Problem::daughter, &p};

    father.join();
    mother.join();
    son.join();
    daughter.join();

    return 0;
}
