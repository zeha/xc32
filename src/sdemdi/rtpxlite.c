#ident "sde/rtpxlite/rtpxlite.c: Copyright (c) MIPS Technologies, Inc. All rights reserved."

#define SDEMDI_GLUE

/*
 * [[MIPSTHREADS unpublished work copyright (c) MIPS Technologies, Inc.]]
 */


/* 
 * Pthread-lite for MT ASE
 *
 * Only intended as an example of how to use the low-level MT facilities,
 * not as real production code.
 *
 * It is only ever going to work on a CPU implementing the MT ASE with
 * NumTC thread contexts.
 *
 * Only supports:
 *   Up to NumTC threads (creating more will cause fatal exception)
 *   Fixed size thread stacks
 *   Simple fast mutexes
 *   Condition variables
 *   Thread specific data
 *   Integer code
 * Does not support:
 *   FPU, COP2, UDI or DSP ASE
 *   Thread priority or schedparams
 *   Timers
 *   Signals
 *   Semaphores
 *   Thread cancellation
 *   Mutex attributes
 *   Thread attributes
 *   etc.
 */

#if __mips == 32 && __mips_isa_rev >= 2
#include <sys/types.h>
#include <mips/cpu.h>
#include <mips/mt.h>
#include <mips/atomic.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sdethread.h>
#include <sched.h>

#if ! __mips16
#define RTPX_INLINE inline
#else /* __mips16 */
#define RTPX_INLINE __attribute__((noinline,nomips16))
#endif /* __mips16 */

/* So that the local copies are used. */
#undef _m32c0_mfc0
#undef _m32c0_mtc0
#undef _mips_mfc0
#undef _mips_mtc0
#undef _m32c0_mftc0
#undef _m32c0_mttc0
#undef _m32c0_mttgpr
#undef mips32_jr_hb
#undef _mips_atomic_update
#undef _mips_atomic_cas

extern void _pthread_mt_fork (void *);
extern void _pthread_mt_fork_child (void *);

#define _m32c0_mfc0(reg, sel)					\
__extension__ ({						\
	register unsigned long __r;				\
	__asm__ __volatile__ (					\
		"mfc0 %0, $%1, %2"				\
		: "=d" (__r)					\
		: "JK" (reg), "JK" (sel));			\
	__r;							\
})

#define _m32c0_mtc0(reg, sel, val)				\
do {								\
	__asm__ __volatile__ (					\
		"%(mtc0 %z0, $%1, %2; ehb%)"			\
		:						\
		: "dJ" ((reg32_t)(val)), "JK" (reg), "JK" (sel)	\
		: "memory");					\
} while (0)

#define _mips_mfc0(r)		_m32c0_mfc0(r, 0)
#define _mips_mtc0(r, v)	_m32c0_mtc0(r, 0, v)

#define _m32c0_mftc0(rt, sel)					\
__extension__ ({						\
	register unsigned long __r;				\
	__asm__ __volatile__(					\
		".set push		\n"			\
		".set mt		\n"			\
		"mftc0 %0, $%1, %2	\n"			\
		".set pop		"			\
		: "=d" (__r)					\
		: "JK" (rt), "JK" (sel));			\
	__r;							\
})

#define _m32c0_mttc0(rd, sel, val)				\
do {								\
        __asm__ __volatile__(					\
		".set push			\n"		\
		".set mt			\n"		\
                "%(mttc0 %z0, $%1, %2; ehb%)	\n"		\
		".set pop			"		\
                :						\
		: "dJ" ((reg32_t)(val)), "JK" (rd), "JK" (sel)	\
		: "memory");					\
} while (0)

#define _m32c0_mttgpr(rd, val)					\
do {								\
        __asm__ __volatile__(					\
		".set push			\n"		\
		".set mt			\n"		\
                "%(mttgpr %z0, $%1; ehb%)	\n"		\
		".set pop			"		\
                :						\
		: "dJ" ((reg32_t)(val)), "JK" (rd)		\
		: "memory");					\
} while (0)

#define mips32_jr_hb()						\
	__asm__ __volatile__(					\
	"	bltzal	$0,0f\n"				\
	"0:	addiu	$31,1f-0b\n"				\
	"	jr.hb	$31\n"					\
	"1:"							\
		: : : "$31")

static RTPX_INLINE void
rtpx_jr_hb (void)
{
	mips32_jr_hb ();
}

static RTPX_INLINE unsigned int
rtpx_mt_dmt (void)
{
	return mips_mt_dmt ();
}

static RTPX_INLINE unsigned int
rtpx_mt_emt (void)
{
	return mips_mt_emt ();
}

static RTPX_INLINE unsigned int
rtpx_mt_dvpe (void)
{
	return mips_mt_dvpe ();
}

static RTPX_INLINE unsigned int
rtpx_mt_evpe (void)
{
	return mips_mt_evpe ();
}

static RTPX_INLINE unsigned int
rtpx_mt_yield (unsigned int yq)
{
	return mips_mt_yield (yq);
}

static inline void
rtpx_atomic_delay(void)
{
	int __r0, __r1;

	__asm__ __volatile__ (
	"	mfc0	%0,$2,2		# cp0.tcbind\n"
	"	mfc0	%1,$9		# cp0.count\n"
	"	srl	%0,21\n"
	"	xor	%0,%1\n"
	"	srl	%1,%0,4\n"
	"	xor	%0,%1\n"
	"	and	%0,0xf\n"
	"0:	%(bnez	%0,0b\n"
	"	 sub	%0,1%)"
		: "=r" (__r0), "=r" (__r1));
}

/* atomic r-m-w worker functions */
static RTPX_INLINE unsigned int
rtpx_atomic_update (volatile unsigned int *wp,
		     unsigned int clear,
		     unsigned int addend)
{
	unsigned int __old, __sc;

	while (1) {
		__asm__ __volatile__ (
		"	ll	%0,%1\n"	
		"	and	%2,%0,%3\n"	
		"	addu	%2,%4\n"	
		"	sc	%2,%1\n"	
			: "=&d" (__old), "+m" (*(wp)), "=&d" (__sc)
			: "dJK" (~(clear)), "dJI" (addend)); 
		if (__builtin_expect(__sc != 0, 1))
			break;
		rtpx_atomic_delay();
	}
	return __old;
}

static RTPX_INLINE unsigned int
rtpx_atomic_cas (volatile unsigned int *wp, unsigned int new, unsigned int cmp)
{
	unsigned int __old, __sc;

	while (1) {
		__asm__ __volatile__ (	
		"	ll	%0,%1\n"	
		"	%(bne	%0,%z3,0f\n"	
		"	move	%2,%z4%)\n"	
		"	sc	%2,%1\n"	
		"0:"				
			: "=&d" (__old), "+m" (*(wp)), "=&d" (__sc)
			: "dJ" (cmp), "dJ" (new)); 
		if (__old != cmp || __builtin_expect(__sc != 0, 1))
			break;
		rtpx_atomic_delay();
	}
	return __old; 
}

#define _mips_atomic_update rtpx_atomic_update
#define _mips_atomic_cas rtpx_atomic_cas

/* Set when threading enabled */
#pragma weak __isthreaded
int __isthreaded = 0;

/* THREADING ACTIVE FLAG */
#pragma weak __sdethread_threading
int __sdethread_threading = 0;

//#define RTPX_ITC_AVAIL			/* Use ITC, else use LL/SC & spin */
#define	RTPX_THREADS_MAX	9 		/* Maximum number of threads */
#define RTPX_KEYS_MAX		16		/* Maximum number of thread specific data keys */
#define RTPX_STACK_SIZE		32768		/* Fixed stack size */

/* MIPS MT ITC hardware view as 32 or 64-bit object. */
typedef union {
    uint32_t		iv32;
    uint64_t		iv64;
} mips_itc_view_t;

