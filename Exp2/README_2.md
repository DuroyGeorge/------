# 实验二 进程通信与内存管理



## **2.1** 进程的软中断通信

### man命令

![截屏2023-11-08 10.24.11](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.24.11.png)

![截屏2023-11-08 10.24.41](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.24.41.png)

![截屏2023-11-08 10.27.26](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.27.26.png)

![截屏2023-11-08 10.26.21](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.26.21.png)

![截屏2023-11-08 10.27.07](/Users/duroy/Library/Application Support/typora-user-images/截屏2023-11-08 10.27.07.png)

### 编写实验代码需要考虑的问题：

**(1) 父进程向子进程发送信号时，如何确保子进程已经准备好接收信号？**

1. 子进程应当在启动后立即设置好信号处理函数，并确保已经阻塞或等待信号。一种通用的方法是使用`sigprocmask` 阻塞信号。这样只有在子进程准备好了之后才会开始处理父进程发送的信号，而不会立即处理信号。
2. 在子进程的初始化代码中，可以使用某种同步机制来确保子进程已准备好接收信号。这可以是一个标志变量，当子进程准备好后设置为true。父进程可以等待这个标志变量变为true，然后再发送信号。

**(2)如何阻塞住子进程，让子进程等待父进程发来信号？**

使用`sigwait`来等待特定的信号，阻塞挂起子进程。

### 程序

