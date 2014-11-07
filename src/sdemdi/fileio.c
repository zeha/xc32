#define KERNEL
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#undef KERNEL
#include <termios.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define NFILES 10

struct mdifile {
  int		count;
  int		mdifd;
};

struct file {
  int		count;
  struct mdifile *mdifp;
};

static struct mdifile mdifiles[NFILES] = {
  {1, 0},
  {1, 1},
  {1, 2}
};

static struct file files[NFILES] = {
  {1, &mdifiles[0]}, 
  {1, &mdifiles[1]},
  {1, &mdifiles[2]}
};

int 
open (const char *name, int oflags, ...)
{
  struct file *fp = 0;
  struct mdifile *mfp = 0;
  int fd, sfd;
  int i;

  for (fd = 0; fd < NFILES; fd++)
    if (files[fd].count == 0)
      {
	fp = &files[fd];
	break;
      }

  if (!fp)
    {
      errno = EMFILE;
      return -1;
    }

  if (! strcmp (name, "/dev/tty")
      || ! strcmp (name, "/dev/console"))
    mfp = &mdifiles[0];
  else if ((sfd = _mdi_open (name, oflags, 0666)) >= 0)
    {
      int i;
      for (i = 0; i < NFILES; i++) 
	if (mdifiles[i].count == 0)
	  {
	    mfp = &mdifiles[i];
	    mfp->mdifd = sfd;
	    break;
	  }
    }

  if (!mfp)
    {
      errno = ENOENT;
      return -1;
    }

  fp->count = 1;
  fp->mdifp = mfp;
  mfp->count++;
  return fd;
}


int
close (int fd)
{
  if (fd < NFILES && --files[fd].count == 0)
    {
      struct file *fp = &files[fd];
      struct mdifile *mfp = fp->mdifp;
      if (--mfp->count == 0 && mfp->mdifd != 0)
	_mdi_close (mfp->mdifd);
      fp->mdifp = 0;
      return 0;
    }

  errno = EBADF;
  return -1;
}

int
dup2 (int fd, int nfd)
{
  struct file *fp;

  if (fd >= NFILES || files[fd].count == 0
      || nfd >= NFILES || files[nfd].count != 0)
    {
      errno = EBADF;
      return -1;
    }

  fp = &files[nfd];
  fp->count = 1;
  fp->mdifp = files[fd].mdifp;
  fp->mdifp->count++;
  return nfd;
}

int
dup (int fd)
{
  struct file *fp;
  for (fp = files; fp < &files[NFILES]; fp++)
    if (fp->count == 0)
      return dup2 (fd, fp - files);
  errno = EMFILE;
  return -1;
}

#if 0
int
isatty (int fd)
{
  if (fd < NFILES && files[fd].count != 0)
    return (files[fd].mdifp->mdifd == 0);
  errno = EBADF;
  return -1;
}
#endif

ssize_t
read (int fd, void *buf, size_t n)
{
  if (fd < NFILES && files[fd].count != 0)
    return _mdi_read (files[fd].mdifp->mdifd, buf, n);
  errno = EBADF;
  return -1;
}

ssize_t
write (int fd, const void *buf, size_t n)
{
  if (fd < NFILES && files[fd].count != 0)
    return _mdi_write (files[fd].mdifp->mdifd, (void *)buf, n);
  errno = EBADF;
  return -1;
}

ssize_t
writev (int fd, const struct iovec *iov, int niov)
{
  ssize_t total = 0;

  if (fd >= NFILES || files[fd].count == 0) 
    {
      errno = EBADF;
      return -1;
    }

  for (; niov > 0; iov++, niov--)
    {
      ssize_t n;
      n = _mdi_write (files[fd].mdifp->mdifd, iov->iov_base, iov->iov_base);
      if (n < 0)
	return -1;
      total += n;
    }
  return total;
}


int gettimeofday (struct timeval *tv, struct timezone *tz)
{
  if (tv)
    timerclear (tv);
  if (tz)
    tz->tz_minuteswest = tz->tz_dsttime = 0;
  return (0);
}    


static int
mkstat (int mdifd, struct stat *stb)
{
    /* construct default entry */
    memset (stb, 0, sizeof (struct stat));
    if (mdifd == 0)
      {
	stb->st_mode = S_IFCHR|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWOTH;
	stb->st_rdev = 1;
	stb->st_size = 0;
      }
    else
      {
	stb->st_mode = S_IFCHR|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWOTH;
	stb->st_rdev = 2;
	stb->st_size = 1;
      }
    stb->st_nlink = 1;
    return 0;
}