/* MIPS MT ITC hardware cell */
struct mips_itc_cell {
    mips_itc_view_t	bypass;		/* Bypass view */
    mips_itc_view_t 	status;		/* Status view */
    mips_itc_view_t	ef;		/* E/F Mutex blocking view */
    mips_itc_view_t	ef_try;		/* E/F Mutex non-blocking view */
    mips_itc_view_t	pv;		/* P/V Semaphore blocking view */
    mips_itc_view_t	pv_try;		/* P/V Semaphore non-blocking view */
    mips_itc_view_t	rsvd_0;		/* Reserved for MIPS MT */
    mips_itc_view_t	rsvd_1;		/* Reserved for MIPS MT */
    mips_itc_view_t	impl_0;		/* Implementation specific */
    mips_itc_view_t	impl_1;		/* Implementation specific */
    mips_itc_view_t	impl_2;		/* Implementation specific */
    mips_itc_view_t	impl_3;		/* Implementation specific */
    mips_itc_view_t	impl_4;		/* Implementation specific */
    mips_itc_view_t	impl_5;		/* Implementation specific */
    mips_itc_view_t	impl_6;		/* Implementation specific */
    mips_itc_view_t	impl_7;		/* Implementation specific */
};

typedef volatile struct mips_itc_cell mips_itc_cell_t;

/* MIPS MT ITC status view bits */
#define MIPS_ITC_STATUS_RF	0x40000	/* Reset fifo */
#define MIPS_ITC_STATUS_FIFO	0x20000	/* Is a fifo */
#define MIPS_ITC_STATUS_T	0x10000	/* Trap enable */
#define MIPS_ITC_STATUS_FULL	0x00002	/* Fifo full */
#define MIPS_ITC_STATUS_EMPTY	0x00001	/* Fifo empty */

/* RTPX ITC free list */
struct rtpx_itc {
    struct rtpx_itc	*next;		/* next ITC in free list */
    mips_itc_cell_t	*cell;		/* hardware ITC cell pointer */
};

typedef struct rtpx_itc rtpx_itc_t;

#ifdef RTPX_ITC_AVAIL
static rtpx_itc_t *rtpx_itc_list;
#endif


/* RTPX simple mutex
   Using ITC if available, else a software spinlock. */
struct rtpx_mutex {
#ifdef RTPX_ITC_AVAIL
    rtpx_itc_t		*itc;
#else
    volatile uint32_t	flags;
#   define RTPX_MUTEX_LOCKED	0x0001
#endif
};

typedef struct rtpx_mutex rtpx_mutex_t;

/* RTPX semaphore
   Using ITC if available, else as a software spin wait. */
struct rtpx_sem {
#ifdef RTPX_ITC_AVAIL
    rtpx_itc_t		*itc;
#else
    volatile int32_t	count;
#endif
};

typedef struct rtpx_sem rtpx_sem_t;

/* 
 * RTPX thread state
 *
 * One per TC, pointed to by its TCContext register.
 */
struct _rtpx_thread {
    /* hooks used by thread creation primitive in rtpxasm.S */
    void		*th_stktop;		/* top of stack */
    void		(*th_start)(struct _rtpx_thread *);

    unsigned int	th_flags;		/* thread state flags */
#define RTPX_THF_ALIVE			0x01	
#define RTPX_THF_EXITING		0x02	
#define RTPX_THF_ZOMBIE			0x04	

    unsigned short	th_index;		/* thread table index */
    unsigned short 	th_gen;			/* thread generation number */

    unsigned short	th_vpe;			/* hardware vpe number */
    unsigned short	th_tc;			/* hardware tc number */

    pthread_attr_t	th_tha;			/* thread attributes */

    int			th_errno;		/* XXX per-thread errno */

    struct _rtpx_waitq	*th_waitq;		/* wait queue head */
    struct _rtpx_thread	*th_next;		/* wait queue link */
    rtpx_sem_t		th_wait_sem;		/* wait semaphore */

    rtpx_sem_t		th_join_sem;		/* for join wait */
    rtpx_sem_t		th_exit_sem;		/* for exit wait */
    int			th_joining;		/* join in progress */
    void		*th_jval;		/* join return value */

    rtpx_sem_t		th_child_sem;		/* for thread create ack */
    const pthread_attr_t *th_child_tha;		/* new child's attributes */
    void 		*(*th_child_fun)(void *);/* new child's start func */
    void 		*th_child_arg;		/* new child's start arg */
    pthread_t		th_child_id;		/* new child's id - returned to parent */

    const void		*th_tsd[RTPX_KEYS_MAX];	/* thread-specific data pointers */
    unsigned int	*th_tsdcount;		/* number of values in thread */

    /* fixed size thread stack, forcing 64-bit alignment */
    uint64_t		th_stack[RTPX_STACK_SIZE / sizeof(uint64_t)];
};

typedef struct _rtpx_thread rtpx_thread_t;

/* Return pthread_t software id for an internal thread */
#define rtpx_pthread_t(th) ((pthread_t)(th))

/* Active thread table */
static rtpx_thread_t rtpx_threads[RTPX_THREADS_MAX];
static volatile unsigned int rtpx_num_threads = 0;
static rtpx_mutex_t rtpx_thread_mx;

/* Default mutex attributes */
static pthread_mutexattr_t default_mxa;

/* Default condition variable attributes */
static pthread_condattr_t default_cda;

/* Default thread attributes */
static pthread_attr_t default_tha;


static RTPX_INLINE unsigned int
rtpx_mt_acquire (void)
{
    extern int _smtc;

    if (_smtc)
	return (rtpx_mt_dvpe () & MVPCONTROL_EVP) != 0;
    else
	return (rtpx_mt_dmt () & VPECONTROL_TE) != 0;
}

static RTPX_INLINE unsigned int
rtpx_mt_release (void)
{
    extern int _smtc;

    if (_smtc)
	return (rtpx_mt_evpe () & MVPCONTROL_EVP) != 0;
    else
	return (rtpx_mt_emt () & VPECONTROL_TE) != 0;
}

static RTPX_INLINE void
rtpx_mt_exit (void)
{
    extern int _smtc;

    if (_smtc) {
	mips32_settcstatus (mips32_gettcstatus () & ~TCSTATUS_A);
	rtpx_jr_hb ();
    } else
	rtpx_mt_yield (0);
}

static RTPX_INLINE int
rtpx_mt_fork_by_hand (rtpx_thread_t *th, int tc)
{
    unsigned int tcstatus;
    int status = 1;

    mips32_mt_settarget (tc);
    rtpx_mt_dvpe ();
    mips32_mt_settchalt (TCHALT_H);
    rtpx_jr_hb ();

    tcstatus = mips32_mt_gettcstatus ();
    if (!(tcstatus & TCSTATUS_A)) {
	status = 0;
	_m32c0_mttgpr (4, th);
	mips32_mt_settcrestart (_pthread_mt_fork_child);
	mips32_mt_settcstatus (tcstatus | TCSTATUS_A);
    }

    mips32_mt_settchalt (0);
    rtpx_mt_evpe ();

    return status;
}

static RTPX_INLINE void
rtpx_mt_fork (rtpx_thread_t *th)
{
    extern int _smtc;
    int i;

    if (_smtc)
	for (i = 0; i < RTPX_THREADS_MAX; i++) {
	    if (!rtpx_threads[i].th_stktop)
		break;
	    if (rtpx_threads[i].th_vpe == th->th_vpe)
		continue;
	    if (rtpx_mt_fork_by_hand (th, rtpx_threads[i].th_tc) == 0)
		return;
	}
    _pthread_mt_fork (th);
}

