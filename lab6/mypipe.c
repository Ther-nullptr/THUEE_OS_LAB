#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>

#define PIPE_BUFFER_SIZE 4096
#define PIPE_NUMBER 1000

#define min(a, b) ((a) < (b) ? (a) : (b))

int p_read; // the pointer to read
int p_write; // the pointer to write
atomic_t flag; // to detect whether the buffer is full or empty 
char* kernel_buffer; // the buffer in kernel space

struct semaphore sem_full;
struct semaphore sem_empty;

// register the methods of device
static int mypipe_open(struct inode *inode, struct file *file);
static int mypipe_release(struct inode *inode, struct file *file);
static ssize_t mypipe_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t mypipe_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);

// define the file operations   
static struct file_operations mypipe_fops = {
    .open = mypipe_open,
    .release = mypipe_release,
    .read = mypipe_read,
    .write = mypipe_write,
};

static int __init mypipe_init(void)
{
    int ret;
    ret = register_chrdev(PIPE_NUMBER, "mypipe", &mypipe_fops);
    kernel_buffer = kmalloc(PIPE_BUFFER_SIZE, GFP_KERNEL);
    memset(kernel_buffer, 0, PIPE_BUFFER_SIZE);
    sema_init(&sem_full, 1);
    sema_init(&sem_empty, 0);
    printk(KERN_INFO "mypipe: register device successfully\n");
    return 0;
}

static void __exit mypipe_exit(void)
{
    unregister_chrdev(PIPE_NUMBER, "mypipe");
    kfree(buffer);
    printk(KERN_INFO "mypipe: unregister device successfully\n");
}

static ssize_t mypipe_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t actual_read_length = 0;
    if (flag == 0 && p_read == p_write)
    {
        // block the read process
        down_interruptible(&sem_empty);
    }
    else if (p_read < p_write)
    {
        actual_read_length = min(count, p_write - p_read); // the actual length to read is limited by the length of the buffer
        copy_to_user(buf, buffer + p_read, actual_read_length);
    }
    else
    {
        actual_read_length = min(count, PIPE_BUFFER_SIZE - (p_read - p_write));
        ssize_t max_no_iterable = PIPE_BUFFER_SIZE - p_read;
        if (actual_read_length <= max_no_iterable)
        {
            copy_to_user(buf, buffer + p_read, actual_read_length);
        }
        else
        {
            copy_to_user(buf, buffer + p_read, max_no_iterable);
            copy_to_user(buf + max_no_iterable, buffer, actual_read_length - max_no_iterable);
        }
    }
    p_read = (p_read + actual_read_length) % PIPE_BUFFER_SIZE;
    flag = 0; // buffer will become empty after reading
    // wake up the write process
    up(&sem_full);
    return actual_read_length;
}

static ssize_t mypipe_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t actual_write_length = 0;
    if (flag == 1 && p_read == p_write)
    {
        down_interruptible(&sem_full);
    }
    if (p_read > p_write)
    {
        actual_write_length = min(count, p_read - p_write);
        copy_from_user(buffer + p_write, buf, actual_write_length);
    }
    else
    {
        actual_write_length = min(count, PIPE_BUFFER_SIZE - (p_write - p_read));
        ssize_t max_no_iterable = PIPE_BUFFER_SIZE - p_write;
        if (actual_write_length <= max_no_iterable)
        {
            copy_from_user(buffer + p_write, buf, actual_write_length);
        }
        else
        {
            copy_from_user(buffer + p_write, buf, max_no_iterable);
            copy_from_user(buffer, buf + max_no_iterable, actual_write_length - max_no_iterable);
        }
    }
    p_write = (p_write + actual_write_length) % PIPE_BUFFER_SIZE;
    flag = 1; // buffer will become full after writing
    // wake up the read process
    up(&sem_empty);
    return actual_write_length;
}

module_init(mypipe_init);
module_exit(mypipe_exit);