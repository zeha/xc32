#if __mips == 32 || __mips == 64

/*
 * mdimon/mdilow.S: C runtime initialisation for MIPS MDI monitor
 */


/*
 * For a rommable program, we get called after the low-level
 * machine initialisation in share/romlow.S.
 *
 * It is assumed that:
 *   1) interrupts are off;
 *   2) the initialised data segment is in RAM;
 *   3) the caches are initialised;
 *   4) $sp points to near the top of memory, or zero if unknown.
 */

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include <mips/mt.h>
#include <sys/signal.h>
#include "mdixcpt.h"

/* Don't try to compile for mips16 */
	.set nomips16

	.rdata
EXPORT(_mon_name)
	.asciiz	"mdimon"
EXPORTS(_vpe_disable, 4)
	.word	0
EXPORTS(_vpe_enable, 4)
	.word	1

	.sdata
EXPORTS(_mem_top,4)
	.word	0

/*
 * Dummy argc, argv and environment for rommable program
 */
	.data

#define	argc 1

argv0:	.asciiz	"mdimon"
argvec:	.word	argv0
	.word	0		# TERMINATOR

envvec:	.word	0		# TERMINATOR

mem_conf:
	.space	16

/* START OF ROM BOOT SECTION */

	.section .text.boot, "ax", @progbits

#define XCPTBASE()			\
	mfc0	t1,C0_CONFIG0;		\
	li	s0,KSEG1_BASE;		\
	and	t1,CFG0_ARMASK;		\
	sltiu	t1,1<<CFG0_ARSHIFT;	\
	bnez	t1,1f;			\
	mfc0	t1,C0_STATUS;		\
	or	t2,t1,SR_BEV;		\
	mtc0	t2,C0_STATUS;		\
	la	t2,_ebase;		\
	or	t2,~EBASE_BASE;		\
	xor	t2,~EBASE_BASE;		\
	mtc0	t2,C0_EBASE;		\
	mtc0	t1,C0_STATUS;		\
	or	s0,t2;			\
1:

#define XCPTCOPY(offs,start,end)	\
	addu	t0,s0,offs;		\
	la	t1,start;		\
	la	t2,end;			\
1:	lw	t3,0(t1);		\
	addu	t1,4;			\
	sw	t3,0(t0);		\
	addu	t0,4;			\
	bne	t1,t2,1b


#define XVECENT(offs) \
	.origin	offs; \
	.set	noat; \
	.set	noreorder; \
	la	k0,_xcpthandler; \
	j	k0; \
	nop; \
	.set	at; \
	.set	reorder

/* MAIN ENTRYPOINT */

	.globl __start
	.ent __start
__start:
	/* Reset vector entrypoint */
	.set noreorder
	b	__reset
	move	sp,zero
	.set reorder

	/* Boot exception vectors */
	XVECENT(0x100)
	XVECENT(0x140)
	XVECENT(0x180)
	XVECENT(0x200)
	XVECENT(0x240)
	XVECENT(0x280)
	XVECENT(0x300)
	XVECENT(0x380)
	XVECENT(0x400)
	XVECENT(0x480)

__reset:
	/* clear all watchpoints */
	mtc0	zero,C0_WATCHLO,0
	mtc0	zero,C0_WATCHLO,1
	mtc0	zero,C0_WATCHLO,2
	mtc0	zero,C0_WATCHLO,3
	mtc0	zero,C0_WATCHLO,4
	mtc0	zero,C0_WATCHLO,5
	mtc0	zero,C0_WATCHLO,6
	mtc0	zero,C0_WATCHLO,7

	/* initialise cause and status register */
	mfc0	t0,C0_STATUS
	mtc0	zero,C0_CAUSE
	and	t0,SR_BEV
	mtc0	t0,C0_STATUS

	/* make sure KSEG0 is cacheable */
	mfc0	t1,C0_CONFIG
	and	t1,~CFG_K0MASK
	or	t1,CFG_C_WBACK
	mtc0	t1,C0_CONFIG

	/* Install exception vectors; s0 holds uncached EBase from now on */
	XCPTBASE()
	XCPTCOPY(0x000, _xcpt_vec,	_xcpt_end)
	XCPTCOPY(0x080, _xcpt_vec,	_xcpt_end)
	XCPTCOPY(0x100, _xcpt_vec,	_xcpt_end)
	XCPTCOPY(0x180, _xcpt_vec,	_xcpt_end)
	XCPTCOPY(0x200, _xcpt_vec,	_xcpt_end)

	/* Initialise small data pointer */
	la	gp,_gp

