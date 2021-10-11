/* @(#)check_stack.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 * Until such time as the kernel can produce accurate core dumps of an
 * address space (rather than just of a traditional address space), ld.so
 * "knows" about the implementation of stack frames in SunOS.  In particular,
 * it "knows" that the environment strings are placed at the "top end" of
 * the stack frame on process initialization.  This program verifies that
 * this knowledge remains "true" -- and will cause a build of ld.so to fail
 * in the event it ever becomes "not true."  (Note: of course, this technique
 * itself depends upon the build environment matching the execution one --
 * not always a guarantee.)
 */

#include <sysexits.h>
#include <machine/vmparam.h>

extern	char **environ;

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	int user_stack;
	int ps = getpagesize();
	char c, *cp, **cpp;

	ps = getpagesize();
	cpp = environ;
	do {
		cp = *cpp++;
	} while (*cpp);
	do {
		cp++;
	} while (*cp);
	user_stack = (int)(cp + ps - 1) & ~(ps - 1); 
	if (user_stack != USRSTACK) {
		printf(
	    "Error: USRSTACK calculated as: 0x%x\nUSRSTACK defined as 0x%x\n",
		    user_stack, USRSTACK);
		exit(0);
		/*NOTREACHED*/
	}
	exit(0);
	/*NOTREACHED*/
}
