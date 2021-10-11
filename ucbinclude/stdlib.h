/*	@(#)stdlib.h 1.1 92/07/30 SMI; from include/stdlib.h 1.6 */

/*
 * stdlib.h
 */

#ifndef	__stdlib_h
#define	__stdlib_h

#include <sys/stdtypes.h>	/* to get size_t */

extern unsigned int _mb_cur_max;
#define MB_CUR_MAX    _mb_cur_max

#define	mblen(s, n)	mbtowc((wchar_t *)0, s, n)

/* declaration of various libc functions */
extern int	abort(/* void */);
extern int	abs(/* int j */);
extern double	atof(/* const char *nptr */);
extern int	atoi(/* const char *nptr */);
extern long int	atol(/* const char *nptr */);
extern char *	bsearch(/* const void *key, const void *base, size_t nmemb,
		    size_t size, int (*compar)(const void *, const void *) */);
extern char *	calloc(/* size_t nmemb, size_t size */);
extern int	exit(/* int status */);
extern int	free(/* void *ptr */);
extern char *	getenv(/* const char *name */);
extern char *	malloc(/* size_t size */);
extern int	qsort(/* void *base, size_t nmemb, size_t size,
		    int (*compar)(const void *, const void *) */);
extern int	rand(/* void */);
extern char *	realloc(/* void *ptr, size_t size */);
extern int	srand(/* unsigned int seed */);

extern int    mbtowc(/* wchar_t *pwc, const char *s, size_t n */);
extern int    wctomb(/* char *s, wchar_t wchar */);
extern size_t mbstowcs(/* wchar_t *pwcs, const char *s, size_t n */);
extern size_t wcstombs(/* char *s, const wchar_t *pwcs, size_t n */);

#endif
