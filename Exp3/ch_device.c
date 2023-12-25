#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define MAJOR_NUM 290

static DECLARE_WAIT_QUEUE_HEAD(readers_queue);
static DECLARE_WAIT_QUEUE_HEAD(writers_queue);
static DEFINE_MUTEX(sem);
static int buffer = 0;
static int reader_count = 0;
static int writer_count = 0;

static ssize_t mymodule_read(struct file *filp, char __user *buf, size_t len,
                             loff_t *off) {
  int retval;

  mutex_lock(&sem);
  reader_count++;
  mutex_unlock(&sem);

  // Wait until there are no writers
  wait_event_interruptible(writers_queue, writer_count == 0);

  // Read from buffer
  retval = copy_to_user(buf, &buffer, sizeof(buffer));

  mutex_lock(&sem);
  reader_count--;
  if (reader_count == 0) {
    // Signal writers that readers are done
    wake_up(&writers_queue);
  }
  mutex_unlock(&sem);

  return retval ? -EFAULT : sizeof(buffer);
}

static ssize_t mymodule_write(struct file *filp, const char __user *buf,
                              size_t len, loff_t *off) {
  int retval;
  mutex_lock(&sem);
  writer_count++;
  mutex_unlock(&sem);

  wait_event_interruptible(readers_queue,
                           reader_count == 0 && writer_count == 1);

  // Write to buffer
  retval = copy_from_user(&buffer, buf, sizeof(buffer));

  mutex_lock(&sem);
  writer_count--;
  // Signal readers that writers are done
  wake_up(&readers_queue);
  mutex_unlock(&sem);

  return retval ? -EFAULT : sizeof(buffer);
}

static int mymodule_open(struct inode *inode, struct file *filp) {
  printk("mymodule opened\n");
  return 0;
}

static int mymodule_release(struct inode *inode, struct file *filp) {
  printk("mymodule released\n");
  return 0;
}

static struct file_operations mymodule_fops = {
    .read = mymodule_read,
    .write = mymodule_write,
    .open = mymodule_open,
    .release = mymodule_release,
};

static int __init mymodule_init(void) {
  int ret;
  ret = register_chrdev(MAJOR_NUM, "mymodule", &mymodule_fops);
  if (ret) {
    printk("mymodule register failure\n");
    return ret;
  }
  printk("mymodule register success\n");
  return 0;
}

static void __exit mymodule_exit(void) {
  unregister_chrdev(MAJOR_NUM, "mymodule");
  printk("mymodule unregister success\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("duroy");
MODULE_DESCRIPTION("A simple example Linux module");
