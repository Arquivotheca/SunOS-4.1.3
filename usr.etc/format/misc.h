
/*      @(#)misc.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#ifndef	_MISC_
#define	_MISC_

/*
 * This file contains declarations pertaining to the miscellaneous routines.
 */
#include <setjmp.h>

/*
 * This defines the structure of a saved environment.  It consists of the
 * environment itself, a pointer to the next environment on the stack, and
 * flags to tell whether the environment is active, etc.
 */
struct env {
	jmp_buf env;				/* environment buf */
	struct	env *ptr;			/* ptr to next on list */
	char	flags;				/* flags */
};
extern	struct env *current_env;
/*
 * This macro saves the current environment in the given structure and
 * pushes the structure onto our enivornment stack.  It initializes the
 * flags to zero (inactive).
 */
#define saveenv(x)	{ \
			x.ptr = current_env; \
			current_env = &x; \
			(void) setjmp(x.env); \
			x.flags = 0; \
			}
/*
 * This macro marks the environment on the top of the stack active.  It
 * assumes that there is an environment on the stack.
 */
#define useenv()	(current_env->flags |= ENV_USE)
/*
 * This macro marks the environment on the top of the stack inactive.  It
 * assumes that there is an environment on the stack.
 */
#define	unuseenv()	(current_env->flags &= ~ENV_USE)
/*
 * This macro pops an environment off the top of the stack.  It
 * assumes that there is an environment on the stack.
 */
#define	clearenv()	(current_env = current_env->ptr)
/*
 * These are the flags for the environment struct.
 */
#define	ENV_USE		0x01			/* active */
#define	ENV_CRITICAL	0x02			/* in critical zone */
#define	ENV_ABORT	0x04			/* abort pending */

/*
 * This structure is used to keep track of the state of the tty.  This
 * is necessary because some of the commands turn off echoing.
 */
struct ttystate {
	struct	sgttyb ttystate;		/* buffer for ioctls */
	int	modified;			/* echoing off flag */
	int	ttyfile;			/* file for ioctls */
};
/*
 * This is the number lines assumed for the tty.  It is designed to work
 * on terminals as well as sun monitors.
 */
#define	TTY_LINES	24

#endif	!_MISC_

