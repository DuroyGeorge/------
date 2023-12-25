#include <stdio.h>
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
    printf("child: pid = %d\n", pid);            /* A */
    printf("child: pid1 = %d\n", pid1);          /* B */
    printf("global = %c %p\n", global, &global); /* C */
  } else {                                       /* parent process */
    pid1 = getpid();
    global = 'P';
    printf("parent: pid = %d\n", pid);            /* C */
    printf("parent: pid1 = %d\n", pid1);          /* D */
    printf("global = %c %p\n ", global, &global); /* E */
    wait(NULL);
  }
  global = 'G';
  printf("global = %c\n", global); /* F */
  return 0;
}