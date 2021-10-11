/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)cgxfind.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include <colorbuf.h>
#include <sys/mman.h>

int CGXBase;
static int CGXFile;
#define	CGXSIZE	(16*1024)
#define	CGXALIGN getpagesize()

_core_cgxopen()
{
	int p;

	if (CGXBase)
		return (CGXBase);
	CGXFile = open("/dev/cg0", 2);
	if (CGXFile < 0)	{
		/* perror("cgxfind"); */
		return(0);
	}
	p = (int)malloc(CGXSIZE+CGXALIGN);
	p = (p + CGXALIGN -1) & ~(CGXALIGN-1);
	if (mmap(p, CGXSIZE, PROT_WRITE, MAP_SHARED, CGXFile, 0))
		/* perror("cgxfind mmap") */ ;
	CGXBase = p;
	return(p);
}

static mmap(addr, len, prot, share, fd, pos)
{
	return(syscall(64+7, addr, len,	prot, share, fd, pos));
}

_core_cgxclose()
{
	close(CGXFile);
	free(CGXBase);
	CGXBase=0;
}
