/* PIC32C: moved from vfscanf.c */
#include <_ansi.h>
#include <reent.h>
#include <newlib.h>
#include <ctype.h>
#include <wctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <wchar.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "local.h"
#include "../stdlib/local.h"

size_t
_DEFUN(_sfread_r, (ptr, buf, size, count, fp),
       struct _reent * ptr _AND
       _PTR buf _AND
       size_t size _AND
       size_t count _AND
       FILE * fp)
{
  register size_t resid;
  register char *p;
  register int r;
  size_t total;

  if ((resid = count * size) == 0)
    return 0;

  total = resid;
  p = buf;

  while (resid > (r = fp->_r))
    {
      _CAST_VOID memcpy ((_PTR) p, (_PTR) fp->_p, (size_t) r);
      fp->_p += r;
      fp->_r = 0;
      p += r;
      resid -= r;
      if (__ssrefill_r (ptr, fp))
        {
          /* no more input: return partial result */
          return (total - resid) / size;
        }
    }
  _CAST_VOID memcpy ((_PTR) p, (_PTR) fp->_p, resid);
  fp->_r -= resid;
  fp->_p += resid;
  return count;
}
