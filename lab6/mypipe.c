#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/slab.h>

#define PIPE_BUFFER_SIZE 16
#define IGNORE_BUFFER_SIZE 16
#define PIPE_NUMBER 200

#define min(a, b) ((a) < (b) ? (a) : (b))

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ther-nullptr");
MODULE_DESCRIPTION("a pipe");

static int flag = 0; // whether the buffer is empty or full
static ssize_t p_read; // the pointer to read
static ssize_t p_write; // the pointer to write
static char* kernel_buffer; // the buffer in kernel space

static struct mutex mutex_buffer;
static struct semaphore sem;

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
    memset(kernel_buffer, 0, PIPE_BUFFER_SIZE);
    mutex_init(&mutex_buffer);
    sema_init(&sem, 1);
    printk(KERN_INFO":mypipe register successfully\n");
    return 0;
}

static void __exit mypipe_exit(void)
{
    unregister_chrdev(PIPE_NUMBER, "mypipe");
    kfree(kernel_buffer);
    sema_init(&sem, 0);
    mutex_destroy(&mutex_buffer);
    printk(KERN_INFO":mypipe unregister successfully\n");
}

module_init(mypipe_init);
module_exit(mypipe_exit);
