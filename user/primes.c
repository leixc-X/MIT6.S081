// a concurrent version of prime sieve using pipes

#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1
#define stdin 0
#define stdout 1
#define stderr 2
#define PRIMESNUMS 35

void sievePrime(int *left) { // read from left[READ], write from right[WRITE]
  close(left[WRITE]);

  int prime, temp, pid, right[2];

  if (read(left[READ], &prime, sizeof(int)) == 0) {
    close(left[READ]);
    exit(0);
  }

  pipe(right);
  printf("prime: %d\n", prime);

  if ((pid = fork()) < 0) {
    fprintf(stderr, "primes: fork() failed\n");
    close(left[READ]);
    close(right[READ]);
    close(right[WRITE]);

    exit(1);
  } else if (pid > 0) { // parent process
    close(right[READ]);
    while (read(left[READ], &temp, sizeof(int))) {
      if (temp % prime == 0)
        continue;
      write(right[WRITE], &temp, sizeof(int));
    }
    close(right[WRITE]);
    wait(0);
    exit(0);
  } else { // child process
    sievePrime(right);
    exit(0);
  }
}

int main(int argc, char **argv) {
  int pid, pipfd[2];
  pipe(pipfd);

  if ((pid = fork()) < 0) {
    fprintf(stderr, "primes: fork() failed\n");
    exit(1);
  } else if (pid > 0) { // parent process
    close(pipfd[READ]);
    for (int i = 2; i <= PRIMESNUMS; ++i) {
      write(pipfd[WRITE], &i, sizeof(int));
    }
    close(pipfd[WRITE]);
    wait(0);
    exit(0);

  } else { // child process
    sievePrime(pipfd);
    exit(0);
  }
}
