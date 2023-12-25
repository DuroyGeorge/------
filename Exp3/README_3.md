# **Linux的动态模块与设备驱动**



## 测试驱动

`syscall`，内核不允许修改。

查看众多文档，在ARM架构的平台上找到了系统调用表的地址，并且找到了第169号是gettimeofday的系统调用。但最终也没能成功修改系统调用表，因为这一段是read-only，不准写。

![image-20231204200000375](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/image-20231204200000375.png)

![截屏2023-12-04 20.25.29](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-12-04 20.25.29.png)

## ch_device驱动

```c
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
```



### 代码解释

头文件包含：
```c
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
```
这些头文件包含了模块所需的Linux内核函数和数据结构的定义。

```c
#define MAJOR_NUM 290
```
定义了字符设备的主设备号。

```c
static DECLARE_WAIT_QUEUE_HEAD(readers_queue);
static DECLARE_WAIT_QUEUE_HEAD(writers_queue);
static DEFINE_MUTEX(sem);
```
`readers_queue` 和 `writers_queue` 是等待队列头，用于实现读者和写者的等待。

`sem` 是互斥信号量，用于对共享资源进行互斥访问。

```c
static int buffer = 0;
```
`buffer` 是共享的缓冲区，读者从中读取数据，写者向其中写入数据。

```c
static int reader_count = 0;
static int writer_count = 0;
```
`reader_count` 记录当前读者的数量，`writer_count` 记录当前写者的数量。

```c
static ssize_t mymodule_read(struct file *filp, char __user *buf, size_t len,
                             loff_t *off)
```
读者函数，负责从缓冲区中读取数据，使用了互斥信号量 `sem` 进行保护。

通过等待队列 `writers_queue` 实现读者等待，确保没有写者在写入时进行读取。

```c
static ssize_t mymodule_write(struct file *filp, const char __user *buf,
                              size_t len, loff_t *off)
```
写者函数，负责向缓冲区中写入数据，使用了互斥信号量 `sem` 进行保护。

通过等待队列 `readers_queue` 实现写者等待，确保没有读者在读取时进行写入。

```c
static int mymodule_open(struct inode *inode, struct file *filp);
static int mymodule_release(struct inode *inode, struct file *filp);
```
`mymodule_open`：模块打开时调用的函数，输出一条内核日志。

`mymodule_release`：模块释放时调用的函数，输出一条内核日志。

```c
static struct file_operations mymodule_fops = {
    .read = mymodule_read,
    .write = mymodule_write,
    .open = mymodule_open,
    .release = mymodule_release,
};
```
定义了文件操作结构体，将模块的读、写、打开和释放函数与对应的系统调用关联起来。

```c
static int __init mymodule_init(void);
static void __exit mymodule_exit(void);
```
`mymodule_init`：模块初始化函数，在模块加载时调用，注册字符设备。