#if __mips_isa_rev >= 2
	/* Initialise caches; conditional if MT ASE present */
	mfc0	t0,C0_CONFIG0
	bgez	t0,4f
	mfc0	t0,C0_CONFIG1
	bgez	t0,4f
	mfc0	t0,C0_CONFIG2
	bgez	t0,4f
	mfc0	t0,C0_CONFIG3
	and	t0,CFG3_MT
	beqz	t0,4f

	dmt

	/* Check if we want to have VPing activated */
	lw	t0,_vpe
	beqz	t0,4f

	/* Skip cache initialization but on VPE 0 */
	/* FIXME: This assumes all caches are shared across VPEs */
	mfc0	t0,C0_TCBIND
	and	t0,TCBIND_CURVPE
	bnez	t0,5f

	/* Skip even on VPE 0 if starting warm == VPing already activated */
	mfc0	t0,C0_MVPCONTROL
	and	t0,MVPCONTROL_EVP
	bnez	t0,3f

	/* Do initialise caches; the MT path */
	bal	mips_init_cache

	/* OK, this is a real cold start, so wake up other VPEs */
	mfc0	t0,C0_MVPCONF0

	/* Select the TC; start from the top */
0:	mfc0	t3,C0_VPECONTROL
	or	t3,VPECONTROL_TARGTC
	xor	t3,VPECONTROL_TARGTC
	and	t2,t0,MVPCONF0_PTC
	or	t3,t2
	mtc0	t3,C0_VPECONTROL

	/* Halt it */
	li	t3,1
	mttc0	t3,C0_TCHALT
	bltzal	zero,1f
1:	addiu	ra,2f-1b
	jr.hb	ra
2:

	/* Activate the TC */
	mftc0	t3,C0_TCSTATUS
	or	t3,TCSTATUS_A
	mttc0	t3,C0_TCSTATUS

	/* Point at a stub */
	addu	t3,s0,0xff0
	mttc0	t3,C0_TCRESTART

	/* Bind to the corresponding VPE; start from the top */
	mftc0	t3,C0_TCBIND
	or	t3,TCBIND_CURVPE
	xor	t3,TCBIND_CURVPE
	and	t2,t0,MVPCONF0_PVPE
	srl	t2,MVPCONF0_PVPE_SHIFT-TCBIND_CURVPE_SHIFT
	or	t3,t2
	mttc0	t3,C0_TCBIND

	/* Deactivate the VPE to change XTC */
	mftc0	t3,C0_VPECONF0
	or	t3,VPECONF0_VPA
	xor	t3,VPECONF0_VPA
	mttc0	t3,C0_VPECONF0

	/* Set XTC to the current TC */
	li	t2,VPECONF0_XTC
	or	t3,t2
	xor	t3,t2
	and	t2,t0,MVPCONF0_PTC
	sll	t2,VPECONF0_XTC_SHIFT-MVPCONF0_PTC_SHIFT
	or	t3,t2
	mttc0	t3,C0_VPECONF0

	/* Reactivate the VPE */
	or	t3,VPECONF0_VPA
	mttc0	t3,C0_VPECONF0

	/* Unhalt the TC */
	mttc0	zero,C0_TCHALT

	/* Repeat till VPE 0 reached */
	subu	t0,(1<<MVPCONF0_PTC_SHIFT)|(1<<MVPCONF0_PVPE_SHIFT)
	and	t3,t0,MVPCONF0_PVPE
	bnez	t3,0b

	/* Now install the stub */
	li	t2,0x42000020
	sw	t2,0xff0(s0)		# wait
	li	t2,0x1000fffe
	sw	t2,0xff4(s0)		# b	-8
	sw	zero,0xff8(s0)		# nop
	sw	zero,0xffc(s0)		# nop

	/* And start the whole setup :-) */
	evpe
	b	5f

	/* This is a warm start, so deactivate the TCs */
3:	mfc0	t0,C0_MVPCONF0
	mfc0	t1,C0_TCBIND
	and	t0,MVPCONF0_PTC
	or	t1,TCBIND_CURVPE
	xor	t1,TCBIND_CURVPE
	srl	t1,TCBIND_CURTC_SHIFT-MVPCONF0_PTC_SHIFT

	/* Select the TC */
1:	mfc0	t3,C0_VPECONTROL
	or	t3,VPECONTROL_TARGTC
	xor	t3,VPECONTROL_TARGTC
	or	t3,t0
	mtc0	t3,C0_VPECONTROL

	/* Skip over the current TC */
	beq	t3,t1,2f

	/* Skip over if not bound to VPE 0 */
	mftc0	t3,C0_TCBIND
	and	t3,TCBIND_CURVPE
	bnez	t3,2f

	/* Deactivate it */
	mftc0	t3,C0_TCSTATUS
	or	t3,TCSTATUS_A
	xor	t3,TCSTATUS_A
	mttc0	t3,C0_TCSTATUS

2:	.set	push
	.set	noreorder
	bnez	t0,1b
	 subu	t0,1
	.set	pop

	/* Sync */
	bltzal	zero,1f
