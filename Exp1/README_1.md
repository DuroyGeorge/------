# openEuler 系统环境试验

## 1.0  搭建openEuler操作系统实验环境

按照实验指导书的说明，完成华为云服务器的配置。依据个人习惯，采用vscode+remote-ssh操作云服务器。

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-19 20.17.54.png)

### 过程中遇到了一些问题并成功解决，如下：

**FTP协议不能工作，vscode不能加载华为云的文件系统**

一、ESC的网络安全组配置默认未开通一些必要的端口号，设置开通端口号

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/image-20231019203308825.png)

二、设置sshd_config ,将

`AllowAgentForwarding yes `

`AllowTcpForwarding yes `

`GatewayPorts no`

的注释去掉。

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-19 20.34.48.png)

## 1.1进程相关编程实验

`#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  pid_t pid, pid1;
  char global;
  /* fork a child process */
  pid = fork();
  if (pid < 0) { /* error occurred */
    fprintf(stderr, "Fork Failed");
    return 1;
  } else if (pid == 0) { /* child process */
    pid1 = getpid();
    global = 'C';
    printf("child: pid = %d\n", pid);   /* A */
    printf("child: pid1 = %d\n", pid1); /* B */
    printf("global = %c\n", global);    /* C */
  } else {/* parent process */
    pid1 = getpid();
    global = 'P';
    printf("parent: pid = %d\n", pid);   /* C */
    printf("parent: pid1 = %d\n", pid1); /* D */
    printf("global = %c\n", global);     /* E */
    // wait(NULL);
  }
  global = 'G';
  printf("global = %c\n", global); /* F */
  return 0;
}`

多次运行的结果如下图，

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-19 20.42.37.png)

### 对程序进行分析：

**fork()：** 用于创建子进程，其对父进程的返回值为子进程的PID，对子进程的返回值为0.如果创建成功则返回的值一定不为负值，否则则返回负值。

**进程空间：** 通过`fork()`创建的子进程会完全的复制父进程的地址空间，通过操作系统的虚拟内存机制映射到不同的物理内存空间。这样虽然是同一个变量，但被映射到不同的物理内存段，所以值可以不同。程序中`global`在父子进程中可以被分别修改，而不互相影响。

父子进程之间的地址空间关系如下：

- 子进程的地址空间是父进程的副本，包括代码段、数据段和堆栈等。
- 子进程可以通过修改自己的地址空间来独立地运行，并且对地址空间的修改不会影响到父进程或其他子进程。

最后`return`前再一次输出`global`的值,父子进程都将值设置为`G`打印输出。父子进程中全局变量的*logical adress*是相同的，但是*physical address*是不同的。

**wait()：** 

1. 同步子进程和父进程：当父进程调用wait()函数时，它会暂停自己的执行，直到子进程终止。这样可以确保父进程在子进程完成后再继续执行，实现了父子进程的同步。
2. 收集子进程的退出状态：`wait()` 函数可以获取子进程的退出状态。当子进程终止时，它会向父进程发送一个终止信号，并将退出状态传递给父进程。父进程可以通过`wait()`函数获取这个退出状态，并根据不同的退出状态进行相应的处理。
3. 避免僵尸进程：如果父进程没有调用`wait()`函数来回收子进程的资源，那么子进程将成为僵尸进程。通过调用`wait()`函数，父进程及时回收子进程的资源，避免了僵尸进程的产生。

在本程序中，使用`wait()`会让父进程阻塞，等待子进程结束。可以判断，先打印出的`G`是由子进程执行的。去除wait()与保留wait()的差别在于能否保证最后的`G`的打印顺序固定。



**孤儿进程：**指在父进程退出或终止后，子进程继续运行而没有父进程的进程。当父进程终止时，操作系统将会将子进程的父进程设置为根进程。根进程的PID是1。根进程会负责回收孤儿进程的资源。这包括释放内存、关闭打开的文件描述符等。操作系统接管这些孤儿进程，并负责回收它们的资源，确保系统资源的正常释放。在父进程中调用`wait()`或`waitpid()`等待子进程的终止，能避免产生孤儿进程。

**并发进程的调度：**并发进程调度是操作系统中的一个重要任务，用于决定在多个就绪态进程之间如何分配CPU时间，以实现高效的并发执行。并发进程调度的目标是最大化系统的吞吐量、响应时间和公平性。

