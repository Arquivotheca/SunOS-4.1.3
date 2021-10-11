#include <stdio.h>

int            *
intalloc(n)
	register unsigned n;
{
	register int	*p;

	if (p = (int *) calloc((unsigned) 1, n))
		return (p);

	(void) fprintf(stderr, "out of memory");
	fatal("out of memory");
	/* NOTREACHED */
}
