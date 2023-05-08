# 实验6 管道驱动程序开发

## 1 问题描述

编写一个设备驱动程序mypipe实现管道，该驱动程序创建两个设备实例，一个针对管道的输入端，另一个针对管道的输出端。另外，你还需要编写两个测试程序，一个程序向管道的输入端写入数据，另一个程序从管道的输出端读出数据，从而实现两个进程间通过你自己实现的管道进行数据通信。

## 2 实验平台

本实验在x86_64 平台的Ubuntu 20.04 LTS 操作系统上进行，编程语言采用C++11，构建工具采用GNU Make 4.2.1。

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

当缓冲区为空时，读操作会阻塞，直到缓冲区中有数据；当缓冲区满时，写操作会阻塞，直到缓冲区中有空间。因此需要使用两个信号量`sem_empty`和`sem_full`来实现这一功能。

### 4.1 init

在`init`函数中，需要完成以下几个步骤：

1. 注册字符设备驱动；
2. 分配管道需要使用的内核缓冲区；
3. 初始化管道通信所需的信号量；

代码如下：

```c
```

### 4.2 exit

在`exit`函数中，需要完成以下几个步骤：

1. 取消注册字符设备驱动；
2. 释放管道使用的内核缓冲区；

### 4.3 read

首先分析4个参数：

* struct file *file: 需要进行读的文件
* char __user *buf: **用户空间**的缓冲区
* size_t count: 需要读取的字节数
* loff_t *f_pos: 存储文件偏移量的指针

我们重点使用`buf`和`count`两个参数。

### 4.4 write

4个参数的含义同上。

注意在实验过程中，可以随时插入`printk`来打印信息，以便于调试。

## 参考文献

https://zhuanlan.zhihu.com/p/442934053

https://zhuanlan.zhihu.com/p/47168082

https://zhuanlan.zhihu.com/p/58489873

https://www.cnblogs.com/Anker/p/3271773.html

https://segmentfault.com/a/1190000009528245

https://github.com/torvalds/linux/blob/master/fs/pipe.c

https://www.cnblogs.com/52php/tag/%E8%BF%9B%E7%A8%8B%E9%80%9A%E4%BF%A1/

https://man7.org/linux/man-pages/man2/memfd_create.2.html

https://drustz.com/posts/2015/09/27/step-by-step-shell1/#a5

https://zorrozou.github.io/docs/books/linuxde-jin-cheng-jian-tong-xin-guan-dao.html

https://www.cntofu.com/book/46/linux_system/linuxxi_tong_bian_cheng_zhi_wen_jian_yu_io_ff08_ba.md

http://blog.logan.tw/2013/01/linux-driver.html

https://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html