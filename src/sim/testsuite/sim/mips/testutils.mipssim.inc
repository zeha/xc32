# MIPS simulator testsuite utility functions.
# Copyright (C) 2004, 2007 Free Software Foundation, Inc.
# Contributed by Chris Demetriou of Broadcom Corporation.
#
# This file is part of the GNU simulators.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


# $1, $2, $4, $5, $6, are used as temps by the macros defined here.

	.macro writemsg msg
	.data
901:	.ascii	"\msg\n"
902:
	.previous
	la	$5, 901b
	li	$6, 902b - 901b
	li	$4, 1
	.set push
	.set noreorder
	li	$2, 3
	jal	_mdi_syscall
	nop
	.set pop
	.endm


	.macro exit rc
	.set push
	.set noreorder
	li	$4, \rc
	j	_exit
	nop
	.set pop
	.endm


	.macro setup

	.set push
	.set noreorder
	.global _exit
	.ent _exit
_exit:
	li	$2, 8
	jal	_mdi_syscall
	nop
	.end _exit
	.global abort
	.ent abort
abort:
	li	$2, 8
	jal	_mdi_syscall
	nop
	.end abort
	.set pop

	.global _start
	.global __start
	.ent _start
_start:
__start:
	.set push
	.set noreorder
	la	$31, _start2
	and	$31, ~0x20000000
	jr	$31
	nop
	.aent _start2
_start2:
	# clear BEV bit, set MX bit
	li	$31, 0x01000000
	mtc0	$31, $12
	j	DIAG
	nop
	.set pop
	.end _start

	.global _fail
	.ent _fail
_fail:
	writemsg "fail\n***  EXIT code 1"
	exit 1
	.end _fail

	.global _pass
	.ent _pass
_pass:
	writemsg "pass\n*** EXIT code 0"
	exit 0
	.end _pass

	.endm	# setup


	.macro pass
	.set push
	.set noreorder
	j	_pass
	nop	
	.set pop
	.endm


	.macro fail
	.set push
	.set noreorder
	j	_fail
	nop	
	.set pop
	.endm


	.macro load32 reg, val
	li	\reg, \val
	.endm


	.macro load64 reg, val
	dli	\reg, \val
	.endm


	.macro loadaddr reg, addr
	la	\reg, \addr
	.endm


	.macro checkreg reg, expreg
	.set push
	.set noat
	.set noreorder
	beq	\expreg, \reg, 901f
	nop
	fail
901:
	.set pop
	.endm


	.macro check32 reg, val
	.set push
	.set noat
	load32	$1, \val
	checkreg \reg, $1
	.set pop
	.endm


	.macro check64 reg, val
	.set push
	.set noat
	load64	$1, \val
	checkreg \reg, $1
	.set pop
	.endm


	.ent _mdi_syscall
_mdi_syscall:
	li	$3, 45
	li	$2, -1
	jr	$31
	.end _mdi_syscall