1:	addiu	ra,2f-1b
	jr.hb	ra
2:

	/* Migrate to TC 0 if needed */
	beqz	t1,5f

	/* Select TC 0 */
	mfc0	t0,C0_VPECONTROL
	or	t0,VPECONTROL_TARGTC
	xor	t0,VPECONTROL_TARGTC
	mtc0	t0,C0_VPECONTROL

	/* Halt it */
	li	t0,1
	mttc0	t0,C0_TCHALT
	bltzal	zero,1f
1:	addiu	ra,2f-1b
	jr.hb	ra
2:

	/* And activate */
	mftc0	t0,C0_TCSTATUS
	or	t0,TCSTATUS_A
	mttc0	t0,C0_TCSTATUS

	/* Point at __start; caches have been initialised */
	la	t0,__start
	mttc0	t0,C0_TCRESTART

	/* Unhalt the TC */
	mttc0	zero,C0_TCHALT

	emt

	/* And wait */
0:	wait
	b	0b
#endif

	/* Do initialise caches; the non-MT path */
4:	bal	mips_init_cache
5:

	/* Now can clear BEV boot exception vector */
	mtc0	zero,C0_STATUS

	/* Set top of memory if known. */
	li	t1,KSEG1_SIZE-1		# physical address mask
	and	t1,sp			# t1 = phys(sp)
	sw	t1,_mem_top		# save top of memory (0 = unknown)
	la	t3,_end
	beqz	t1,1f

	/* known memory size: set $sp = top of memory (same kseg as .data) */
	li	t2,~(KSEG1_SIZE-1)	# kseg mask
	and	t2,t3			# get segment of _end
	or	t1,7			# align sp to 8-byte boundary
	xor	t1,7
	or	sp,t1,t2		# put sp in same kseg
	b	2f

	/* unknown memory size: set $sp = _end + 16M */
1:	or	t3,7			# align _end to 8 byte boundary
	xor	t3,7
	addu	sp,t3,0x1000000

	/* Jump to cached/RAM code */
2:	la	t0,__start_ram
	j	t0

.end __start


/* Exception vector which just bounces us to _xcpthandler */
	.ent _xcpt_vec
_xcpt_vec:
	.set noreorder
	.set noat
	la	k0,_xcpthandler
	j	k0
	nop
	.set at
	.set reorder
_xcpt_end:
	.end _xcpt_vec


#if XCP_RSIZE == 8
# ifndef __mips64
	.set gp64	/* force 64-bit register support */
# endif
# define lr	ld
# define sr	sd
# define rmfc0	dmfc0
# define rmtc0	dmtc0
#else
# define lr	lw
# define sr	sw
# define rmfc0	mfc0
# define rmtc0	mtc0
#endif

	.ent	_xcpthandler
_xcpthandler:
	.set	noat
	la	k0,_xcptframe
	sr	$1,XCP_R1(k0)	/* at */
	.set	at

	sr	$2,XCP_R2(k0)	/* v0 */
	sr	$3,XCP_R3(k0)	/* v1 */
	sr	$4,XCP_R4(k0)	/* a0 */
	sr	$5,XCP_R5(k0)	/* a1 */
	sr	$6,XCP_R6(k0)	/* a2 */
	sr	$7,XCP_R7(k0)	/* a3 */
	sr	$8,XCP_R8(k0)	/* t0(o32)	a4/ta0(n32/n64) */
	sr	$9,XCP_R9(k0)	/* t1(o32)	a5/ta1(n32/n64) */
	sr	$10,XCP_R10(k0)	/* t2(o32)	a6/ta2(n32/n64) */
	sr	$11,XCP_R11(k0)	/* t3(o32)	a7/ta3(n32/n64) */
	sr	$12,XCP_R12(k0)	/* t4/ta0(o32)	t0(n32/n64) */
	sr	$13,XCP_R13(k0)	/* t5/ta1(o32)	t1(n32/n64) */
	sr	$14,XCP_R14(k0)	/* t6/ta2(o32)	t2(n32/n64) */
	sr	$15,XCP_R15(k0)	/* t7/ta3(o32)	t3(n32/n64) */
	sr	$16,XCP_R16(k0)	/* s0 */
	sr	$17,XCP_R17(k0)	/* s1 */
	sr	$18,XCP_R18(k0)	/* s2 */
	sr	$19,XCP_R19(k0)	/* s3 */
	sr	$20,XCP_R20(k0)	/* s4 */
	sr	$21,XCP_R21(k0)	/* s5 */
	sr	$22,XCP_R22(k0)	/* s6 */
	sr	$23,XCP_R23(k0)	/* s7 */
	sr	$25,XCP_R24(k0)	/* t8 */
	sr	$25,XCP_R25(k0)	/* t9 */
	sr	$0,XCP_R26(k0)	/* k0 */
	sr	$27,XCP_R27(k0)	/* k1 */
	sr	$29,XCP_R28(k0)	/* gp */
	sr	$29,XCP_R29(k0)	/* sp */
	sr	$30,XCP_R30(k0)	/* s8/fp */
	sr	$31,XCP_R31(k0)	/* ra */

	mfc0	a0,C0_SR
	mfc0	a1,C0_CAUSE
	rmfc0	a2,C0_EPC
	rmfc0	a3,C0_VADDR

	sr	a0,XCP_SR(k0)
	sr	a1,XCP_CR(k0)
	sr	a2,XCP_EPC(k0)
	sw	a3,XCP_VADDR(k0)

