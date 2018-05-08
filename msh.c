#include "shlib.h"

int fd = -1, i, j, n;
char buf[512], ans[512];

int loadCommand() {
  while (1) {
    if (i == n){
      n = read(fd, buf, sizeof(buf));
      i = 0;
    }
    if (n > 0) {
      while (i < n) {
        char temp = buf[i++];
        if (temp == '\n') {
          if (j >= 512) {
            return 0;
          }
          ans[j] = 0;
          if (j > 0) {
            j = 0;
            return 1;
          }
        }
        else {
          ans[j++] = temp;
          if (j >= 512) {
            return 0;
          }
        }
      }
    }
    else {
      if (j >= 512) {
        return 0;
      }
      if (j == 0) {
        return 0;
      }
      ans[j] = 0;
      if (j > 0) {
        j = 0;
        return 1;
      }
    }
  }
  return 1;
}

int main(int argc, char*argv[])
{
  if (argc != 2) {
    printf(1, "Usage: msh COMMAND_FILE\n");
    exit();
  }

  printf(1, "Reading commands in file %s...\n", argv[1]);

  if ((fd = open(argv[1], 0)) < 0) {
    printf(1, "Read file %s error!\n", argv[1]);
    exit();
  }
  
  j = i = n = 0;
  while (loadCommand())
  {
    printf(1, "Start run command %s!\n", ans);
    if(fork1() == 0)
      runcmd(parsecmd(ans));
    wait();
  }

  printf(1, "Already ran the commands in file %s!\n", argv[1]);

  exit();
}
