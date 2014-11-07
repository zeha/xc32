/*
 * [[FGT copyright (c) 2002 MIPS Technologies, Inc.]]
 */

/*
 * MIPS SDE kit MIPS MDI interface definitions
 */


#ifndef __ASSEMBLER__
extern int	_mdi_open (const char *, int, int);
extern int	_mdi_close (int);
extern int	_mdi_read (int, void *, int);
extern int	_mdi_write (int, const void *, int);
extern long	_mdi_lseek32 (int, long, int);
extern int	_mdi_getchar (int /*chan*/);
extern int	_mdi_putchar (int /*chan*/, int /*c*/);
struct timeval;
extern int	_mdi_gettime (struct timeval *);
extern int	_mdi_exit (int);
extern int	_mdi_isatty (int);
extern int	_mdi_syscall ();
extern int	_mdi_profil (int);
extern int	_mdi_sighook (int, void *);
#endif

#define _MDI_SYSCALL_OPEN	0
#define _MDI_SYSCALL_CLOSE	1
#define _MDI_SYSCALL_READ	2
#define _MDI_SYSCALL_WRITE	3
#define _MDI_SYSCALL_GETCHAR	4
#define _MDI_SYSCALL_PUTCHAR	5
#define _MDI_SYSCALL_LSEEK32	6
#define _MDI_SYSCALL_GETTIME	7
#define _MDI_SYSCALL_EXIT	8
#define _MDI_SYSCALL_MOVED	9
#define _MDI_SYSCALL_GETARGS	10
#define _MDI_SYSCALL_ISATTY	11
#define _MDI_SYSCALL_PROFIL	12
#define _MDI_SYSCALL_SIGHOOK	13
