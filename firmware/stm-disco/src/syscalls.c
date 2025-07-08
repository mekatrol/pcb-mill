#include <sys/stat.h>
#include <errno.h>

static int errno_var;

int *__errno(void)
{
    return &errno_var;
}

extern int _end; // linker symbol for end of RAM/heap start
static char *heap_end;

int _sbrk(int incr)
{
    char *prev_heap_end;
    if (heap_end == 0)
    {
        heap_end = (char *)&_end;
    }
    prev_heap_end = heap_end;
    heap_end += incr;
    return (int)prev_heap_end;
}

int _write(int file, char *ptr, int len) { return len; }
int _close(int file) { return -1; }
int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
void _exit(int status)
{
    while (1)
        ;
}
int _kill(int pid, int sig)
{
    *__errno() = EINVAL;
    return -1;
}
int _getpid(void) { return 1; }
