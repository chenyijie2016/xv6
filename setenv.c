#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

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

  if(set_env(add, name, (argv + firstIndex), argc - firstIndex) != 0) {
    printf(1, "Error! Environment Variable: %s  Unet!\n", name);
  }
  else {
    printf(1, "Environment Variable: %s  Set!\n", name);
  }

  exit();
}