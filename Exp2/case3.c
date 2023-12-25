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