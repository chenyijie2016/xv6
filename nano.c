//
// Created by cyj on 18-5-8.
//
#include "types.h"
#include "user.h"
#include "date.h"
#include "ctype.h"
#include "fcntl.h"

#define stdin 0
#define stdout 1
#define stderr 2
#define KILO_QUIT_TIMES 3
#define BUFFER_LENGTH 1024
typedef struct erow {
    int idx;            /* Row index in the file, zero-based. */
    int size;           /* Size of the row, excluding the null term. */
    char chars[1024];        /* Row content. */
} erow;

struct editorConfig {
    int cx, cy;  /* Cursor x and y position in characters */
    int rowoff;     /* Offset of row displayed. */
    int coloff;     /* Offset of column displayed. */
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    int numrows;    /* Number of rows */

    erow row[1024];      /* Rows */
    int dirty;      /* File modified but not saved. */
    char filename[128]; /* Currently open filename */
    char statusmsg[80];
    struct rtcdate r;
};

static struct editorConfig E;
enum KEY_ACTION {
    KEY_NULL = 0,       /* NULL */
    CTRL_C = 3,         /* Ctrl-c */
    CTRL_D = 4,         /* Ctrl-d */
    CTRL_F = 6,         /* Ctrl-f */
    CTRL_H = 8,         /* Ctrl-h */
    TAB = 9,            /* Tab */
    CTRL_L = 12,        /* Ctrl+l */
    ENTER = 13,         /* Enter */
    CTRL_Q = 17,        /* Ctrl-q */
    CTRL_S = 19,        /* Ctrl-s */
    CTRL_U = 21,        /* Ctrl-u */
    ESC = 27,           /* Escape */
    BACKSPACE = 127,   /* Backspace */
    /* The following are just soft codes, not really reported by the*/

            ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,

};
#define black 0x0
#define blue 0x1
#define green 0x2
#define red   0xc
#define yellow 0x6
#define white 0xf

void editorSetStatusMessage(const char *);

int editorReadKey(void);

void editorInsertRow(int, char *, uint);

void editorFreeRow(erow *);

void editorDelRow(int);

char *editorRowsToString(int *);

void editorRowInsertChar(erow *, int, int);

void editorRowAppendString(erow *, char *, uint);

void editorRowDelChar(erow *, int);

void editorInsertChar(int);

void editorInsertNewline(void);

int editorFileWasModified(void);

void editorDelChar();

void editorOpen(char *);

void read_char(char);

void read_file(int);

int editorSave(void);

void editorRefreshScreen(void);

void initEditor(void);

void editorProcessKeypress(void);

void editorClearScreen(void);

void editorMoveCursor(int);

static int change = 0;

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf(stderr, "Usage: nano <filename>");
        exit();
    }

    initEditor();
    editorOpen(argv[1]);
    editorRefreshScreen();

    while (1)
    {


        if (change)
        {
            editorRefreshScreen();
            change = 0;
        }


        editorProcessKeypress();
    }
}

void editorSetStatusMessage(const char *msg)
{
    char status[80] = {0};
    date(&E.r);
    sprintf(status, "                              %d-%d-%d %d:%d:%d", E.r.year, E.r.month, E.r.day,
            (E.r.hour + 8) % 24,
            E.r.minute, E.r.second);
    for (int i = 80 * 23; i < 80 * 24; i++)
    {
        setcrtcc(i, status[i % 80], 0xf, black);
    }

    for (int i = 80 * 24; i < 80 * 25 && i < 80 * 24 + strlen(msg); i++)
        setcrtcc(i, msg[i % 80] & 0xff, black, white);
}

void initEditor(void)
{
    editorClearScreen();
    // clean screen
    setcurpos(0);
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.dirty = 0;
    //E.row = NULL;
    E.screenrows = 23;
    E.screencols = 80;

}

void editorRefreshScreen(void)
{
    char msg[80] = {0};
    sprintf(msg, "%s %d lines", E.filename, E.numrows);
    editorSetStatusMessage(msg);

    setcurpos(E.cx + E.cy * 80);
    for (int i = 0; i < E.screenrows; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            setcrtcc(i * E.screencols + j, ' ', black, white);
        }

        if (E.numrows > i)
            for (int j = 0; j < E.screencols && j < E.row[i + E.rowoff].size; j++)
            {
                setcrtcc(i * E.screencols + j, E.row[i + E.rowoff].chars[j + E.coloff] & 0xff, black, white);
            }
    }
}