/* Return own thread context data, from TC context register */
static RTPX_INLINE rtpx_thread_t *
rtpx_self (void)
{
    uint32_t context = mips32_gettccontext();
    return (rtpx_thread_t *) context;
}

/* Find rtpx thread context matching pthread id. */
/* rtpx_thread_mx mutex must be held when this is called. */
static rtpx_thread_t *
rtpx_find_thread (pthread_t id)
{
    rtpx_thread_t  *ath = (rtpx_thread_t *) id;
    if (ath < rtpx_threads || ath >= &rtpx_threads[RTPX_THREADS_MAX]
	|| ! (ath->th_flags & RTPX_THF_ALIVE))
	return NULL;
    return ath;
}

#ifdef RTPX_ITC_AVAIL

/* Allocate an ITC off the free list, else return NULL. */
static rtpx_itc_t *
rtpx_itc_alloc (void)
{
    int released = rtpx_mt_acquire ();
    rtpx_itc_t *itc = rtpx_itc_list;
    if (itc)
	rtpx_itc_list = itc->next;
    if (released)
	rtpx_mt_release ();
    return itc;
}

/* Return an ITC to the free list. */
static void
rtpx_itc_free (rtpx_itc_t *itc)
{
    int released = rtpx_mt_acquire ();
    itc->next = rtpx_itc_list;
    rtpx_itc_list = itc;
    if (released)
	rtpx_mt_release ();
}
#endif

/* 
 * Implement a counting semaphore using either ITC or LL/SC 
 */

static int
rtpx_sem_init (rtpx_sem_t *rsem, int init)
{
#ifdef RTPX_ITC_AVAIL
    if (!(rsem->itc = rtpx_itc_alloc ()))
	return ENOSPC;
    rsem->itc->cell->bypass.iv32 = init;	/* initial value */
    rsem->itc->cell->status.iv32 = 0;		/* not empty or full */
#else
    rsem->count = init;
#endif
    return 0;
}

static int
rtpx_sem_destroy (rtpx_sem_t *rsem)
{
#ifdef RTPX_ITC_AVAIL
    rtpx_itc_free (rsem->itc);
#endif
    return 0;
}

static int
rtpx_sem_wait (rtpx_sem_t *rsem)
{
#ifdef RTPX_ITC_AVAIL
    if (rsem->itc->cell->pv.iv32 == 0)	/* atomic decrement */
	return EINVAL;		/* shouldn't happen! */
#else
    /* spin until count was positive */
    while ((int)mips_atomic_dec (&rsem->count) <= 0) {
	/* cleanup bad decrement */
	mips_atomic_inc (&rsem->count);
	if (__isthreaded)
	    rtpx_mt_yield (-1);
    }
#endif
    return 0;
}

#if notused
static int
rtpx_sem_trywait (rtpx_sem_t *rsem)
{
#ifdef RTPX_ITC_AVAIL
    if (rsem->itc->cell->pv_try.iv32 == 0)	/* atomic decrement */
	return EAGAIN;
    return 0;
#else
    /* succeed if count was positive */
    if ((int)mips_atomic_dec (&rsem->count) <= 0) {
	/* cleanup on failure */
	(void) mips_atomic_inc (&rsem->count);
	return EAGAIN;
    }
#endif
    return 0;
}
#endif /* notused */

static void
rtpx_sem_signal (rtpx_sem_t *rsem)
{
#ifdef RTPX_ITC_AVAIL
    lock->itc->cell->pv.iv32 = 1;		/* increment */
#else
    (void) mips_atomic_inc (&rsem->count);
#endif
}

/* 
 * Implement a simple mutex using either ITC or LL/SC 
 */

static int
rtpx_mutex_init (rtpx_mutex_t *rmx)
{
#ifdef RTPX_ITC_AVAIL
    if (!(rmx->itc = rtpx_itc_alloc ()))
	return ENOSPC;
    rmx->itc->cell->bypass.iv32 = 1;		/* initial value */
    rmx->itc->cell->status.iv32 = MIPS_ITC_STATUS_FULL;
#else
    rmx->flags = 0;
#endif
    return 0;
}

static int
rtpx_mutex_destroy (rtpx_mutex_t *rmx)
{
#ifdef RTPX_ITC_AVAIL
    rtpx_itc_free (rmx->itc);
#endif
    return 0;
}

static int
rtpx_mutex_lock (rtpx_mutex_t *rmx)
{
#ifdef RTPX_ITC_AVAIL
    if (rmx->itc->cell->ef.iv32 == 0)	/* block until full */
	return EINVAL;			/* shouldn't happen */
#else
    /* spin until not locked */
    while (mips_atomic_bis (&rmx->flags, RTPX_MUTEX_LOCKED) != 0) {
	if (__isthreaded)
	    rtpx_mt_yield (-1);
    }
#endif
    return 0;
}

static int
rtpx_mutex_trylock (rtpx_mutex_t *rmx)
{
#ifdef RTPX_ITC_AVAIL
    if (rmx->itc->cell->ef_try.iv32 == 0)	/* returns 0 if locked */
	return EAGAIN;
#else
    if (mips_atomic_bis (&rmx->flags, RTPX_MUTEX_LOCKED) != 0)
	return EAGAIN;
#endif
    return 0;
}

static int
rtpx_mutex_unlock (rtpx_mutex_t *rmx)
{
#ifdef RTPX_ITC_AVAIL
    rmx->itc->cell->ef.iv32 = 1;		/* fill */
#else
    (void) mips_atomic_bic (&rmx->flags, RTPX_MUTEX_LOCKED);
#endif
    return 0;
}


static void pthread_hoop (rtpx_thread_t *);

/* Thread specific data handling */
static void _rtpx_tsdinit (void);
static void _rtpx_tsdcleanup (void);
static void _rtpx_flockfilecleanup (void);

/* Stdio library file lock destructor */
static void (*rtpx_flock_dtor) (void);


#ifndef __OPTIMIZE__
/* When optimizing the call to sdethread_too_small should be deleted,
   and if not will call a link error.  When not optimizing we need to
   map to a function which does exist and generates a run-time. */
#define __sdethread_mismatch abort
#else
extern void __sdethread_mismatch (void);
#endif

