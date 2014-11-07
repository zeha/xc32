#if __mips == 32 && __mips_isa_rev >= 2

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include <mips/mt.h>

	.set	nomips16

/*
 * Per thread data structure, must match definition
 * of "struct rtpx_thread" in rtpxlite.c.
 */
	.struct 0
TH_STKTOP:	PTR	0
TH_START:	PTR	0
	.previous

LEAF(_pthread_mt_fork)
	la	a1,1f			# new thread starts this label
	.set	push
	.set	mt
	fork	a0, a1, a0		# passing arg0 to child
	.set	pop
	jr	ra			# return to parent

	/* We are now executing in the new thread context */

/* WARNING:
 * For a brief period we are operating on the same stack as our parent,
 * but the parent may continue running and push more data onto its stack.
 * So if we take an interrupt while in the new thread context there is a
 * window where the interrupt handler could push an exception stack frame
 * onto the wrong stack and corrupt the parent thread's stack.
 * So we MUST use an exception handler which switches to an exception level
 * stack frame, rather than reusing the current "user" stack.
 */

AENT(_pthread_mt_fork_child)
1:	mfc0	a1,C0_TCCONTEXT		# get our thread pointer
	PTR_L	sp,TH_STKTOP(a1)	# get initial stack top
	PTR_L	t9,TH_START(a1)		# get start func
	PTR_SUBU sp,NARGSAVE*SZARG	# arg save area
	la	gp,_gp			# load global pointer
	jr	t9			# start thread code
END(_pthread_mt_fork)

#endif /* __mips == 32 && __mips_isa_rev >= 2 */