int
fstat (int fd, struct stat *stb)
{
  if (fd < NFILES && files[fd].count != 0)
    return mkstat (files[fd].mdifp->mdifd, stb);
  errno = EBADF;
  return -1;
}

int
stat (const char *name, struct stat *stb)
{
  if (! strcmp (name, "/dev/tty")
      || ! strcmp (name, "/dev/console"))
    return mkstat (0, stb);
  errno = ENOENT;
  return -1;
}

int
ioctl (int fd, unsigned long cmd, ...)
{
  if (fd < NFILES && files[fd].count != 0)
    {
      if (cmd == TIOCGETA)
	{
	  if (_mdi_isatty (files[fd].mdifp->mdifd))
	    /* make isatty() library func succeed */
	    return 0;
	}
      errno = ENOTTY;
    }
  else
    errno = EBADF;
  return -1;
}

int 
fcntl (int fd, int cmd, ...)
{
  if (fd < NFILES && files[fd].count != 0)
    errno = EINVAL;
  else
    errno = EBADF;
  return -1;
}

off_t
lseek (int fd, off_t offset, int whence)
{
  if (fd < NFILES && files[fd].count != 0)
    {
      struct file *fp = &files[fd];
      struct mdifile *mfp = fp->mdifp;
      long pos = _mdi_lseek32 (mfp->mdifd, (long)offset, whence);
      if (pos == -1)
	{
	  errno = ESPIPE;
	  return (off_t)-1;
	}
      return (off_t)pos;
    }

  errno = EBADF;
  return -1;
}

int
chdir (const char *name)
{
  if (strcmp (name, "/"))
    {
      errno = ENOENT;
      return -1;
    }
  return 0;
}

int
getdtablesize ()
{
    return NFILES;
}

char *
getenv (const char *name)
{
  return 0;
}

int
kill (pid_t pid, int signal)
{
  return 0;
}



extern int	errno;

extern char     _end[];
void *		_minbrk;
void *		_maxbrk;
static void *	curbrk = 0;
static void *	curbrk_hi = 0;
static int pagesize = 4096;

#ifndef MINHEAP
#define MINHEAP		(16 * pagesize)
#endif

#ifndef MAXSTACK
#define MAXSTACK	(8 * pagesize)
#endif

int
getpagesize (void)
{
    return pagesize;
}

static void
adjust_pagesize (ptrdiff_t size) {
    while (((size / 64) < pagesize) && (pagesize > 16))
	pagesize /= 2;
}


void *
_sbrkhi (int n)
{
    extern unsigned long _mem_top;
    void *p;
    
    if (!_minbrk)
	/* heap normally starts just after data segment */
	_minbrk = _end;

    if (!_maxbrk) {
	unsigned long sp;
	unsigned long min, max;

	__asm__ __volatile__ ("move %0,$sp" : "=d" (sp));
	sp &= 0x1fffffff;
	min = (unsigned long)_minbrk & 0x1fffffff;
	if (sp < min) {
	    /* stack is below heap start (unusual) */
	    if (_mem_top >= min) {
		/* memory top is above heap, use that as heap limit.  */
		max = _mem_top;
	    }
	    else {
		/* memory top is unknown, or below data(!),
		   use MINHEAP bytes after the heap start,
		   and hope for the best! */
		max = min + MINHEAP;
	    }
	}
	else {
	    /* stack is above heap (usual case), so we use the space
	       between the "bottom" of the stack as the heap limit. */
	    max = sp - MAXSTACK;
	}

	/* put maxbrk in same kseg as _minbrk */
	if (((unsigned long)_minbrk >> 29) == 4)
	    _maxbrk = (void *) (0x80000000 | max);
	else
	    _maxbrk = (void *) (0xa0000000 | max);
    }

    if (!curbrk)
	curbrk = _minbrk;

    if (!curbrk_hi)
	curbrk_hi = _maxbrk;

    p = curbrk_hi;
    if (n > 0) {
	if (curbrk_hi - n < curbrk) {
	    errno = ENOMEM;
	    return (void *)-1;
	}
    } else {
	if (curbrk_hi - n > _maxbrk) {
	    errno = EINVAL;
	    return (void *)-1;
	}
    }
    curbrk_hi -= n;
    return p;
}

