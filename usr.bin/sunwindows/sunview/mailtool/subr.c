#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)subr.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - miscellaneous subroutines
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>

char	*strcpy();

/*
 * Save a copy of a string.
 */
char *
mt_savestr(s)
	register char  *s;
{
	register char  *t;
	extern char    *malloc();
	extern char    *mt_cmdname;

	t = malloc((unsigned)(strlen(s) + 1));
	if (t == NULL) {
		(void)fprintf(stderr, "%s: Out of memory!\n", mt_cmdname);
		mt_stop_mail(0);
		mt_done(1);
	}
	(void)strcpy(t, s);
	return (t);
}

u_long
mt_current_time()
{
	struct timeval  tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	return (tv.tv_sec);
}


/* is s1 a proper prefix of s2 */
int
mt_is_prefix(s1, s2)
	register char  *s1, *s2;
{
	while (*s1 != '\0' && *s2 != '\0')
		if (*s1++ != *s2++)
			return (0);
	return (1);
}
	

/* writes full path name for file into buffer. knows about ~ */
void
mt_full_path_name(file, buffer)
	char           *file, *buffer;
{
/* not used - hala	char           *s; */

	*buffer = '\0';
	if (file == NULL) {}
	else if (*file == '/')
		(void) strcat(buffer, file);
	else	{
		(void) strcat(buffer, getenv("HOME"));
		if (*file == '~')
			file++;
		if (file[strlen(buffer) - 1] != '/')
			(void) strcat(buffer, "/");
		(void) strcat(buffer, file);
	}
}

