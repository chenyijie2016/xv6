#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
#include "fs.h"

#define FILE_PATH_LEN 1024

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  int length = strlen(path);
  int found = 0;
  int n;
  char file_path[FILE_PATH_LEN];
  struct env* pathenv = 0;

  begin_op();

  if((ip = namei(path)) == 0){
    if (sysEnv.envNum > 0) {
      pathenv = &(sysEnv.data[0]);
    }

    uint pathIndex = 0;
    uint len = pathenv->len;

    while (pathIndex < len && !found) {
      //clear
      memset(file_path, 0, sizeof(char) * FILE_PATH_LEN);

      int lenPathEnv = strlen(pathenv->text[pathIndex]);
      n = 0;

      //copy
      for(n=0; n < lenPathEnv; n++){
        file_path[n] = pathenv->text[pathIndex][n];
      }

      //copy the path from parameter
      for(n=0; n < length; n++){
        file_path[lenPathEnv + n] = path[n];
      }

      //set string end
      file_path[lenPathEnv + length] = '\0';

      if((ip = namei(file_path)) != 0){
        path = file_path;
        found = 1;
      }

      pathIndex++;
    }
    
    // nout found
    if(!found) {
      end_op();
      cprintf("exec: fail\n");
      return -1;
    }
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}


// Add here
struct envs sysEnv;

char*
mstrcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
mstrcmp(const char *p, const char *q)
{
  while(*p &&*q && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

int sys_setenv(void) {
  int add;
  argint(0, &add);
  char* name;
  argstr(1, &name);
  char** argv;
  argstr(2, &argv);
  int len;
  argint(3, &len);

  uint i = 0;

  for (; i < sysEnv.envNum; i++) {
    if (mstrcmp(name, sysEnv.data[i].name) == 0) {
      if (!add) {
        sysEnv.data[i].len = 0;
      }
      break;
    }
  }
  if (i == sysEnv.envNum) {
    if (sysEnv.envNum == ENV_MAX_NUM) {
      return 2;
    }
    sysEnv.envNum++;
    mstrcpy(sysEnv.data[i].name, name);
    sysEnv.data[i].len = 0;
  }

  uint j = 0;

  for (j = 0; j < len && (sysEnv.data[i].len) < ENV_CONTENT_NUM; j++, (sysEnv.data[i].len)++) {
    mstrcpy(sysEnv.data[i].text[sysEnv.data[i].len], argv[j]);
  }

  if (j < len && sysEnv.data[i].len == ENV_CONTENT_NUM) {
    return 1;
  }

  return 0;
}

int sys_getenv(void) {
  int type;
  void* penv;
  char* name;
  argint(0, &type);
  argstr(1, &penv);
  argstr(2, &name);
  if (type == 0) {
    if (0 <= (int)penv && (int)penv < (int)sysEnv.envNum) {
      strncpy(name, sysEnv.data[(int)penv].name, ENV_CONTENT_LEN);
    }
    return sysEnv.envNum;
  }
  else {
    for (uint i = 0; i < sysEnv.envNum; i++) {
      if (strncmp(name, sysEnv.data[i].name, ENV_CONTENT_LEN) == 0) {
        memmove((void*)penv, (void*)(&(sysEnv.data[i])), (sizeof(struct env) + sizeof(char) - 1)/sizeof(char));
        return 0;
      }
    }
    return 1;
  }
  return 0;
}