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