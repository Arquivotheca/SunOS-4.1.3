/*	@(#)param.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _NSE_PARAM_H
#define _NSE_PARAM_H

/* Including <sys/param.h> to get MAXPATHLEN, MIN, and MAX definitions. */
#include <sys/param.h>

#define NSE_MAXHOSTNAMELEN	32

#define NSE_TMPDIR		"/tmp"
#define NSE_ISDIR(statp)	(((statp)->st_mode & S_IFMT) == S_IFDIR)
#define	NSE_DIR_MODE	0775		/* Creation mode for directories */

#define NSE_STREQ(a, b)			(strcmp(a, b) == 0)
#define NSE_STRDUP(str)			((str != NULL)? \
					    strcpy(nse_malloc((unsigned) (strlen(str)+1)), str):\
					    NULL)
#define NSE_NEW(tagtype)		((tagtype) \
					    nse_calloc(sizeof(struct tagtype), 1))
#define NSE_DISPOSE(p)			if (p != NULL) { \
						free((char *) p); \
						p = NULL; \
					}

/*
 * Return values of commonly used library routines.
 */
char		*malloc();
char		*calloc();
char		*nse_malloc();
char		*nse_calloc();
char		*strcpy();
char		*strcat();
char		*rindex();
char		*index();
char		*getenv();
int		free();
char		*sprintf();
char		*ctime();
long		time();

#endif
