/* Microchip CCI Backend
   Copyright (C) 2018
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/*
 *  Common CCI Backend definitions for Microchip Compilers
 */

#include <stdio.h>
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "cpplib.h"
#include "target.h"

#define CCI_H "config/mchp-cci/cci.h"
#include CCI_H

#define CCI__(x) CCI_##x
#define CCI_(x) CCI__(x)

#define CCI(kind,ccikey,tgtkey,n) \
  { CCI_(kind),ccikey,(intptr_t)tgtkey,n },

static const struct CCIext {
  CCI_KIND   kind;
  const char *ccikey;
  intptr_t   tgtkey;
  int        n;
} CCIext[] = {
#include CCI_H
};

static void
cci_define(struct cpp_reader *pfile, const char *keyword, const char *target)
{
  gcc_assert (keyword);

  const size_t key_len = strlen (keyword);
  const size_t tgt_len = target ? strlen (target) : 0;

  /* xmalloc either succeedes or doesn't return so no need to check for success */
  char *buffer = (char *)xmalloc (key_len + tgt_len + 2);

  /* set buffer to a "keyword=target" string (target may be empty/missing) */
  memcpy (buffer, keyword, key_len);
  buffer[key_len] = '=';
  if (tgt_len)
    memcpy (buffer + key_len + 1, target, tgt_len);
  buffer[key_len + tgt_len + 1] = '\0';

  cpp_define (pfile, buffer);
  free (buffer);
}

static void
cci_attribute(struct cpp_reader *pfile, const char *keyword, const char *target,
              int varargs, int n)
{
  int params_specified = 0;
  char *buffer=NULL,*c=NULL,*tmp=NULL;
  int size;
  int i;

  gcc_assert (keyword && target);

  // check for a couple of special cases
  if (!strcmp (target, TGT_UNSUPPORTED)) {
#define TGT_MSG "unsupported(\"%s is unsupported on this platform\")"
    tmp = (char *)xcalloc (sizeof(TGT_MSG)+strlen(keyword), sizeof(char));
    sprintf (tmp, TGT_MSG, keyword);
#undef TGT_MSG
    target = tmp;
  }
  else if(!strcmp (target, TGT_ERROR)) {
#define TGT_MSG "target_error(\"%s is unsupported on this platform\")"
    tmp = (char *)xcalloc (sizeof(TGT_MSG)+strlen(keyword), sizeof(char));
    sprintf (tmp, TGT_MSG, keyword);
#undef TGT_MSG
    target = tmp;
  }
  if (n) {
    for (const char *c = target; *c; c++) {
      if ((*c == 'P') && (c[1] >= '0') && (c[1] <= '9')) {
        params_specified=1;
        break;
      }
    }
  }
  size = strlen(keyword)+1+sizeof("=__attribute__(())")+strlen(target)+1;
  if (n) {
    if (params_specified == 0) {
      size += 8 * n;  /* up to 99 params: Pnn, */
    } else {
      size += strlen(target) + strlen("()\n");
    }
  }
  if (varargs) size += strlen("=()(),...__VA_ARGS__")+1;
  buffer = (char *)xcalloc(size,sizeof(char));
  c = buffer;
  c += sprintf(c,"%s",keyword);
  if (n || varargs) {
    *c++ = '(';
    for (i = 1; i <= n; i++) {
      c += sprintf(c, "P%d", i);
      if (i < n) *c += ',';
    }
    if (varargs) {
      if (n) *c++=',';
      c += sprintf(c, "...");
    }
    *c++ = ')';
  }
  c += sprintf(c, "=__attribute__((%s", target);
  if ((n || varargs) && !params_specified) {
    *c++ = '(';
    for (i = 1; i <= n; i++) {
      c += sprintf(c, "P%d", i);
      if (i < n) *c += ',';
    }
    if (varargs) {
      if (n) *c++=',';
      c += sprintf(c, "__VA_ARGS__");
    }
    *c++ = ')';
  }
  *c++ = ')';
  *c++ = ')';
  *c++ = 0;

  if (buffer) {
    cpp_define(pfile, buffer);
    free(buffer);
  }
  free(tmp);
}

/*
 * called from SUBTARGET_CPU_CPP_BUILTINS
 */
void mchp_init_cci (struct cpp_reader *pfile) {
  if (!CCI_MCHP)
    return;

  for (unsigned i = 0; i != sizeof(CCIext)/sizeof(*CCIext); ++i) {
    const struct CCIext &ext = CCIext[i];
    switch (ext.kind) {
    case CCI_define:
      cci_define (pfile, ext.ccikey, (const char *)ext.tgtkey);
      break;
    case CCI_attribute:
    case CCI_attribute_n:
      cci_attribute (pfile, ext.ccikey, (const char *)ext.tgtkey, 0,
                     (ext.kind == CCI_attribute ? 0 : ext.n));
      break;
    case CCI_attribute_v:
    case CCI_attribute_nv:
      cci_attribute (pfile, ext.ccikey, (const char *)ext.tgtkey, 1,
                     (ext.kind == CCI_attribute_v ? 0 : ext.n));
      break;
    default:
      gcc_unreachable ();
    }
  }
}
