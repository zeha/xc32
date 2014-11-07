/* bin.c -- Raw Binary Output

   Copyright (c) 1993-2003 MIPS Technologies, Inc. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License or
   as later amended.

   This program is distributed in the hope that it will be useful,
   BUT WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details. 

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "sysdep.h"

#define BUFSIZE		8192	/* max bytes per block */

static FILE		*binfp;
static unsigned long	binfirst = ~0;
static unsigned long	binaddr = ~0;
static int 		binlen;
static unsigned char	*binbuf;

static void
binFlush ()
{
    if (binlen > 0) {
	fseek (binfp, binaddr - binfirst, 0);
	fwrite (binbuf, 1, binlen, binfp);
	binlen = 0;
    }
}

void
BinStart (FILE *fp, unsigned long ep)
{
    binbuf = xmalloc (BUFSIZE);
    binfp = fp;
}


void
BinOutput (unsigned long addr, unsigned char byte)
{
    if (addr != binaddr + binlen || binlen == BUFSIZE) {
	binFlush ();
	binaddr = addr;
	if (binfirst == ~0)
	  binfirst = addr;
    }
    binbuf[binlen++] = byte;
}


void
BinEnd (unsigned long ep)
{
    binFlush ();
    free (binbuf);
}

