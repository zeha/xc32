/* MIPS SDE threads compatibility routines for libgcc2 and libobjc.  */
/* Compile this one with gcc.  */
/* Copyright (C) 2006 Free Software Foundation, Inc.
   Contributed by Nigel Stephens <nigel@mips.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

#ifndef GCC_GTHR_MIPSSDE_H
#define GCC_GTHR_MIPSSDE_H

#ifdef _LIBOBJC

/* libobjc requires the full-blown pthreads.  */
#include "gthr-posix.h"

#else

/* MIPS SDE threading API specific definitions.
   Easy, since the interface is pretty much one-to-one.  */

#define __GTHREADS 1

#include <sdethread.h>
#include <unistd.h>

typedef __sdethread_key_t __gthread_key_t;
typedef __sdethread_once_t __gthread_once_t;
typedef __sdethread_mutex_t __gthread_mutex_t;

#define __GTHREAD_MUTEX_INIT __SDETHREAD_MUTEX_INITIALIZER("gthr")
#define __GTHREAD_ONCE_INIT __SDETHREAD_ONCE_INIT

#if SUPPORTS_WEAK && GTHREAD_USE_WEAK

#pragma weak __sdethread_once
#pragma weak ___sdethread_once
#pragma weak __sdethread_key_create
#pragma weak __sdethread_key_delete
#pragma weak __sdethread_getspecific
#pragma weak __sdethread_setspecific
#pragma weak __sdethread_mutex_lock
#pragma weak __sdethread_mutex_trylock
#pragma weak __sdethread_mutex_unlock
#pragma weak __sdethread_threading

static inline int
__gthread_active_p (void)
{
  if (&__sdethread_threading)
    return 1;
  return 0;
}

#else /* not SUPPORTS_WEAK */

static inline int
__gthread_active_p (void)
{
  return 1;
}

#endif /* SUPPORTS_WEAK */

static inline int
__gthread_once (__gthread_once_t *once, void (*func) (void))
{
  if (__gthread_active_p ())
    return __sdethread_once (once, func);
  else
    return -1;
}

static inline int
__gthread_key_create (__gthread_key_t *key, void (*dtor) (void *))
{
  return __sdethread_key_create (key, dtor);
}

static inline int
__gthread_key_delete (__gthread_key_t key)
{
  return __sdethread_key_delete (key);
}

static inline void *
__gthread_getspecific (__gthread_key_t key)
{
  return __sdethread_getspecific (key);
}

static inline int
__gthread_setspecific (__gthread_key_t key, const void *ptr)
{
  return __sdethread_setspecific (key, ptr);
}

static inline int
__gthread_mutex_lock (__gthread_mutex_t *mutex)
{
  if (__gthread_active_p ())
    return __sdethread_mutex_lock (mutex);
  else
    return 0;
}

static inline int
__gthread_mutex_trylock (__gthread_mutex_t *mutex)
{
  if (__gthread_active_p ())
    return __sdethread_mutex_trylock (mutex);
  else
    return 0;
}

static inline int
__gthread_mutex_unlock (__gthread_mutex_t *mutex)
{
  if (__gthread_active_p ())
    return __sdethread_mutex_unlock (mutex);
  else
    return 0;
}

#endif /* _LIBOBJC */

#endif /* ! GCC_GTHR_MIPSSDE_H */