`mymodule_exit`：模块退出函数，在模块卸载时调用，注销字符设备。

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("duroy");
MODULE_DESCRIPTION("A simple example Linux module");
```
包含模块的许可证、作者和描述信息。



### 工作原理

**初始化：**

模块加载时，`mymodule_init` 函数被调用，注册字符设备。设备的主设备号为 290。

**打开设备：**

当用户空间应用程序通过系统调用 `open` 打开设备时，`mymodule_open` 函数被调用，输出一条内核日志。

**读者访问：**

当用户空间应用程序通过系统调用 `read` 读取设备时，`mymodule_read` 函数被调用。读者在进入读者临界区之前会增加 `reader_count` 计数器，表示当前有读者正在访问。然后，它等待写者队列 `writers_queue` 上的条件，确保没有写者在写入时进行读取。读者从共享缓冲区 `buffer` 中读取数据，并通过 `copy_to_user` 将数据传递给用户空间。读者完成读取后，减少 `reader_count` 计数器，如果没有其他读者，就唤醒写者队列 `writers_queue`。

**写者访问：**

当用户空间应用程序通过系统调用 `write` 写入设备时，`mymodule_write` 函数被调用。写者在进入写者临界区之前会增加 `writer_count` 计数器，表示当前有写者正在访问。然后，它等待读者队列 `readers_queue` 上的条件，确保没有读者在读取时进行写入。写者通过 `copy_from_user` 从用户空间接收数据，并将数据写入共享缓冲区 `buffer`。写者完成写入后，减少 `writer_count` 计数器，并唤醒读者队列 `readers_queue`。

**关闭设备：**

当用户空间应用程序通过系统调用 `close` 关闭设备时，`mymodule_release` 函数被调用，输出一条内核日志。

**模块卸载：**

当模块卸载时，`mymodule_exit` 函数被调用，注销字符设备。通过使用互斥信号量 `sem` 和等待队列 `readers_queue`、`writers_queue`，该驱动模块确保了对共享资源的安全访问，避免了读者与写者之间的竞态条件。读者和写者之间的同步通过等待队列的机制实现，保证了在任何时刻只有一个读者或一个写者能够访问共享资源。

工作原理

初始化：

模块加载时，mymodule_init 函数被调用，注册字符设备。设备的主设备号为 290。

打开设备：

当用户空间应用程序通过系统调用 open 打开设备时，mymodule_open 函数被调用，输出一条内核日志。

读者访问：

当用户空间应用程序通过系统调用 read 读取设备时，mymodule_read 函数被调用。读者在进入读者临界区之前会增加 reader_count 计数器，表示当前有读者正在访问。然后，它等待写者队列 writers_queue 上的条件，确保没有写者在写入时进行读取。读者从共享缓冲区 buffer 中读取数据，并通过 copy_to_user 将数据传递给用户空间。读者完成读取后，减少 reader_count 计数器，如果没有其他读者，就唤醒写者队列 writers_queue。

写者访问：

当用户空间应用程序通过系统调用 write 写入设备时，mymodule_write 函数被调用。写者在进入写者临界区之前会增加 writer_count 计数器，表示当前有写者正在访问。然后，它等待读者队列 readers_queue 上的条件，确保没有读者在读取时进行写入。写者通过 copy_from_user 从用户空间接收数据，并将数据写入共享缓冲区 buffer。写者完成写入后，减少 writer_count 计数器，并唤醒读者队列 readers_queue。

关闭设备：

当用户空间应用程序通过系统调用 close 关闭设备时，mymodule_release 函数被调用，输出一条内核日志。

模块卸载：

当模块卸载时，mymodule_exit 函数被调用，注销字符设备。通过使用互斥信号量 sem 和等待队列 readers_queue、writers_queue，该驱动模块确保了对共享资源的安全访问，避免了读者与写者之间的竞态条件。读者和写者之间的同步通过等待队列的机制实现，保证了在任何时刻只有一个读者或一个写者能够访问共享资源。

![截屏2023-12-04 20.05.43](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-12-04 20.05.43.png)

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-12-04 20.05.43.png)

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-12-04 20.05.43.png)

## 聊天软件



```c
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_USERS 5

int fd;
int buf;
int Main_spinlock = 1;
int selected_user = -1;
int readcount[MAX_USERS] = {0};
int writecount[MAX_USERS] = {0};
int privatecount[MAX_USERS] = {0};
pthread_mutex_t scanf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RW_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RR_mutex = PTHREAD_MUTEX_INITIALIZER;

int len(int *);
void show();
void correct(int *);
void writer(long);
void reader(long);
void *dowork(void *);

int main() {
  pthread_t users[MAX_USERS];
  for (long i = 0; i < MAX_USERS; ++i) {
    pthread_create(&users[i], NULL, dowork, (void *)i);
  }
  fd = open("/dev/ch_device", O_RDWR);
  while (1) {
    printf("please choose a user to execute 0-%d, -1 to exit\n", MAX_USERS - 1);
    pthread_mutex_lock(&scanf_mutex);
    int temp;
    scanf("%d%*c", &temp);
    pthread_mutex_unlock(&scanf_mutex);
    if (temp == -1) {
      break;
    } else {
      selected_user = temp;
    }
    Main_spinlock = 1;
    while (Main_spinlock)
      ;
  }
  pthread_mutex_destroy(&scanf_mutex);
  pthread_mutex_destroy(&RW_mutex);
  pthread_mutex_destroy(&RR_mutex);
  close(fd);
  return 0;
}

