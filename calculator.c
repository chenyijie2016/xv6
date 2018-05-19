#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "calculate.h"

char Calculatable[2048];

int main(int argc, char* argv[]) {
  if (argc < 2 || (argv[1][0] == '-' && argv[1][1] == 'h')) {
    printf(1, "Usage: cal CALCULATABLE TARGET\n");
    exit();
  }
  for (int j = 1; j < argc; j++) {
    strcpy(Calculatable + strlen(Calculatable), argv[j]);
    if (j < argc - 1) {
      Calculatable[strlen(Calculatable)] = ' ';
    }
  }
  int ok;
  int mlen = (int)strlen(Calculatable);
  for (int i = 0; i < mlen; i++) {
    if (Calculatable[i] == '{') {
      Calculatable[i] = '(';
    }
    if (Calculatable[i] == '}') {
      Calculatable[i] = ')';
    }
  }
  double ans = calculate(Calculatable, &ok);
  if (ok == 0) {
    printf(1, "%f\n", ans);
    exit();
  }
  else {
    printf(1, "Parse Error!\n");
    exit();
  }
}
