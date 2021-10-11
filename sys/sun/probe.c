#ifndef lint
static	char sccsid[] = "@(#)probe.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Common routines for probing memory.
 * Typically called by drivers during initialization to
 * see if a device is present on the Mainbus.
 */
#include <sys/types.h>
#include <sun/fault.h>

static label_t *saved_jb;
static label_t jb;

/*
 * Probe a location by attempting to read a word
 */
peek(a)
	short *a;
{
	int val;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val & 0xFFFF);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to read a byte
 */
peekc(a)
	char *a;
{
	int val;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val & 0xFF);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to read a long
 */
peekl(a, val)
	long *a;
	long *val;
{

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		*val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to write a long word
 */
pokel(a, val)
	long *a;
	long val;
{
	register int err = 0;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		pokefault = -1;
		*a = val;
		flush_writebuffers_to((addr_t)a);
		/* if we get here, it may have worked */
		if (pokefault == 1)
			err = 1;
	} else {
		/* A fault occured */
		err = 1;
	}
	nofault = saved_jb;
	pokefault = 0;
	return (err);
}

/*
 * Probe a location by attempting to write a word
 */
poke(a, val)
	short *a;
	short val;
{
	register int err = 0;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		pokefault = -1;
		*a = val;
		flush_writebuffers_to((addr_t)a);
		/* if we get here, it may have worked */
		if (pokefault == 1)
			err = 1;
	} else {
		/* A fault occured */
		err = 1;
	}
	nofault = saved_jb;
	pokefault = 0;
	return (err);
}

/*
 * Probe a location by attempting to write a byte
 */
pokec(a, val)
	char *a;
	char val;
{
	register int err = 0;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		pokefault = -1;
		*a = val;
		flush_writebuffers_to((addr_t)a);
		/* if we get here, it may have worked */
		if (pokefault == 1)
			err = 1;
	} else {
		/* A fault occured */
		err = 1;
	}
	nofault = saved_jb;
	pokefault = 0;
	return (err);
}
