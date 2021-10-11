/* @(#) stackdep.h 1.1 92/07/30 */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * sparc requires stack be aligned on 8 byte boundary.
 * usage:
 *	stkalign_t foo[SIZE];
 * or
 *	stkalign_t *foo;
 *	foo = (stkalign_t *)malloc(sizeof (stkalign_t) * SIZE);
 */
typedef	double		stkalign_t;		/* alignment for stack */

/* align adr to legal stack boundary */
#define	STACKALIGN(adr)	(((int)(adr)) & ~(sizeof (stkalign_t) - 1))

#define	MINSIGSTKSZ	8000	/* min. size of signal stack in stkalign_t's */

/* for finding top of stack of stkalign_t's */
#define	STACKTOP(base, size) ((base) + (size))

#define	STACKDOWN	TRUE	/* TRUE if stacks grow down */

#define	PUSH(sp, obj, type)	*--((type *)(sp)) = (obj);
#define	POP(sp, obj, type)	(obj) = *((type *)(sp))++;
