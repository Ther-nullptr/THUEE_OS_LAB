# 实验4 处理机调度——实时处理机调度

## 1 实验目的

1. 理解常见的实时调度算法，如RMS算法、EDF算法、LLF算法等；
2. 对不同的实时调度算法进行仿真实现，比较不同算法的优劣。

## 2 实验平台

本实验在x86_64平台的Ubuntu 20.04 LTS操作系统上进行，编程语言采用C++11。

## 3 实验原理

### 3.1 实时调度算法

1. RMS算法：为每个进程分配一个与事件发生频率成正比的优先级，运行时调度程序总是调度优先级最高的就绪进程，必要时将抢先当前进程。
2. EDF算法：当一个事件发生时，对应的进程被加到就绪队列，该队列按照截止时限排序。
3. LLF算法：总是选择裕度最小的进程运行：裕度=(截止时刻-当前时刻)-剩余运行时间。

### 3.2 语言支持

三种算法都需要用到优先队列来实现预备事件的自动排序，C++11中的`std::priority_queue`可以满足要求，而排序规则可以通过重载`operator()`来实现。

## 4 实验过程

### 4.1 数据预处理

实验所提供的测例提供了两种类型的事件：周期性事件和非周期性事件。为了方便之后的算法进行预处理，需要将周期性事件转换为非周期性事件。具体来说，截止时限即为事件下一次发生的时间，而周期则为事件发生的间隔：

```cpp
if (is_cycle)
{
    // periodic task
    int i = 0;
    while(1)
    {
        // do something
        int actual_in_time = in_time + i * period_or_stop_time;
        if (actual_in_time > total_time)
        {
            break;
        }
        Event event{index : i, in_time : actual_in_time, total_run_time : run_time, stop_time : actual_in_time + period_or_stop_time, event_name : event_name, time_pointer : 0, priority : 1000 / period_or_stop_time};
        event_queue.push(event);
        i++;
    }
}
```

非周期事件则正常处理。两种事件都被放入按照进入时间排列的优先队列中。

### 4.2 调度算法实现

首先确定基本框架：

```cpp
class Strategy
{
public:
    Strategy() {}
    virtual ~Strategy() {}
    virtual result_pair run(event_queue_type &events, int total_time) = 0;
    virtual void preempt(int start_time, int end_time) = 0;

protected:
    bool is_running = false;
    bool succeed = true;
    Event current_event;
    std::vector<Result> results;
};
```

这其中，`run`函数用于执行调度算法，`preemption`函数用于处理抢占事件。`run`函数的返回值为一个`result_pair`，包含了调度结果和是否成功的信息。`preemption`函数的参数为抢占开始时间和结束时间。之后在编写具体算法时，只需要继承`Strategy`类并实现这两个函数即可。

之后，在算法中，使用一个循环来模拟时间片。首先检测是否有事件到达，如果有，则将其放入优先级队列（就绪队列）：

```cpp
// prepare the event schedule queue
Event event = events.top();
while (event.in_time == i - 1)
{
    events.pop();
    event_schedule_queue.push(event);
    if (events.empty())
    {
        break;
    }
    event = events.top();
}
```

每次循环，如果当前没有进程在运行，则从就绪队列中取出优先级最高的进程运行；如果当前有进程在运行，则判断是否有更高优先级的进程到达，如果有，则抢占当前进程，将其放回就绪队列，然后运行新的进程。每次循环，时间本身的时间指针都会加一，如果时间指针等于进程的运行时间，则说明进程已经运行完毕，将其放入结果队列中。

```cpp
int start_time;
if (!is_running)
{
    if (!event_schedule_queue.empty())
    {
        current_event = event_schedule_queue.top();
        event_schedule_queue.pop();
        start_time = i; // begin to run
        is_running = true;
    }
}

if (is_running)
{
    current_event.time_pointer = current_event.time_pointer + 1;

    // detect preempt
    if (event_arrive)
    {
        preempt(i);
        event_arrive = false;
    }

    if (i > current_event.stop_time) // fail to schedule
    {
        succeed = false;
        break;
    }

    if (current_event.time_pointer == current_event.total_run_time) // finish running
    {
        Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : i, event_name : current_event.event_name, is_interrupted : 0};
        results.push_back(result);
        is_running = false;
    }
}
i++;
```

