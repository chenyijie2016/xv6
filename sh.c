#include "shlib.h"

int
main(void)
{
  static char buf[100];
  int fd;

  if(fork1() == 0)
    runcmd(parsecmd("msh STARTCOMMAND"));
  wait();
  
  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    int len =strlen(buf);
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[len-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        printf(2, "cannot cd %s\n", buf+3);
      continue;
    }
    if(len >= 2 && buf[len-2] == '?'){
      buf[len-2] = 0;
      if(cmplt(buf) < 0)
        printf(2, "auto complete %s failed\n", buf);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait();
  }
  exit();
}