int len(int *arr) {
  int sum = 0;
  for (int i = 0; i < MAX_USERS; ++i) {
    sum += arr[i];
  }
  return sum;
}

void *dowork(void *arg) {
  long id = (long)arg;
  while (1) {
    while (id != selected_user)
      ;
    printf("User %ld: is working\n", id);
    printf("0 to write,1 to read\n");
    pthread_mutex_lock(&scanf_mutex);
    int temp;
    scanf("%d%*c", &temp);
    pthread_mutex_unlock(&scanf_mutex);
    if (temp) {
      reader(id);
    } else {
      writer(id);
    }
    selected_user = -1;
    Main_spinlock = 0;
  }
  return NULL;
}

void writer(long thread_id) {
  if (pthread_mutex_trylock(&RW_mutex)) {
    if (writecount[thread_id] == 0) {
      show();
      return;
    }
  }
  if (len(privatecount) != 0 &&
      (privatecount[thread_id] == 0 || privatecount[thread_id] == 2)) {
    show();
    pthread_mutex_unlock(&RW_mutex);
    return;
  }
  writecount[thread_id] = 1;
  pthread_mutex_lock(&scanf_mutex);
  printf("Writer Thread %ld: Please enter a number:\n ", thread_id);

  // private chatting
  char input[100] = {0};
  scanf("%s%*c", input);
  if (input[0] == '@') {
    privatecount[thread_id] = 1;
    int target = atoi(input + 1);
    privatecount[target] = 2;
    printf(
        "Writer Thread %ld writes to %d privately: Please enter a number:\n ",
        thread_id, target);
    scanf("%d%*c", &buf);
    write(fd, &buf, sizeof(buf));
    printf("0 to close,1 to continue\n");
    int temp = 0;
    scanf("%d%*c", &temp);
    if (temp) {
    } else {
      writecount[thread_id] = 0;
      pthread_mutex_unlock(&RW_mutex);
    }
    pthread_mutex_unlock(&scanf_mutex);
    return;
  } else {
    buf = atoi(input);
    write(fd, &buf, sizeof(buf));
    printf("0 to close,1 to continue\n");
    int temp = 0;
    scanf("%d%*c", &temp);
    if (temp) {
    } else {
      writecount[thread_id] = 0;
      pthread_mutex_unlock(&RW_mutex);
    }
    pthread_mutex_unlock(&scanf_mutex);
    return;
  }
}

void reader(long thread_id) {
  pthread_mutex_lock(&RR_mutex);
  int length = len(readcount);
  correct(&length);
  if (length == 0) {
    if (pthread_mutex_trylock(&RW_mutex) && writecount[thread_id] == 0) {
      show();
      pthread_mutex_unlock(&RR_mutex);
      return;
    }
  }
  if (len(privatecount) != 0 && privatecount[thread_id] == 0) {
    show();
    pthread_mutex_unlock(&RR_mutex);
    pthread_mutex_unlock(&RW_mutex);
    return;
  }
  readcount[thread_id] = 1;
  pthread_mutex_unlock(&RR_mutex);
  read(fd, &buf, sizeof(buf));
  printf("Reader Thread %ld: Read %d\n", thread_id, buf);
  printf("0 to close,1 to continue\n");
  pthread_mutex_lock(&scanf_mutex);
  int temp;
  scanf("%d%*c", &temp);
  pthread_mutex_unlock(&scanf_mutex);
  if (temp) {
  } else {
    if (len(privatecount) != 0 && privatecount[thread_id] == 2) {
      for (int i = 0; i < MAX_USERS; ++i) {
        privatecount[i] = 0;
      }
    }
    pthread_mutex_lock(&RR_mutex);
    readcount[thread_id] = 0;
    if (len(readcount) == 0) {
      pthread_mutex_unlock(&RW_mutex);
    }
    pthread_mutex_unlock(&RR_mutex);
  }
}

