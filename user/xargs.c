// a simple version of the UNIX xargs program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define stdin 0
#define stdout 1
#define stderr 2

int main(int argc, char *argv[]) {
  // 保存命令行参数
  char *new_argv[MAXARG];

  if (argc < 2) {
    fprintf(stderr, "Usage: xargs <command>...\n");
    exit(1);
  }

  // 获取 xargs 后的参数, new_argv[0] 就是执行的命令
  for (int i = 1; i < argc; i++) {
    new_argv[i - 1] = argv[i];
  }

  int n, pid, buf_idx = 0;
  char buf, cur_buf[1024];

  // 读取标准输入的内容
  while ((n = read(stdin, &buf, 1)) > 0) {

    if (buf == '\n') {
      cur_buf[buf_idx] = 0; // 0作为字符串结尾的标志
      if ((pid = fork()) < 0) {
        fprintf(stderr, "xargs fork() fail");
        exit(1);
      } else if (pid == 0) { // child process
        new_argv[argc - 1] = cur_buf;
        new_argv[argc] = 0;
        exec(new_argv[0], new_argv);

      } else {
        wait(0);
        buf_idx = 0;
      }

    } else {
      cur_buf[buf_idx++] = buf;
    }
  }

  exit(0);
}