![image-20231108102937787](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/image-20231108102937787.png)

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
pid_t pid1 = -1, pid2 = -1;
void inter_handler() {
  // TODO
  kill(pid1, SIGSTKFLT);
  kill(pid2, SIGCHLD);
}
void waiting() {
  // TODO
  signal(SIGINT, inter_handler);
  signal(SIGQUIT, inter_handler);
  signal(SIGALRM, inter_handler);
  alarm(5);
  pause();
}
int main() {
  // TODO: 五秒之后或接收到两个信号
  while (pid1 == -1)
    pid1 = fork();
  if (pid1 > 0) {
    while (pid2 == -1)
      pid2 = fork();
    if (pid2 > 0) {
      // TODO: 父进程
      waiting();
      waitpid(pid1, NULL, 0);
      waitpid(pid2, NULL, 0);
      printf("\nParent process is killed!!\n");
    } else {
      // TODO: 子进程2
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      sigaddset(&mask, SIGQUIT);
      sigprocmask(SIG_BLOCK, &mask, NULL);
      sigset_t sub_mask;
      sigemptyset(&sub_mask);
      sigaddset(&sub_mask, SIGCHLD);
      sigprocmask(SIG_BLOCK, &sub_mask, NULL);
      int temp;
      sigwait(&sub_mask, &temp);
      if (temp == SIGCHLD)
        printf("\nChild process2 is killed by parent!!\n");
      sigprocmask(SIG_UNBLOCK, &mask, NULL);
      return 0;
    }
  } else {
    // TODO：子进程 1
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    sigset_t sub_mask;
    sigemptyset(&sub_mask);
    sigaddset(&sub_mask, SIGSTKFLT);
    sigprocmask(SIG_BLOCK, &sub_mask, NULL);
    int temp;
    sigwait(&sub_mask, &temp);
    if (temp == SIGSTKFLT)
      printf("\nChild process1 is killed by parent!!\n");
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return 0;
  }
  return 0;
}
```

1. 包含了必要的头文件，其中包括 `<signal.h>` 用于处理信号，`<stdio.h>` 用于标准输入输出，`<stdlib.h>` 用于标准库函数，`<sys/types.h>` 和 `<sys/wait.h>` 用于处理进程等，`<unistd.h>` 用于POSIX API函数。

2. 定义了一些全局变量，`pid1` 和 `pid2` 是两个进程ID，初始化为-1。

3. `inter_handler` 函数是一个信号处理函数，当它被调用时，它会发送两个不同的信号给 `pid1` 和 `pid2` 进程。

4. `waiting` 函数用于设置信号处理函数，然后使用 `alarm` 函数设置一个5秒的定时器，最后调用 `pause` 函数来挂起进程，等待信号的到来。

5. `main` ：
   - 在主循环中，创建了两个子进程 `pid1` 和 `pid2`。
   - 父进程中，调用 `waiting` 函数等待信号的到来，然后使用 `waitpid` 函数等待两个子进程的退出。
   - 子进程2中，使用 `sigset_t` 和 `sigprocmask` 设置信号掩码，只允许 `SIGCHLD` 信号通过，然后使用 `sigwait` 函数等待 `SIGCHLD` 信号，当收到信号后输出消息并退出。
   - 子进程1中，使用 `sigset_t` 和 `sigprocmask` 设置信号掩码，只允许 `SIGSTKFLT` 信号通过，然后使用 `sigwait` 函数等待 `SIGSTKFLT` 信号，当收到信号后输出消息并退出。

### 最初猜想

不论是按下中断键还是等待定时器触发，最终的结果都将是父进程等待两个子进程退出后输出 "Parent process is killed!!"，而子进程1和子进程2将分别输出 "Child process1 is killed by parent!!" 和 "Child process2 is killed by parent!!"。

### 实际结果

**delete**

![截屏2023-11-08 10.32.51](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.32.51.png)

**quit**

![截屏2023-11-08 10.33.21](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.33.21.png)

**alarm**

![](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 10.32.16.png)

1. 主程序开始执行，首先声明了全局变量 `pid1` 和 `pid2`，并初始化 `pid1` 和 `pid2` 为-1。

2. 进入 `main` 函数，进入主循环，该循循环不断尝试创建两个子进程，`pid1` 和 `pid2`。

3. 当父进程成功创建 `pid1` 后，它进入第一个内部循环，尝试创建 `pid2`。

4. 当父进程成功创建 `pid2` 后，它继续执行，在这一阶段程序分成了三个进程：父进程、`pid1` 子进程和 `pid2` 子进程。

5. 父进程调用 `waiting` 函数，这个函数设置了信号处理函数并启动了一个5秒的定时器，然后调用 `pause` 挂起自己，等待信号的到来。父进程的任务似乎是等待子进程的退出并在之后输出一条消息。

6. `pid1` 子进程和 `pid2` 子进程分别进入它们的代码块。这些代码块使用 `sigset_t` 和 `sigprocmask` 设置了信号掩码，然后调用 `sigwait` 函数等待信号的到来。

7. 在等待期间，父进程可能会被打断。这可以是由于按下 Ctrl+C 、Ctrl+\，或者等待5秒后，`alarm` 触发了 `SIGALRM` 信号。在任何一种情况下，`inter_handler` 函数会被调用，它向 `pid1` 和 `pid2` 子进程发送不同的信号。

8. 子进程1和子进程2分别接收到信号，然后输出相应的消息，并在消息输出后退出。

9. 父进程继续执行 `waiting` 函数中的 `pause`，等待两个子进程退出。

10. 当两个子进程都退出后，父进程输出 "Parent process is killed!!"，然后程序结束。

三种软中断的结果完全相同,以下是结果及其分析：

按下 Ctrl+\ 或 Ctrl+C ：

- 如果你在程序运行的过程中按下 Ctrl+\或 Ctrl+C ，那么父进程会接收到相应的信号，触发 `inter_handler` 函数。

- `inter_handler` 函数会向 `pid1` 和 `pid2` 发送 `SIGSTKFLT` 和 `SIGCHLD` 信号，导致两个子进程分别接收到不同的信号。
- 子进程1会输出 "Child process1 is killed by parent!!"，子进程2会输出 "Child process2 is killed by parent!!"。
- 然后父进程会继续执行 `main` 函数中的 `waitpid`，等待信号。
- 最终父进程等待两个子进程退出后输出 "Parent process is killed!!"。

不进行任何操作：

- 如果不进行任何操作，程序会在 `waiting` 函数中调用 `alarm` 设置了一个5秒的定时器，然后调用 `pause` 暂时挂起。
- 5秒后，`alarm` 定时器会触发，向父进程发送 `SIGALRM` 信号，然后父进程调用 `inter_handler` 函数。
- `inter_handler` 函数会向 `pid1` 和 `pid2` 发送 `SIGSTKFLT` 和 `SIGCHLD` 信号，导致两个子进程分别接收到不同的信号。
- 子进程1会输出 "Child process1 is killed by parent!!"，子进程2会输出 "Child process2 is killed by parent!!"。
- 然后父进程会继续执行 `main` 函数中的 `waitpid`，等待信号。
- 最终父进程等待两个子进程退出后输出 "Parent process is killed!!"。

不论是按下 Ctrl+\、Ctrl+C，还是等待定时器触发，最终都会触发 `inter_handler` 函数，导致两个子进程被杀死，然后父进程会等待两个子进程退出后才退出。

### kill命令

在这段程序中，`kill` 命令被使用了两次，分别在 `inter_handler` 函数中的两处，作用如下：

第一次用于向 `pid1` 进程发送 `SIGSTKFLT` 信号。`kill(pid1, SIGSTKFLT)` 这个命令的作用是向 `pid1` 子进程发送 `SIGSTKFLT` 信号，该信号的名称表明它与堆栈故障（stack fault）有关。发送此信号的目的是通知 `pid1` 子进程退出或执行一些特定的操作。

第二次用于向 `pid2` 进程发送 `SIGCHLD` 信号。`kill(pid2, SIGCHLD)` 这个命令的作用是向 `pid2` 子进程发送 `SIGCHLD` 信号，信号通常表示子进程的状态发生了变化（退出或停止）。发送此信号的目的是通知 `pid2` 子进程退出或执行一些特定的操作。

执行 `kill` 命令后的现象是，被指定的进程（`pid1` 或 `pid2`）会接收到相应的信号，根据信号的类型执行相应的信号处理函数或默认操作。在程序中，这两个 `kill` 命令是在 `inter_handler` 函数中调用的，因此它们的目的是触发相应的信号处理逻辑，导致子进程执行特定的操作（输出一条消息）并退出。最终，父进程会等待两个子进程退出，并输出 "Parent process is killed!!"。



进程可以通过两种主动退出的方式来终止自身，即通过调用 `exit()` 系统调用或发送信号给自身。这两种方式各有用途，取决于情境和需求，没有一种方式一定比另一种更好，而是要根据具体情况来决定。

进程可以调用 `exit()` 函数来正常退出。这个函数会执行一些清理工作，如关闭文件描述符、释放内存等，并向操作系统报告进程的退出状态。通常，进程的退出状态为0表示正常退出，非零值可能表示错误或其他情况。使用 `exit()` 是一种清晰、规范的退出方式，适合在正常情况下终止进程。

进程也可以发送特定的信号给自身，例如，使用 `kill(getpid(), SIGTERM)` 可以发送 `SIGTERM` 信号给自身，从而终止进程。这种方式可以用于异常情况下的退出，例如在处理某些错误时，或者根据特定条件终止进程。然而，这种方式可能需要进一步的信号处理逻辑，以确保资源被正确释放，不会产生资源泄漏或不一致的状态。

如果进程需要执行一些清理工作，例如关闭文件、释放内存等，或者需要返回一个状态码以指示退出状态，那么使用 `exit()` 是更好的选择。如果进程需要在某些特殊情况下退出，例如处理错误或外部条件变化，那么发送信号给自身可能更合适，但需要确保信号处理逻辑正确处理资源和状态。

## **2.2** 进程的管道通信

### 程序

**有锁**

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int pid1, pid2; // 定义两个进程变量
int main() {
  int fd[2];
  char InPipe[4001]; // 定义读缓冲区
  char c1 = '1', c2 = '2';
  while (pipe(fd))
    ;
  while ((pid1 = fork()) == -1)
    ;                        // 如果进程 1 创建不成功,则空循环
  if (pid1 == 0) {           // 如果子进程 1 创建成功,pid1 为进程号
    lockf(fd[1], F_LOCK, 0); // 锁定管道
    for (int i = 0; i < 2000; i++) {
      write(fd[1], &c1, 1);   // 向管道写入字符’1’
    };                        // 分 2000 次每次向管道写入字符’1’
    sleep(5);                 // 等待读进程读出数据
    lockf(fd[1], F_ULOCK, 0); // 解除管道的锁定
    exit(0);                  // 结束进程 1
  } else {
    while ((pid2 = fork()) == -1)
      ; // 若进程 2 创建不成功,则空循环
    if (pid2 == 0) {
      lockf(fd[1], 1, 0);
      for (int i = 0; i < 2000; i++) {
        write(fd[1], &c2, 1);
      }; // 分 2000 次每次向管道写入字符’2’
      sleep(5);
      lockf(fd[1], 0, 0);
      exit(0);
    } else {
      waitpid(pid1, NULL, 0);    // 等待子进程 1 结束
      waitpid(pid2, NULL, 0);    // 等待子进程 2 结束
      read(fd[0], InPipe, 4000); // 从管道中读出 4000 个字符
      InPipe[4000] = 0;          // 加字符串结束符
      printf("%s\n", InPipe);    // 显示读出的数据
      exit(0);                   // 父进程结束
    }
  }
}
```

