/* 
 * mdimon/mdilow.S: C runtime initialisation for IDT/sim
 *
 * This is the main entrypoint of the program.
 *
 * It is assumed that:
 *   1) the initialised data segment is in RAM;
 *   2) the caches, tlb etc. are initialised;
 *   3) $sp points to near the top of memory.
 */

#include <mips/asm.h>
#include <mips/regdef.h>

	.sdata

	.globl	_mem_top
_mem_top:
	.word	0

/*
 * Dummy argc, argv and environment for IDT/sim
 */

	.data

#define	argc 1

argv0:	.asciiz	"sim"
	
argvec:	.word	argv0
	.word	0		# TERMINATOR
	
envvec:	.word	0		# TERMINATOR
	
mem_conf:
	.space	16

LEAF(__start)
	la	gp,_gp			# initialise gp
	la	t0,_end + 4096		# temporary stack
	and	sp,t0,~7
	
	la	a0,mem_conf
	jal	sim_get_mem_conf
	
	lw	t0,mem_conf+0
	sw	t0,_mem_top		# save (claimed) top of memory
	and	t0,~7			# round it down
	
	la	t1,_end			# get data address
	and	t1,~0x1fffffff		# get segment
	or	sp,t1,t0		# put sp in same segment

	/* jump to C startup code with fake (argc, argv, envp) */
	li	a0,argc
	la	a1,argvec
	la	a2,envvec

	/* fake a subroutine call with zero return address */
	move	ra,zero
	la	t0,_start
	j	t0
	
	j	ra			# dummy end
END(__start)

LEAF(__exit)
	# special simulator trap
	break	0x3ff
END(__exit)
	

LEAF(mips_clean_cache)
	# no caches on simulator
	j	ra
END(mips_clean_cache)
		
/*
 * Include linkages to IDT/sim prom routines
 */
#include "idtsim.h"
PROM_LINK(sim_read, PROM_READ)
PROM_LINK(sim_write, PROM_WRITE)
PROM_LINK(sim_open, PROM_OPEN)
PROM_LINK(sim_close, PROM_CLOSE)
PROM_LINK(sim_putchar, PROM_PUTCHAR)
PROM_LINK(sim_getchar, PROM_GETCHAR)
PROM_LINK(sim_printf, PROM_PRINTF)
PROM_LINK(_mon_printf, PROM_PRINTF)
PROM_LINK(sim_get_mem_conf, PROM_GETMCONF)
PROM_LINK(sim_exit, PROM_RETURN)

	
