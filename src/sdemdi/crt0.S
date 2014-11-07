/* 
 * crt0.S: SDE-MIPS C runtime startup code.
 *
 * Also defined here is _exit().
 */
	
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include <sys/init.h>

#ifndef SR_FR	
#define SR_FR		0x04000000	/* 64-bit fpu registers */
#endif
	
#ifndef SR_MX	
#define SR_MX		0x01000000	/* enable MDMX ASE */
#endif
			
/* Don't try to compile for mips16 */
	.set nomips16
	
	.sdata
EXPORTS(environ,4)
	.word	0
	
EXPORTS(_nofpu,4)
	.word	1		# initially no fpu support

#if __mips64
#define	sr	sd
#define	lr	ld
#define rs	8
#else
#define	sr	sw
#define	lr	lw
#define rs	4
#endif

#define ARGSIZE			(4*rs)
	
#define FRAMESIZE(nsave)	((ARGSIZE + ((nsave)*rs) + 7) & ~7)
		
LEAF(_start)
	subu	sp,FRAMESIZE(1)
	sr	ra,ARGSIZE(sp)	# ra == 0 to terminate stack trace
	
	la	gp,_gp		# initialise $gp

	/* save argc, argv, and environ for main() */
	move	s0,a0
	move	s1,a1
	sw	a2,environ
	
	/* clear bss */
	la	a0,_fbss
	la	a2,_end
	.set noreorder
3:	sw	zero, 0(a0)
	bltu	a0, a2, 3b
	addiu	a0, a0, 4		/* Delay slot.  */
	.set reorder
	
	mfc0	t0,C0_SR
#if (! __mips_soft_float && ! __mips_no_float)
	or	t0,SR_CU1	# enable Cp1 instructions
#if __mips_fpr == 64
	or	t0,SR_FR	# enable all 32 f.p. registers
#else	
	and	t0,~SR_FR	# disable all 32 f.p. registers
#endif
	sw	zero,_nofpu
#else	
	and	t0,~SR_CU1	# disable Cp1 instructions
#endif
	or      t0,SR_MX
	mtc0	t0,C0_SR

#if (! __mips_soft_float && ! __mips_no_float)
	ssnop; ssnop; ehb
	ctc1	zero,$31	# initialise fpu csr
#endif
		
	la	a0, _fini
	jal	atexit
	
	jal	_init
	
	/* rv = main(argc, argv, environ) */
	move	a0,s0
	move	a1,s1
	lw	a2,environ
	jal	main

	/* exit(rv) */
	move	a0,v0
	jal	exit

	/* just in case !! */
1:	break	1
	b	1b
	j	ra			# end of subroutine marker
END(_start)


LEAF(_exit)
	j	__exit		# really, really exit!
END(_exit)

/* Breakpoint location in r/w memory for gdb dummy calls */	
.comm __CALL_DUMMY_ADDRESS,4
	