![截屏2023-11-08 14.52.18](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-08 14.52.18.png)

**无锁**

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int pid1, pid2; // 定义两个进程变量
int main() {
  int fd[2];
  char InPipe[4001]; // 定义读缓冲区
  char c1 = '1', c2 = '2';
  while (pipe(fd))
    ;
  while ((pid1 = fork()) == -1)
    ;                        // 如果进程 1 创建不成功,则空循环
  if (pid1 == 0) {           // 如果子进程 1 创建成功,pid1 为进程号
    // lockf(fd[1], F_LOCK, 0); // 锁定管道
    for (int i = 0; i < 2000; i++) {
      write(fd[1], &c1, 1);   // 向管道写入字符’1’
    };                        // 分 2000 次每次向管道写入字符’1’
    sleep(5);                 // 等待读进程读出数据
    // lockf(fd[1], F_ULOCK, 0); // 解除管道的锁定
    exit(0);                  // 结束进程 1
  } else {
    while ((pid2 = fork()) == -1)
      ; // 若进程 2 创建不成功,则空循环
    if (pid2 == 0) {
      lockf(fd[1], 1, 0);
      for (int i = 0; i < 2000; i++) {
        write(fd[1], &c2, 1);
      }; // 分 2000 次每次向管道写入字符’2’
      sleep(5);
      lockf(fd[1], 0, 0);
      exit(0);
    } else {
      waitpid(pid1, NULL, 0);    // 等待子进程 1 结束
      waitpid(pid2, NULL, 0);    // 等待子进程 2 结束
      read(fd[0], InPipe, 4000); // 从管道中读出 4000 个字符
      InPipe[4000] = 0;          // 加字符串结束符
      printf("%s\n", InPipe);    // 显示读出的数据
      exit(0);                   // 父进程结束
    }
  }
}
```

![截屏2023-11-08 14.53.02](/Users/duroy/Library/Application Support/typora-user-images/截屏2023-11-08 14.53.02.png)



### 最初猜想

有锁：

- 子进程1先获得锁，向`pipe`里写入`c2`。子进程2尝试获得锁，失败后被挂起等待锁被释放。子进程1释放锁后子进程2获得锁，向`pipe`里写入`c2`。然后两个子进程均结束，父进程去读`pipe`，最后`printf`。

无锁：

- 子进程1和子进程2随机向`pipe`写入，最后父进程`printf`是`1`和`2`混杂在一起的。

### 实际结果

实际结果与最初猜想一致。原因分析如下：

这与使用了 `lockf` 有关。`lockf` 函数用于锁定管道的写入端，以确保一个进程能够独占地向管道写入数据，而其他进程在锁未解除之前会被阻塞。

在代码中，子进程1首先创建并运行，它使用 `lockf` 锁定了管道的写入端，然后写入了2000个字符 `1`。然后子进程2创建并运行，但由于管道的写入端被子进程1锁定，子进程2无法继续写入数据，因此它在 `lockf` 处被阻塞，等待锁解除。一旦子进程1解锁了管道的写入端，子进程2才能继续执行，然后它写入了2000个字符 `2`。

这种锁定机制导致了子进程1和子进程2的写入操作是串行的，而不是并行的。因此，它们分别写入了2000个字符 `1`和 `2`，而不是交替写入。这是使用 `lockf` 实现的互斥锁效果。

而无锁的话，则不会阻塞另一个进程，两个进程同时会向`pipe`里写入。

### 同步与互斥

1. 子进程1使用 `lockf` 函数锁定了管道的写入端，以确保它独占地向管道写入字符 '1'，这是互斥的一部分。

2. 子进程2也使用 `lockf` 函数来锁定管道的写入端，以确保它独占地向管道写入字符 '2'，同样也是互斥的一部分。

3. 父进程等待子进程1和子进程2的结束，以确保数据写入完成后再读取管道中的数据，这是同步的一部分。

如果不进行同步与互斥控制，可能会导致以下后果：

1. 多个进程同时向管道写入数据，导致数据混乱，难以区分哪个进程写入了什么。
2. 没有互斥控制的情况下，多个进程同时访问管道，可能引发竞争条件，导致数据错误或不一致。
3. 如果没有适当的同步控制，读取数据的进程可能会在数据写入之前尝试读取，导致数据丢失或错误的读取。

## **2.3** 内存的分配与回收

### 程序

```c
#include <stdio.h>
#include <stdlib.h>

