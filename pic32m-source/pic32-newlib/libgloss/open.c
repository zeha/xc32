/* open.c -- open a file.
 * 
 */
#include <errno.h>
#include <_ansi.h>

/*
 * open -- open a file descriptor. We don't have a filesystem, so
 *         we return an error.
 */
int
_DEFUN (open, (buf, flags, mode),
       const char *buf _AND
       int flags _AND
       int mode)
{
  errno = EIO;
  return (-1);
}