void *
_sbrk (int n)
{
    extern unsigned long _mem_top;
    void *p;
    
    if (!_minbrk)
	/* heap normally starts just after data segment */
	_minbrk = _end;

    if (!_maxbrk) {
	unsigned long sp;
	unsigned long min, max;

	__asm__ __volatile__ ("move %0,$sp" : "=d" (sp));
	sp &= 0x1fffffff;
	min = (unsigned long)_minbrk & 0x1fffffff;
	if (sp < min) {
	    /* stack is below heap start (unusual) */
	    if (_mem_top >= min) {
		/* memory top is above heap, use that as heap limit.  */
		max = _mem_top;
	    }
	    else {
		/* memory top is unknown, or below data(!),
		   use MINHEAP bytes after the heap start,
		   and hope for the best! */
		max = min + MINHEAP;
	    }
	}
	else {
	    /* stack is above heap (usual case), so we use the space
	       between the "bottom" of the stack as the heap limit. */
	    max = sp - MAXSTACK;
	}

	/* put maxbrk in same kseg as _minbrk */
	if (((unsigned long)_minbrk >> 29) == 4)
	    _maxbrk = (void *) (0x80000000 | max);
	else
	    _maxbrk = (void *) (0xa0000000 | max);
    }

    if (!curbrk)
	curbrk = _minbrk;

    if (!curbrk_hi)
	curbrk_hi = _maxbrk;

    p = curbrk;
    if (n > 0) {
	if (curbrk + n > curbrk_hi) {
	    errno = ENOMEM;
	    return (void *)-1;
	}
    } else {
	if (curbrk + n < _minbrk) {
	    errno = EINVAL;
	    return (void *)-1;
	}
    }
    curbrk += n;
    return p;
}

void *
sbrk (int n)
{
    void *p;

    p = _sbrk(n);
    if (n > 0)
	memset (p, 0, n);

    return p;
}




#ifndef __LIBC_THREAD_H__

#define _ATEXIT_CHUNK   32 
struct _atexit { 
    struct _atexit      *next; 
    int                 nexfun; 
    void                (*funtab[_ATEXIT_CHUNK])(void); 
}; 

static int exiting;
static struct _atexit *_atexit;

extern void (*__cleanup);

int atexit (void (*fun)(void))
{
    struct _atexit *ex;

    if (exiting) {
	return -1;
    }

    ex = _atexit;
    if (ex == NULL || ex->nexfun >= _ATEXIT_CHUNK) {
	if (! (ex = malloc (sizeof *ex))) {
	    return -1;
	}
	ex->nexfun = 0;
	ex->next = _atexit;
	_atexit = ex;
    }
    ex->funtab[ex->nexfun++] = fun;

    return 0;
}

void exit (int status)
{
    struct _atexit *ex;

    exiting = 1;

    for (ex = _atexit; ex; ex = ex->next)
	while (ex->nexfun > 0)
	    (ex->funtab[--ex->nexfun]) ();

    if (__cleanup)
	(__cleanup) ();

    _exit (status);
}
#endif

#ifdef __LIBC_THREAD_H__
#pragma weak _libc_private_storage
void *
_libc_private_storage (struct _thread_private_key_struct *key,
		       void *storage, size_t size, void *err)
{
    return storage;
}
#endif

/* single-threaded version of atomic r-m-w worker function */
#pragma weak _mips_atomic_update
unsigned int 
_mips_atomic_update (volatile unsigned int *obj,
		     unsigned int clr, unsigned int add)
{
    unsigned int old = *obj;
    *obj = (old & ~clr) + add;
    return old;
}

#include <stdarg.h>
#include <stdio.h>

void
_mon_printf (const char *fmt, ...)
{
#if 1
  _mdi_write (2, fmt, strlen (fmt));
#else
  va_list ap;
  va_start(ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end(ap);
#endif
}

void
_mon_putc (char c)
{
#if 1
  _mdi_putchar (2, c);
#else
  fputc (c, stderr);
#endif
}

void
_mon_write (const char *buf, unsigned int nb)
{
  _mdi_write (2, buf, nb);
}

int
_mon_getc (int canblock)
{
#if 1
  return _mdi_getchar (0);
#else
  return fgetc (stderr);
#endif
}