#define PROCESS_NAME_LEN 32   /*进程名长度*/
#define MIN_SLICE 10          /*最小碎片的大小*/
#define DEFAULT_MEM_SIZE 1024 /*内存大小*/
#define DEFAULT_MEM_START 0   /*起始位置*/
/* 内存分配算法 */
#define MA_FF 1
#define MA_BF 2
#define MA_WF 3

/*描述每一个空闲块的数据结构*/
struct free_block_type {
  int size;
  int start_addr;
  struct free_block_type *next;
};

/*指向内存中空闲块链表的首指针*/
struct free_block_type *free_block;

/*每个进程分配到的内存块的描述*/
struct allocated_block {
  int pid;
  int size;
  int start_addr;
  char process_name[PROCESS_NAME_LEN];
  struct allocated_block *next;
};

/*进程分配内存块链表的首指针*/
struct allocated_block *allocated_block_head = NULL;
int mem_size = DEFAULT_MEM_SIZE; /*内存大小*/
int ma_algorithm = MA_FF;        /*当前分配算法*/
static int pid = 0;              /*初始pid*/
int flag = 0;                    /*设置内存大小标志*/

/*初始化空闲块，默认为一块，可以指定大小及起始地址*/
struct free_block_type *init_free_block(int mem_size) {
  struct free_block_type *fb;
  fb = (struct free_block_type *)malloc(sizeof(struct free_block_type));
  if (fb == NULL) {
    printf("No mem\n");
    return NULL;
  }
  fb->size = mem_size;
  fb->start_addr = DEFAULT_MEM_START;
  fb->next = NULL;
  return fb;
}

void display_menu(); /*显示菜单*/

int display_mem_usage(); /*显示内存使用*/

int set_mem_size(); /*设置内存的大小*/

void set_algorithm(); /*设置当前的分配算法*/

int new_process(); /*创建新的进程，主要是获取内存的申请数量*/

void rearrange(int algorithm); /*按指定的算法整理内存空闲块链表*/

void rearrange_FF(); /*按 FF 算法重新整理内存空闲块链表*/

void rearrange_BF(); /*按 BF 算法重新整理内存空闲块链表*/

void rearrange_WF(); /*按 WF 算法重新整理内存空闲块链表*/

int allocate_mem(struct allocated_block *ab); /*分配内存模块*/

void kill_process(); /*删除进程，归还分配的存储空间，并删除描述该进程内存分配的节点*/

struct allocated_block *find_process(); /*查找指定pid的进程*/

void do_exit(); /*释放链表并退出*/

int free_mem(struct allocated_block *ab); /*释放内存模块*/

int dispose(struct allocated_block *free_ab); /*释放ab数据结构节点*/

int len(struct free_block_type *fbt); /*统计空闲表长*/

