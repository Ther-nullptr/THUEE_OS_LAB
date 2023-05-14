# 实验1 进程间的同步/互斥问题——银行柜员服务问题

## 1 实验目的

1. 通过对进程间通信同步/互斥问题的编程实现，加深理解信号量和P、V操作的原理； 
2. 对Windows或Linux涉及的几种互斥、同步机制有更进一步的了解； 
3. 熟悉Windows或Linux中定义的与互斥、同步有关的函数。

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

### 4.4 输出

为了更直观地显示服务情况，我们需要对一些服务信息进行记录。

首先是记录时间的问题。我们以程序开始的时刻为0时刻，每一个时间片的长度为100ms（亦即，假设一个顾客等待的时间为2，那么它实际等待的时间为200ms），这样可以方便地计算出每一个顾客的到达时间和服务时间。

```cpp
int64_t get_time_stamp_milliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

int get_time_slice()
{
    int64_t now_time = get_time_stamp_milliseconds();
    int64_t time_diff = now_time - start_time;
    return std::round((float)time_diff / time_slice);
}
```

每进行一次操作，都需要记录下当前的时间片，最后将记录结果输出到一个文件中。

## 5 实验结果

选取默认测例如下：

```bash
1 1 10
2 5 2
3 6 3
```

设置柜员数为3，运行输出如下：
```bash
Number of servers: 3
Customer 0 start at 1, service time 10
Customer 1 start at 5, service time 2
Customer 2 start at 6, service time 3
[23:58:39.869] (time step:0) Server 0 is ready
[23:58:39.870] (time step:0) Server 1 is ready
[23:58:39.870] (time step:0) Server 2 is ready
[23:58:39.870] (time step:0) Customer 0 is ready
[23:58:39.870] (time step:0) Customer 0 will come after 1 time slides
[23:58:39.870] (time step:0) Customer 1 is ready
[23:58:39.870] (time step:0) Customer 1 will come after 5 time slides
[23:58:39.870] (time step:0) Customer 2 will come after 6 time slides
[23:58:39.870] (time step:0) Customer 2 is ready
[23:58:39.970] (time step:1) Customer 0 is entering the bank
[23:58:39.970] (time step:1) Server 0 is serving customer 0
[23:58:40.370] (time step:5) Customer 1 is entering the bank
[23:58:40.370] (time step:5) Server 1 is serving customer 1
[23:58:40.470] (time step:6) Customer 2 is entering the bank
[23:58:40.470] (time step:6) Server 2 is serving customer 2
[23:58:40.571] (time step:7) Customer 1 is leaving the bank
[23:58:40.771] (time step:9) Customer 2 is leaving the bank
[23:58:40.971] (time step:11) Customer 0 is leaving the bank
[23:58:40.971] (time step:11) All customers have been served
[23:58:40.971] (time step:11) Server 2 is stopping
[23:58:40.971] (time step:11) Server 0 is stopping
[23:58:40.971] (time step:11) Server 1 is stopping
[23:58:40.976] (time step:11) Engine is destructing
```

文件输出如下（其中从左到右分别为顾客编号、进入银行的时间、开始服务的时间、离开银行的时间和服务柜员）：

```bash
0 1 1 11 0
1 5 5 7 1
2 6 6 9 2
```

为了进一步验证算法的正确性，可以编写生成测试样例的程序`generate_examples.py`，生成的测试样例如下：

```bash
0 1 6
1 2 7
2 3 10
3 5 4
4 5 2
5 6 9
6 7 8
7 8 3
8 8 2
9 9 1
```

设置柜员数为3，运行输出如下：

