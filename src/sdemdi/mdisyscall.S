/*
 * [[FGT copyright (c) 2002 MIPS Technologies, Inc.]]
 */

/*
 * mdimon/mdisyscall.S: host i/o interface via MDI
 */


#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include <sys/errno.h>
#include "mdi.h"


/*
 * A "system call" to MIPS MDI host debugger, is normally caught
 * by setting an invisible breakpoint on the entrypoint of
 * this function.
 */

/* Don't try to compile for mips16 */
	.set nomips16
	
LEAF(_mdi_syscall)
	li	v1,ENOSYS
	li	v0,-1
	j	ra
END(_mdi_syscall)
	
LEAF(_mdi_open)
	li	v0,_MDI_SYSCALL_OPEN
	b	_mdi_syscall
END(_mdi_open)

LEAF(_mdi_close)
	li	v0,_MDI_SYSCALL_CLOSE
	b	_mdi_syscall
END(_mdi_close)

LEAF(_mdi_read)
	li	v0,_MDI_SYSCALL_READ
	b	_mdi_syscall
END(_mdi_read)

LEAF(_mdi_write)
	li	v0,_MDI_SYSCALL_WRITE
	b	_mdi_syscall
END(_mdi_write)

LEAF(_mdi_lseek32)
	li	v0,_MDI_SYSCALL_LSEEK32
	b	_mdi_syscall
END(_mdi_lseek32)

LEAF(_mdi_isatty)
	li	v0,_MDI_SYSCALL_ISATTY
	b	_mdi_syscall
END(_mdi_isatty)
	
LEAF(_mdi_getchar)
	li	v0,_MDI_SYSCALL_GETCHAR
	b	_mdi_syscall
END(_mdi_getchar)

LEAF(_mdi_putchar)
	li	v0,_MDI_SYSCALL_PUTCHAR
	b	_mdi_syscall
END(_mdi_putchar)
	
LEAF(_mdi_gettime)
	li	v0,_MDI_SYSCALL_GETTIME
	move	t0,ra
	bal	_mdi_syscall
	beqz	a0,1f
	sw	v0,0(a0)
	sw	v1,4(a0)
1:	move	ra,t0
	j	ra
END(_mdi_gettime)
	
LEAF(_mdi_getargs)
	li	v0,_MDI_SYSCALL_GETARGS
	b	_mdi_syscall
END(_mdi_getargs)
	
LEAF(_mdi_profil)
XLEAF(_sbd_profil)
	li	v0,_MDI_SYSCALL_PROFIL
	b	_mdi_syscall
END(_mdi_profil)
		
LEAF(_mdi_sighook)
	li	v0,_MDI_SYSCALL_SIGHOOK
	b	_mdi_syscall
END(_mdi_sighook)
	
LEAF(_mdi_exit)
	li	v0,_MDI_SYSCALL_EXIT
	b	_mdi_syscall
END(_mdi_exit)

LEAF(_mdi_moved)
	li	v0,_MDI_SYSCALL_MOVED
	b	_mdi_syscall
END(_mdi_moved)
