#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

static LOAD_ENV_USED = 0;

// NOTHING - READ$ - LOADING_NAME - READ CH TO NAME- READ$ - LOADED_NAME- 
#define LOADING_NAME 0
#define NOTHING      1
#define LOADING_TEXT 2
int main(int argc, char* argv[]) {
  if (LOAD_ENV_USED++) {
    printf(1, "Already run the function!\n");
  }
  printf(1, "Loading file...\n");
  char buf[512], name[ENV_CONTENT_LEN], text[ENV_CONTENT_LEN];
  int fd;
  if((fd = open("ENV", 0)) >= 0){
    int n, i, j = 0, state = NOTHING, lenName, lenText;
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
            printf(1, "Found ENV_NAME:%s\n", name);
          }
          // a new env
          else if(state == LOADING_TEXT) {
            if (j > 0) {
              if (j >= ENV_CONTENT_LEN) {
                close(fd);
                exit();
              }
              text[j] = 0;
              lenText = j;
              printf(1, "loaded text %s\n", text);
              set_env(2, name, text, 1);
            }
            state = LOADING_NAME;
            j = 0;
          }
        }
        else if (temp == ' ' || temp == '\n' || temp == '\r' || temp == '\t') {
          if (state == LOADING_TEXT) {
            if (j > 0) {
              if (j >= ENV_CONTENT_LEN) {
                close(fd);
                exit();
              }
              text[j] = 0;
              lenText = j;
              printf(1, "loaded text %s\n", text);
              set_env(2, name, text, 1);
            }
            j = 0;
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
    if (j > 0) {
      if (j >= ENV_CONTENT_LEN) {
        close(fd);
        exit();
      }
      text[j] = 0;
      lenText = j;
      printf(1, "Finally loaded text %s (len %d)\n", text, lenText);
      set_env(2, (char*)name, text, (unsigned)1);
    }
    close(fd);
  }
  else {
    printf(1, "Load File Error!\n");
  }
  exit();
}
