#include <stdio.h>
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
    // execlp("./system_call", "system_call", NULL);
    system("./system_call");
    printf("child process PID: %d\n", pid1);
  } else { /* parent process */
    pid1 = getpid();
    printf("parent process PID: %d\n", pid1);
    printf("child process1 PID: %d\n", pid);
  }
  return 0;
}