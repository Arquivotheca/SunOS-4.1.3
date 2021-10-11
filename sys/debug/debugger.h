/*	@(#)debugger.h 1.1 92/07/30	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Global declarations used for all kernel debuggers.
 */

#ifndef _debug_debugger_h
#define _debug_debugger_h

#include <setjmp.h>
#include <debug/debug.h>

#define LINEBUFSZ	128	/* size of input buffer */

func_t ktrace;			/* kernel's trace routine */
func_t monnmi;			/* monitor's nmi routine */
int nobrk;			/* flag used to control page allocation */
int dotrace;			/* ptrace says to single step */
int dorun;			/* ptrace says to run */
int foundu;			/* found valid u symbol table entry */
int lastpm;			/* last pmeg stolen */
int lastpg;			/* last page stolen */
int pagesused;			/* total number of pages used by debugger */
int scbstop;			/* stop when scbsync routine is called */
char myname[];			/* name of the debugger */
char aline[];			/* generic buffer used for console input */
struct regs *reg;		/* pointer to debuggee's saved registers */
jmp_buf debugregs;		/* context for debugger */
jmp_buf mainregs;		/* context for debuggee */

/*
 * Because of the way typedef's work, we cannot declare abort_jmp
 * to be jmp_buf * and do reasonable things with them.  So we
 * declare another typedef which hides this.
 */
typedef int *jmp_buf_ptr;

jmp_buf_ptr abort_jmp;		/* pointer to saved context for tty interrupt */
jmp_buf_ptr nofault;		/* pointer to saved context for fault traps */
jmp_buf_ptr curregs;		/* pointer to saved context for each process */

/*
 * Standard function declarations
 */
struct scb *getvbr();
int trap();
int trace();
int fault();
func_t readfile();

#endif /*!_debug_debugger_h*/
