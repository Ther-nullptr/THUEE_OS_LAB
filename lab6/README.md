# 实验6 管道驱动程序开发

## 1 问题描述

编写一个设备驱动程序mypipe实现管道，该驱动程序创建两个设备实例，一个针对管道的输入端，另一个针对管道的输出端。另外，你还需要编写两个测试程序，一个程序向管道的输入端写入数据，另一个程序从管道的输出端读出数据，从而实现两个进程间通过你自己实现的管道进行数据通信。

## 2 实验平台

本实验在x86_64平台的Ubuntu 20.04 LTS 操作系统上进行，编程语言采用C++11，构建工具采用GNU Make 4.2.1。

## 3 实验原理

在进行正式的实验之前，我们首先需要了解一些基本的知识，如Linux中的一些机制及其对应的系统函数，这些知识将会在实验中用到。

### 3.1 Linux中的管道实现

在Linux中，管道本质上就是一个文件，前面的进程以写方式打开文件，后面的进程以读方式打开。但是管道本身并不占用磁盘或者其他外部存储的空间，而只占用内存空间。

Linux中的管道分为两种：

1. 匿名管道（Anonymous Pipe）：只能在父子进程或者兄弟进程之间使用，因为管道的两端都是由同一个进程创建的，所以只能在同一个进程中使用。使用`pipe()`函数创建。
2. 命名管道（Named Pipe）：可以在不同的进程之间使用，因为命名管道是由一个进程创建，然后由另一个进程打开的。使用`mkfifo()`函数创建。

#### 3.1.1 匿名管道

Linux中的管道通常被认为是半双工的，但在实际使用时，一般仅使用其单工模式，即数据只能从管道的一端流向另一端，而不能双向流动，如下图所示：

