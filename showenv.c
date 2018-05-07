#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[]) {
  struct env m;

  char name[ENV_CONTENT_LEN];

  if (argc != 2) {
    printf(1, "Usage: showenv ENV_NAME\nENV_NAMEs: ");
    int size;
    size = getenv(0, -1, 0);
    for (int i = 0; i < size; i++) {
      getenv(0, i, &name);
      printf(1, "%d:%s ", i, name);
    }
    printf(1, "\n");
    exit();
  }

  if (getenv(1, &m, argv[1]) != 0) {
    printf(1, "Error: Environment %s doesn't exist!\n", argv[1]);
    exit();
  }

  printf(1, "Environment Variables %s: ", argv[1]);
  for (uint i = 0; i < m.len; i++) {
    printf(1, "%s ", m.text[i]);
  }
  printf(1, "\n");
  exit();
}