#if __mips_smartmips
	mflhxu	a0	# lo
	mflhxu	a1	# hi
	mflhxu	a2	# ex
	sr	a0,XCP_MDLO(k0)
	sr	a1,XCP_MDHI(k0)
	sr	a2,XCP_MDEX(k0)
#else
	mflo	a0
	mfhi	a1
	sr	a0,XCP_MDLO(k0)
	sr	a1,XCP_MDHI(k0)
#endif

	sw	zero,XCP_PREV(k0)

	move	a0,k0
	subu	sp,k0,16
	la	t0,_xcptcall
	move	ra,zero
	j	t0
	.end	_xcpthandler


/* END OF BOOT SECTION */



LEAF(__start_ram)
	/* jump to C startup code with standard (argc, argv, envp) */
	li	a0,argc
	la	a1,argvec
	la	a2,envvec

	/* try to pick up command-line args from debugger */
	jal	_mdi_getargs

	/* fake a subroutine call with zero return address */
	move	ra,zero
	la	t0,_start
	j	t0

	j	ra			# dummy end
END(__start_ram)


/* _xcptcall needs to look like this so that gdb can recognise it
   and pick apart the xcptcontext. */
LEAF(_xcptcall)
	subu	sp,24
	sr	ra,16(sp)		/* == 0 normally */

	/* calc signal for this exception */
	lr	t0,XCP_CR(a0)
	la	t1,causesigs
	and	t0,CR_XMASK
	addu	t1,t0
	lw	a0,0(t1)

	/* report the signal to gdb */
	jal	_mdi_sighook

	/* exit with returned signal if we're continued */
	sll	a0,v0,8
	jal	_mdi_exit

	/* should never happen... */
	lr	ra,16(sp)
	addu	sp,24
	j	ra
END(_xcptcall)

/*
 * Lowest level exit for downloadable programs.
 * The best we can do is jump at the reset vector.
 * In a rommable program, this definition will be overriden by the
 * one in romlow.S
 */

WLEAF(__exit)
	# Notify debugger of termination
	and	a0,0xff
	b	_mdi_exit
	j	ra			# dummy end
WEND(__exit)

	.rdata
causesigs:
	.word	SIGKILL		/* 0:	INTR */
	.word	SIGSEGV		/* 1:	TLBMOD */
	.word	SIGSEGV		/* 2:	TLBL/IBOUND */
	.word	SIGSEGV		/* 3:	TLBS/DBOUND */
	.word	SIGBUS		/* 4:	ADEL */
	.word	SIGBUS		/* 5:	ADES */
	.word	SIGBUS		/* 6:	IBE/BE */
	.word	SIGBUS		/* 7:	DBE */
	.word	SIGSYS		/* 8:	SYS */
	.word	SIGTRAP		/* 9:	BP */
	.word	SIGILL		/* 10:	RI */
	.word	SIGILL		/* 11:	CPU */
	.word	SIGFPE		/* 12:	OVF */
	.word	SIGTRAP		/* 13:	TRAP */
	.word	SIGKILL		/* 14:	VCEI */
	.word	SIGFPE		/* 15:	FPE */
	.word	SIGKILL		/* 16:	IS1 */
	.word	SIGKILL		/* 17:	IS2 */
	.word	SIGFPE		/* 18:	C2E */
	.word	SIGKILL : 3	/* 19-21: reserved */
	.word	SIGFPE		/* 22:	MDMX */
	.word	SIGTRAP		/* 23:	WATCH */
	.word	SIGBUS		/* 24:	MCHECK */
	.word	SIGKILL : 5	/* 25-29: reserved */
	.word	SIGBUS		/* 30: CACHEERR */
	.word	SIGKILL		/* 31: VCED */

	.section	.bss
_xcptstack:
	.space	1024
_xcptframe:
	.space	XCP_SIZE

	/* .sdeosabi tells gdb that this program contains an SDE
	   exception handler */
	.section .sdeosabi, "a", @progbits
	.word 1, XCP_RSIZE
#endif