不同算法之间的差异主要在于`preemption`的实现以及就绪队列的排序规则。接下来将分算法进行讨论。

#### 4.2.1 RMS算法

RMS算法会根据进程的频率确定优先级，频率越高，优先级越高。非周期性事件的优先级规定为0，周期性事件的优先级为`1000/period`。排序规则如下：

```cpp
struct rms_cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.priority == b.priority)
        {
            return a.in_time > b.in_time;
        }
        return a.priority < b.priority;
    }
};
```

抢占规则为：如果就绪队列队首的优先级高于当前进程的优先级，则抢占。抢占时，将当前进程放回就绪队列，然后运行新的进程。

```cpp
void preempt(int preempt_time) override
{
    Event next_event = event_schedule_queue.top();
    if (next_event.priority > current_event.priority && !(current_event.time_pointer == current_event.total_run_time))
    {
        Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : preempt_time - 1, event_name : current_event.event_name, is_interrupted : 1};
        current_event.time_pointer = current_event.time_pointer - 1;
        start_time = preempt_time; 
        next_event.time_pointer = next_event.time_pointer + 1;
        event_schedule_queue.pop();
        event_schedule_queue.push(current_event);
        results.push_back(result);
        current_event = next_event;
    }
}
```

> 这里需要强调一下时间片的使用方法，假设事件A的运行时间为0~5和10~15，事件B的运行时间为5~10，即事件A被B打断。那么事件A所占据的时间片为1、2、3、4、5和11、12、13、14、15，事件B所占据的时间片为6、7、8、9、10。但最后在记录信息时，事件A的各个属性都被记载为5的倍数。RMS、EDF和LLF算法都是这样处理的。

#### 4.2.2 EDF算法

EDF算法会根据进程的截止时间确定优先级，截止时间越早，优先级越高。排序规则如下：

```cpp
struct edf_cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.stop_time == b.stop_time)
        {
            return a.in_time > b.in_time;
        }
        return a.stop_time > b.stop_time;
    }
};
```

抢占规则为：如果就绪队列队首的截止时间早于当前进程的最早截止时间，则抢占。抢占时，将当前进程放回就绪队列，然后运行新的进程。

```cpp
void preempt(int preempt_time) override
{
    Event next_event = event_schedule_queue.top();
    if (next_event.stop_time < current_event.stop_time && !(current_event.time_pointer == current_event.total_run_time))
    {
        Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : preempt_time - 1, event_name : current_event.event_name, is_interrupted : 1};
        current_event.time_pointer = current_event.time_pointer - 1;
        start_time = preempt_time; 
        next_event.time_pointer = next_event.time_pointer + 1;
        event_schedule_queue.pop();
        event_schedule_queue.push(current_event);
        results.push_back(result);
        current_event = next_event;
    }
}
```

#### 4.2.3 LLF算法

LLF算法会根据进程的松弛度确定优先级，松弛度越大，优先级越高。排序规则如下：

```cpp
struct llf_cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.laxity == b.laxity)
        {
            return a.in_time > b.in_time;
        }
        return a.laxity > b.laxity;
    }
};
```

抢占规则为：如果就绪队列队首的松弛度大于当前进程的松弛度，则抢占。抢占时，将当前进程放回就绪队列，然后运行新的进程。