void quicksort(struct free_block_type **fbt_array, int start, int end,
               int algorithm); /*辅助数据进行各自算法的排序*/

int sumOffree_block();
void retrench();

int main() {
  char choice;
  pid = 0;
  free_block = init_free_block(mem_size); //初始化空闲区
  while (1) {
    display_menu(); //显示菜单
    fflush(stdin);
    choice = getchar(); //获取用户输入
    switch (choice) {
    case '1':
      set_mem_size();
      break; //设置内存大小
    case '2':
      set_algorithm();
      flag = 1;
      break; //设置算法
    case '3':
      new_process();
      flag = 1;
      break; //创建新进程
    case '4':
      kill_process();
      flag = 1;
      break; //删除进程
    case '5':
      display_mem_usage();
      flag = 1;
      break; //显示内存使用
    case '0':
      do_exit();
      exit(0);
    // 释放链表并退出
    default:
      break;
    }
    getchar();
  }
}

/*显示菜单*/
void display_menu() {
  printf("\n");
  printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
  printf("2 - Select memory allocation algorithm\n");
  printf("3 - New process \n");
  printf("4 - Terminate a process \n");
  printf("5 - Display memory usage \n");
  printf("0 - Exit\n");
}

/* 显示当前内存的使用情况，包括空闲区的情况和已经分配的情况 */
int display_mem_usage() {
  struct free_block_type *fbt = free_block;
  struct allocated_block *ab = allocated_block_head;
  if (fbt == NULL)
    return (-1);
  printf("----------------------------------------------------------\n");
  /* 显示空闲区 */
  printf("Free Memory:\n");
  printf("%20s %20s\n", " start_addr", " size");
  while (fbt != NULL) {
    printf("%20d %20d\n", fbt->start_addr, fbt->size);
    fbt = fbt->next;
  }
  /* 显示已分配区 */
  printf("\nUsed Memory:\n");
  printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
  while (ab != NULL) {
    printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name, ab->start_addr,
           ab->size);
    ab = ab->next;
  }
  printf("----------------------------------------------------------\n");
  return 0;
}

/*设置内存的大小*/
int set_mem_size() {
  int size;
  if (flag != 0) { //防止重复设置
    printf("Cannot set memory size again\n");
    return 0;
  }
  printf("Total memory size =");
  scanf("%d", &size);
  if (size > 0) {
    mem_size = size;
    free_block->size = mem_size;
  }
  flag = 1;
  return 1;
}

/* 设置当前的分配算法 */
void set_algorithm() {
  int algorithm;
  printf("\t1 - First Fit\n");
  printf("\t2 - Best Fit \n");
  printf("\t3 - Worst Fit \n");
  scanf("%d", &algorithm);
  if (algorithm >= 1 && algorithm <= 3)
    ma_algorithm = algorithm;
  //按指定算法重新排列空闲区链表
  rearrange(ma_algorithm);
}

/*按指定的算法整理内存空闲块链表*/
void rearrange(int algorithm) {
  switch (algorithm) {
  case MA_FF:
    rearrange_FF();
    break;
  case MA_BF:
    rearrange_BF();
    break;
  case MA_WF:
    rearrange_WF();
    break;
  }
}

/*按 FF 算法重新整理内存空闲块链表*/
void rearrange_FF() {
  int length = len(free_block);
  if (length == 0)
    return;
  struct free_block_type *fbt_array[length];
  struct free_block_type *fbt = free_block;
  for (int i = 0; i < length; i++) {
    fbt_array[i] = fbt;
    fbt = fbt->next;
  }
  quicksort(fbt_array, 0, length - 1, MA_FF);
  for (int i = 0; i < length - 1; i++) {
    fbt_array[i]->next = fbt_array[i + 1];
  }
  fbt_array[length - 1]->next = NULL;
  free_block = fbt_array[0];
}

/*按 BF 算法重新整理内存空闲块链表*/
void rearrange_BF() {
  int length = len(free_block);
  if (length == 0)
    return;
  struct free_block_type *fbt_array[length];
  struct free_block_type *fbt = free_block;
  for (int i = 0; i < length; i++) {
    fbt_array[i] = fbt;
    fbt = fbt->next;
  }
  quicksort(fbt_array, 0, length - 1, MA_BF);
  for (int i = 0; i < length - 1; i++) {
    fbt_array[i]->next = fbt_array[i + 1];
  }
  fbt_array[length - 1]->next = NULL;
  free_block = fbt_array[0];
}

/*按 WF 算法重新整理内存空闲块链表*/
void rearrange_WF() {
  int length = len(free_block);
  if (length == 0)
    return;
  struct free_block_type *fbt_array[length];
  struct free_block_type *fbt = free_block;
  for (int i = 0; i < length; i++) {
    fbt_array[i] = fbt;
    fbt = fbt->next;
  }
  quicksort(fbt_array, 0, length - 1, MA_WF);
  for (int i = 0; i < length - 1; i++) {
    fbt_array[i]->next = fbt_array[i + 1];
  }
  fbt_array[length - 1]->next = NULL;
  free_block = fbt_array[0];
}