__attribute__((nomips16)) void
_pthread_init (void)
{
    extern int _smtc;
    extern int _vpe;
    unsigned int mvpconf0, tcstatus;
    unsigned int nvpe, ntc, ntc_res;
    unsigned int vpe, tc;
    unsigned int myvpe;
    rtpx_thread_t *th;

    pthread_mutexattr_init (&default_mxa);
    pthread_condattr_init (&default_cda);
    pthread_attr_init (&default_tha);

    /* Make sure that we can store the underlying pthread data
       structures inside the sdethread data structures */
    if (sizeof (__sdethread_mutex_t) < sizeof (pthread_mutex_t)
	|| sizeof (__sdethread_once_t) < sizeof (pthread_once_t)
	|| sizeof (__sdethread_sigstate_t) < sizeof (pthread_sigstate_np_t)
	|| sizeof (__sdethread_key_t) != sizeof (pthread_key_t))
	__sdethread_mismatch ();

    /* Does this CPU support MT ASE? */
    if (! (mips32_getconfig0 () & CFG0_M)
	|| ! (mips32_getconfig1 () & CFG1_M)
	|| ! (mips32_getconfig2 () & CFG2_M)
	|| ! (mips32_getconfig3 () & CFG3_MT))
	return;			/* no */

    /* Don't attempt to support multi-threading but on VPE 0 for now. */
    myvpe = (mips32_gettcbind () & TCBIND_CURVPE) >> TCBIND_CURVPE_SHIFT;
    if (myvpe)
	return;

#ifdef RTPX_ITC_AVAIL
    /* find all available ITCs and add to ITC free list */
    /* TODO read cache tags to find ITC and scan */
    if (!rtpx_itc_list)
	/* no ITCs found */
	abort ();
#endif

    /* Disable multi-threading altogether */
    rtpx_mt_dvpe ();
    rtpx_mt_dmt ();

    /* Put MVPE into 'configuration state' */
    mips32_setmvpcontrol (mips32_getmvpcontrol () | MVPCONTROL_VPC);

    /* Get the number of VPEs and TCs; ignore other VPEs if requested so. */
    mvpconf0 = mips32_getmvpconf0();
    nvpe = ((mvpconf0 & MVPCONF0_PVPE) >> MVPCONF0_PVPE_SHIFT) + 1;
    ntc = ((mvpconf0 & MVPCONF0_PTC) >> MVPCONF0_PTC_SHIFT) + 1;

    /* Reserve TCs for other VPEs if requested. */
    ntc_res = _vpe ? nvpe : 1;

    /* Start from VPE 0. */
    vpe = 0;

    /* Set the MVP bits. */
    mips32_setvpeconf0 (mips32_getvpeconf0 () | VPECONF0_MVP);

    /* Leave the last VPE - 1 number of TCs intact. */
    for (th = rtpx_threads, tc = 0; tc < ntc - (ntc_res - 1); th++, tc++) {

	if (tc < RTPX_THREADS_MAX) {
	    th->th_vpe = vpe;
	    th->th_tc = tc;
	    th->th_index = tc;		/* could be different */

	    /* initial top of stack for new thread */
	    th->th_stktop = &th->th_stack[sizeof (th->th_stack)
					  / sizeof (th->th_stack[0])];
	    
	    /* thread hoop initialises thread context on thread start */
	    th->th_start = pthread_hoop;
	}

	/* Set up the TC */
	mips32_mt_settarget (tc);
	
	/* Halt TC  */
	if (tc != 0) {
	    /* Don't halt ourself! */
	    mips32_mt_settchalt (TCHALT_H);
	    rtpx_jr_hb ();
	}

	if (tc < RTPX_THREADS_MAX) {
	    /* Set thread context to point to own thread table entry */
	    mips32_mt_settccontext (th);

	    /* Bind TC to VPE. */
	    mips32_mt_settcbind ((vpe << TCBIND_CURVPE_SHIFT)
				 | (tc << TCBIND_CURTC_SHIFT));

	    if (_smtc && vpe != myvpe) {
		/* Set this VPE's KSEG0 to cacheable */
		mips32_mt_setc0config (mips32_getconfig0 ());
		/* Clear pending interrupts, etc. */
		mips32_mt_setc0cause (0);

		/* Enable multi-threading within this VPE */
		mips32_mt_setc0status (0);
		mips32_mt_setvpecontrol (mips32_mt_getvpecontrol () |
					 VPECONTROL_TE);
		/* Enable the VPE */
		mips32_mt_setvpeconf0 (mips32_mt_getvpeconf0 () |
				       VPECONF0_VPA);
	    }
	    
	    tcstatus = mips32_mt_gettcstatus ();
	    /* All TCs operate in kernel mode. */
	    tcstatus &= ~TCSTATUS_TKSU;
	    if (tc != 0)
		/* Not allocated yet. */
		tcstatus &= ~TCSTATUS_A;
	    /* Dynamically allocatable */
	    tcstatus |= TCSTATUS_DA;
	    mips32_mt_settcstatus (tcstatus);

	    /* Unhalt the TC. */
	    if (tc != 0)
		mips32_mt_settchalt (0);
	}

	if (_smtc)
	    vpe = (vpe + 1) % nvpe;
    }

    /* Enable the VPE */
    mips32_setvpeconf0 (mips32_getvpeconf0 () | VPECONF0_VPA);

    /* Release config state */
    mips32_setmvpcontrol (mips32_getmvpcontrol () & ~MVPCONTROL_VPC);

    /* Initialise thread 0 software state */
    th = &rtpx_threads[0];

    /* Set default thread attributes */
    th->th_tha = default_tha;

    /* initialise per-thread semaphores */
    /* XXX could we perhaps do this for all threads in the loop above */
    rtpx_sem_init (&th->th_wait_sem, 0);	/* for cond wait etc */
    rtpx_sem_init (&th->th_join_sem, 0);	/* for join wait */
    rtpx_sem_init (&th->th_exit_sem, 0);	/* for exit wait */
    rtpx_sem_init (&th->th_child_sem, 0);	/* for thread creation */

    /* mark self as alive and findable */
    th->th_flags |= RTPX_THF_ALIVE;
    rtpx_num_threads = 1;
    __isthreaded = 1;
    __sdethread_threading = 1;

    /* initialise thread exit/join/detach mutex */
    rtpx_mutex_init (&rtpx_thread_mx);

    /* initialise thread specific data handling */
    _rtpx_tsdinit ();

    /* all done, let the fun begin */
    rtpx_mt_emt ();
    rtpx_mt_evpe ();
}



/*
 * Pthread creation
 */

/* Called by _pthread_mt_fork() primitive with:
 *   pth = parent thread pointer
 */
static void
pthread_hoop (rtpx_thread_t *pth)
{
    rtpx_thread_t *th = rtpx_self ();
    void *(*fun) (void *);
    void *arg;

    /* initialise per-thread semaphores */
    rtpx_sem_init (&th->th_wait_sem, 0);	/* for cond wait etc */
    rtpx_sem_init (&th->th_join_sem, 0);	/* for join wait */
    rtpx_sem_init (&th->th_exit_sem, 0);	/* for exit wait */
    rtpx_sem_init (&th->th_child_sem, 0);	/* for thread creation */

    /* get our thread attributes from parent thread */
    th->th_tha = *pth->th_child_tha;

    /* get start function and argument from parent thread */
    fun = pth->th_child_fun;
    arg = pth->th_child_arg;
    
    /* mark self as alive and findable */
    th->th_flags |= RTPX_THF_ALIVE;
   
    /* pass our pthread id back to the parent and signal it */
    pth->th_child_id = rtpx_pthread_t (th);
    rtpx_sem_signal (&pth->th_child_sem);

    /* run the new thread, until it returns. */
    pthread_exit ((*fun) (arg));
}


int
pthread_create (pthread_t *rv, const pthread_attr_t *tha,
		void *(*fun) (void *), void *arg)
{
    rtpx_thread_t *th;

    if (!__isthreaded)
	/* Threading isn't enabled */
	return EAGAIN;

    th = rtpx_self ();

    if (tha == NULL)
	tha = &default_tha;

    /* check for valid thread attributes */
    if (tha->tha_detachstate != PTHREAD_CREATE_JOINABLE
	&& tha->tha_detachstate != PTHREAD_CREATE_DETACHED)
	return EINVAL;
    if (tha->tha_inheritsched != PTHREAD_INHERIT_SCHED)
	return EINVAL;
    if (tha->tha_stacksize != 0)
	return EINVAL;
    if (tha->tha_stackaddr != NULL)
	return EINVAL;

    /* assume thread creation will work and bump number of active threads */
    mips_atomic_inc (&rtpx_num_threads);

    /* thread start data passed through parent to child */
    th->th_child_tha = tha;	/* new thread attributes */
    th->th_child_fun = fun;	/* start function */
    th->th_child_arg = arg;	/* start function arg */

    /* jump through hoop */
    rtpx_mt_fork (th);

    /* wait for new thread to start and pass back its ID */
    rtpx_sem_wait (&th->th_child_sem);
    *rv = th->th_child_id;

    return 0;
}