![image.png](https://s2.loli.net/2023/05/02/eBgXf6RnxoMJHqu.png)

因此在实现时，需要禁用父进程的读和子进程的写。

#### 3.1.2 命名管道

命名管道在底层的实现跟匿名管道完全一致，区别只是命名管道会有一个全局可见的文件名以供别人open打开使用。此外，命名管道中的两个进程无需具有亲缘关系。

### 3.2 Linux管道的工作机制

linux中管道的基本实现细节如下：

1. 管道是由内核管理的一个缓冲区，管道单独构成一种文件系统，并且只存在于内存中，并不占用磁盘或者其他外部存储的空间；这一缓冲的大小通常为4096字节。
2. 当管道中没有信息的话，从管道中读取的进程会等待，直到另一端的进程放入信息；当管道被放满信息的时候，尝试放入信息的进程会等待，直到另一端的进程取出信息。
3. 两个进程建立管道连接时，将两个file结构指向同一个临时的 VFS 索引节点，而这个 VFS 索引节点又指向一个物理页面。
4. 当两个进程都终结的时候，管道也自动消失。

### 3.3 Linux驱动程序开发

一个基本的Linux驱动代码如下：

```cpp
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int __init example_init(void) 
{
    printk("<1>EXAMPLE: init\n");
    return 0;
}

static void __exit example_exit(void) 
{
    printk("<1>EXAMPLE: exit\n");
}

module_init(example_init);
module_exit(example_exit);
```

驱动代码只有一个入口点和一个出口点，把驱动加载到内核中，会执行module_init函数定义的函数；当驱动从内核被卸载时，会调用module_exit函数定义的函数。

当编译驱动时，需要使用如下所示的makefile:
```makefile
obj-m += xxx.o
KERNELBUILD := /lib/modules/$(shell uname -r)/build
default:
	make -C $(KERNELBUILD) M=$(PWD) modules
clean:
	make -C $(KERNELBUILD) M=$(PWD) clean
```

此处需要开发的管道驱动程序是一个**字符设备驱动**，因此需要使用`cdev`结构体来描述设备，使用`file_operations`结构体来描述设备的操作。

字符设备驱动通常被放置在`/dev`下，在系统中添加一个驱动意味着在内核中注册它，可以通过`register_chrdev`来实现这一点：

```c
int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);
```

同理也有取消注册函数：

```c
void unregister_chrdev(unsigned int major, const char *name);
```

`file_operations`结构体持有指向驱动程序定义的在设备上执行各种操作的函数的指针。该结构的每一个字段都对应于由驱动程序定义的一些函数的地址，以处理所请求的操作。以下列举了一些常用的字段：

```c
struct file_operations {
	struct module *owner;
	ssize_t(*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t(*write) (struct file *, const char __user *, size_t, loff_t *);
	int (*open) (struct inode *, struct file *);
	int (*release) (struct inode *, struct file *);
};
```

一个注册示例如下：

```c
struct file_operations fops = {
	read: device_read,
	write: device_write,
	open: device_open,
	release: device_release
};
```

## 4 实验过程

首先梳理一下管道缓冲区的使用逻辑：为了最大限度利用缓冲区，本项目将缓冲区建模为环形队列，使用指针`p_write`和`p_read`分别指向写入和读取的位置：
1. 当`p_write`和`p_read`指向同一位置时，说明缓冲区为空或满。为了区分这两种情况，使用一个标志位`full`来标记缓冲区是否满；
2. 当`p_write > p_read`时，说明缓冲区的内容是连续的，此时缓冲区的大小为`p_write - p_read`；
3. 当`p_write < p_read`时，说明缓冲区的内容是不连续的，此时缓冲区的大小为`p_write + BUFFER_SIZE - p_read`；

当进行读操作时，写操作会被阻塞；当进行写操作时，读操作会被阻塞。

### 4.1 init

在`init`函数中，需要完成以下几个步骤：

1. 注册字符设备驱动；
2. 分配管道需要使用的内核缓冲区；
3. 初始化管道通信所需的信号量/互斥锁。

代码如下：

```c
static int __init mypipe_init(void)
{
    int ret;
    ret = register_chrdev(PIPE_NUMBER, "mypipe", &mypipe_fops);
    kernel_buffer = kmalloc(PIPE_BUFFER_SIZE, GFP_KERNEL);
    memset(kernel_buffer, 0, PIPE_BUFFER_SIZE);
    mutex_init(&mutex_buffer);
    sema_init(&sem, 1);
    printk(KERN_INFO":mypipe register successfully\n");
    return 0;
}
```

### 4.2 exit

在`exit`函数中，需要完成以下几个步骤：

1. 取消注册字符设备驱动；
2. 释放管道使用的内核缓冲区；
3. 删除管道通信所需的信号量/互斥锁。

代码如下：

```c
static void __exit mypipe_exit(void)
{
    unregister_chrdev(PIPE_NUMBER, "mypipe");
    kfree(kernel_buffer);
    sema_init(&sem, 0);
    mutex_destroy(&mutex_buffer);
    printk(KERN_INFO":mypipe unregister successfully\n");
}
```

### 4.3 read

首先分析4个参数：

* struct file *file: 需要进行读的文件
* char __user *buf: **用户空间**的缓冲区
* size_t count: 需要读取的字节数
* loff_t *f_pos: 存储文件偏移量的指针

我们重点使用`buf`和`count`两个参数。

每次读时，将flag置为0。由于我们将buffer建模为环形队列，所以需要分以下情况讨论：

1. `p_write == p_read && flag == 0`：此时缓冲区为空，不进行读操作；
2. `p_write > p_read`：此时缓冲区的内容一定是连续的，空闲缓冲区大小为`p_write - p_read`，因此可以直接使用`copy_to_user`函数将缓冲区的内容拷贝到用户空间；
3. 其他：此时缓冲区的内容可能是不连续的，空闲缓冲区大小为`p_write + BUFFER_SIZE - p_read`，当写入的缓冲区不连续时，需要分两次拷贝，第一次拷贝`BUFFER_SIZE - p_read`个字节，第二次拷贝`p_write`个字节。

处理逻辑如图所示：

![b11dea8d75bbf6f6c554c128c0ab032.jpg](https://s2.loli.net/2023/05/10/O9oLYTwhsiH87xB.jpg)

```c
static ssize_t mypipe_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    // lock the buffer
    mutex_lock_killable(&mutex_buffer);
    ssize_t actual_read_length = 0;
    int ret = 0;
    down_interruptible(&sem);

    if (p_read == p_write && flag == 0)
    {
        printk(KERN_WARNING":the buffer is empty and will not be readable until the next write");
        goto end_read;
    }
    
    if (p_read < p_write)
    {
        actual_read_length = min(count, p_write - p_read); // the actual length to read is limited by the length of the buffer
        ret |= copy_to_user(buf, kernel_buffer + p_read, actual_read_length);
    }
    else
    {
        actual_read_length = min(count, PIPE_BUFFER_SIZE - (p_read - p_write));
        ssize_t max_no_iterable = PIPE_BUFFER_SIZE - p_read;
        if (actual_read_length <= max_no_iterable)
        {
            ret |= copy_to_user(buf, kernel_buffer + p_read, actual_read_length);
        }
        else
        {
            ret |= copy_to_user(buf, kernel_buffer + p_read, max_no_iterable);
            ret |= copy_to_user(buf + max_no_iterable, kernel_buffer, actual_read_length - max_no_iterable);
        }
    }
    
    printk(KERN_INFO":read %zu bytes\n", actual_read_length);
    printk(KERN_INFO":p_read before %zu\n", p_read);
    p_read = (p_read + actual_read_length) % PIPE_BUFFER_SIZE;
    printk(KERN_INFO":change p_read to %zu\n", p_read);
    
end_read:
    // wake up the write process
    up(&sem);
    flag = 0;
    if (ret != 0)
    {
        printk(KERN_ALERT"Error in reading from pipe.\n");
        return -EFAULT;
    }
    mutex_unlock(&mutex_buffer);
    return actual_read_length;
}
```

### 4.4 write

4个参数的含义同上。每次写时，将flag置为1。同样需要分以下情况讨论：

1. `p_write == p_read && flag == 1`：此时缓冲区已满，不进行写操作；
2. `p_write < p_read`：此时将要被写入的缓冲区一定是连续的，空闲缓冲区大小为`p_read - p_write`，因此可以直接使用`copy_from_user`函数将用户空间的内容拷贝到缓冲区；
3. 其他：此时将要被写入的缓冲区可能是不连续的，空闲缓冲区大小为`p_read + BUFFER_SIZE - p_write`，当写入的缓冲区不连续时，需要分两次拷贝，第一次拷贝`BUFFER_SIZE - p_write`个字节，第二次拷贝`p_read`个字节。

处理逻辑如图所示：

![be7a94856ec48e7da0344c07800d13c.jpg](https://s2.loli.net/2023/05/10/PTLSWs41RcyxvoC.jpg)

这里特别注意，当buffer满的时候，我们需要将`actual_write_length`设置为`IGNORE_BUFFER_SIZE`，否则用户空间的`write`函数会一直阻塞。

```c
static ssize_t mypipe_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    // lock the buffer
    mutex_lock_killable(&mutex_buffer);
    ssize_t actual_write_length = 0;
    int ret = 0;
    down_interruptible(&sem);

    if (p_read == p_write && flag == 1)
    {
        printk(KERN_WARNING":the buffer is full and will not be writeable until the next read");
        actual_write_length = IGNORE_BUFFER_SIZE;
        goto end_write;
    }
    
    if (p_read > p_write)
    {
        actual_write_length = min(count, p_read - p_write);
        ret |= copy_from_user(kernel_buffer + p_write, buf, actual_write_length);
    }
    else
    {
        actual_write_length = min(count, PIPE_BUFFER_SIZE - (p_write - p_read));
        ssize_t max_no_iterable = PIPE_BUFFER_SIZE - p_write;
        if (actual_write_length <= max_no_iterable)
        {
            ret |= copy_from_user(kernel_buffer + p_write, buf, actual_write_length);
        }
        else
        {
            ret |= copy_from_user(kernel_buffer + p_write, buf, max_no_iterable);
            ret |= copy_from_user(kernel_buffer, buf + max_no_iterable, actual_write_length - max_no_iterable);
        }
    }
    
    printk(KERN_INFO":write %zu bytes\n", actual_write_length);
    printk(KERN_INFO":p_write before %zu\n", p_write);
    p_write = (p_write + actual_write_length) % PIPE_BUFFER_SIZE;
    printk(KERN_INFO":change p_write to %zu\n", p_write);

end_write:
    // wake up the read process
    up(&sem);
    flag = 1;
    
    if (ret != 0)
    {
        printk(KERN_INFO":Error in writing to pipe.\n");
    }
    mutex_unlock(&mutex_buffer);
    return actual_write_length;
}
```

注意在实验过程中，可以随时插入`printk`来打印信息，以便于调试。

## 5 实验结果

### 5.1 编译与安装

为方便管理驱动的编译，编写`Makefile`如下：

```makefile
obj-m += mypipe.o
KERNELBUILD := /lib/modules/$(shell uname -r)/build
default:
	make -C $(KERNELBUILD) M=$(shell pwd) modules
clean:
	make -C $(KERNELBUILD) M=$(shell pwd) clean
```

使用`sudo make`指令编译驱动，得到`mypipe.ko`文件。

使用以下指令载入驱动：

```bash
sudo insmod mypipe.ko
dmesg | tail -n 1
```

显示结果如下，说明载入成功：
```bash
[  243.705553] mypipe: register device successfully
```

安装设备：
```bash
sudo mknod /dev/mypipe c 200 0 # c: char device, 400: major number, 0: minor number 
sudo chmod 666 /dev/mypipe
```

若想卸载驱动，使用以下指令：

```bash
sudo rmmod mypipe
dmesg | tail -n 1
sudo rm -f /dev/mypipe
```

显示结果如下，说明卸载成功：
```bash
[  259.466596] mypipe: unregister device successfully
```
### 5.2 测试

为了方便说明问题，我们将`mypipe`的buffer size设置为16，不过在实际应用场景中，这一值应该为4096。
#### 5.2.1 命令行操作

我们模拟linux中命名管道的操作：

```bash
$ sudo echo 114514 > /dev/mypipe
$ sudo echo 114514 > /dev/mypipe
$ sudo echo 1919810 > /dev/mypipe
$ sudo cat /dev/mypipe 
```

输出结果如下：
```bash
114514
114514
19
```

使用`dmesg`查看日志：

![image.png](https://s2.loli.net/2023/05/10/WnJH9FmqlXDPe8B.png)

可见其工作逻辑符合预期。

#### 5.2.2 模拟文件读写

编写两个测试程序，一个向`mypipe`写入数据，另一个从`mypipe`读取数据：

```c
// pipe_write.c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PIPE_BUFFER_SIZE 16

int main(int argc, char *argv[])
{
    char buffer[PIPE_BUFFER_SIZE];

    int fd = open("/dev/mypipe", O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("[ERROR] Fail to open pipe for writing data.\n");
        exit(1);
    }
    
    int n = atoi(argv[1]);

    // write n numbers to the pipe
    for (int i = 0; i < n; i++)
    {
        buffer[i] = i + 97;
        printf("write %c\n", i + 97);
    }
    
    write(fd, &buffer, strlen(buffer));
    close(fd);
    return 0;
}
```

```cpp
// pipe_read.c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd = open("/dev/mypipe", O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("[ERROR] Fail to open pipe for reading data.\n");
        exit(1);
    }
    
    int n = atoi(argv[1]);
    
    char *buffer;
    buffer = (char *)malloc(n * sizeof(char));
    read(fd, buffer, n);
    
    for (int i = 0; i < n; i++)
    {
        printf("read %c\n", buffer[i]);
    }
    
    close(fd);
    free(buffer);
    return 0;
}
```

测试结果如下：

![image.png](https://s2.loli.net/2023/05/10/ZvVquKmSGR35j8w.png)

运行日志：

![image.png](https://s2.loli.net/2023/05/10/1fKSnWJDuRxXm2E.png)

可以看到，该驱动设备的工作符合预期。

## 6 实验感想

这是我第一次接触内核编程，我学习到了很多linux的系统调用在c语言中的写法，同时我对linux内部的工作机制有了更加深入的理解，衷心感谢助教和老师的辛勤付出！