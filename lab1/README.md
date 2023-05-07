# 实验1 进程间的同步/互斥问题——银行柜员服务问题

## 1 实验目的

1. 通过对进程间通信同步/互斥问题的编程实现，加深理解信号量和P、V操作的原理； 
2. 对Windows或Linux涉及的几种互斥、同步机制有更进一步的了解； 
3. 熟悉Windows或Linux中定义的与互斥、同步有关的函数

## 2 实验平台

本实验在x86_64平台的Ubuntu 20.04 LTS操作系统上进行，编程语言采用C++11 和Python 3.8.10，构建工具采用GNU Make 4.2.1。

## 3 实验原理

### 3.1 语言支持

本实验基于C++11中提供的线程库。线程库中提供了`std::thread`类，可以用于创建线程。`std::thread`类的构造函数接受一个可调用对象作为参数，创建一个线程并执行该可调用对象。可调用对象可以是函数指针、函数对象、lambda表达式等；另外，`std::mutex`类可以用于创建互斥量，`std::condition_variable`类可以用于创建条件变量。利用互斥量和条件变量，可以进一步构造信号量semaphore。

### 3.2 算法思路

柜员（teller）和顾客（customer）的行为可以抽象为一个状态机。假设柜员有`n`个，则柜员和顾客的行为逻辑应该描述为：

1. 1位柜员每次只能为1位顾客服务，需要使用mutex；
2. 柜员需要知道当前有多少位顾客在等待，需要使用semaphore；
3. 顾客遵循先来先服务的原则，因此需要使用队列进行建模。

## 4 实验过程

### 4.1 semaphore

C++11线程库中并没有提供现成的信号量类，因此需要自己实现。信号量可以借助`mutex`和`condition_variable`实现。`mutex`用于保护信号量的内部状态，`condition_variable`用于阻塞线程。

```cpp
class Semaphore
{
public:
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    ~Semaphore() = default;

    Semaphore(int init_count, int max_count) : cnt(init_count), max(max_count)
    {
        assert(init_count <= max_count && init_count >= 0 && max_count >= 0);
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
```

构造函数定义了信号量的初始资源和最大资源，`Down()`和`Up()`分别对应P操作和V操作。当执行`Down()`时，如果信号量的资源数为0，则阻塞线程，直到资源数大于0；当执行`Up()`时，将资源数加1，并唤醒一个等待线程。


### 4.2 engine

`Engine`类用于模拟柜员和顾客的行为。`Engine`类的构造函数接受柜员数量`n`，并创建`n`个柜员线程。`engine`类提供了`AddCustomer()`方法，用于添加顾客。`engine`类的`Run()`方法用于启动柜员线程，`Wait()`方法用于等待柜员线程结束。

```cpp
class Engine
{
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

public:
    Engine(int n) : tellers(n)
    {
        for (int i = 0; i < n; ++i)
        {
            tellers[i] = std::thread{&Engine::Teller, this, i};
        }
    }

    void AddCustomer()
    {
        std::unique_lock<std::mutex> lg{mtx};
        ++waiting;
        cv.notify_one();
    }

    void Run()
    {
        for (auto &t : tellers)
        {
            t.join();
        }
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lg{mtx};
        while (waiting > 0)
        {
            cv.wait(lg);
        }
    }
};
```

https://www.zhihu.com/question/564376546

