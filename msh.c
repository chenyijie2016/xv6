#include "shlib.h"
#include "stack.h"

#define DEFALUT_LINE_SIZE         512

// 32 should be enough... for use
#define DEFALUT_VARIABLE_NUM      32
#define DEFALUT_VARIABLE_NAME     64
#define DEFALUT_VARIABLE_CONTENT  512

int fd = -1, i, j, n, parsedollarindex, mcmdindex;
char buf[512], ans[512], mcmd[2048], name[DEFALUT_VARIABLE_NAME];
int quoteSign = ' ';

struct variable {
  char name[DEFALUT_VARIABLE_NAME];
  char value[DEFALUT_VARIABLE_CONTENT];
};
struct variable vs[DEFALUT_VARIABLE_NUM];
int vlen = 0;
int vsize = DEFALUT_VARIABLE_NUM;

STATE getvariable() {
  int tempIndex;
  for (tempIndex = 0; tempIndex < vlen; tempIndex++) {
    if (strcmp(name, vs[tempIndex].name) == 0) {
      if (mcmdindex + strlen(vs[tempIndex].value) >= 2048 - 1) {
        return STATE_ERROR;
      }
      strcpy(mcmd + mcmdindex, vs[tempIndex].value);
      mcmdindex += strlen(vs[tempIndex].value);
      return STATE_OK;
    }
  }
  if (tempIndex == vlen) {
    // Find in sysenv;
    struct env m;
    int sysenvnum = getenv(1, &m, name);
    if (sysenvnum == 0 && m.len > 0) {
      // Found!
      int variablesTotalLen = -1;
      for (uint i = 0; i < m.len; i++) {
        variablesTotalLen += (1 + strlen(m.text[i]));
      }
      if (mcmdindex + variablesTotalLen >= 2048 - 1) {
        return STATE_ERROR;
      }
      strcpy(mcmd + mcmdindex, m.text[0]);
      mcmdindex += strlen(m.text[0]);
      for (int j = 0; j < m.len; j++) {
        strcpy(mcmd + mcmdindex, ":");
        mcmdindex += 1;
        strcpy(mcmd + mcmdindex, m.text[j]);
        mcmdindex += strlen(m.text[j]);
      }
    }
  }
  return STATE_OK;
}

STATE setVariables(char* name, char* value) {
  for (int i = 0; i < vlen; i++) {
    if (strcmp(name, vs[i].name) == 0) {
      if (strlen(value) >= DEFALUT_VARIABLE_CONTENT) {
        return STATE_OVERFLOW;
      }
      strcpy(vs[i].value, value);
      return STATE_OK;
    }
  }
  if (vlen == vsize || strlen(name) >= DEFALUT_VARIABLE_NAME) {
    return STATE_OVERFLOW;
  }
  strcpy(vs[vlen].name, name);
  strcpy(vs[vlen].value, value);
  vlen++;
  printf(1, "Set Var %s with %s!\n", name, value);
  return STATE_OK;
}

int loadCommand() {
  while (1) {
    if (i == n){
      n = read(fd, buf, sizeof(buf));
      i = 0;
    }
    if (n > 0) {
      while (i < n) {
        char temp = buf[i++];
        if (temp == '\'') {
          if (quoteSign == '\'') {
            quoteSign = ' ';
            // continue;
          }
          else if (quoteSign == ' ') {
            quoteSign = '\'';
            // continue;
          }
        }
        else if (temp == '\"') {
          if (quoteSign == '\"') {
            quoteSign = ' ';
            // continue;
          }
          else if (quoteSign == ' ') {
            quoteSign = '\"';
            // continue;
          }
        }
        if (temp == '\n') {
          if (j >= 512) {
            return 0;
          }
          ans[j] = 0;
          if (quoteSign == ' ') {
            if (j > 0) {
              j = 0;
              return 1;
            }
          }
        }
        ans[j++] = temp;
        if (j >= 512) {
          return 0;
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

STATE parsedollar() {
  int quote = ' ', anslen = strlen(ans);
  parsedollarindex = 0;
  mcmdindex = 0;
  for (; parsedollarindex < anslen; parsedollarindex++) {
    switch (ans[parsedollarindex]) {
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
      if (quote == '\"') {
        // Get the word;
        int nameindex = 0;
        for (parsedollarindex++; ; parsedollarindex++) {
          if (!ans[parsedollarindex] || strchr(" \t\r\n\v\"$", ans[parsedollarindex])) {
            name[nameindex++] = 0;
            parsedollarindex --;
            break;
          }
          name[nameindex++] = ans[parsedollarindex];
          if (nameindex >= DEFALUT_VARIABLE_NAME) {
            return STATE_ERROR;
          }
        }
        if (getvariable() == STATE_ERROR) {
          return STATE_ERROR;
        }
        break;
      }
      default:
        mcmd[mcmdindex++] = ans[parsedollarindex];
        if (mcmdindex >= 2048) {
          return STATE_ERROR;
        }
    }
  }
  mcmd[mcmdindex] = 0;
  return STATE_OK;
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
    if (parsedollar() != STATE_OK) {
      printf(1, "Parse $ error!\n");
      continue;
    }
    printf(1, "Run command %s!\n", mcmd);
    int run = mcmd[0];
    for (int j = 0; mcmd[j]; j++) {
      if (strchr(" \t\v\r\n", mcmd[j])) {
        break;
      }
      if (mcmd[j] == '=') {
        run = 0;
        mcmd[j] = 0;
        setVariables(mcmd, mcmd + j + 1);
      }
    }
    if (run) {
      if(fork1() == 0)
        runcmd(parsecmd(mcmd));
      wait();
    }
  }

  printf(1, "Already ran the commands in file %s!\n", argv[1]);

  exit();
}
