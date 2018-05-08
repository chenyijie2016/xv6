#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[]) {
  struct env m;
  int size;
  char name[ENV_CONTENT_LEN];
  if (argc != 2 || (argv[1][0] == '-' && argv[1][1] != 'a')) {
    printf(1, "Usage: showenv ENV_NAME\n       showenv -a\nENV_NAMEs: \n");
    size = getenv(0, (void*)-1, (char*)0);
    for (int i = 0; i < size; i++) {
      getenv(0, (void*)i, name);
      printf(1, "   %s\n", name);
    }
    exit();
  }

  if (argv[1][0] == '-') {
    printf(1, "ENV_NAMEs: \n");
    size = getenv(0, (void*)-1, (char*)0);
    for (int i = 0; i < size; i++) {
      getenv(0, (void*)i, name);
      printf(1, " %s\n", name);
      if (getenv(1, &m, name) == 0) {
        for (uint i = 0; i < m.len; i++) {
          printf(1, "   %s\n", m.text[i]);
        }
      }
    }
    exit();
  }
  if (getenv(1, &m, argv[1]) != 0) {
    printf(1, "Error: Environment %s doesn't exist!\n", argv[1]);
    exit();
  }

  printf(1, "Environment Variables %s: \n", argv[1]);
  for (uint i = 0; i < m.len; i++) {
    printf(1, "   %s\n", m.text[i]);
  }
  exit();
}