#include <pthread.h>
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
    // pthread_mutex_lock(&signal);
    sheared_var += *p;
    // pthread_mutex_unlock(&signal);
  }
  return NULL;
}