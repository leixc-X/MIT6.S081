// a program "ping-pong" a byte between two processes over a pair of pipes

#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1
#define stderr 2

int main(int argc, char *argv[]) {
  // implement single pipe
  /*
  int p[2], pid;
  char readText[64];

  pipe(p);
  if ((pid = fork()) < 0) {
    fprintf(2, "fork error...\n");
    exit(1);
  } else if (pid == 0) { // child process, print pid and received "ping", write
                         // "pong" to the parent process by the pipe
    read(p[0], readText, sizeof(readText));
    printf("%d: received: %s\n", getpid(), readText);
    write(p[1], "pong", 4);
    exit(0);
  } else { // parent process, send "ping" and read "pong" from child process
    write(p[1], "ping", 4);
    wait(0); // 阻塞父进程，等待子进程
    read(p[0], readText, sizeof(readText));
    printf("%d: received: %s\n", getpid(), readText);
    exit(0);
  }
  */

  // implement by a pair of pipes
  int fdp2c[2], fdc2p[2], pid;
  char textp2c[64], textc2p[64];

  pipe(fdc2p);
  pipe(fdp2c);

  if ((pid = fork()) < 0) {
    fprintf(stderr, "fork error...\n");
    exit(1);
  } else if (pid == 0) { // child process, print pid and received "ping", write
                         // "pong" to the parent process by the pipe
    close(fdp2c[WRITE]);
    close(fdc2p[READ]);

    read(fdp2c[READ], textp2c, sizeof(textp2c));
    close(fdp2c[READ]);

    printf("%d: received: %s\n", getpid(), textp2c);

    write(fdc2p[WRITE], "pong", 4);
    close(fdc2p[WRITE]);

    exit(0);
  } else { // parent process, send "ping" and read "pong" from child process
    close(fdp2c[READ]);
    close(fdc2p[WRITE]);

    write(fdp2c[WRITE], "ping", 4);
    close(fdp2c[WRITE]);

    read(fdc2p[READ], textc2p, sizeof(textc2p));
    close(fdc2p[READ]);
    printf("%d: received: %s\n", getpid(), textc2p);

    exit(0);
  }
}
