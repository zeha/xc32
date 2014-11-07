#ident "sde/rtpxlite/rtpxstub.c: Copyright (c) MIPS Technologies, Inc. All rights reserved."

/* 
 * __sdethread stubs for pre-MIPSr2 implementations
 */

#if __mips != 32 || __mips_isa_rev < 2

#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>

#include <errno.h>
#undef errno

#include <libc_thread.h>
#include <sdethread.h>

#pragma weak __isthreaded
int __isthreaded = 0;

#pragma weak _pthread_init
void _pthread_init (void);
void _pthread_init()
{}

int
__sdethread_mutex_init (__sdethread_mutex_t *mx, const char *name)
{
  return 0;
}

int
__sdethread_mutex_destroy (__sdethread_mutex_t *mx)
{
  return 0;
}

int
__sdethread_mutex_lock (__sdethread_mutex_t *mx)
{
  return 0;
}

int
__sdethread_mutex_trylock (__sdethread_mutex_t *mx)
{
  return 0;
}

int
__sdethread_mutex_unlock (__sdethread_mutex_t *mx)
{
  return 0;
}

void
__sdethread_sigdisable (__sdethread_sigstate_t *state) 
{/* no signals */}

void
__sdethread_sigrestore (const __sdethread_sigstate_t *state) 
{/* no signals */}

int
___sdethread_once (__sdethread_once_t *once, void (*fn)(void))
{
  if (once->so_state == __SDETHREAD_ONCE_NEVER) {
    once->so_state = __SDETHREAD_ONCE_DONE;
    (*fn) ();
  }
  return 0;
}

#undef __sdethread_once
int __sdethread_once (__sdethread_once_t *oncep, void (*funcp)(void))
{
  return ___sdethread_once (oncep, funcp);
}

int *
__sdethread_errno_pointer (void)
{
  return &errno;
}

__attribute__((__nomips16__)) 
int
__sdethread_set_errno (int err, int rv)
{
  errno = err;
  return rv;
}

void *
_libc_private_storage (struct _thread_private_key_struct *key,
		       void *storage, size_t size, void *err)
{
  /* return static storage only */
  return storage;
}

#include <unistd.h>

pid_t
getpid (void)
{
  return 1;
}


pid_t fork(void)
{
  errno = ENOSYS;
  return -1;
}

int execl (const char *path, const char *arg, ...)
{
  errno = ENOSYS;
  return -1;
}

int execv (const char *path, const char * const *argv)
{
  errno = ENOSYS;
  return -1;
}

#include <sys/wait.h>

pid_t	waitpid (pid_t pid, int *status, int options)
{
  errno = ENOSYS;
  return -1;
}

#endif /* __mips != 32 || __mips_isa_rev < 2 */
