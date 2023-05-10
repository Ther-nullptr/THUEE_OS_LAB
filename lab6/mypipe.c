#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/slab.h>

#define PIPE_BUFFER_SIZE 4096
#define PIPE_NUMBER 238

#define min(a, b) ((a) < (b) ? (a) : (b))

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ther-nullptr");
MODULE_DESCRIPTION("a pipe");

static int flag; // to detect whether the buffer is full or empty 
static ssize_t p_read; // the pointer to read
static ssize_t p_write; // the pointer to write
static char* kernel_buffer; // the buffer in kernel space

static struct mutex mutex_buffer;
static struct semaphore sem_full;
static struct semaphore sem_empty;

static ssize_t mypipe_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    // lock the buffer
    mutex_lock_killable(&mutex_buffer);
    ssize_t actual_read_length = 0;
    int ret = 0;
    while (flag == 0 && p_read == p_write) // wait until the buffer is readable
    {
        // block the read process
        down_interruptible(&sem_empty);
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
    p_read = (p_read + actual_read_length) % PIPE_BUFFER_SIZE;
    flag = 0; // buffer will become empty after reading
    // wake up the write process
    up(&sem_full);
    if (ret != 0)
    {
        printk(KERN_ALERT "Error in reading from pipe.\n");
        return -EFAULT;
    }

    mutex_unlock(&mutex_buffer);
    return actual_read_length;
}

static ssize_t mypipe_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    // lock the buffer
    mutex_lock_killable(&mutex_buffer);
    ssize_t actual_write_length = 0;
    int ret = 0;
    while (flag == 1 && p_read == p_write) // wait until the buffer is writable
    {
        down_interruptible(&sem_full);
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
    p_write = (p_write + actual_write_length) % PIPE_BUFFER_SIZE;
    flag = 1; // buffer will become full after writing
    // wake up the read process
    up(&sem_empty);
    if (ret != 0)
    {
        printk(KERN_INFO "Error in writing to pipe.\n");
    }
    mutex_unlock(&mutex_buffer);
    return actual_write_length;
}

static int mypipe_open(struct inode * inode, struct file * file) 
{
    return 0;
}

static int mypipe_release(struct inode * inode, struct file * file)
{
    return 0;
}

// define the file operations   
static struct file_operations mypipe_fops = {
    .owner = THIS_MODULE,
    .read = mypipe_read,
    .write = mypipe_write,
    .open = mypipe_open,
    .release = mypipe_release
};

static int __init mypipe_init(void)
{
    int ret;
    ret = register_chrdev(PIPE_NUMBER, "mypipe", &mypipe_fops);
    kernel_buffer = kmalloc(PIPE_BUFFER_SIZE, GFP_KERNEL);
    sema_init(&sem_full, 0);
    sema_init(&sem_empty, 1);
    printk(KERN_INFO "mypipe: register device successfully\n");
    return 0;
}

static void __exit mypipe_exit(void)
{
    unregister_chrdev(PIPE_NUMBER, "mypipe");
    kfree(kernel_buffer);
    printk(KERN_INFO "mypipe: unregister device successfully\n");
}

module_init(mypipe_init);
module_exit(mypipe_exit);
