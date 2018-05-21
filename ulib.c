#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define RAND_MAX  (1<<31) - 1
static unsigned int random_seed;

char *
strcpy(char *s, char *t)
{
    char *os;

    os = s;
    while ((*s++ = *t++) != 0);
    return os;
}

int
strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
        p++, q++;
    return (uchar) *p - (uchar) *q;
}

uint
strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++);
    return n;
}

void *
memset(void *dst, int c, uint n)
{
    stosb(dst, c, n);
    return dst;
}

char *
strchr(const char *s, char c)
{
    for (; *s; s++)
        if (*s == c)
            return (char *) s;
    return 0;
}

char *
gets(char *buf, int max)
{
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;)
    {
        cc = read(0, &c, 1);
        if (cc < 1)
            break;
        buf[i++] = c;
        if (c == '\n' || c == '\r')
            break;
    }
    buf[i] = '\0';
    return buf;
}

int
stat(char *n, struct stat *st)
{
    int fd;
    int r;

    fd = open(n, O_RDONLY);
    if (fd < 0)
        return -1;
    r = fstat(fd, st);
    close(fd);
    return r;
}

int
atoi(const char *s)
{
    int n;

    n = 0;
    while ('0' <= *s && *s <= '9')
        n = n * 10 + *s++ - '0';
    return n;
}

char *itoa(int val, char *buf, unsigned radix)
{
    char *p;
    char *firstdig;
    char temp;
    unsigned digval;
    p = buf;
    if (val < 0)
    {
        *p++ = '-';
        val = (unsigned long) (-(long) val);
    }
    firstdig = p;
    do
    {
        digval = (unsigned) (val % radix);
        val /= radix;

        if (digval > 9)
            *p++ = (char) (digval - 10 + 'a');
        else
            *p++ = (char) (digval + '0');
    } while (val > 0);

    *p-- = '\0';
    do
    {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;
        --p;
        ++firstdig;
    } while (firstdig < p);
    return buf;
}

void *
memmove(void *vdst, void *vsrc, int n)
{
    char *dst, *src;

    dst = vdst;
    src = vsrc;
    while (n-- > 0)
        *dst++ = *src++;
    return vdst;
}

void *memcpy(void *vdst, void *vsrc, uint n)
{
    return memmove(vdst, vsrc, n);
}

void srand(int seed)
{
    random_seed = seed;
}

unsigned int myrand(void)
{
    random_seed = (random_seed * 1664525 + 1013904223) % RAND_MAX;
    return random_seed;
}

unsigned int randrange(int min, int max)
{
    if (max < min)
    {
        printf(2, "Error randrange out of range %d %d", min, max);
        return 0;
    }
    return myrand() % (max - min) + min;
}

char *strcat(char *pre, const char *next)
{
    if (pre == 0 || next == 0)
        return pre;
    char *tmp_ptr = pre + strlen(pre);

    while ((*tmp_ptr++ = *next++) != '\0');

    return pre;
}

static int sprintf_string_index;

void sputc(char *dst, char c)
{
    dst[sprintf_string_index++] = c;
}

void sprintint(char *dst, int xx, int base, int sgn)
{
    static char digits[] = "0123456789ABCDEF";
    char buf[16];
    int i, neg;
    uint x;

    neg = 0;
    if (sgn && xx < 0)
    {
        neg = 1;
        x = -xx;
    } else
    {
        x = xx;
    }

    i = 0;
    do
    {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);
    if (neg)
        buf[i++] = '-';

    while (--i >= 0)
        sputc(dst, buf[i]);
}


void sprintf(char *dst, const char *fmt, ...)
{
    sprintf_string_index = 0;
    char *s;
    int c, i, state;
    uint *ap;

    state = 0;
    ap = (uint *) (void *) &fmt + 1;
    for (i = 0; fmt[i]; i++)
    {
        c = fmt[i] & 0xff;
        if (state == 0)
        {
            if (c == '%')
            {
                state = '%';
            } else
            {
                sputc(dst, c);
            }
        } else if (state == '%')
        {
            if (c == 'd')
            {
                sprintint(dst, *ap, 10, 1);
                ap++;
            } else if (c == 'x' || c == 'p')
            {
                sprintint(dst, *ap, 16, 0);
                ap++;
            } else if (c == 's')
            {
                s = (char *) *ap;
                ap++;
                if (s == 0)
                    s = "(null)";
                while (*s != 0)
                {
                    sputc(dst, *s);
                    s++;
                }
            } else if (c == 'c')
            {
                sputc(dst, *ap);
                ap++;
            } else if (c == '%')
            {
                sputc(dst, c);
            } else
            {
                // Unknown % sequence.  Print it to draw attention.
                sputc(dst, '%');
                sputc(dst, c);
            }
            state = 0;
        }
    }
}

int snprintf(char *dst, uint size, const char *fmt, ...)
{
    sprintf_string_index = 0;
    char *s;
    char temp[1024] = {0};
    int c, i, state;
    uint *ap;

    state = 0;
    ap = (uint *) (void *) &fmt + 1;
    for (i = 0; fmt[i]; i++)
    {
        c = fmt[i] & 0xff;
        if (state == 0)
        {
            if (c == '%')
            {
                state = '%';
            } else
            {
                sputc(temp, c);
            }
        } else if (state == '%')
        {
            if (c == 'd')
            {
                sprintint(temp, *ap, 10, 1);
                ap++;
            } else if (c == 'x' || c == 'p')
            {
                sprintint(temp, *ap, 16, 0);
                ap++;
            } else if (c == 's')
            {
                s = (char *) *ap;
                ap++;
                if (s == 0)
                    s = "(null)";
                while (*s != 0)
                {
                    sputc(temp, *s);
                    s++;
                }
            } else if (c == 'c')
            {
                sputc(temp, *ap);
                ap++;
            } else if (c == '%')
            {
                sputc(temp, c);
            } else
            {
                // Unknown % sequence.  Print it to draw attention.
                sputc(temp, '%');
                sputc(temp, c);
            }
            state = 0;
        }
    }

    memmove(dst, temp, size * sizeof(char));
    if (strlen(temp) > size)
    {
        return size;
    }
    return strlen(temp);
}

char *strstr(const char *s1, const char *s2)
{
    int n;
    if (*s2)
    {
        while (*s1)
        {
            for (n = 0; *(s1 + n) == *(s2 + n); n++)
            {
                if (!*(s2 + n + 1))
                    return (char *) s1;
            }
            s1++;
        }
        return NULL;
    } else
        return (char *) s1;
}

int
memcmp(const void *v1, const void
*v2, uint n)
{
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0)
    {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}
