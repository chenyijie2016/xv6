#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

struct env* curEnv = 0;
struct envs sysEnv;

uint envNum;

void clearEnv() {
  struct env* temp = curEnv, *temp2 = 0;
  temp2 = temp->next;
  while (temp2) {
    temp->next = 0;
    temp = temp2;
    temp2 = temp->next;
    free(temp);
  }
  memset(curEnv->text, 0, sizeof(char)*ENV_CONTENT_LEN);
}

void moveToTail(char* src) {
  while (curEnv->next) {
    curEnv = curEnv->next;
  }
  curEnv->next = malloc(sizeof(struct env));
  curEnv = curEnv->next;
  curEnv->next = 0;
  strcpy(curEnv->name, src);
}

void initCurEnv(char* src) {
  curEnv->next = 0;
  strcpy(curEnv->name, src);
}

void printEnv() {
  printf(1, "%s:", curEnv->name);
  while(curEnv) {
    printf(1, " %s", curEnv->text);
    curEnv = curEnv->next;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3 || (argv[1][0] == '-' && (argv[1][1] != 'a' || argc < 4))) {
    printf(1, "Usage: \n  setenv ENV_NAME ENV_CONTENT1 ENV_CONTENT2 ... ENV_CONTENTn  to set;\n  setenv -a ENV_NAME ENV_CONTENT1 ENV_CONTENT2 ... ENV_CONTENTn  to add;\n  setenv -h  for help;\n", argc-1);
    exit();
  }

  uint firstIndex = 1;
  uint add = 0;

  if (argv[1][0] == '-') {
    add = 1;
    firstIndex = 2;
  }

  char* name = argv[firstIndex++];
  uint i = 0;

  for (; i < envNum; i++) {
    if (strcmp(name, sysEnv.data[i].name) == 0) {
      curEnv = &sysEnv.data[i];
      if (add) {
        moveToTail(name);
      }
      else {
        clearEnv();
      }
      break;
    }
  }
  if (i == envNum) {
    envNum++;
    curEnv = &sysEnv.data[i];
    initCurEnv(name);
  }

  while (firstIndex < argc) {
    strcpy(curEnv->text, argv[firstIndex++]);
    if (firstIndex < argc) {
      moveToTail(name);
    }
  }

  printf(1, "Environment Variable Set!\n");

  curEnv = &sysEnv.data[i];

  printEnv();

  exit();
}