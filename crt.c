#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"


extern int editStatus;

static struct {
    struct spinlock lock;
    int locking;
} cons;

static ushort *crt = (ushort *) P2V(0xb8000);
#define CRTPORT 0x3d4

int
sys_getcrtc(void)
{
    int x, y;
    argint(0, &x);
    argint(1, &y);
    return crt[y * 80 + x];
}

int
sys_setcrtc(void)
{
    int pos, c;
    argint(0, &pos);
    argint(1, &c);
    crt[pos] = c;

    return c;
}

int
sys_getcurpos(void)
{
    int pos;
    // Cursor position: col + 80*row.
    outb(CRTPORT, 14);
    pos = inb(CRTPORT + 1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT + 1);


    return pos;
}

int
sys_setcurpos(void)
{
    int pos;
    argint(0, &pos);
    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);

    return 1;
}

int
sys_geteditstatus(void)
{
    return editStatus;
}

int
sys_seteditstatus(void)
{
    argint(0, &editStatus);
    return editStatus;
}

int sys_setcrtcc(void)
{
    int pos, c, wdcolor, bgcolor;
    argint(0, &pos);
    argint(1, &c);
    argint(2, &wdcolor);
    argint(3, &bgcolor);
    int shift = (wdcolor << 12) | (bgcolor << 8);
    crt[pos++] = (c & 0xff) | shift;
    crt[pos] = c;
}
