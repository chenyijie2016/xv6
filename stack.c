#include "stack.h"
#include "shlib.h"
#include "user.h"

STATE stack_init (struct stack* s, int elemsize, int stacksize) {
  if (!s) {
    return STATE_ERROR;
  }
  if (elemsize % sizeof(void*)) {
    elemsize = (elemsize / sizeof(void*) + 1) * sizeof(void*);
  }
  if (elemsize <= 0) {
    elemsize = sizeof(void*);
  }
  if (stacksize <= 0) {
    stacksize = INIT_STACK_SIZE;
  }
  s->elemsize = elemsize;
  s->stacksize = stacksize;
  s->base = (void*)malloc(elemsize * stacksize);
  if (s->base) {
    s->topelem = s->base;
    return STATE_OK;
  }
  return STATE_OVERFLOW;
}

STATE stack_free (struct stack* s) {
  if (!s) {
    return STATE_ERROR;
  }
  free(s->base);
  s->stacksize = 0;
  s->topelem = s->base = 0;
  return STATE_OK;
}

STATE stack_bigger (struct stack* s) {
  void* temp = (void*)malloc(s->elemsize * s->stacksize * 2);
  if (!temp) {
    return STATE_OVERFLOW;
  }
  s->stacksize *= 2;
  free(s->base);
  s->base = temp;
  return STATE_OK;
}

STATE stack_push (struct stack* s, void* elem) {
  if (!s) {
    return STATE_ERROR;
  }
  if ((s->topelem - s->base) * sizeof(void*) >= s->stacksize * s->elemsize) {
    stack_bigger(s);
  }
  memmove(s->topelem, elem, s->elemsize / sizeof(void*));
  s->topelem += s->elemsize / sizeof(void*);
  return STATE_OK;
}

STATE stack_top (struct stack* s, void* elem) {
  if (!s || !elem || stack_isempty(s)) {
    return STATE_ERROR;
  }
  memmove(elem, (s->topelem - s->elemsize / sizeof(void*)), s->elemsize / sizeof(void*));
  return STATE_OK;
}

STATE stack_pop (struct stack* s, void* elem) {
  if (!s || stack_isempty(s)) {
    return STATE_ERROR;
  }
  if (elem) {
    memmove(elem, (s->topelem - s->elemsize / sizeof(void*)), s->elemsize / sizeof(void*));
  }
  s->topelem -= s->elemsize / sizeof(void*);
  return STATE_OK;
}

int stack_isempty (struct stack* s) {
  if (!s || s->base == s->topelem) {
    return 1;
  }
  return 0;
}