常见的并发进程调度算法有先来先服务、最短作业优先、优先级调度、时间片轮转、多级反馈队列、最短剩余时间优先等，每种算法都有其适用的场景和优缺点。具体使用哪种算法取决于系统的需求和性能目标。

在本程序中，多次执行的结果不尽相同，这是因为每一次程序运行的cpu调度情况不同。父子进程按照调度规则并发的执行，导致输出的顺序有所差异。

`#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  pid_t pid, pid1;
  /* fork a child process */
  pid = fork();
  int x = 0;
  if (pid < 0) { /* error occurred */
    fprintf(stderr, "Fork Failed");
    return 1;
  } else if (pid == 0) { /* child process */
    pid1 = getpid();
    execlp("./system_call", "system_call", NULL);
    system("./system_call");
    printf("child process PID: %d\n", pid1);
  } else { /* parent process */
    pid1 = getpid();
    printf("parent process PID: %d\n", pid1);
    printf("child process1 PID: %d\n", pid);
  }
  return 0;
}`

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 00.55.43.png)

​											system()

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 00.54.57.png)

​											exec()

**system与exec：** `system()`和`exec()`都是在程序中执行外部命令或程序的函数，但它们在功能和使用方式上有一些区别。

system():
- 它会创建一个子进程来执行指定的命令，并等待命令执行完毕后返回。所以PID会与原进程不同。
- 通过shell来解释和执行命令，可以在命令中使用shell的各种特性。
- 返回值是命令的退出状态码,可以通过该返回值来判断命令是否成功执行。

exec():
- 它会替换当前进程的映像（代码和数据）为指定的外部程序，并在该程序上下文中继续执行。并不创建新的进程，而是在当前进程的上下文中执行新的程序。因此，原有进程的PID和其他属性保持不变。
- 直接执行可执行文件，而不需要通过shell进行解释。因此，它比`system()`函数更高效。
- 执行成功时不会返回，因为当前进程已经被替换为新程序。如果`exec()`函数返回了，说明执行失败。

## 1.2  线程相关编程实验

`#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int sheared_var = 0;
void *thread_function(void *arg);
pthread_mutex_t signal = PTHREAD_MUTEX_INITIALIZER;
int main() {
  pthread_t thread1, thread2;
  int ad = 100;
  int su = -100;
  pthread_create(&thread1, NULL, thread_function, &ad);
  printf("thread1 create success\n");
  pthread_create(&thread2, NULL, thread_function, &su);
  printf("thread2 create success\n");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_mutex_destroy(&signal);
  printf("variable result: %d\n", sheared_var);
  return 0;
}
void *thread_function(void *arg) {
  int *p = (int *)arg;
  int i, tmp;
  for (i = 0; i < 100000; i++) {
    pthread_mutex_lock(&signal);
    sheared_var += *p;
    pthread_mutex_unlock(&signal);
  }
  return NULL;
}`

未引入信号量时多次运行结果如下图，

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 16.02.28.png)

引入信号量进行同步后，结果如下图，

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 16.01.33.png)

### 对程序进行分析：

**线程与进程：**线程和进程是操作系统中并发执行的基本单位，它们有以下区别：

1. 进程是程序的执行实例，是资源分配的基本单位，拥有独立的内存空间和系统资源。线程是进程的子任务，是CPU调度的基本单位，共享进程的内存和资源。
2. 每个进程都有独立的内存空间和系统资源，包括文件描述符、打开的文件、网络连接等。而线程共享所属进程的内存空间和资源，同一进程内的线程可以直接访问和共享数据。
3. 进程调度和切换开销较大，因为涉及到切换内存空间和上下文的操作。线程的调度和切换开销较小，因为它们共享相同的内存空间和资源，只需要切换线程的上下文。
4. 进程间通信需要使用特定的机制，如管道、消息队列、共享内存等，来实现进程间的数据交换和同步。线程之间可以直接共享内存和数据，因此线程间通信相对简单。
5. 由于进程拥有独立的内存空间，一个进程的异常不会影响其他进程的执行。而线程共享进程的内存空间，一个线程的异常可能导致整个进程崩溃。

**线程间的同步与互斥：** 在本程序中，多次运行结果不尽相同，因为两个线程对同一个数据进行读写操作没有进行同步互斥操作，出现了两个线程同时读写发生冲突的问题。具体的说是一个线程修改完变量还未写回内存，另一个线程又对内存中的变量进行了修改。在多线程编程中，为了确保线程之间的正确协作和共享资源的安全访问，需要使用同步和互斥机制。下面是一些常见的线程同步和互斥的实现方法：

