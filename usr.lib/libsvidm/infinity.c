#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)infinity.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifdef i386
 static long    INF_VAL[] =             {0x00000000, 0x7ff00000};
#else
 static long    INF_VAL[] =             {0x7ff00000, 0x00000000};
#endif
#define Inf		(*(double*)INF_VAL)

double
infinity()
{
	return Inf;
}
