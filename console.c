#include "cursor.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"

static void consputc(int);

static int panicked = 0;
static struct {
  struct spinlock lock;
  int locking;
} cons;

static void printint(int xx, int base, int sign){
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

void cprintf(char *fmt, ...){
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void panic(char *s){
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

static void cgaputc(int c){
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white
  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

//wdcolor: word color       one-digut hex number
//bgcolor: backround color  one-digit hex nunmber
/*type: 0x0--black
        0x1--dark blue
        0x2--dark green
        0x3--dark cyan
        0x4--dark red
        0x5--dark pink
        0x6--brown
        0x7--bright grey
        0x8--dark grey
        0x9--bright blue
        0xA--bright green
        0xB--bright cyan
        0xC--bright red
        0xD--bright pink
        0xE--bright yellow
        0xF--white
*/
void cgaputcolorfulc(int c, int wdcolor, int bgcolor){
  if(wdcolor<0 || wdcolor>=16 || bgcolor<0 || wdcolor>=16)
  return;
  int pos;
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else{ 
    int shift =(wdcolor<<12) | (bgcolor<<8);
    crt[pos++] = (c&0xff) | shift;
  }
  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");
  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }
  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

void consputc(int c){
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}

void killLine(){
  int i;
  for(i = input.e; i < input.d; i++){
    cursorMoveRight();
    input.e++;
  }
  while(input.d != input.w &&
      input.buf[(input.d-1) % INPUT_BUF] != '\n'){
      input.e--;
      input.d--;
      consputc(BACKSPACE);
  }
}

void outputChar(int c){
  if(input.d == input.e){
    input.buf[input.e++ % INPUT_BUF] = c;
    input.d++;
    consputc(c);
  }
  else if(c == '\n'){
    input.buf[input.d++ % INPUT_BUF] = c;
    while(input.e < input.d){
      input.e++;
      cursorMoveRight();
    }
    consputc(c);    
  }
  else{
    int i;
    for(i = input.d-1; i >= input.e; i--)
      input.buf[(i+1) % INPUT_BUF] = input.buf[i % INPUT_BUF];
    input.buf[input.e % INPUT_BUF] = c;
    for(i = input.e; i <= input.d; i++)
      consputc(input.buf[i % INPUT_BUF]);
    input.d++;
    input.e++;
    for(i = input.e; i < input.d; i++)
      cursorMoveLeft();
  }
}

void removeChar(){
  if(input.d == input.e){
    input.e--;
    input.d--;
    consputc(BACKSPACE);    
  }
  else{
    cursorMoveLeft();
    int i;
    for(i = input.e; i < input.d; i++)
      input.buf[(i-1+INPUT_BUF) % INPUT_BUF] = input.buf[i % INPUT_BUF];
    input.e--;
    input.d--;
    for(i = input.e; i < input.d; i++)
      consputc(input.buf[i % INPUT_BUF]);
    for(i = input.e; i < input.d; i++)
      cursorMoveLeft();
  }
}

void loadPrevHis(){
  if(currentLine == firstLine)
    return;
  killLine();
  currentLine--;
  int i;
  for(i = 0; i < hisLength[currentLine]; i++)
    outputChar(hisContent[currentLine][i]);
}

void loadNextHis(){
  if(currentLine == hisLine)
    return;
  killLine();
  currentLine++;
  int i;
  for(i = 0; i < hisLength[currentLine]; i++)
    outputChar(hisContent[currentLine][i]);
}

void consoleintr(int (*getc)(void)){
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      killLine();
      break;
    case C('H'): case '\x7f':  // Backspace
      if(input.e != input.w)
        removeChar();
      break;
    case KEY_UP:
      loadPrevHis();
      break;
    case KEY_DN:
      loadNextHis();
      break;
    case KEY_LF:
      if(input.e != input.w){
        cursorMoveLeft();
        input.e--;
      }
      break;
    case KEY_RT:
      if(input.d != input.e){
        cursorMoveRight();
        input.e++;
      }
      break;
    default:
      if(c != 0 && input.d-input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        outputChar(c);
        if(c == '\n' || c == C('D') || input.d == input.r+INPUT_BUF){
          input.w = input.d;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int consoleread(struct inode *ip, char *dst, int n){
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n'){
      if(hisPos != 0){
        hisLength[hisLine] = hisPos;
        hisPos = 0;
        hisLine = (hisLine+1) % HISTORY_LOAD;
        currentLine = hisLine;
        hisLength[hisLine] = 0;
        if(hisLength[hisLine] != 0)
        firstLine = hisLine + 1;
      }
      break;
    }
    else
      hisContent[hisLine][hisPos++] = c;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n){
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void){
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