int
pthread_attr_init (pthread_attr_t *tha)
{
    memset (tha, 0, sizeof (pthread_attr_t));
    tha->tha_detachstate = PTHREAD_CREATE_JOINABLE;
    tha->tha_scope = PTHREAD_SCOPE_PROCESS;
    tha->tha_inheritsched = PTHREAD_INHERIT_SCHED;
    return 0;
}

int pthread_attr_destroy (pthread_attr_t *tha)
{
    /* nothing to do */
    return 0;
}

/*
 * Pthread exit and join
 */

 __attribute__ ((noreturn)) void
pthread_exit (void *v)
{
    rtpx_thread_t *cth;
    unsigned int oflags;

    if (!__isthreaded)
	exit (0);

    cth = rtpx_self ();

    /* prevent recursion */
    oflags = mips_atomic_bis (&cth->th_flags, RTPX_THF_EXITING);
    if (!(oflags & RTPX_THF_EXITING)) {

	/* clean up per thread stdio file locks */
	if (rtpx_flock_dtor)
	    rtpx_flock_dtor ();
	
	/* clean up thread specific data */
	_rtpx_tsdcleanup ();
    }

    /* exit program when final thread expires */
    if (mips_atomic_dec (&rtpx_num_threads) == 1)
	exit (0);

    /* prevent races when testing zombie status */
    rtpx_mutex_lock (&rtpx_thread_mx);

    /* wakeup any threads trying to join with us */
    if (cth->th_tha.tha_detachstate == PTHREAD_CREATE_JOINABLE) {
	/* store return value for joiners and mark as zombie */
	cth->th_jval = v;
	cth->th_flags |= RTPX_THF_ZOMBIE;
	rtpx_mutex_unlock (&rtpx_thread_mx);

	/* notify joiner that we've got a result to be collected */
	rtpx_sem_signal (&cth->th_join_sem);

	/* then wait for it to pick it up */
	rtpx_sem_wait (&cth->th_exit_sem);
    }
    else {
	rtpx_mutex_unlock (&rtpx_thread_mx);
    }

    /* bump thread generation number, mark as dead */
    cth->th_gen++;
    cth->th_flags = 0;

    /* release per-thread semaphores */
    rtpx_sem_destroy (&cth->th_wait_sem);
    rtpx_sem_destroy (&cth->th_join_sem);
    rtpx_sem_destroy (&cth->th_exit_sem);
    rtpx_sem_destroy (&cth->th_child_sem);

    /* terminate this TC */
    for (;;) 
	rtpx_mt_exit ();
}

int
pthread_join (pthread_t thid, void **rvp)
{
    rtpx_thread_t *oth;

    if (!__isthreaded)
	return 0;

    if (thid == pthread_self ())
	return EDEADLK;
    
    /* No other thread can join, detach or exit while this mutex is locked. */
    rtpx_mutex_lock (&rtpx_thread_mx);

    oth = rtpx_find_thread (thid);

    /* Check that it's a joinable thread, and no other thread is already
       waiting to join to it. */
    if (!oth || oth->th_tha.tha_detachstate != PTHREAD_CREATE_JOINABLE 
	|| oth->th_joining) {
	rtpx_mutex_unlock (&rtpx_thread_mx);
	return oth ? EINVAL : ESRCH;
    }

    /* Prevent anyone else from joining to that thread. */
    oth->th_joining = 1;

    /* Now we've recorded the join, we can release the lock */
    rtpx_mutex_unlock (&rtpx_thread_mx);

    /* Wait for thread to exit (become a zombie). */
    rtpx_sem_wait (&oth->th_join_sem);

    /* Thread is now a zombie, pick up its exit status. */
    if (rvp)
	*rvp = oth->th_jval;

    /* Tell the thread to terminate itself and release its resources. */
    rtpx_sem_signal (&oth->th_exit_sem);

    return 0;
}

int
pthread_detach (pthread_t thid)
{
    rtpx_thread_t  *oth;
    int             rv;

    if (!__isthreaded)
	return 0;

    /* No other thread can join, detach or exit while this mutex is locked. */
    rtpx_mutex_lock (&rtpx_thread_mx);

    oth = rtpx_find_thread (thid);

    /* Check that the named thread is a joinable thread, and that no
       other thread is already waiting to join to it. */
    if (oth && oth->th_tha.tha_detachstate == PTHREAD_CREATE_JOINABLE
	&& !oth->th_joining) {
	if (oth->th_flags & RTPX_THF_ZOMBIE) {
	    /* Already zombified and waiting to be joined to,
	       so tell it to terminate. */
	    rtpx_sem_signal (&oth->th_exit_sem);
	}
	else {
	    /* Set thread to detached state, so it won't wait to be joined. */
	    oth->th_tha.tha_detachstate = PTHREAD_CREATE_DETACHED;
	}
	rv = 0;
    }
    else if (oth)
	rv = EINVAL;
    else
	rv = ESRCH;

    rtpx_mutex_unlock (&rtpx_thread_mx);
    return rv;
}

pthread_t
pthread_self (void)
{
    if (__isthreaded)
	return rtpx_pthread_t (rtpx_self ());
    else
	return NULL;
}

int
sched_yield (void)
{
    if (__isthreaded)
	rtpx_mt_yield (-1);
    return 0;
}

/*
 * Pthread mutexes
 */

/* XXX Really nasty hack: I'm going to use the pthread mx_waitq field
   to hold the rtpx mutex structure. We must be sure that it's the same
   size, i.e. the size of a single pointer. */
#define pthread_to_rtpx_mutex(mx) ((rtpx_mutex_t *)(void *)&((mx)->mx_waitq))

static int
pthread_mutex_init_internal (pthread_mutex_t *mx)
{
    /* Check for valid attributes. */
    if (mx->mx_mxa.mxa_type != PTHREAD_MUTEX_DEFAULT_NP 
	&& mx->mx_mxa.mxa_type != PTHREAD_MUTEX_FAST_NP)
	return EINVAL;
    if (mx->mx_mxa.mxa_protocol != PTHREAD_PRIO_NONE)
	return EINVAL;
    if (mx->mx_mxa.mxa_pshared != PTHREAD_PROCESS_PRIVATE)
	return EINVAL;

    /* Set sensible values in pthread mutex. */
    mx->mx_flags = 0;
    mx->mx_olink = NULL;
    mx->mx_owner = NULL;

    /* Initialise an rtpx simple mutex inside the pthread mutex. */
    return rtpx_mutex_init (pthread_to_rtpx_mutex (mx));
}


int
pthread_mutex_init (pthread_mutex_t *mx,
		    const pthread_mutexattr_t *mxa)
{
    if (! mxa)
	mxa = &default_mxa;
    mx->mx_mxa = *mxa;
    return pthread_mutex_init_internal (mx);
}

int
pthread_mutex_destroy (pthread_mutex_t *mx)
{
    return rtpx_mutex_destroy (pthread_to_rtpx_mutex (mx));
}

/* Setup a statically initialised mutex the first time it's
   locked. Called when mx->mx_flags != 0, which indicates its an
   unused statically initialised mutex. */
       
static void
pthread_static_mutex_init (pthread_mutex_t *mx)
{
    /* Atomically set mx_flags to zero, and return old value.  If
       someone got in and set it to zero before us, then there's *
       nothing to do. */
    if (mips_atomic_swap (&mx->mx_flags, 0) != 0)
	if (pthread_mutex_init_internal (mx) != 0)
	    abort ();		/* not pretty! */
}


int
pthread_mutex_lock (pthread_mutex_t *mx)
{
    if (!__isthreaded)
	return 0;
    if (mx->mx_flags != 0)
	/* initialise static mutex first time */
	pthread_static_mutex_init (mx);
    return rtpx_mutex_lock (pthread_to_rtpx_mutex (mx));
}

