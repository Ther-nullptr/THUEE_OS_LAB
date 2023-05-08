# 实验1 进程间的同步/互斥问题——银行柜员服务问题

## 1 实验目的

1. 通过对进程间通信同步/互斥问题的编程实现，加深理解信号量和P、V操作的原理； 
2. 对Windows或Linux涉及的几种互斥、同步机制有更进一步的了解； 
3. 熟悉Windows或Linux中定义的与互斥、同步有关的函数

## 2 实验平台

本实验在x86_64平台的Ubuntu 20.04 LTS操作系统上进行，编程语言采用C++11，构建工具采用GNU Make 4.2.1。

## 3 实验原理

### 3.1 语言支持

本实验基于C++11中提供的线程库。线程库中提供了`std::thread`类，可以用于创建线程。`std::thread`类的构造函数接受一个可调用对象作为参数，创建一个线程并执行该可调用对象。可调用对象可以是函数指针、函数对象、lambda表达式等；另外，`std::mutex`类可以用于创建互斥量，`std::condition_variable`类可以用于创建条件变量。利用互斥量和条件变量，可以进一步构造信号量semaphore。

### 3.2 算法思路

柜员（server）和顾客（customer）的行为可以抽象为一个状态机。假设柜员有`n`个，则柜员和顾客的行为逻辑应该描述为：

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

    void WakeUpAll()
    {
        std::unique_lock<std::mutex> lg{mtx};
        cnt = max;
        cv.notify_all();
    }

private:
    int cnt;
    int max;
    std::condition_variable cv;
    mutable std::mutex mtx;
};
```

构造函数定义了信号量的初始资源和最大资源，`Down()`和`Up()`分别对应P操作和V操作。当执行`Down()`时，如果信号量的资源数为0，则阻塞线程，直到资源数大于0；当执行`Up()`时，将资源数加1，并唤醒一个等待线程。`WakeUpAll()`方法用于唤醒所有等待线程，通常用于程序结束时。

### 4.2 customer

```cpp
class Customer
{
public:
    Customer &operator=(const Customer &) = delete;
    ~Customer() = default;

    Customer(int index, int start_time, int service_time): index(index), start_time(start_time), service_time(service_time), sem(0, 1)
    {
        
    }

    Customer(const Customer &customer): index(customer.index), start_time(customer.start_time), service_time(customer.service_time), sem(customer.sem)
    {
        
    }

    void print_info()
    {
        std::cout << "Customer " << index << " start at " << start_time << ", service time " << service_time << std::endl;
    }

    void up()
    {
        sem.Up();  
    };

    void down()
    {
        sem.Down();
    };

    // get functions, not changeable
    const int get_index() const noexcept
    {
        return index;
    }

    const int get_start_time() const noexcept
    {
        return start_time;
    }

    const int get_service_time() const noexcept
    {
        return service_time;
    }

private:
    int index;
    int start_time;
    int service_time;
    Semaphore sem;
};
```

customer类内置`start_time`，`serivce_time`等多个属性，同时内置一个信号量`sem`，用于同步。`up()`和`down()`分别对应P操作和V操作。

### 4.3 engine

engine类中内置模拟逻辑。首先定义每一个customer线程的逻辑：

```cpp
void run_customer(Customer& customer)
{
    int wait_time = customer.get_start_time();

    // wait some time, using std::chrono
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time * 100));

    // get the number (which means enqueue)
    {
        std::unique_lock<std::mutex> lock(enqueue_mtx);
        customer_queue.emplace(&customer);
        begin_serve_sem.Up();
    }

    // wait the service
    customer.down();
};
```

customer线程首先等待一段时间，模拟到达银行的时刻。随后在取号过程中，customer进入队列，等待柜员叫号。当柜员叫号时，customer线程被唤醒，开始服务。

之后是server线程的逻辑：

```cpp
void run_server(int server_id)
{
    while (true)
    {
        // wait for the queue to be not empty
        begin_serve_sem.Down();

        if (detect_stopable())
        {
            break;
        }

        // dequeue
        Customer* customer_ptr = nullptr;
        {
            std::unique_lock<std::mutex> lock(dequeue_mtx);
            customer_ptr = customer_queue.front();
            customer_queue.pop();
        }

        // service
        std::this_thread::sleep_for(std::chrono::milliseconds(customer_ptr->get_service_time() * 100));

        // finish customer thread
        customer_ptr->up();
        served_customer_num++;
    }
};

bool detect_stopable()
{
    std::unique_lock<std::mutex> lock(detect_mtx);
    return (served_customer_num == customers.size());
}
```

每当有顾客到来时，柜员线程被唤醒，从队列中取出顾客，开始服务。线程会睡眠一段时间，模拟服务过程。当服务结束时，顾客线程被唤醒，使得顾客线程结束。

最后是总的执行逻辑：

```cpp
    void execute()
    {
        // begin all the server threads
        server_threads.reserve(server_num);
        for (int i = 0; i < server_num; ++i)
        {
            server_threads.emplace_back(&Engine::run_server, this, i);
            print_thread_safely({"Server ", std::to_string(i), " is ready"}); 
        }

        // begin all the customer threads
        customer_threads.reserve(customers.size());
        for (int i = 0; i < customers.size(); ++i)
        {
            customer_threads.emplace_back(&Engine::run_customer, this, std::ref(customers[i]));
            print_thread_safely({"Customer ", std::to_string(i), " is ready"});
        }
        for (int i = 0; i < customers.size(); ++i)
        {
            customer_threads[i].join();
        }

        std::cout << "All customers have been served" << std::endl;
        begin_serve_sem.WakeUpAll();

        for (int i = 0; i < server_num; ++i)
        {
            if (server_threads[i].joinable())
            {
                server_threads[i].join();
            }
        }
    }
```

首先启动所有的柜员线程和顾客线程，随后等待所有顾客线程结束。当所有顾客线程结束后，由于此时服务队列为空，所以柜员线程会被阻塞在`begin_serve_sem.Down()`处，此时调用`begin_serve_sem.WakeUpAll()`可以唤醒所有柜员线程，之后等待所有柜员线程结束。由于此时满足`served_customer_num == customers.size()`，所以所有柜员线程会跳出循环。

当然，以上方法只是一个权宜之计，实际上顾客的数量是无法预知的，这样做只是为了方便快速退出程序。

另外一个需要注意的点是，当唤醒全部线程后，必须要等待所有server线程结束。否则，由于server线程与主线程是分离的，主线程结束之后会调用engine类的析构函数，此时engine类中的信号量会被销毁，而server线程还在使用信号量，会导致程序崩溃。

```bash
Engine is destructing
terminate called without an active exception
[1]    26166 abort      ./main 7
```

### 4.4 可视化

为了更直观地显示服务情况，我们需要对一些服务信息进行记录。

首先是记录时间的问题。我们以程序开始的时刻为0时刻，每一个时间片的长度为100ms（亦即，假设一个顾客等待的时间为2，那么它实际等待的时间为200ms），这样可以方便地计算出每一个顾客的到达时间和服务时间。

https://www.zhihu.com/question/564376546

