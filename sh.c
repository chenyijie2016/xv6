#include "shlib.h"

void parsedollar(char* nbuf, char* buf) {
  char name[100];
  int quote = ' ', buflen = strlen(buf);
  int parsebufindex = strimAndTrip(buf) - buf;
  int nbufindex = 0;
  for (; parsebufindex < buflen; parsebufindex++) {
    switch (buf[parsebufindex]) {
      case '\'':
        if (quote == ' ') {
          quote = '\'';
          break;
        }
        else if (quote == '\'') {
          quote = ' ';
          break;
        }
      case '\"':
        if (quote == ' ') {
          quote = '\"';
          break;
        }
        else if (quote == '\"') {
          quote = ' ';
          break;
        }
      case '$':
      if (quote == '\"' || quote == ' ') {
        // Get the word;
        int nameindex = 0;
        for (parsebufindex++; ; parsebufindex++) {
          if (!buf[parsebufindex] || strchr(" \t\r\n\v\"$", buf[parsebufindex])) {
            name[nameindex++] = 0;
            parsebufindex --;
            break;
          }
          name[nameindex++] = buf[parsebufindex];
        }
        // Find in sysenv;
        struct env m;
        int sysenvnum = getenv(1, &m, name);
        if (sysenvnum == 0 && m.len > 0) {
          // Found!
          int variablesTotalLen = -1;
          for (uint i = 0; i < m.len; i++) {
            variablesTotalLen += (1 + strlen(m.text[i]));
          }
          if (nbufindex + variablesTotalLen < 256 - 2) {
            strcpy(nbuf + nbufindex, m.text[0]);
            nbufindex += strlen(m.text[0]);
            for (int j = 0; j < m.len; j++) {
              strcpy(nbuf + nbufindex, ":");
              nbufindex += 1;
              strcpy(nbuf + nbufindex, m.text[j]);
              nbufindex += strlen(m.text[j]);
            }
          }
        }
        break;
      }
      default:
        nbuf[nbufindex++] = buf[parsebufindex];
        if (nbufindex >= 256 - 2) {
          break;
        }
    }
  }
  nbuf[nbufindex++] = '\n';
  nbuf[nbufindex] = 0;
}

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
    parsedollar(nbuf, buf);
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
    if(fork1() == 0)
      runcmd(parsecmd(nbuf));
    wait();
  }
  exit();
}