int
pthread_mutex_trylock (pthread_mutex_t *mx)
{
    if (!__isthreaded)
	return 0;
    if (mx->mx_flags != 0)
	/* initialise static mutex first time */
	pthread_static_mutex_init (mx);
    return rtpx_mutex_trylock (pthread_to_rtpx_mutex (mx));
}

int
pthread_mutex_unlock (pthread_mutex_t *mx)
{
    if (!__isthreaded)
	return 0;
    return rtpx_mutex_unlock (pthread_to_rtpx_mutex (mx));
}

int
pthread_mutex_setname_np (pthread_mutex_t *mx, const char *name)
{
    /* ignore */
    return 0;
}

int
pthread_mutexattr_init (pthread_mutexattr_t *mxa)
{
    memset (mxa, 0, sizeof (pthread_mutexattr_t));
    mxa->mxa_pshared = PTHREAD_PROCESS_PRIVATE;
    mxa->mxa_protocol = PTHREAD_PRIO_NONE;
    mxa->mxa_type = PTHREAD_MUTEX_DEFAULT_NP;
    return 0;
}

int
pthread_mutexattr_destroy (pthread_mutexattr_t *mxa)
{
    /* nothing to do */
    return 0;
}

int
pthread_mutexattr_setname_np (pthread_mutexattr_t *mxa, 
					  const char *name)
{
    /* nothing to do */
    return 0;
}

int
pthread_mutexattr_settype_np (pthread_mutexattr_t *mxa, int type)
{
    if (type != PTHREAD_MUTEX_DEFAULT_NP && type != PTHREAD_MUTEX_FAST_NP)
	return EINVAL;
    return 0;
}

/*
 * Pthread condition variables
 */

int
pthread_cond_init (pthread_cond_t *cond, const pthread_condattr_t *cda)
{
    if (! cda)
	cda = &default_cda;
    cond->cd_cda = *cda;
    cond->cd_waitq.wq_first = NULL;
    return 0;
}

int pthread_cond_destroy (pthread_cond_t *cond)
{
    /* nothing to do */
    return 0;
}

int pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mx)
{
    rtpx_thread_t *cth;
    rtpx_thread_t **qth = &cond->cd_waitq.wq_first;

    if (!__isthreaded)
	return 0;

    cth = rtpx_self ();

    /* Add this thread to end of cond variable's waitq. */
    cth->th_next = NULL;
    rtpx_mt_acquire ();
    while (*qth)
	qth = &(*qth)->th_next;
    *qth = cth;
    rtpx_mt_release ();

    /* Keep a pointer to head of waitq. */
    cth->th_waitq = &cond->cd_waitq;

    /* Release the protecting mutex. */
    pthread_mutex_unlock (mx);

    /* WINDOW: predicate could be changed after the mutex_unlock, and
       a signal sent before we wait on the semaphore, but that's OK,
       because we're now on the waitq so the semaphore will still have
       been signalled and we won't block on the wait semaphore. */

    /* Wait for signal to be sent to our thread's wait semaphore */
    rtpx_sem_wait (&cth->th_wait_sem);

    /* WINDOW: predicate could be changed after signalling us but
       before we relock the mutex below, but that's OK because we're
       unblocked now, and our caller is required to retest the
       predicate. */

    /* Reacquire mutex. */
    pthread_mutex_lock (mx);

    return 0;
}

int pthread_cond_signal (pthread_cond_t *cond)
{
    rtpx_thread_t *nth;
    
    if (!__isthreaded)
	return 0;

    /* Pop one thread off cond variable waitq. */
    /* TODO think of a cute way to do this without dmt/emt */
    rtpx_mt_acquire ();
    if (nth = cond->cd_waitq.wq_first)
	cond->cd_waitq.wq_first = nth->th_next;
    rtpx_mt_release ();

    /* If there was a waiting thread, then wake it up. */
    if (nth)
	rtpx_sem_signal (&nth->th_wait_sem);

    return 0;
}

int pthread_cond_broadcast (pthread_cond_t *cond)
{
    rtpx_thread_t *qth, *nth;

    if (!__isthreaded)
	return 0;

    /* Atomically get head of waitq, and clear it. */
    qth = (rtpx_thread_t *) mips_atomic_swap ((unsigned int *)&cond->cd_waitq.wq_first, (unsigned int)NULL);

    /* Wake up all threads on original wait queue. */
    while (qth) {
	nth = qth->th_next;
	rtpx_sem_signal (&qth->th_wait_sem);
	qth = nth;
    }
    
    return 0;
}

