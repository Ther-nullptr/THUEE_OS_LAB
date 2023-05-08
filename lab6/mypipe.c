#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <stdio.h>
#include <semaphore.h>

#define PIPE_BUFFER_SIZE 4096
#define PIPE_NUMBER 1000

#define min(a, b) ((a) < (b) ? (a) : (b))

int flag; // to detect whether the buffer is full or empty 
int p_read; // the pointer to read
int p_write; // the pointer to write
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
    sema_init(&sem_full, 0);
    sema_init(&sem_empty, 1);
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
    int actual_length;
    if (p_read < p_write)
    {
        actual_length = min(count, p_write - p_read); // the actual length to read is limited by the length of the buffer
        copy_to_user(buf, buffer + p_read, actual_length);
        p_read = (p_read + actual_length) % PIPE_BUFFER_SIZE;
    }
    else
    {
        actual_length = min(count, PIPE_BUFFER_SIZE - (p_read - p_write));
    }
}

module_init(mypipe_init);
module_exit(mypipe_exit);