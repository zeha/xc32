#ident "sde/rtpxlite/rtpxstub.c: Copyright (c) MIPS Technologies, Inc. All rights reserved."

#define SDEMDI_GLUE

/*
 * [[MIPSTHREADS unpublished work copyright (c) MIPS Technologies, Inc.]]
 */


/* 
 * Pthread stubs for pre-MIPSr2 implementations
 */

#if __mips != 32 || __mips_isa_rev < 2
#include <sys/types.h>
#include <mips/cpu.h>
#include <mips/mt.h>
#include <mips/atomic.h>

#include <sys/errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>

#pragma weak _pthread_init
void _pthread_init (void);
void _pthread_init()
{
}

#pragma weak pthread_self=_stub_pthread_self
pthread_t _stub_pthread_self (void);
pthread_t _stub_pthread_self (void)
{
  return 0;
}

#pragma weak pthread_join=_stub_pthread_join
pthread_t _stub_pthread_join (pthread_t, void **);
pthread_t _stub_pthread_join (pthread_t thread, void **thread_return)
{
  return 0;
}

int _stub_pthread_attr_setscope (pthread_attr_t *, int);
int _stub_pthread_attr_setscope (pthread_attr_t *attr, int scope)
{
  return 0;
}

#pragma weak pthread_create=_stub_pthread_create
int _stub_pthread_create(pthread_t *, pthread_attr_t *attr, void *(*)(void *), void *);
int _stub_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void * arg)
{
  return 0;
}

#pragma weak pthread_exit=_stub_pthread_exit
int _stub_pthread_exit(void *);
int _stub_pthread_exit(void *retval)
{
  return 0;
}

#pragma weak pthread_cond_wait=_stub_pthread_cond_wait
int _stub_pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int _stub_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  return 0;
}

#pragma weak pthread_cond_init=_stub_pthread_cond_init
int _stub_pthread_cond_init(pthread_cond_t *, pthread_condattr_t *);
int _stub_pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr)
{
  return 0;
}

#pragma weak pthread_mutexattr_init=_stub_pthread_mutexattr_init
int _stub_pthread_mutexattr_init (pthread_mutexattr_t *);
int _stub_pthread_mutexattr_init (pthread_mutexattr_t *mxa)
{
  return 0;
}

#pragma weak pthread_mutexattr_destroy=_stub_pthread_mutexattr_destroy
int _stub_pthread_mutexattr_destroy (pthread_mutexattr_t *);
int _stub_pthread_mutexattr_destroy (pthread_mutexattr_t *mxa)
{
  return 0;
}

#pragma weak pthread_mutexattr_setname_np=_stub_pthread_mutexattr_setname_np
int _stub_pthread_mutexattr_setname_np (pthread_mutexattr_t *, const char *);
int _stub_pthread_mutexattr_setname_np (pthread_mutexattr_t *mxa, const char *name)
{
  return 0;
}

#pragma weak pthread_mutexattr_settype_np=_stub_pthread_mutexattr_settype_np
int _stub_pthread_mutexattr_settype_np (pthread_mutexattr_t *, int);
int _stub_pthread_mutexattr_settype_np (pthread_mutexattr_t *mxa, int type)
{
  return 0;
}

#pragma weak pthread_mutex_init
int pthread_mutex_init(pthread_mutex_t *mx,const pthread_mutexattr_t *mxa)
{
  return 0;
}

#pragma weak pthread_mutex_destroy
int pthread_mutex_destroy(pthread_mutex_t *mx)
{
  return 0;
}

#pragma weak pthread_mutex_lock
int pthread_mutex_lock(pthread_mutex_t *mx)
{
  return 0;
}

#pragma weak pthread_mutex_unlock
int pthread_mutex_unlock(pthread_mutex_t *mx)
{
  return 0;
}

#pragma weak _pthread_cleanup_push
void _pthread_cleanup_push (struct _pthread_handler_rec *rec)
{
}

#pragma weak _pthread_cleanup_pop
void _pthread_cleanup_pop (struct _pthread_handler_rec *rec)
{
}

#pragma weak _pthread_cleanup_push
void _pthread_cleanup_push_defer (struct _pthread_handler_rec *rec)
{
}

#pragma weak _pthread_cleanup_pop
void _pthread_cleanup_pop_restore (struct _pthread_handler_rec *rec)
{
}



#pragma weak _pthread_once=_stub_pthread_once
#pragma weak pthread_once=_stub_pthread_once
int _stub_pthread_once (pthread_once_t *, void (*)(void));
int _stub_pthread_once (pthread_once_t *once, void (*fn)(void))
{
    if (once->state == PTHREAD_ONCE_NEVER) {
	once->state = PTHREAD_ONCE_DONE;
	(*fn) ();
    }
    return 0;
}


typedef struct stub_tsd {
    int		tsd_used;
    void	*tsd_value;
} stub_tsd_t;

#define STUB_KEYS_MAX	10
static  stub_tsd_t tsd[STUB_KEYS_MAX];

#pragma weak pthread_key_create=_stub_pthread_key_create
int
_stub_pthread_key_create (pthread_key_t * key, void (*destructor) (void *))
{
    stub_tsd_t     *tsdp;

    for (tsdp = tsd; tsdp < &tsd[STUB_KEYS_MAX]; tsdp++)
	if (!tsdp->tsd_used) {
	    tsdp->tsd_used = 1;	/* exists but no value */
	    *key = tsdp - tsd;
	    return 0;
	}
    return EAGAIN;
}

#pragma weak pthread_key_delete=_stub_pthread_key_delete
int
_stub_pthread_key_delete (pthread_key_t key)
{
    stub_tsd_t     *tsdp;

    if (key >= STUB_KEYS_MAX)
	return EINVAL;

    tsdp = &tsd[key];

    if (!tsdp->tsd_used)
	/* no longer valid */
	return EINVAL;

    if (tsdp->tsd_value)
	/* value still exists */
	return EBUSY;

    tsdp->tsd_used = 0;
    tsdp->tsd_value = NULL;
    return 0;
}


#pragma weak pthread_setspecific=_stub_pthread_setspecific
int
_stub_pthread_setspecific (pthread_key_t key, void *value)
{
    stub_tsd_t     *tsdp;

    if (key >= STUB_KEYS_MAX)
	return EINVAL;

    tsdp = &tsd[key];
    if (!tsdp->tsd_used)
	return EINVAL;

    tsdp->tsd_value = value;
    return 0;
}

#pragma weak pthread_getspecific=_stub_pthread_getspecific
void           *
_stub_pthread_getspecific (pthread_key_t key)
{
    if (key >= STUB_KEYS_MAX)
	return NULL;
    return tsd[key].tsd_value;
}

#endif /* __mips != 32 || __mips_isa_rev < 2 */