/*创建新的进程，主要是获取内存的申请数量*/
int new_process() {
  struct allocated_block *ab;
  int size;
  int ret;
  ab = (struct allocated_block *)malloc(sizeof(struct allocated_block));
  if (!ab)
    exit(-5);
  ab->next = NULL;
  pid++;
  sprintf(ab->process_name, "PROCESS-%02d", pid);
  ab->pid = pid;
  printf("Memory for %s:", ab->process_name);
  scanf("%d", &size);
  if (size > 0)
    ab->size = size;
  ret = allocate_mem(ab); /* 从空闲区分配内存，ret==1表示分配ok*/
  /*如果此时allocated_block_head尚未赋值，则赋值*/
  if ((ret == 1) && (allocated_block_head == NULL)) {
    allocated_block_head = ab;
    return 1;
  }
  /*分配成功，将该已分配块的描述插入已分配链表*/
  else if (ret == 1) {
    ab->next = allocated_block_head;
    allocated_block_head = ab;
    return 2;
  } else if (ret == -1) { /*分配不成功*/
    printf("Allocation fail\n");
    free(ab);
    return -1;
  }
  return 3;
}

/*分配内存模块*/
int allocate_mem(struct allocated_block *ab) {
  struct free_block_type *fbt, *pre;
  int request_size = ab->size;
  fbt = pre = free_block;
  if (free_block == NULL)
    return (-1);
  if (fbt->size >= request_size + MIN_SLICE) {
    ab->start_addr = fbt->start_addr;
    ab->size = request_size;
    struct free_block_type *new_fbt =
        (struct free_block_type *)malloc(sizeof(struct free_block_type));
    new_fbt->start_addr = fbt->start_addr + request_size;
    new_fbt->size = fbt->size - request_size;
    free_block = new_fbt;
    new_fbt->next = fbt->next;
    free(fbt);
    rearrange(ma_algorithm);
    return 1;
  } else if (fbt->size >= request_size) {
    ab->start_addr = fbt->start_addr;
    ab->size = fbt->size;
    free_block = fbt->next;
    free(fbt);
    rearrange(ma_algorithm);
    return 1;
  }
  fbt = fbt->next;
  while (fbt != NULL) {
    //  1. 找到可满足空闲分区且分配后剩余空间足够大，则分割
    if (fbt->size >= request_size + MIN_SLICE) {
      ab->start_addr = fbt->start_addr;
      ab->size = request_size;
      struct free_block_type *new_fbt =
          (struct free_block_type *)malloc(sizeof(struct free_block_type));
      new_fbt->start_addr = fbt->start_addr + request_size;
      new_fbt->size = fbt->size - request_size;
      pre->next = new_fbt;
      new_fbt->next = fbt->next;
      free(fbt);
      rearrange(ma_algorithm);
      return 1;
      //  2. 找到可满足空闲分区且但分配后剩余空间比较小，则一起分配
    } else if (fbt->size >= request_size) {
      ab->start_addr = fbt->start_addr;
      ab->size = fbt->size;
      pre->next = fbt->next;
      free(fbt);
      rearrange(ma_algorithm);
      return 1;
    } else {
      pre = pre->next;
      fbt = fbt->next;
    }
  }
  if (sumOffree_block() >= request_size) {
    //  3.找不可满足需要的空闲分区但空闲分区之和能满足需要，则采用内存紧缩技术,进行空闲分区的合并，然后再分配
    retrench();
    return allocate_mem(ab);
  }
  //  4. 在成功分配内存后，应保持空闲分区按照相应算法有序
  //  5. 分配成功则返回 1，否则返回-1
  return -1;
}

/*删除进程，归还分配的存储空间，并删除描述该进程内存分配的节点*/
void kill_process() {
  struct allocated_block *ab;
  int pid;
  printf("Kill Process, pid=");
  scanf("%d", &pid);
  ab = find_process(pid);
  if (ab != NULL) {
    free_mem(ab); /*释放ab 所表示的分配区*/
    dispose(ab);  /*释放ab 数据结构节点*/
  }
}

struct allocated_block *find_process(int pid) {
  struct allocated_block *temp = allocated_block_head;
  while (temp != NULL) {
    if (temp->pid == pid)
      return temp;
    temp = temp->next;
  }
  return NULL;
}

