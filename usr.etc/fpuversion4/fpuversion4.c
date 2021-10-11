#ifndef lint
static char     sccsid[] = "@(#)fpuversion4.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * /usr/etc/fpuversion4 - print out Sun-4 floating-point controller version
 * from fsr
 */

extern
                getfsr();

main()
{
	int             v;

	v = (getfsr() >> 17) & 7;
	printf(" Sun-4 floating-point controller version %d found.\n", v);

	if (v == 7) {
		printf(" Note that version 7 is reserved by convention to indicate that \n");
		printf(" floating-point hardware is not present or not enabled.\n");
		printf(" Floating-point instructions are emulated by kernel code.\n");
		exit(1);
	} else
		exit(0);
	/* NOTREACHED */
}
