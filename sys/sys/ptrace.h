/*	@(#)ptrace.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sys_ptrace_h
#define _sys_ptrace_h

/*
 * Request values for the ptrace system call
 */
enum ptracereq {
	PTRACE_TRACEME = 0,		/* 0, by tracee to begin tracing */
	PTRACE_CHILDDONE = 0,		/* 0, tracee is done with his half */
	PTRACE_PEEKTEXT,		/* 1, read word from text segment */
	PTRACE_PEEKDATA,		/* 2, read word from data segment */
	PTRACE_PEEKUSER,		/* 3, read word from user struct */
	PTRACE_POKETEXT,		/* 4, write word into text segment */
	PTRACE_POKEDATA,		/* 5, write word into data segment */
	PTRACE_POKEUSER,		/* 6, write word into user struct */
	PTRACE_CONT,			/* 7, continue process */
	PTRACE_KILL,			/* 8, terminate process */
	PTRACE_SINGLESTEP,		/* 9, single step process */
	PTRACE_ATTACH,			/* 10, attach to an existing process */
	PTRACE_DETACH,			/* 11, detach from a process */
	PTRACE_GETREGS,			/* 12, get all registers */
	PTRACE_SETREGS,			/* 13, set all registers */
	PTRACE_GETFPREGS,		/* 14, get all floating point regs */
	PTRACE_SETFPREGS,		/* 15, set all floating point regs */
	PTRACE_READDATA,		/* 16, read data segment */
	PTRACE_WRITEDATA,		/* 17, write data segment */
	PTRACE_READTEXT,		/* 18, read text segment */
	PTRACE_WRITETEXT,		/* 19, write text segment */
	PTRACE_GETFPAREGS,		/* 20, get all fpa regs */
	PTRACE_SETFPAREGS,		/* 21, set all fpa regs */
#ifdef sparc
	/* currently unimplemented */
	PTRACE_GETWINDOW,		/* 22, get register window n */
	PTRACE_SETWINDOW,		/* 23, set register window n */
#else !sparc
	PTRACE_22,			/* 22, filler */
	PTRACE_23,			/* 23, filler */
#endif !sparc
	PTRACE_SYSCALL,                 /* 24, trap next sys call */
	PTRACE_DUMPCORE,                /* 25, dump process core */
#ifdef  i386
	PTRACE_SETWRBKPT,		/* 26, set write breakpoint */
	PTRACE_SETACBKPT,		/* 27, set access breakpoint */
	PTRACE_CLRDR7,			/* 28, clear debug register 7 */
#else
	PTRACE_26,			/* 26, filler */
	PTRACE_27,			/* 27, filler */
	PTRACE_28,			/* 28, filler */
#endif
	PTRACE_GETUCODE,		/* 29, get u.u_code */
};

#ifdef KERNEL
/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct ipc {
	int	ip_lock;		/* Locking between processes */
	short	ip_error;		/* The child encountered an error? */
	enum	ptracereq ip_req;	/* The type of request */
	caddr_t	ip_addr;		/* The address in the child */
	caddr_t	ip_addr2;		/* The address in the parent */
	int	ip_data;		/* Data or number of bytes */
	struct	regs ip_regs;		/* The regs, psw and pc */
	int	ip_nbytes;		/* # of bytes being moved in bigbuf */
	caddr_t	ip_bigbuf;		/* Dynamic buffer for large transfers */
#ifdef FPU
	struct	fpu ip_fpu;		/* Floating point processor info */
#endif FPU
#ifdef sparc
	struct	rwindow	ip_rwindow;	/* RISC register window */
#endif sparc
	struct  vnode *ip_vp;           /* vnode for core dump */
};
#endif KERNEL

#endif /*!_sys_ptrace_h*/