/*将 ab 所表示的已分配区归还，并进行可能的合并*/
int free_mem(struct allocated_block *ab) {
  int algorithm = ma_algorithm;
  struct free_block_type *fbt, *pre, *work;
  fbt = (struct free_block_type *)malloc(sizeof(struct free_block_type));
  if (!fbt)
    return -1;
  fbt->start_addr = ab->start_addr;
  fbt->size = ab->size;
  fbt->next = NULL;
  // 1. 将新释放的结点插入到空闲分区队列末尾
  struct free_block_type *temp = free_block;
  if (temp == NULL) {
    free_block = fbt;
    return 1;
  }
  while (temp->next != NULL) {
    temp = temp->next;
  }
  temp->next = fbt;
  // 2. 对空闲链表按照地址有序排列
  rearrange(MA_FF);
  // 3. 检查并合并相邻的空闲分区
  pre = free_block;
  work = free_block->next;
  while (work != NULL) {
    if (pre->start_addr + pre->size == work->start_addr) {
      pre->size += work->size;
      pre->next = work->next;
      free(work);
      work = pre->next;
    } else {
      pre = pre->next;
      work = work->next;
    }
  }
  // 4. 将空闲链表重新按照当前算法排序
  rearrange(algorithm);
  return 1;
}
/*释放 ab数据结构节点*/
int dispose(struct allocated_block *free_ab) {
  struct allocated_block *pre, *ab;
  if (free_ab == allocated_block_head) { /*如果要释放第一个节点*/
    allocated_block_head = allocated_block_head->next;
    free(free_ab);
    return 1;
  }
  pre = allocated_block_head;
  ab = allocated_block_head->next;
  while (ab != free_ab) {
    pre = ab;
    ab = ab->next;
  }
  pre->next = ab->next;
  free(ab);
  return 2;
}

/*释放链表并退出*/
void do_exit() {
  struct allocated_block *ab = allocated_block_head;
  while (ab != NULL) {
    allocated_block_head = allocated_block_head->next;
    free(ab);
    ab = allocated_block_head;
  }
  struct free_block_type *fbt = free_block;
  while (fbt != NULL) {
    free_block = free_block->next;
    free(fbt);
    fbt = free_block;
  }
}

/*统计空闲表长*/
int len(struct free_block_type *fbt) {
  int len = 0;
  while (fbt != NULL) {
    len++;
    fbt = fbt->next;
  }
  return len;
}

/*辅助数据进行各自算法的排序*/
void quicksort(struct free_block_type **fbt_array, int start, int end,
               int algorithm) {
  if (start < 0 || start > end || end > len(free_block))
    return;
  int i = start, j = end;
  int pivot = i;
  switch (algorithm) {
  case MA_FF:
    while (i < j) {
      while (fbt_array[j]->start_addr >= fbt_array[pivot]->start_addr && i < j)
        j--;
      if (i < j) {
        struct free_block_type *temp = fbt_array[j];
        fbt_array[j] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = j;
      }
      while (fbt_array[i]->start_addr <= fbt_array[pivot]->start_addr && i < j)
        i++;
      if (i < j) {
        struct free_block_type *temp = fbt_array[i];
        fbt_array[i] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = i;
      }
    }
    break;
  case MA_BF:
    while (i < j) {
      while (fbt_array[j]->size >= fbt_array[pivot]->size && i < j)
        j--;
      if (i < j) {
        struct free_block_type *temp = fbt_array[j];
        fbt_array[j] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = j;
      }
      while (fbt_array[i]->size <= fbt_array[pivot]->size && i < j)
        i++;
      if (i < j) {
        struct free_block_type *temp = fbt_array[i];
        fbt_array[i] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = i;
      }
    }
    break;
  case MA_WF:
    while (i < j) {
      while (fbt_array[j]->size <= fbt_array[pivot]->size && i < j)
        j--;
      if (i < j) {
        struct free_block_type *temp = fbt_array[j];
        fbt_array[j] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = j;
      }
      while (fbt_array[i]->size >= fbt_array[pivot]->size && i < j)
        i++;
      if (i < j) {
        struct free_block_type *temp = fbt_array[i];
        fbt_array[i] = fbt_array[pivot];
        fbt_array[pivot] = temp;
        pivot = i;
      }
    }
    break;
  default:
    break;
  }
  quicksort(fbt_array, start, pivot - 1, algorithm);
  quicksort(fbt_array, pivot + 1, end, algorithm);
}

/*检查总内存够不够分配*/
int sumOffree_block() {
  int sum = 0;
  struct free_block_type *fbt = free_block;
  while (fbt != NULL) {
    sum += fbt->size;
    fbt = fbt->next;
  }
  return sum;
}