1. 互斥锁：互斥锁是最常用的同步机制之一。每个互斥锁在任意时刻只能被一个线程持有，其他线程需要等待锁的释放才能访问共享资源。线程在进入临界区前先尝试获取互斥锁，成功获取后进入临界区执行操作，执行完后释放锁。
2. 信号量：信号量是一种计数器，用于保护共享资源的访问。它可以控制同时访问资源的线程数量。当计数器大于零时，线程可以继续访问资源；当计数器为零时，线程需要等待其他线程释放资源。

选择合适的同步和互斥机制取决于具体的应用场景和需求。需要根据并发访问的模式、性能要求和线程间的依赖关系来选择适当的方法。

`#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
void *thread_function(void *arg);
pthread_mutex_t signal = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread1, thread2;
int main() {
  pthread_create(&thread1, NULL, thread_function, NULL);
  printf("thread1 create success\n");
  pthread_create(&thread2, NULL, thread_function, NULL);
  printf("thread2 create success\n");
  pthread_join(thread1, NULL);
  printf("thread1 systemcall return\n");
  pthread_join(thread2, NULL);
  printf("thread2 systemcall return\n");
  return 0;
}
void *thread_function(void *arg) {
  pthread_t tid = syscall(SYS_gettid);
  if (pthread_self() == thread1) {
    printf("thread1 tid = %lu ,pid = %d\n", tid, getpid());
  } else {
    printf("thread2 tid == %lu ,pid = %d\n", tid, getpid());
  }
  // execlp("./system_call", "system_call", NULL);
  system("./system_call");
  pthread_exit(NULL);
}`

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 16.55.08.png)

​										system()

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 16.56.33.png)

​										exec()

线程在调用`system()`后创建了新的进程来执行其他程序，所以PID会与原来进程的PID不同。线程1的TID是父进程PID+1，下一个新的线程2的TID是父进程PID+2。观察到此规律后提出假设。PID又可以看成是进程的主线程的TID，子线程的TID就是由主线程的TID加上若干个1得到的。

调用`exec()`时则会将整个进程的上下文刷新给目标程序，所以PID未发生变化。又因为上下文已经改变，所以新程序不会再执行原来程序的余下代码。

## 1.3  自旋锁实验

`/**
 *spinlock.c
 *in xjtu
 *2023.8
 */
#include <pthread.h>
#include <stdio.h>
// 定义自旋锁结构体
typedef struct {
  int flag;
} spinlock_t;
// 初始化自旋锁
void spinlock_init(spinlock_t *lock) { lock->flag = 0; }
// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
  while (__sync_lock_test_and_set(&lock->flag, 1)) {
    // 自旋等待
  }
}
// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) { __sync_lock_release(&lock->flag); }
// 共享变量
int shared_value = 0;
// 线程函数
void *thread_function(void *arg) {
  spinlock_t *lock = (spinlock_t *)arg;
  for (int i = 0; i < 5000; ++i) {
    spinlock_lock(lock);
    shared_value++;
    spinlock_unlock(lock);
  }
  return NULL;
}
int main() {
  pthread_t thread1, thread2;
  spinlock_t lock;
  // 输出共享变量的值
  printf("shared_value = %d\n", shared_value);
  // 初始化自旋锁
  spinlock_init(&lock);
  // 创建两个线程
  if (pthread_create(&thread1, NULL, thread_function, &lock) != 0) {
    fprintf(stderr, "create thread1 failed\n");
    return 0;
  }
  printf("thread1 create success\n");
  if (pthread_create(&thread2, NULL, thread_function, &lock) != 0) {
    fprintf(stderr, "create thread2 failed\n");
    return 0;
  }
  printf("thread2 create success\n");
  // 等待线程结束
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  // 输出共享变量的值
  printf("shared_value = %d\n", shared_value);
  return 0;
}`

运行结果如下图，

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-10-20 16.46.32.png)

### 对程序进行分析：

`// 输出共享变量的值`,简单的 `printf()`就可以。

`// 初始化自旋锁`，调用已写好的的初始化函数即可。

`// 创建两个线程`，调用`pthread_create()`,放个`if`用来处理创建线程失败的情况。

`// 等待线程结束`，调用`pthread_join()`。

`// 输出共享变量的值`,简单的 `printf()`就可以。

程序按照线程互斥锁同步运行，最终得到设想的结果。
