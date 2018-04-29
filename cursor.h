#ifndef CURSOR_H
#define CURSOR_H

#include "memlayout.h"
#include "types.h"
#include "kbd.h"
#include "x86.h"

#define CRTPORT 0x3d4
#define BACKSPACE 0x100
#define INPUT_BUF 128

static ushort *crt = (ushort*)P2V(0xb8000);
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

int cursorGetPosition();
void cursorSetPosition(int pos);
void cursorMoveLeft();
void cursorMoveRight();

#endif