/*紧缩*/
void retrench() {
  struct allocated_block *ab = allocated_block_head;
  struct allocated_block *pre = allocated_block_head;
  ab->start_addr = 0;
  ab = ab->next;
  while (ab != NULL) {
    ab->start_addr = pre->start_addr + pre->size;
    pre = pre->next;
    ab = ab->next;
  }
  int sum = sumOffree_block();
  struct free_block_type *new_fbt =
      (struct free_block_type *)malloc(sizeof(struct free_block_type));
  new_fbt->size = sum;
  new_fbt->start_addr = pre->start_addr + pre->size;
  new_fbt->next = NULL;
  while (free_block != NULL) {
    struct free_block_type *fbt = free_block;
    free_block = free_block->next;
    free(fbt);
  }
  free_block = new_fbt;
}
```

### 比较

####  1. 首次适应算法：

- **算法思想：**
  - 分配第一个足够大的空闲块以容纳进程。
- **优点：**
  - 简单易实现。
  - 分配迅速，因为使用第一个合适的空闲块。
- **缺点：**
  - 可能导致碎片问题，导致内存利用效率低。
  - 不能始终找到最佳适应的空闲块，导致剩余的空闲块较大。
- **提高查找性能：**
  - 使用有序链表可以提高查找性能，减少寻找合适块所需的迭代次数。

#### 2. 最佳适应算法：

- **算法思想：**
  - 分配最小的足够大的空闲块以容纳进程。
- **优点：**
  - 倾向于最小化碎片，因为它分配所需的确切大小或接近它。
- **缺点：**
  - 相对于首次适应，实现较为复杂。
  - 由于需要寻找最佳适应块，分配可能稍慢。
- **提高查找性能：**
  - 与首次适应类似，维护有序链表可提高查找性能。

#### 3. 最坏适应算法：

- **算法思想：**
  - 分配最大的可用空闲块给进程。
- **优点：**
  - 可能减少碎片问题，因为它留下较小的空闲块较少。
- **缺点：**
  - 相对于首次适应，实现较为复杂。
  - 可能导致内存利用效率低，因为它留下较小的空闲块。
- **提高查找性能：**
  - 同样，维护有序链表可帮助提高查找最大可用块的性能。

### 排序

#### 1. 首次适应算法：

1. 遍历空闲块链表，将其按照起始地址从小到大排序。
2. 排序后，分配时直接从链表头开始查找第一个满足条件的空闲块。

#### 2. 最佳适应算法：

1. 遍历空闲块链表，将其按照大小从小到大排序。
2. 排序后，分配时直接从链表头开始查找第一个满足条件的空闲块

#### 3. 最坏适应算法：

1. 遍历空闲块链表，将其按照大小从大到小排序。
2. 排序后，分配时直接从链表头开始查找第一个满足条件的空闲块。

`quicksort`函数接受一个存储空闲块链表的数组 `fbt_array`，以及排序的起始和结束索引。在函数内部，根据不同的算法（MA_FF、MA_BF、MA_WF），使用快速排序对数组进行排序。实际的排序逻辑在`switch`语句中，根据算法选择不同的比较条件。

这个排序函数被三个不同的排序算法调用，分别是`rearrange_FF`、`rearrange_BF`和`rearrange_WF`。在这些函数中，先获取空闲块链表的数组表示，然后调用`quicksort`进行排序，最后重新构建有序的空闲块链表。

### 碎片

在内存管理中，内碎片和外碎片是两种不同类型的碎片，而紧缩功能主要是用来解决外碎片的问题。

1. **内碎片：**
   
   - **定义：** 内碎片是指已分配给进程的内存块中有一部分空间未被利用，但由于分配策略的原因，这部分空间无法被其他进程使用，造成浪费。
   - **实例：** 如果一个进程请求分配8个单位的内存，而系统只能按照10个单位来分配，那么就会有2个单位的内存空间被浪费，这部分未被使用的空间就是内碎片。
   
2. **外碎片：**
   
   - **定义：** 外碎片是指未分配的内存块中有一部分零散的小空间，虽然总和可能足够分配给进程，但无法满足某一特定进程的内存需求。

   - **实例：** 当多次分配和释放内存后，系统中可能会存在一些小块的未分配内存，但它们无法被单个进程利用，因为它们太小。这些零散的小块就构成了外碎片。
   
     ![截屏2023-11-09 18.27.57](https://duroy-picgo.obs.cn-north-4.myhuaweicloud.com/img/截屏2023-11-09 18.27.57.png)
   
3. **紧缩功能解决的问题：**
   
   - 紧缩功能主要是用来解决外碎片的问题。当系统中存在外碎片时，紧缩功能会尝试将内存块重新组织，将零散的小块合并成更大的连续块，以便满足大内存需求的进程。这样做可以减少外碎片，提高内存利用率。

紧缩功能的实现在`retrench`函数中，它会对已分配的内存块进行重新整理，合并相邻的空闲块，从而减少外碎片。

### 回收内存

**空闲块合并的详细过程：**

1. **初始化指针：**
   
   初始化两个指针 `pre` 和 `work`，分别指向空闲块链表的头部（`free_block`）和第二个空闲块。
   
   ```c
   pre = free_block;
   work = free_block->next;
   ```
   
2. **遍历空闲块链表：**
   
   使用循环遍历空闲块链表，检查相邻的空闲块是否可以合并。
   
3. **检查相邻的空闲块：**
   
   在循环中，检查当前空闲块和下一个空闲块是否相邻。
   
   ```c
   if (pre->start_addr + pre->size == work->start_addr) 
   ```
   
4. **合并空闲块：**
   
   如果发现相邻的空闲块，就将它们合并为一个更大的空闲块，更新 `pre` 的大小和指向 `work->next`。
   
   ```c
   pre->size += work->size;
   pre->next = work->next;
   ```
   
5. **释放被合并的空闲块：**
   
   释放被合并的空闲块节点，即释放 `work` 指向的节点。
   
   ```c
   free(work);
   ```
   
6. **更新指针：**
   
   更新 `work` 指针，使其指向下一个空闲块，继续检查并合并。
   
   ```c
   work = pre->next;
   ```
   
7. **循环结束：**
   
   当遍历完整个空闲块链表时，合并过程完成。

这个过程就是通过检查相邻的空闲块，如果它们可以合并，则将它们合并成一个更大的块。这样的合并操作有助于减少外碎片，提高内存的利用效率。