```cpp
void preempt(int preempt_time) override
{
    Event next_event = event_schedule_queue.top();
    if (next_event.laxity < current_event.laxity && !(current_event.time_pointer == current_event.total_run_time))
    {
        Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : preempt_time - 1, event_name : current_event.event_name, is_interrupted : 1};
        current_event.time_pointer = current_event.time_pointer - 1;
        start_time = preempt_time; 
        next_event.time_pointer = next_event.time_pointer + 1;
        event_schedule_queue.pop();
        event_schedule_queue.push(current_event);
        results.push_back(result);
        current_event = next_event;
    }
}
```

注意，LLF算法还引入一个逻辑——每当有任务到达时，就要重新计算所有任务的松弛度：

```cpp
if (event_arrive)
{
    // update all the laxity in event schedule queue
    std::priority_queue<Event, std::vector<Event>, llf_cmp> temp_queue;
    while (!event_schedule_queue.empty())
    {
        Event temp_event = event_schedule_queue.top();
        event_schedule_queue.pop();
        temp_event.laxity = temp_event.stop_time - (i - 1) - (temp_event.total_run_time - temp_event.time_pointer);
        temp_queue.push(temp_event);
    }
    event_schedule_queue = temp_queue;
    current_event.laxity = current_event.stop_time - (i - 1) - (current_event.total_run_time - current_event.time_pointer);
}
```

## 5 实验结果

编写以下测例，测试以上算法的正确性：

* 三种调度算法都满足绝对的截止时间

测例如下：

```
140
1 1 0 30 10
2 1 0 40 15
3 1 0 50 5
4 0 70 80 5 
5 0 70 85 5
```

该测例仿照课件中的例子，只不过添加了两个非周期性进程，用于测试非周期性进程的调度。结果如下：

RMS：

![image.png](https://s2.loli.net/2023/05/14/wDSrIPOcU5pYFv3.png)

EDF：

![image.png](https://s2.loli.net/2023/05/14/7GbP5uICj9pnFzi.png)

LLF：

![image.png](https://s2.loli.net/2023/05/14/Zlb5qXrYiS1KDCB.png)

与课件中的例子相比较，结果一致（两个非周期事件被插入70~80的空档中），说明正确：

![image.png](https://s2.loli.net/2023/05/14/xrWPAYoF2ThJZR9.png)

* 至少一种调度算法不满足绝对的截止时间

测例如下：

```
140
1 1 0 30 15
2 1 0 40 15
3 1 0 50 5
4 0 135 165 5 
5 0 135 160 5
```

该测例仿照课件中的例子，只不过添加了两个非周期性进程，用于测试非周期性进程的调度。结果如下：

RMS：

![image.png](https://s2.loli.net/2023/05/14/6TMjXSZ3RBkyqsW.png)

EDF：

![image.png](https://s2.loli.net/2023/05/14/LBhKEpHNT2OUXMS.png)

LLF：

![image.png](https://s2.loli.net/2023/05/14/IobrMPpKS1qBlN4.png)

与课件中的例子相比较，结果一致（两个非周期事件被插入时间队列的末尾），说明正确：

![image.png](https://s2.loli.net/2023/05/14/7zjJf2NsaKrgFQH.png)

可以观察到RMS算法调度失败，究其原因在于，调度算法总会优先运行频率更高的事件（B），而在运行该事件时错过了C事件的截止时间。

## 6 思考题

三种调度算法有何优劣？各自适用于什么情境？

| 算法 | 优点                   | 缺点                                                         |
| ---- | ---------------------- | ------------------------------------------------------------ |
| RMS  | 简单易用，算法逻辑简单 | 只擅长处理周期性事件；在CPU使用率较高时失败概率较大；抢占概率高，造成额外开销 |
| EDF  | 调度失败概率低         | 算法复杂                                                     |
| LLF  | 调度失败概率低         | 算法复杂；抢占概率高，造成额外开销                           |



## 7 实验感想

本次实验除了让我更加深刻地理解了调度算法，也让我复习了C++中的一些高级数据结构和面向对象编程，同时也让我意识到精心设计的测例是衡量算法有效性和鲁棒性的有力工具，衷心感谢助教和老师的辛勤付出！