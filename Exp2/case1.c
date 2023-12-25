#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int flag = 0;
pid_t pid1 = -1, pid2 = -1;
void inter_handler() {
  // TODO
  kill(pid1, SIGSTKFLT);
  kill(pid2, SIGCHLD);
  flag++;
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