void editorProcessKeypress(void)
{
    int c = editorReadKey();
    static int quit_times = KILO_QUIT_TIMES;
    if (c == -1)
    {
        return;
    }
    switch (c)
    {
        case ENTER:         /* Enter */
            editorInsertNewline();
            break;
        case CTRL_C:        /* Ctrl-c */
            /* We ignore ctrl-c, it can't be so simple to lose the changes
             * to the edited file. */
            break;
        case CTRL_Q:        /* Ctrl-q */
            /* Quit if the file was already saved. */
            if (E.dirty && quit_times)
            {
                char msg[80] = {0};
                sprintf(msg, "WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
                editorSetStatusMessage(msg);

                quit_times--;
                return;
            }
            editorClearScreen();
            exit();

        case CTRL_S:        /* Ctrl-s */
            editorSave();
            break;

        case BACKSPACE:     /* Backspace */
        case CTRL_H:        /* Ctrl-h */
        case DEL_KEY:
            editorDelChar();
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        case CTRL_L: /* ctrl+l, clear screen */
            /* Just refresht the line as side effect. */
            break;
        case ESC:
            /* Nothing to do for ESC in this mode. */
            break;
        default:
            editorInsertChar(c);
            break;
    }

    quit_times = KILO_QUIT_TIMES; /* Reset it to the original value. */
    change = 1;
}

int editorReadKey(void)
{
    int editstatus = geteditstatus();
    int c = -1;
    if (editstatus > 256)
    {
        c = editstatus - 256;
    }
    switch (editstatus)
    {
        case -2:
            c = CTRL_S;
            break;
        case -3:
            c = CTRL_Q;
            break;
        case -4:
            c = ARROW_UP;
            break;
        case -5:
            c = ARROW_DOWN;
            break;
        case -6:
            c = ARROW_LEFT;
            break;
        case -7:
            c = ARROW_RIGHT;
            break;
        case -8:
            c = ENTER;
            break;
        default:
            break;

    }
    seteditstatus(-1);
    return c;
}

void editorInsertNewline(void)
{
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row)
    {
        if (filerow == E.numrows)
        {
            editorInsertRow(filerow, "", 0);
            goto fixcursor;
        }
        return;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if (filecol >= row->size) filecol = row->size;
    if (filecol == 0)
    {
        editorInsertRow(filerow, "", 0);
    } else
    {
        /* We are in the middle of a line. Split it between two rows. */
        editorInsertRow(filerow + 1, row->chars + filecol, row->size - filecol);
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        //editorUpdateRow(row);
    }
    fixcursor:
    if (E.cy == E.screenrows - 1)
    {
        E.rowoff++;
    } else
    {
        E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
}

static int linelen;
static char line_buffer[BUFFER_LENGTH];

void read_char(char c)
{

    line_buffer[linelen++] = c;
    if (c == '\n')
    {
        line_buffer[--linelen] = '\0';


        editorInsertRow(E.numrows, line_buffer, linelen);
        memset(line_buffer, 0, BUFFER_LENGTH);

        linelen = 0;
    }

}

void read_file(int fd)
{

    char buf[BUFFER_LENGTH];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            read_char(buf[i]);
        }
    }
}


void editorOpen(char *filename)
{
    int fd;

    E.dirty = 0;
    strcpy(E.filename, filename);

    fd = open(filename, O_RDONLY);
    if (!fd)
    {
        return;
    }
    read_file(fd);
    close(fd);
    E.dirty = 0;
    E.numrows--;
    return;
}

