#include <pthread.h>
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
}