#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

#define defaultEnv "ENV"

// NOTHING - READ$ - LOADING_NAME - READ CH TO NAME- READ$ - LOADED_NAME- 
#define LOADING_NAME 0
#define NOTHING      1
#define LOADING_TEXT 2

char buf[512], name[ENV_CONTENT_LEN], text[ENV_CONTENT_LEN];
int fd, n, i, j, state, lenName, lenText, silent;

void set_new_env() {
  if (j > 0) {
    if (j >= ENV_CONTENT_LEN) {
      close(fd);
      exit();
    }
    text[j] = 0;
    lenText = j;
    if (!silent)
      printf(1, "  loaded text %s\n", text);
    setenv(2, name, (char**)(int)text, 1);
  }
  j = 0;
}

int main(int argc, char* argv[]) {
  fd = -1;
  silent = 0;
  if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 's') {
    silent = 1;
  }
  if (argc > 1 + silent) {
    if (!silent)
      printf(1, "Loading file %s...\n", argv[1 + silent]);
    fd = open(argv[1 + silent], 0);
    if (fd < 0) {
      if (!silent)
        printf(1, "Load Error!\n");
      exit();
    }
  }
  if (fd < 0) {
    if (!silent)
      printf(1, "Loading file %s...\n", defaultEnv);
    fd = open(defaultEnv, 0);
  }
  if(fd < 0){
    if (!silent)
      printf(1, "Load Error!\n");
  }
  else {
    j = 0, state = NOTHING;
    while(((i=0), n = read(fd, buf, sizeof(buf)))>0) {
      while (i < n) {
        char temp = buf[i++];
        if (temp == '$') {
          // just started!
          if (state == NOTHING) {
            j = 0;
            state = LOADING_NAME;
          }
          // LOADING_NAME
          else if (state == LOADING_NAME) {
            if (j >= ENV_CONTENT_LEN || j == 0) {
              close(fd);
              exit();
            }
            name[j] = 0;
            state = LOADING_TEXT;
            lenName = j;
            j = 0;
            if (!silent)
              printf(1, "Found ENV_NAME:%s\n", name);
          }
          // a new env
          else if(state == LOADING_TEXT) {
            set_new_env();
            state = LOADING_NAME;
          }
        }
        else if (temp == ' ' || temp == '\n' || temp == '\r' || temp == '\t') {
          if (state == LOADING_TEXT) {
            set_new_env();
          }
        }
        else {
          if (state == LOADING_NAME) {
            name[j++] = temp;
            if (j >= ENV_CONTENT_LEN) {
              close(fd);
              exit();
            }
          }
          else if (state == LOADING_TEXT) {
            text[j++] = temp;
            if (j >= ENV_CONTENT_LEN) {
              close(fd);
              exit();
            }
          }
        }
      }
    }
    set_new_env();    
  }
  exit();
}
