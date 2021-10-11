/*	@(#)sun.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef sunwindow_sun_DEFINED
#define sunwindow_sun_DEFINED

#if !(defined(stdio_DEFINED) || defined(FILE))
#include <stdio.h> 
FILE	*popen();
#endif

int atoi();
double atof();
char *ctime();
char *malloc();
char *calloc();
char *strcpy();
char *sprintf();

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef Bool_DEFINED
typedef enum {False = 0, True = 1} Bool;
#endif

#define strequal(s1, s2) (strcmp(s1, s2) == 0)
/*
 * No longer needed for releases 4.0 and up
 * #define strdup(str)	 (strcpy(malloc((unsigned) strlen(str) + 1), str))
 */
extern char *strdup();
/*
 * Get some storage and copy a string. Note that str is evaluated twice,
 * so no side effects.
 */
#define ord(e)  ((int) (e))
/*
 * Make an enum usable as an integer
 * Successor or predecessor macros might also be convenient, but
 * much less so.
 */

#define FOREVER for (;;)

#ifndef MIN
#define MIN(x, y) ( ((x) < (y)) ? (x) : (y) )
#endif

#ifndef MAX
#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )
#endif

#ifndef LINT_CAST
#ifdef lint
#define LINT_CAST(arg)	(arg ? 0 : 0)
#else
#define LINT_CAST(arg)	(arg)
#endif lint
#endif LINT_CAST

#endif
