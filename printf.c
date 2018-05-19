#include "types.h"
#include "stat.h"
#include "user.h"

static void
cputc(int fd, char c, int bgcolor, int wdcolor)
{
  cwrite(fd, &c, 1, bgcolor, wdcolor);
}

static void
cprintint(int fd, int xx, int base, int sgn, int bgcolor, int wdcolor)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    buf[i++] = '-';

  while(--i >= 0)
    cputc(fd, buf[i], bgcolor, wdcolor);
}

static void
cprintfloat(int fd, double t, int bgcolor, int wdcolor)
{
  static char digits[] = "0123456789";
  char buf[16];
  int i;
  uint x;

  if (t < 0) {
    cputc(fd, '-', bgcolor, wdcolor);
    t = -t;
  }
  t += 0.00005;
  
  x = (uint)(int)t;
  t -= (int)t;
  
  i = 0;
  do{
    buf[i++] = digits[x % 10];
  }while((x /= 10) != 0);

  while(--i >= 0)
    cputc(fd, buf[i], bgcolor, wdcolor);

  cputc(fd, '.', bgcolor, wdcolor);

  for (i = 0; i < 4; i++) {
    t = t * 10;
    x = (uint)(int)t;
    if (x > 9) {
      x = 9;
    }
    t = t - (int)t;
    cputc(fd, digits[x], bgcolor, wdcolor);
  }
}

void
cprintf(int fd, int bgcolor, int wdcolor, char *fmt, ...)
{
  char *s;
  int c, i, state;
  uint *ap;

  state = 0;
  ap = (uint*)(void*)&fmt + 1;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        cputc(fd, c ,bgcolor, wdcolor);
      }
    } else if(state == '%'){
      if(c == 'd'){
        cprintint(fd, *ap, 10, 1,bgcolor, wdcolor);
        ap++;
      } else if(c == 'x' || c == 'p'){
        cprintint(fd, *ap, 16, 0 ,bgcolor, wdcolor);
        ap++;
      } else if(c == 's'){
        s = (char*)*ap;
        ap++;
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          cputc(fd, *s, bgcolor, wdcolor);
          s++;
        }
      } else if(c == 'c'){
        cputc(fd, *ap ,bgcolor, wdcolor);
        ap++;
      } else if(c == 'f'){
        double a;
        memmove((void*)&a, (void*)ap, 8);
        cprintfloat(fd, a, bgcolor, wdcolor);
        ap += 2;
      } else if(c == '%'){
        cputc(fd, c, bgcolor, wdcolor);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        cputc(fd, '%', bgcolor, wdcolor);
        cputc(fd, c, bgcolor, wdcolor);
      }
      state = 0;
    }
  }
}

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    buf[i++] = '-';

  while(--i >= 0)
    putc(fd, buf[i]);
}

static void
printfloat(int fd, double t)
{
  static char digits[] = "0123456789";
  char buf[16];
  int i;
  uint x;

  if (t < 0) {
    putc(fd, '-');
    t = -t;
  }
  t += 0.00005;
  
  x = (uint)(int)t;
  t -= (int)t;
  
  i = 0;
  do{
    buf[i++] = digits[x % 10];
  }while((x /= 10) != 0);

  while(--i >= 0)
    putc(fd, buf[i]);

  putc(fd, '.');

  for (i = 0; i < 4; i++) {
    t = t * 10;
    x = (uint)(int)t;
    if (x > 9) {
      x = 9;
    }
    t = t - (int)t;
    putc(fd, digits[x]);
  }
}

// Print to the given fd. Only understands %d, %x, %p, %s, %f.
void
printf(int fd, char *fmt, ...)
{
  char *s;
  int c, i, state;
  uint *ap;

  state = 0;
  ap = (uint*)(void*)&fmt + 1;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if(state == '%'){
      if(c == 'd'){
        printint(fd, *ap, 10, 1);
        ap++;
      } else if(c == 'x' || c == 'p'){
        printint(fd, *ap, 16, 0);
        ap++;
      } else if(c == 's'){
        s = (char*)*ap;
        ap++;
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          putc(fd, *s);
          s++;
        }
      } else if(c == 'c'){
        putc(fd, *ap);
        ap++;
      } else if(c == 'f'){
        double a;
        memmove((void*)&a, (void*)ap, 8);
        printfloat(fd, a);
        ap += 2;
      } else if(c == '%'){
        putc(fd, c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        putc(fd, '%');
        putc(fd, c);
      }
      state = 0;
    }
  }
}
