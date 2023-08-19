// sleep.c: sleep for a given of time
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  // if no pass an argument, print error message
  if (argc < 2) {
    fprintf(2, "Usage: sleep time...\n");
    exit(1);
  }

  sleep(atoi(argv[1]));

  exit(0);
}
