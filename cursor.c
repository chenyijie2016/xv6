#include "cursor.h"

int cursorGetPosition(){
    int pos = 0;
    outb(CRTPORT, 14);
    pos = inb(CRTPORT+1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT+1);
    return pos;
}

void cursorSetPosition(int pos){
    outb(CRTPORT, 14);
    outb(CRTPORT+1, pos>>8);
    outb(CRTPORT, 15);
    outb(CRTPORT+1, pos);
    return;
}

int cursorMoveLeft(){
    int pos = cursorGetPosition();
    pos--;
    cursorSetPosition(pos);
    return 1;
}

int cursorMoveRight(){
    int pos = cursorGetPosition();
    pos++;
    cursorSetPosition(pos);
    return 1;
}
