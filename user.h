struct stat;
struct rtcdate;
#define NULL (void*)0
// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int date(struct rtcdate *);
int cwrite(int, void*, int, int, int);
int dir(char*);
int setenv(int, char*, char**, unsigned int);
int getenv(int, void*, char*);
int cmplt(char*);

// CRT operations
int getcrtc(int, int);
int setcrtc(int, int);
int getcurpos();
int setcurpos(int);
int geteditstatus();
int seteditstatus(int);
int setcrtcc(int,int,int,int);

// file operation
int stat(char*, struct stat*);



// memory
void *memmove(void*, void*, int);
void* malloc(uint);
void* realloc(void*, uint);
void free(void*);
int memcmp(const void *, const void *, uint );
void* memset(void*, int, uint);
void* memcpy(void*, void*, uint);


// string
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* strcat(char* p ,const char* );
uint strlen(const char*);
char* strcpy(char*, char*);
void sprintf(char*, const char*, ...);
int snprintf(char*, uint, const char*, ...);
char *strstr(const char*,const char*);
int atoi(const char*);
char* itoa(int, char*, unsigned);
