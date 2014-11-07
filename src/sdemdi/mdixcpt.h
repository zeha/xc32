/*
 * [[FGT copyright (c) 1999-2002 MIPS Technologies, Inc.]]
 */

/*
 * m32xcpt.h : MIPS32 exception handling
 */


#ifndef _M32XCPT_H_
#define _M32XCPT_H_

#if (__mips >= 3 && __mips != 32) || __mips64
#define XCP_RSIZE	8
#define xcpreg	.dword	0
#else
#define XCP_RSIZE	4
#define xcpreg	.word	0
#endif

/* xcptcontext for assembler */
	.struct 0
XCP_SR:		xcpreg
XCP_CR:		xcpreg
XCP_EPC:	xcpreg
XCP_VADDR:	xcpreg

XCP_R0:	xcpreg
XCP_R1:		xcpreg
XCP_R2:		xcpreg
XCP_R3:		xcpreg
XCP_R4:		xcpreg
XCP_R5:		xcpreg
XCP_R6:		xcpreg
XCP_R7:		xcpreg
XCP_R8:		xcpreg
XCP_R9:		xcpreg
XCP_R10:		xcpreg
XCP_R11:		xcpreg
XCP_R12:		xcpreg
XCP_R13:		xcpreg
XCP_R14:		xcpreg
XCP_R15:		xcpreg
XCP_R16:		xcpreg
XCP_R17:		xcpreg
XCP_R18:		xcpreg
XCP_R19:		xcpreg
XCP_R20:		xcpreg
XCP_R21:		xcpreg
XCP_R22:		xcpreg
XCP_R23:		xcpreg
XCP_R24:		xcpreg
XCP_R25:		xcpreg
XCP_R26:		xcpreg
XCP_R27:		xcpreg
XCP_R28:		xcpreg
XCP_R29:		xcpreg
XCP_R30:		xcpreg
XCP_R31:		xcpreg

/* common used register names */
#define XCP_ZERO	XCP_R0
#define XCP_AT		XCP_R1
#define XCP_V0		XCP_R2
#define XCP_V1		XCP_R3
#define XCP_A0		XCP_R4
#define XCP_A1		XCP_R5
#define XCP_A2		XCP_R6
#define XCP_A3		XCP_R7
#define XCP_K0		XCP_R26
#define XCP_K1		XCP_R27
#define XCP_GP		XCP_R28
#define XCP_SP		XCP_R29
#define XCP_S8		XCP_R30
#define XCP_RA		XCP_R31

#if 0
#define XCP_S0		XCP_R16
#define XCP_S1		XCP_R17
#define XCP_S2		XCP_R18
#define XCP_S3		XCP_R19
#define XCP_S4		XCP_R20
#define XCP_S5		XCP_R21
#define XCP_S6		XCP_R22
#define XCP_S7		XCP_R23
#define XCP_T0		XCP_R8
#define XCP_T1		XCP_R9
#define XCP_T2		XCP_R10
#define XCP_T3		XCP_R11
#define XCP_T4		XCP_R12
#define XCP_T5		XCP_R13
#define XCP_T6		XCP_R14
#define XCP_T7		XCP_R15
#define XCP_T8		XCP_R24
#define XCP_T9		XCP_R25
#endif

XCP_MDLO:	xcpreg
XCP_MDHI:	xcpreg
XCP_MDEX:	xcpreg

XCP_PREV:	.word 0
XCP_XCLASS:	.word 0
XCP_SIGNO:	.word 0
XCP_SRSCTL:	.word 0
XCP_SIZE:	
	.previous


#undef xcpreg

#endif /*_M32XCPT_H_*/
