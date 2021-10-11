#ifndef lint
static char     sccsid[] = "@(#)dosys.c 1.7 86/02/27 SMI";	/* from S5R2 1.2 */
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <vfork.h>
#include "defs.h"

#define SHELLCOM "/bin/sh"
#define NO	0
#define YES	1

extern char	*strrchr();
extern int	errno;
static int             wait_pid = 0;

static char	*
execat(s1, s2, si)
	register char	*s1,
	                *s2;
	char		*si;
{
	register char	*s;

	s = si;
	while (*s1 && *s1 != ':')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return (*s1 ? ++s1 : 0);
}

/*
 * execlp(name, arg,...,(char *)0)	(like execl, but does path search)
 * execvp(name, argv)	(like execv, but does path search) 
 */
static int
execvp(name, argv)
	char	*name,
	        **argv;
{
	register        etxtbsy = 1;
	register        eacces = 0;
	register char	*cp;
	char	*pathstr;
	char	*shell;
	char	fname[MAXPATHLEN + 1];
	extern unsigned sleep();

	pathstr = varptr("PATH")->varval;
	if (pathstr == 0 || *pathstr == '\0')
		pathstr = ":/bin:/usr/bin";
	shell = varptr("SHELL")->varval;
	if (shell == 0 || *shell == '\0')
		shell = SHELLCOM;
	cp = any(name, '/') ? "" : pathstr;

	do
	{
		cp = execat(cp, name, fname);
retry:
		(void) execv(fname, argv);
		switch (errno)
		{
		case ENOEXEC:
			*argv = fname;
			*--argv = "sh";
			(void) execv(shell, argv);
			return (-1);
		case ETXTBSY:
			if (++etxtbsy > 5)
				return (-1);
			(void) sleep((unsigned) etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOMEM:
		case E2BIG:
			return (-1);
		}
	} while (cp);
	if (eacces)
		errno = EACCES;
	return (-1);
}

/*
 * Close open directory files before exec'ing close everything but 0, 1,
 * and 2 Can't call closedir due to vfork 
 */
static void
doclose()
{
	register int    n,
	                maxfd;

	maxfd = getdtablesize();
	for (n = 3; n < maxfd; n++)
		close(n);
}

int
dosys(comstring, nohalt)
	register char	*comstring;
	int             nohalt;
{
	register char	*p;
	int             status;

	p = comstring;
	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p)
		return (-1);

	if (metas(comstring))
		status = doshell(comstring, nohalt);
	else
		status = doexec(comstring);

	return (status);
}

static int
metas(s)			/* Are there are any  Shell
				 * meta-characters? */
	register char	*s;
{
	while (*s)
		if( is_meta(*s++) )
			return (YES);
	return (NO);
}


static int
doshell(comstring, nohalt)
	register char	*comstring;
	register int    nohalt;
{
	register char	*shell;
	register char	*shellname;

	setenv();
	if ((wait_pid = vfork()) == 0)
	{
		doclose();

		shell = varptr("SHELL")->varval;
		if (shell == 0 || shell[0] == '\0')
			shell = SHELLCOM;
		if ((shellname = strrchr(shell, '/')) == NULL)
			shellname = shell;
		else
			shellname++;
		(void) execl(shell, shellname, (nohalt ? "-c" : "-ce"), comstring, (char *) 0);
		fatal("Couldn't load Shell");
	}
	rstenv();
	return (await());
}

static int
await()
{
	int             intrupt();
	int             status;
	int             pid;

	while ((pid = wait(&status)) != wait_pid)
		if (pid == -1)
			fatal("bad wait code");
	wait_pid = 0;
	return (status);
}


#define	MAXARGV	500

static int
doexec(str)
	register char	*str;
{
	register char	*t;
	register char	**p;
	char	*argv[MAXARGV];

	while (*str == ' ' || *str == '\t')
		++str;
	if (*str == '\0')
		return (-1);	/* no command */

	p = &argv[1];		/* reserve argv[0] in case of execvp failure */
	for (t = str; *t;)
	{
		if (p >= &argv[MAXARGV])
			fatal1("%s: Too many arguments.", str);
		*p++ = t;
		while (*t != ' ' && *t != '\t' && *t != '\0')
			++t;
		if (*t)
			for (*t++ = '\0'; *t == ' ' || *t == '\t'; ++t);
	}

	*p = NULL;

	setenv();
	if ((wait_pid = vfork()) == 0)
	{
		doclose();
		(void) execvp(str, &argv[1]);
		fatal1("Cannot load %s", str);
	}
	rstenv();
	return (await());
}
