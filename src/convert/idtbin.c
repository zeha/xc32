/* idtbin.c -- Compressed Motorola S-Record Output for IDT/sim

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

#define BUFSIZE		250
#define ADDRSIZE	4

static FILE		*idtfp;
static unsigned long	idtaddr = ~0;
static int 		idtlen;
static unsigned char	idtbuf[BUFSIZE];

static void
idtFlush ()
{
    unsigned char cksum = 0;
    int n;

    if (idtlen == 0)
      return;

    fprintf (idtfp, "S3%c", idtlen + ADDRSIZE + 1);
    cksum += idtlen + ADDRSIZE + 1;

    for (n = (ADDRSIZE - 1) * 8; n >= 0; n -= 8) {
	unsigned char ab = idtaddr >> n;
	fputc (ab, idtfp);
	cksum += ab;
    }

    for (n = 0; n < idtlen; n++) {
	putc (idtbuf[n], idtfp);
	cksum += idtbuf[n];
    }

    fputc (~cksum & 0xff, idtfp);
    idtlen = 0;
}


void
IdtStart (FILE *fp, unsigned long ep)
{
    idtfp = fp;
}

void
IdtOutput (unsigned long addr, unsigned char byte)
{
    if (addr != idtaddr + idtlen || idtlen == BUFSIZE) {
	idtFlush ();
	idtaddr = addr;
    }
    idtbuf[idtlen++] = byte;
}

void
IdtEnd (unsigned long ep)
{
    unsigned char cksum = 0;
    int n;

    idtFlush ();
    
    fprintf (idtfp, "S7%c", ADDRSIZE + 1);
    cksum += ADDRSIZE + 1;

    for (n = (ADDRSIZE - 1) * 8; n >= 0; n -= 8) {
	unsigned char ab = ep >> n;
	fputc (ab, idtfp);
	cksum += ab;
    }

    fputc (~cksum & 0xff, idtfp);
}
