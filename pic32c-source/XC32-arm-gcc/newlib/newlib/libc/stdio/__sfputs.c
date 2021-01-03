/* PIC32C: moved from vfprintf.c. */
#include <newlib.h>

#define _NO_POS_ARGS
#ifdef _WANT_IO_POS_ARGS
# undef _NO_POS_ARGS
#endif

#include <_ansi.h>
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <wchar.h>
#include <sys/lock.h>
#include <stdarg.h>
#include "local.h"
#include "../stdlib/local.h"
#include "fvwrite.h"
#include "vfieeefp.h"

int
_DEFUN(__sfputs_r, (ptr, fp, buf, len),
       struct _reent *ptr _AND
       FILE *fp _AND
       _CONST char *buf _AND
       size_t len)
{
	register int i;

#ifdef _WIDE_ORIENT
	if (fp->_flags2 & __SWID) {
		wchar_t *p;

		p = (wchar_t *) buf;
		for (i = 0; i < (len / sizeof (wchar_t)); i++) {
			if (_fputwc_r (ptr, p[i], fp) == WEOF)
				return -1;
		}
	} else {
#else
	{
#endif
		for (i = 0; i < len; i++) {
			if (_fputc_r (ptr, buf[i], fp) == EOF)
				return -1;
		}
	}
	return (0);
}