int
pthread_condattr_init (pthread_condattr_t *cda)
{
    memset (cda, 0, sizeof (pthread_condattr_t));
    cda->cda_pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int
pthread_condattr_destroy (pthread_condattr_t *cda)
{
    /* nothing to do */
    return 0;
}

/*
 * Pthread "once" support
 */

static pthread_mutex_t once_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t once_cond = PTHREAD_COND_INITIALIZER;

int
_pthread_once (pthread_once_t *once, void (*fn)(void))
{
    if (once->state == PTHREAD_ONCE_DONE)
	/* no contention */
	return 0;
    
    if (!__isthreaded) {
	/* simple single-threaded case */
	once->state = PTHREAD_ONCE_DONE;
	(*fn) ();
    }
    else {
	pthread_mutex_lock (&once_lock);
	while (once->state == PTHREAD_ONCE_INPROGRESS) {
	    /* wait until some other thread completes once function */
	    pthread_cond_wait (&once_cond, &once_lock);
	}

	if (once->state == PTHREAD_ONCE_NEVER) {
	    /* we got there first */
	    once->state = PTHREAD_ONCE_INPROGRESS;
	    pthread_mutex_unlock (&once_lock);
	    (*fn) ();
	    pthread_mutex_lock (&once_lock);
	    once->state = PTHREAD_ONCE_DONE;
	    pthread_cond_broadcast (&once_cond);
	}

	pthread_mutex_unlock (&once_lock);
    }

    return 0;
}

int (pthread_once) (pthread_once_t *, void (*)(void))
	__attribute__((alias("_pthread_once")));

/*
 * Pthread thread-specific data support
 */

/* Internal thread-specific data (TSD) key information */
struct rtpx_tsdkey {
    struct rtpx_tsdkey *tsd_next;	/* free list */
    pthread_mutex_t tsd_mx;		/* protecting mutex */	
    unsigned short  tsd_gen;		/* generation number */
    short  	    tsd_count;		/* reference count */
    void            (*tsd_destructor) (void *);	/* destructor */
};

typedef struct rtpx_tsdkey rtpx_tsdkey_t;

/* Global key array and free list */
static pthread_mutex_t tsdkey_mx = PTHREAD_MUTEX_INITIALIZER;
static rtpx_tsdkey_t tsdkey[RTPX_KEYS_MAX];
static rtpx_tsdkey_t *tsdkey_free;

/* Convert internal TSD key data pointer into an external pthread_key value
   and vice versa. */
#define rtpx_key_t(tsdp) \
    (((tsdp - tsdkey) << 16) | tsdp->tsd_gen)
#define key_rtpx_idx(key) \
    ((unsigned int)(key) >> 16)
#define key_rtpx_gen(key) \
    ((unsigned int)(key) & 0xffff)

/* Find internal TSD key data for an external pthread_key value. */
static rtpx_tsdkey_t *
rtpx_tsdkey_find (pthread_key_t key)
{
    unsigned int idx = key_rtpx_idx (key);
    rtpx_tsdkey_t *tsdp;

    if (idx >= RTPX_KEYS_MAX)
	return NULL;
    tsdp = &tsdkey[idx];
    if (tsdp->tsd_gen != key_rtpx_gen (key))
	return NULL;
    return tsdp;
}


/* Allocate a new thread-specific key. */
int
pthread_key_create (pthread_key_t *keyp, void (*dtor)(void *))
{ 
    rtpx_tsdkey_t     *tsdp;

    /* get a new thread specific data key off free list */
    pthread_mutex_lock (&tsdkey_mx);
    tsdp = tsdkey_free;
    if (tsdp)
	tsdkey_free = tsdp->tsd_next;
    pthread_mutex_unlock (&tsdkey_mx);

    if (!tsdp)
	return EAGAIN;

    /* Initialise key data */
    tsdp->tsd_destructor = dtor;
    tsdp->tsd_count = 0;

    /* Return key to caller */
    *keyp = rtpx_key_t (tsdp);

    return 0;
}

/* Delete a thread-specific key - no threads can still be using it. */
int
pthread_key_delete (pthread_key_t key)
{
    rtpx_tsdkey_t     *tsdp;

    if (!(tsdp = rtpx_tsdkey_find (key)))
	return EINVAL;

    pthread_mutex_lock (&tsdp->tsd_mx);

    if (tsdp->tsd_count > 0) {
	/* error - some values still exist in some threads */
	pthread_mutex_unlock (&tsdp->tsd_mx);
	return EBUSY;
    }

    /* bump generation number so key can't be reused */
    tsdp->tsd_gen++;

    pthread_mutex_unlock (&tsdp->tsd_mx);

    /* put key back on free list */
    pthread_mutex_lock (&tsdkey_mx);
    tsdp->tsd_next = tsdkey_free;
    tsdkey_free = tsdp;
    pthread_mutex_unlock (&tsdkey_mx);

    return 0;
}

/* Set thread-specific data value for given key. */
int
pthread_setspecific (pthread_key_t key, const void *value)
{
    unsigned int idx = key_rtpx_idx (key);
    rtpx_thread_t *cth;
    rtpx_tsdkey_t *tsdp;

    if (!(tsdp = rtpx_tsdkey_find (key)))
	return EINVAL;

    cth = rtpx_self ();

    /* There's no need to lock per-thread structures, since no other
       thread ever access them. */

    if (cth->th_tsd[idx] == NULL) {
	/* has no per-thread value currently */
	if (value != NULL) {
	    /* setting value, bump use counts */
	    cth->th_tsdcount++;
	    pthread_mutex_lock (&tsdp->tsd_mx);
	    tsdp->tsd_count++;
	    pthread_mutex_unlock (&tsdp->tsd_mx);
	}
    }
    else {
	/* already has a per-thread value */
	if (value == NULL) {
	    /* clearing value, decrement use counts */
	    cth->th_tsdcount--;
	    pthread_mutex_lock (&tsdp->tsd_mx);
	    tsdp->tsd_count--;
	    assert (tsdp->tsd_count >= 0);
	    pthread_mutex_unlock (&tsdp->tsd_mx);
	}
    }

    /* store the new per-thread value */
    cth->th_tsd[idx] = value;
    return 0;
}

/* Return thread-specific data value for given key. */
void * 
pthread_getspecific (pthread_key_t key)
{
    unsigned int idx = key_rtpx_idx (key);
    rtpx_thread_t  *cth = rtpx_self ();

    /* minimal checking - designed to be fast! */
    if (idx >= RTPX_KEYS_MAX)
	return NULL;
    return (void *)cth->th_tsd[idx];
}


/* Remove all thread-specific data values when a thread exits. */
void
_rtpx_tsdcleanup ()
{
    rtpx_thread_t  *cth = rtpx_self ();
    unsigned int    i;

    /* Continue to attempt to remove TSD data until all have been removed,
       or PTHREAD_DESTRUCTOR_ITERATION attempts have been made. */

    for (i = 0; i < PTHREAD_DESTRUCTOR_ITERATIONS && cth->th_tsdcount; i++) {
	unsigned int keyidx;

	for (keyidx = 0; keyidx < RTPX_KEYS_MAX && cth->th_tsdcount; keyidx++) {
	    const void *data;

	    if (data = cth->th_tsd[keyidx]) {
		rtpx_tsdkey_t     *tsdp;
		void            (*destructor) (void *);

		cth->th_tsd[keyidx] = NULL;
		cth->th_tsdcount--;

		tsdp = &tsdkey[keyidx];
		pthread_mutex_lock (&tsdp->tsd_mx);
		tsdp->tsd_count--;
		assert (tsdp->tsd_count >= 0);
		destructor = tsdp->tsd_destructor;
		pthread_mutex_unlock (&tsdp->tsd_mx);

		if (destructor)
		    (destructor) ((void *)data);
	    }
	}
    }

    cth->th_tsdcount = 0;
}

/* Initialise TSD key data at boot time. */
static void
_rtpx_tsdinit (void)
{
    rtpx_tsdkey_t     *tsdp;
    for (tsdp = tsdkey; tsdp < &tsdkey[RTPX_KEYS_MAX]; tsdp++) {
	pthread_mutex_init (&tsdp->tsd_mx, NULL);
	tsdp->tsd_next = tsdkey_free;
	tsdkey_free = tsdp;
    }
}


/*
 * Miscellaneous stuff used by which needs to be
 * dummied out.
 */

void
_pthread_cleanup_push (struct _pthread_handler_rec *rec)
{
    /* Cancellation not supported, so nothing to do */
}

void
_pthread_cleanup_pop (struct _pthread_handler_rec *rec)
{
    /* Cancellation not supported, so nothing to do */
}

void
_pthread_cleanup_push_defer (struct _pthread_handler_rec *rec)
{
    /* Cancellation not supported, so nothing to do */
}

void
_pthread_cleanup_pop_restore (struct _pthread_handler_rec *rec)
{
    /* Cancellation not supported, so nothing to do */
}


int
pthread_setcancelstate (int new, int *oldp)
{
    /* Cancellation not supported, so nothing (interesting) to do */
    if (oldp)
	*oldp = PTHREAD_CANCEL_DISABLE;
    if (new != PTHREAD_CANCEL_DISABLE)
      return EINVAL;
    return 0;
}

int
pthread_attr_setscope (pthread_attr_t *attr, int scope)
{
    /* setscope not supported, so nothing to do */
    return ENOSYS;
}

int
pthread_attr_setstacksize (pthread_attr_t *attr, size_t size)
{
    /* setstacksize not supported, so nothing to do */
    return ENOSYS;
}

/* Allocate thread local storage for C library.
   Only used by a few non-reentrant functions in libc, which for
   the moment we won't support in rtpxlite. */

#include <libc_thread.h>

void *
_libc_private_storage (struct _thread_private_key_struct *key,
		       void *storage, size_t size, void *err)
{
    return storage;
}

/* Someone calls getpid()! */

#include <unistd.h>

pid_t
getpid (void)
{
    return 1;
}

int
nanosleep (const struct timespec *req, struct timespec *res)
{
    unsigned long count;

    /* We're probably going to be running on MIPSsim, not a real CPU,
       so no point in doing this accurately using the Count register.
       Since we'd end up sleeping for a huge time on the MIPSsim host
       if we assumed a "realistic" CPU frequency, we'll just do a
       simple delay loop assuming that we're on a CPU running at
       100kHz and therefore taking 10us per instruction. */

#ifdef FPGA
/* Assume 40MHz FPGA */
#define CPU_FREQUENCY	40000000
#else
/* Assume 100KHz MIPSsim */
#define CPU_FREQUENCY	100000
#endif

#define NS_PER_LOOP ((1000000000 / CPU_FREQUENCY) * 4)

    count = (req->tv_sec * (1000000000 / NS_PER_LOOP) 
	     + req->tv_nsec / NS_PER_LOOP);
    while (count-- != 0) {
	if (__isthreaded)
	    rtpx_mt_yield (-1);
    }

    /* return remaining time */
    if (res)
	res->tv_sec = res->tv_nsec = 0;
    return 0;
}

unsigned int
sleep (unsigned int secs)
{
    struct timespec ts;
    ts.tv_sec = secs;
    ts.tv_nsec = 0;
    nanosleep (&ts, NULL);
    return 0;
}


/*
 * SDE C library file locking.
 */

/* REENTRANT FILE LOCK */
struct __sdethread_flock {
    pthread_mutex_t	mx;
    pthread_t		owner;
    unsigned int	nlock;
    pthread_cond_t	notbusy;
};


/* Register C library's thread file lock destructor callback. */
void
__sdethread_flock_init (void (*dtor) (void))
{
    rtpx_flock_dtor = dtor;
}

/* Create a file lock */
__sdethread_flock_t 
__sdethread_flock_create (__sdethread_flock_t *flockp, int fd)
{
    static pthread_mutex_t flock_mx = PTHREAD_MUTEX_INITIALIZER;
    __sdethread_flock_t fpl;

    /* Check with mutex protection, in case we're
       in a race with another thread. */
    pthread_mutex_lock (&flock_mx);
    if (!(fpl = *flockp)) {
	/* We really are the first user, so allocate a new lock */
	fpl = malloc (sizeof (struct __sdethread_flock));
	if (!fpl)
	    abort ();
	/* Initialise the lock */
	fpl->nlock = 0;
	if (pthread_mutex_init (&fpl->mx, NULL) != 0)
	    abort ();
	if (pthread_cond_init (&fpl->notbusy, NULL) != 0)
	    abort ();
	*flockp = fpl;
    }
    pthread_mutex_unlock (&flock_mx);

    return fpl;
}

/* Lock file lock - recursive locking is allowed. */
void 
__sdethread_flock_lock (__sdethread_flock_t fpl)
{
    if (pthread_mutex_lock (&fpl->mx))
	abort ();
    if (fpl->nlock != 0) {
	/* lock is busy, is this thread the owner? */
	if (!pthread_equal (fpl->owner, pthread_self ())) {
	    /* not owner, wait until not busy */
	    do {
		pthread_cond_wait (&fpl->notbusy, &fpl->mx);
	    } while (fpl->nlock != 0);
	}
    }
    /* record the lock */
    fpl->owner = pthread_self ();
    fpl->nlock++;
    pthread_mutex_unlock (&fpl->mx);
}

/* Try to lock file lock - recursive locking is allowed. */
int
__sdethread_flock_trylock (__sdethread_flock_t fpl)
{
    if (pthread_mutex_trylock (&fpl->mx) != 0)
	/* couldn't lock mutex */
	return -1;
    if (fpl->nlock != 0) {
	/* lock is busy, is this thread the owner? */
	if (!pthread_equal (fpl->owner, pthread_self ())) {
	    /* not owner, return error */
	    pthread_mutex_unlock (&fpl->mx);
	    return -1;
	}
    }
    /* record the lock */
    fpl->owner = pthread_self ();
    fpl->nlock++;
    pthread_mutex_unlock (&fpl->mx);
    return 0;
}

/* Unlock a file lock. */
void
__sdethread_flock_unlock (__sdethread_flock_t fpl)
{
    unsigned int nlock;
    pthread_mutex_lock (&fpl->mx);
    nlock = --fpl->nlock;
    pthread_mutex_unlock (&fpl->mx);
    if (nlock == 0)
	/* all our locks removed, wake up other threads, */
	pthread_cond_signal (&fpl->notbusy);
}

/* Release all recursive locks when file is closed, or a thread exits
   with locks still held. */
void
__sdethread_flock_release (__sdethread_flock_t fpl)
{
    if (pthread_mutex_lock (&fpl->mx) == 0) {
	if (fpl->nlock != 0 && pthread_equal (fpl->owner, pthread_self ())) {
	    fpl->nlock = 0;
	    pthread_mutex_unlock (&fpl->mx);
	    pthread_cond_signal (&fpl->notbusy);
	}
	else {
	    pthread_mutex_unlock (&fpl->mx);
	}
    }
}

/* Release and free lock when file is closed */
void
__sdethread_flock_free (__sdethread_flock_t fpl)
{
    __sdethread_flock_release (fpl);
    free (fpl);
}

/*
 * SDE C library thread support
 */

/* Apart from __sdethread_mutex_init, most sdethread functions map
   one-to-one with the similarly named pthread functions. The
   sdethread objects are designed to be large enough to hold RTPX's
   equivalent pthread object implementations directly. This may not be
   true for other thread libraries/kernels, of course; in which case
   some indirection will be required. */

int __sdethread_mutex_init (__sdethread_mutex_t *mx, const char *name)
{
    /* Add mutexattr argument */
    return pthread_mutex_init ((pthread_mutex_t *)mx, NULL);
}

#if 0
/* For even more efficiency we could do a direct alias like this... */
int __sdethread_mutex_lock (__sdethread_mutex_t *)
	__attribute__((alias("pthread_mutex_lock")));
#endif

int __sdethread_mutex_lock (__sdethread_mutex_t *mx)
{
    return pthread_mutex_lock ((pthread_mutex_t *)mx);
}

int __sdethread_mutex_trylock (__sdethread_mutex_t *mx)
{
    return pthread_mutex_trylock ((pthread_mutex_t *)mx);
}

int __sdethread_mutex_unlock (__sdethread_mutex_t *mx)
{
    return pthread_mutex_unlock ((pthread_mutex_t *)mx);
}

int __sdethread_mutex_destroy (__sdethread_mutex_t *mx)
{
    return pthread_mutex_destroy ((pthread_mutex_t *)mx);
}


int ___sdethread_once (__sdethread_once_t *oncep, void (*funcp)(void))
{
    return _pthread_once ((pthread_once_t *)oncep, funcp);
}

#undef __sdethread_once
int __sdethread_once (__sdethread_once_t *oncep, void (*funcp)(void))
{
    return _pthread_once ((pthread_once_t *)oncep, funcp);
}


int __sdethread_key_create (__sdethread_key_t *keyp, void (*dtor)(void *))
{
    return pthread_key_create ((pthread_key_t *)keyp, dtor);
}

int __sdethread_key_delete (__sdethread_key_t key)
{
    return pthread_key_delete ((pthread_key_t)key);
}

int __sdethread_setspecific (__sdethread_key_t key, const void *data)
{
    return pthread_setspecific ((pthread_key_t)key, data);
}

void * __sdethread_getspecific (__sdethread_key_t key)
{
    return pthread_getspecific ((pthread_key_t)key);
}

/* helper function for assembler routines to set errno */
__attribute__((__nomips16__))
int __sdethread_set_errno (int err, int rv)
{
    errno = err;
    return rv;
}

#undef errno
int * __sdethread_errno_pointer (void)
{
    if (__isthreaded) {
	rtpx_thread_t *cth = rtpx_self ();
	return &cth->th_errno;
    }
    return &errno;
}

void __sdethread_sigdisable (__sdethread_sigstate_t *state)
{/* no signal support */}
    
void __sdethread_sigrestore (const __sdethread_sigstate_t *state)
{/* no signal support */}

int __sdethread_sigdeferred (void)
{return 0;}


#ifndef SDEMDI_GLUE
/* Arrange to have initialisation function called by SDE startup code. */

#include <sys/init.h>

static void
sde_pthread_init (void)
{
    _pthread_init ();
}

DECL_INIT_FUNC (IF_RTOS, sde_pthread_init)

#else
static void sdemdi_pthread_init (void) __attribute__((constructor));

static void
sdemdi_pthread_init (void)
{
    _pthread_init ();
}
#endif

#endif /* __mips == 32 && __mips_isa_rev >= 2 */