```bash
Number of servers: 3
Customer 0 start at 1, service time 6
Customer 1 start at 2, service time 7
Customer 2 start at 3, service time 10
Customer 3 start at 5, service time 4
Customer 4 start at 5, service time 2
Customer 5 start at 6, service time 9
Customer 6 start at 7, service time 8
Customer 7 start at 8, service time 3
Customer 8 start at 8, service time 2
Customer 9 start at 9, service time 1
[00:02:06.353] (time step:0) Server 0 is ready
[00:02:06.353] (time step:0) Server 1 is ready
[00:02:06.353] (time step:0) Server 2 is ready
[00:02:06.353] (time step:0) Customer 0 is ready
[00:02:06.353] (time step:0) Customer 0 will come after 1 time slides
[00:02:06.353] (time step:0) Customer 1 is ready
[00:02:06.353] (time step:0) Customer 1 will come after 2 time slides
[00:02:06.354] (time step:0) Customer 2 is ready
[00:02:06.354] (time step:0) Customer 2 will come after 3 time slides
[00:02:06.354] (time step:0) Customer 3 will come after 5 time slides
[00:02:06.354] (time step:0) Customer 3 is ready
[00:02:06.354] (time step:0) Customer 4 is ready
[00:02:06.354] (time step:0) Customer 4 will come after 5 time slides
[00:02:06.354] (time step:0) Customer 5 is ready
[00:02:06.355] (time step:0) Customer 5 will come after 6 time slides
[00:02:06.355] (time step:0) Customer 6 is ready
[00:02:06.355] (time step:0) Customer 6 will come after 7 time slides
[00:02:06.355] (time step:0) Customer 7 is ready
[00:02:06.355] (time step:0) Customer 7 will come after 8 time slides
[00:02:06.355] (time step:0) Customer 8 is ready
[00:02:06.356] (time step:0) Customer 8 will come after 8 time slides
[00:02:06.356] (time step:0) Customer 9 is ready
[00:02:06.356] (time step:0) Customer 9 will come after 9 time slides
[00:02:06.454] (time step:1) Customer 0 is entering the bank
[00:02:06.454] (time step:1) Server 0 is serving customer 0
[00:02:06.554] (time step:2) Customer 1 is entering the bank
[00:02:06.554] (time step:2) Server 1 is serving customer 1
[00:02:06.655] (time step:3) Customer 2 is entering the bank
[00:02:06.655] (time step:3) Server 2 is serving customer 2
[00:02:06.854] (time step:5) Customer 3 is entering the bank
[00:02:06.855] (time step:5) Customer 4 is entering the bank
[00:02:06.955] (time step:6) Customer 5 is entering the bank
[00:02:07.055] (time step:7) Server 0 is serving customer 3
[00:02:07.055] (time step:7) Customer 0 is leaving the bank
[00:02:07.055] (time step:7) Customer 6 is entering the bank
[00:02:07.155] (time step:8) Customer 7 is entering the bank
[00:02:07.156] (time step:8) Customer 8 is entering the bank
[00:02:07.254] (time step:9) Server 1 is serving customer 4
[00:02:07.255] (time step:9) Customer 1 is leaving the bank
[00:02:07.257] (time step:9) Customer 9 is entering the bank
[00:02:07.455] (time step:11) Server 1 is serving customer 5
[00:02:07.455] (time step:11) Customer 4 is leaving the bank
[00:02:07.455] (time step:11) Server 0 is serving customer 6
[00:02:07.455] (time step:11) Customer 3 is leaving the bank
[00:02:07.655] (time step:13) Server 2 is serving customer 7
[00:02:07.655] (time step:13) Customer 2 is leaving the bank
[00:02:07.955] (time step:16) Server 2 is serving customer 8
[00:02:07.955] (time step:16) Customer 7 is leaving the bank
[00:02:08.155] (time step:18) Server 2 is serving customer 9
[00:02:08.156] (time step:18) Customer 8 is leaving the bank
[00:02:08.255] (time step:19) Customer 6 is leaving the bank
[00:02:08.256] (time step:19) Customer 9 is leaving the bank
[00:02:08.355] (time step:20) Customer 5 is leaving the bank
[00:02:08.355] (time step:20) All customers have been served
[00:02:08.355] (time step:20) Server 0 is stopping
[00:02:08.356] (time step:20) Server 2 is stopping
[00:02:08.356] (time step:20) Server 1 is stopping
[00:02:08.367] (time step:20) Engine is destructing
```

文件输出如下：
```
0 1 1 7 0
1 2 2 9 1
2 3 3 13 2
3 5 7 11 0
4 5 9 11 1
5 6 11 20 1
6 7 11 19 0
7 8 13 16 2
8 8 16 18 2
9 9 18 19 2
```

## 6 思考题

1. 柜员人数和顾客人数对结果分别有什么影响？

* 柜员人数：柜员人数越多，顾客等待时间越短，但是当柜员人数多于顾客人数时，多余的柜员会空闲，造成资源浪费。
* 顾客人数：顾客人数越多，顾客等待时间越长。

以上为定性分析，为了进一步确定柜员人数和顾客人数对结果的影响，我们可以通过实验来定量分析：

固定顾客人数`n=10`，柜员人数`m`从1到10，得到的结果如下：

![time1.png](https://s2.loli.net/2023/05/09/mDCdok5ElLiYw4u.png)

可以发现，当顾客人数一定时，花费时间与柜员人数近似成反比。当柜员人数达到顾客人数的一般以上时，就难以带来时间增益了（当然，这一点和顾客到达时间的分布有关）。

固定柜员人数`m=5`，顾客人数`n`从5到50，得到的结果如下：

![time2.png](https://s2.loli.net/2023/05/09/lg28JY7oXMUKBvc.png)

当柜员人数一定时，花费时间与顾客人数近似成正比。

2. 实现互斥的方法有哪些？各自有什么特点？效率如何？

* 轮转法：要求两个进程严格轮流进入临界区，不满足“空闲让进”，且效率低下。
* 自旋锁：存在违反“忙则等待”的可能，忙等待违反“让权等待”，不影响正常使用但导致效率低。
* Peterson：解决了互斥访问的问题，克服了轮转法的缺点，可以正常工作；但需要忙等待，效率低。
* 禁止中断：进入临界区前指定“关中断”指令，离开后执行“开中断”指令，以避免进程切换，简单，但不适用于多处理器。
* 信号量：引入了一个地位高于进程的管理者来解决公有资源的使用问题，解决了忙等待的问题，但需要额外的数据结构，且容易出错。
* 管程：使用“秘书”进程即管程控制来访。管程是关于共享资源的数据结构以及一组针对该资源的操作过程所构成的软件模块，但并非所有语言都支持管程。
* ...

## 7 实验感想

本次实验中，我学会了如何使用modern C++程序建模进程的同步/互斥问题，同时也对C++中的语法进行了复习，非常感谢助教和老师的辛勤付出！