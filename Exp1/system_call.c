#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  printf("system_call PID: %d\n", getpid());
  return 0;
}