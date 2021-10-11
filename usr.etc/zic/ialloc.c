#ifndef lint
static	char sccsid[] = "@(#)ialloc.c 1.1 92/07/30 SMI"; /* from Arthur Olson's 8.18 */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include <string.h>
#include <stdio.h>

#ifdef __STDC__

#include <stdlib.h>

#define P(s)		s

#define alloc_size_t	size_t

typedef void *		genericptr_t;

#else /* !defined __STDC__ */

extern char *	calloc();
extern char *	malloc();
extern char *	realloc();

#define ASTERISK	*
#define P(s)		(/ASTERISK s ASTERISK/)

typedef unsigned	alloc_size_t;

#define const

typedef char *		genericptr_t;

#endif /* !defined __STDC__ */

#ifndef alloc_t
#define alloc_t	unsigned
#endif /* !alloc_t */

#ifdef MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#else /* !defined MAL */
#define NULLMAL(x)	((x) == NULL)
#endif /* !defined MAL */

#define nonzero(n)	(((n) == 0) ? 1 : (n))

char *	icalloc P((int nelem, int elsize));
char *	icatalloc P((char * old, const char * new));
char *	icpyalloc P((const char * string));
char *	imalloc P((int n));
char *	irealloc P((char * pointer, int size));
void	ifree P((char * pointer));

char *
imalloc(n)
const int	n;
{
#ifdef MAL
	register char *	result;

	result = malloc((alloc_size_t) nonzero(n));
	return NULLMAL(result) ? NULL : result;
#else /* !defined MAL */
	return malloc((alloc_size_t) nonzero(n));
#endif /* !defined MAL */
}

char *
icalloc(nelem, elsize)
int	nelem;
int	elsize;
{
	if (nelem == 0 || elsize == 0)
		nelem = elsize = 1;
	return calloc((alloc_size_t) nelem, (alloc_size_t) elsize);
}

char *
irealloc(pointer, size)
char * const	pointer;
const int	size;
{
	if (NULLMAL(pointer))
		return imalloc(size);
	return realloc((genericptr_t) pointer, (alloc_size_t) nonzero(size));
}

char *
icatalloc(old, new)
char * const		old;
const char * const	new;
{
	register char *	result;
	register	oldsize, newsize;

	newsize = NULLMAL(new) ? 0 : strlen(new);
	if (NULLMAL(old))
		oldsize = 0;
	else if (newsize == 0)
		return old;
	else	oldsize = strlen(old);
	if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
		if (!NULLMAL(new))
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
icpyalloc(string)
const char * const	string;
{
	return icatalloc((char *) NULL, string);
}

void
ifree(p)
char * const	p;
{
	if (!NULLMAL(p))
		(void) free(p);
}

void
icfree(p)
char * const	p;
{
	if (!NULLMAL(p))
		(void) free(p);
}