void show() {
  for (int i = 0; i < MAX_USERS; ++i) {
    if (writecount[i] || privatecount[i] || readcount[i]) {
      printf("%d ", i);
    }
  }
  printf("is occuping the device\n");
}

void correct(int *length) {
  for (int i = 0; i < MAX_USERS; ++i) {
    if (readcount[i] == 1 && writecount[i] == 1) {
      (*length)--;
    }
  }
}
```



### 代码解释



`int fd`: 文件描述符，用于打开/dev/ch_device`进行读写。

`int buf`: 缓冲区，用于存储用户输入的消息内容。

`int Main_spinlock`: 一个类似自旋锁的标志，用于控制用户的选择。

`int selected_user`: 用于记录当前被选择执行操作的用户。

`int readcount[MAX_USERS]`: 记录每个用户的读操作次数。

`int writecount[MAX_USERS]`: 记录每个用户的写操作次数。

`int privatecount[MAX_USERS]`: 记录每个用户进行私聊的状态。

**线程和用户交互：**

`pthread_t users[MAX_USERS]`: 用于存储用户线程的数组。

`pthread_create` 被用于创建多个用户线程，每个线程对应一个用户。

**`dowork`函数：**

每个用户线程在 `dowork` 函数中执行，模拟用户的操作。使用 `pthread_mutex_lock` 和 `pthread_mutex_unlock` 控制对共享资源的访问，例如 `selected_user`。用户可以选择执行读操作或写操作，根据输入 `0` 或 `1` 进行选择。在进行读或写操作之前，需要等待自己被选择（`while (id != selected_user)`）。

**`reader`函数:**

使用 `pthread_mutex_lock` 和 `pthread_mutex_unlock` 控制对 `RR_mutex` 的访问，确保读操作的互斥性。通过 `read` 从设备中读取消息，显示到屏幕上。用户可以选择关闭或继续操作，根据输入确定。

**`writer`函数：**

使用 `pthread_mutex_trylock` 控制对 `RW_mutex` 的访问，避免写者与其他写者或读者冲突。如果用户输入的消息以 `@` 开头，则表示进行私聊。私聊时需要设置相应的 `privatecount`，并向目标用户发送消息。用户可以选择关闭或继续操作，根据输入确定。

**其他辅助函数：**

`show`函数：显示当前正在使用设备的用户。

`correct`函数：用于修正读者数量，确保正确统计读者数量。



### 同步和互斥实现

**选择用户的同步（`Main_spinlock` 和 `selected_user`）：**

`Main_spinlock` 是一个标志，当其为1时，用户线程等待被选择。

用户线程通过 `while (Main_spinlock)` 自旋等待，直到 `Main_spinlock` 变为0。

用户选择执行操作后，设置 `selected_user` 为相应的用户ID，然后将 `Main_spinlock` 设为1，通知其他线程。

主程序循环中通过 `while (Main_spinlock)` 自旋等待用户选择。

**读者写者问题的实现：**

读者线程在执行读操作时，先获取 `RR_mutex` 互斥锁，以保证对 `readcount` 数组的访问是互斥的。

如果是第一个读者，同时没有写者在写，则获取 `RW_mutex` 锁，阻止写者进入。

读者完成读操作后，释放 `RR_mutex` 互斥锁。如果是最后一个读者，释放 `RW_mutex` 锁，允许写者进入。

写者线程在执行写操作时，通过 `RW_mutex` 锁保证写者互斥访问。

### 私聊的实现

**私聊的识别：**

当用户输入消息以 '@' 开头时，表示进行私聊。私聊目标用户的ID紧跟在 '@' 后面。

**私聊的状态记录（`privatecount`）：**

`privatecount` 数组用于记录每个用户的私聊状态。

`privatecount[i]` 等于1表示用户 `i` 正在私聊，等于2表示用户 `i` 正在被私聊。

**私聊操作：**

当进行私聊时，发送消息的用户将自己的 `privatecount` 设置为1，同时设置被私聊用户的 `privatecount` 为2。

向被私聊用户发送消息，并等待输入下一条私聊消息。私聊期间，其他用户的读写操作被暂停，直到私聊结束。

![截屏2023-12-04 19.48.53](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-12-04 19.48.53.png)
