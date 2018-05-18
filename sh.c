#include "shlib.h"

int
main(void)
{
  static char buf[100];
  static char nbuf[256];
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
    // clear space chars in the beginning of buf
    shparsedollar(nbuf, buf);
    int len =strlen(nbuf);
    nbuf[len++] = '\n';
    if(strprefix(nbuf, "cd ")){
      // Chdir must be called by the parent, not the child.
      nbuf[len-1] = 0;  // chop \n
      if(chdir(nbuf+3) < 0)
        printf(2, "cannot cd %s\n", nbuf+3);
      continue;
    }
    if (strprefix(nbuf, "if ") || strprefix(nbuf, "while ")) {
      printf(2, "\"if\" and \"while\" statements can only used in msh.\nPlease write it in files and run \"msh FILENAME\"\n");
      continue;
    }
    if(len >= 2 && nbuf[len-2] == '?'){
      nbuf[len-2] = 0;
      if(cmplt(nbuf) < 0)
        printf(2, "auto complete %s failed\n", nbuf);
      continue;
    }
    int run = 1;
    // check equal
    for (int j = 0; nbuf[j]; j++) {
      if (strchr(" \t\v\r\n", nbuf[j])) {
        break;
      }
      if (nbuf[j] == '=') {
        run = 0;
        nbuf[len - 1] = 0;
        nbuf[j] = 0;
        if (setenv(-1, nbuf, (char**)(int)(nbuf + j + 1), 1) > 0) {
          printf(2, "Set variable fail! Too many variables now!\n");
        }
      }
    }
    if (run) {
      if(fork1() == 0)
        runcmd(parsecmd(nbuf));
      wait();
    }
  }
  exit();
}
