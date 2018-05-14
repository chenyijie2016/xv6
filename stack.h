#ifndef STACK
#define STACK

#define INIT_STACK_SIZE 256

#include "operationstate.h"

struct stack {
  void* base;
  void* topelem;
  int   elemsize;
  int   stacksize;
};

STATE stack_init    (struct stack* s, int elemsize, int stacksize);
STATE stack_free    (struct stack* s);
STATE stack_push    (struct stack* s, void* elem);
STATE stack_top     (struct stack* s, void* elem);
STATE stack_pop     (struct stack* s, void* elem);
int   stack_isempty (struct stack* s);

#endif // !STACK