void editorInsertRow(int at, char *s, uint len)
{

    if (at > E.numrows) return;
//    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    if (at != E.numrows)
    {
        erow buf[1024];
        for (int i = at; i < E.numrows; i++)
        {
            buf[i - at].size = E.row[i].size;
            buf[i - at].idx = E.row[i].idx;
            strcpy(buf[i - at].chars, E.row[i].chars);
            //memcpy(&buf[i - at], &E.row[i], sizeof(erow));
        }

        for (int i = at + 1; i < E.numrows + 1; i++)
        {
            //memcpy(&E.row[i],&buf[i - at - 1], sizeof(erow));
            E.row[i].size = buf[i - at - 1].size;
            E.row[i].idx = buf[i - at - 1].idx;
            strcpy(E.row[i].chars, buf[i - at - 1].chars);
            E.row[i].idx++;
        }

        //memmove(E.row + at + 1, E.row + at, sizeof(E.row[0]) * (E.numrows - at));
        //for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    }

    E.row[at].size = len;
    //E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len + 1);
    E.row[at].idx = at;
    E.numrows++;
    E.dirty++;
}

void editorMoveCursor(int key)
{

    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    int rowlen;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch (key)
    {
        case ARROW_LEFT:
            if (E.cx == 0)
            {
                if (E.coloff)
                {
                    E.coloff--;
                } else
                {
                    if (filerow > 0)
                    {
                        E.cy--;
                        E.cx = E.row[filerow - 1].size;
                        if (E.cx > E.screencols - 1)
                        {
                            E.coloff = E.cx - E.screencols + 1;
                            E.cx = E.screencols - 1;
                        }
                    }
                }
            } else
            {
                E.cx -= 1;
            }
            break;
        case ARROW_RIGHT:
            if (row && filecol < row->size)
            {
                if (E.cx == E.screencols - 1)
                {
                    E.coloff++;
                } else
                {
                    E.cx += 1;
                }
            } else if (row && filecol == row->size)
            {
                E.cx = 0;
                E.coloff = 0;
                if (E.cy == E.screenrows - 1)
                {
                    E.rowoff++;
                } else
                {
                    E.cy += 1;
                }
            }
            break;
        case ARROW_UP:
            if (E.cy == 0)
            {
                if (E.rowoff) E.rowoff--;
            } else
            {
                E.cy -= 1;
            }
            break;
        case ARROW_DOWN:
            if (filerow < E.numrows)
            {
                if (E.cy == E.screenrows - 1)
                {
                    E.rowoff++;
                } else
                {
                    E.cy += 1;
                }
            }
            break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff + E.cy;
    filecol = E.coloff + E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen)
    {
        E.cx -= filecol - rowlen;
        if (E.cx < 0)
        {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

int editorSave(void)
{
    int len;
    char *buf = editorRowsToString(&len);
    int fd = open(E.filename, O_RDWR | O_CREATE);
    if (fd == -1) goto writeerr;

    /* Use truncate + a single write(2) call in order to make saving
     * a bit safer, under the limits of what we can do in a small editor. */
//    if (ftruncate(fd, len) == -1) goto writeerr;
    if (write(fd, buf, len) != len) goto writeerr;

    close(fd);
    free(buf);
    E.dirty = 0;
    char msg[80];
    sprintf(msg, "                      SAVE:    %d bytes written on disk");
    editorSetStatusMessage(msg);
    return 0;

    writeerr:
    free(buf);
    if (fd != -1) close(fd);
    editorSetStatusMessage("Can't save! I/O error");
    return 1;
}

void editorInsertChar(int c)
{
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    if (!row)
    {
        while (E.numrows <= filerow)
            editorInsertRow(E.numrows, "", 0);
    }
    row = &E.row[filerow];
    editorRowInsertChar(row, filecol, c);
    if (E.cx == E.screencols - 1)
        E.coloff++;
    else
        E.cx++;
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, uint len)
{
    //row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(row->chars + row->size, s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c)
{
    if (at > row->size)
    {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        int padlen = at - row->size;
        /* In the next line +2 means: new char and null term. */
        //row->chars = realloc(row->chars, row->size + padlen + 2);

        memset(row->chars + row->size, ' ', padlen);
        row->chars[row->size + padlen + 1] = '\0';
        row->size += padlen + 1;
    } else
    {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        //row->chars = realloc(row->chars, row->size + 2);
        char buf[1024];
        for (int i = at; i < row->size; i++)
            buf[i - at] = row->chars[i];
        for (int i = at + 1; i < row->size + 1; i++)
            row->chars[i] = buf[i - at - 1];
        //memmove(row->chars + at + 1, row->chars + at, row->size - at + 1);
        row->size++;
    }
    row->chars[at] = c;
    //editorUpdateRow(row);
    E.dirty++;
}

char *editorRowsToString(int *buflen)
{
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    /* Compute count of bytes */
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1; /* +1 is for "\n" at end of every row */
    *buflen = totlen;
    totlen++; /* Also make space for nulterm */

    buf = p = malloc(totlen * sizeof(char) + 1);

    //p = buf = malloc(totlen);
    for (j = 0; j < E.numrows; j++)
    {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}

void editorDelChar()
{

    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row || (filecol == 0 && filerow == 0)) return;
    if (filecol == 0)
    {
        /* Handle the case of column 0, we need to move the current line
         * on the right of the previous one. */
        filecol = E.row[filerow - 1].size;
        editorRowAppendString(&E.row[filerow - 1], row->chars, row->size);
        editorDelRow(filerow);
        row = NULL;
        if (E.cy == 0)
            E.rowoff--;
        else
            E.cy--;
        E.cx = filecol;
        if (E.cx >= E.screencols)
        {
            int shift = (E.screencols - E.cx) + 1;
            E.cx -= shift;
            E.coloff += shift;
        }
    } else
    {
        editorRowDelChar(row, filecol - 1);
        if (E.cx == 0 && E.coloff)
            E.coloff--;
        else
            E.cx--;
    }

    E.dirty++;
}

void editorRowDelChar(erow *row, int at)
{
    if (row->size <= at) return;
    memmove(row->chars + at, row->chars + at + 1, row->size - at);

    row->size--;
    E.dirty++;
}

void editorDelRow(int at)
{
    erow *row;

    if (at >= E.numrows) return;
    row = E.row + at;
    editorFreeRow(row);
    memmove(E.row + at, E.row + at + 1, sizeof(E.row[0]) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx++;
    E.numrows--;
    E.dirty++;
}

void editorFreeRow(erow *row)
{
    free(row->chars);
}

void editorClearScreen(void)
{
    for (int i = 0; i < 80 * 25; i++)
        setcrtc(i, ' ');
}