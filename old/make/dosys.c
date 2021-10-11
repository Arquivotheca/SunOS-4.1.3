#ifndef lint
static	char sccsid[] = "@(#)dosys.c 1.1 92/08/05 SMI"; /* from S5R2 1.2 03/28/83 */
#endif

# include "defs"
# include <signal.h>
# include <sys/stat.h>
# include <vfork.h>

extern char Makecall;

dosys(comstring, nohalt)
register CHARSTAR comstring;
int nohalt;
{
	register CHARSTAR p;
	int status;

	p = comstring;
	while(	*p == BLANK ||
		*p == TAB) p++;
	if(!*p)
		return(-1);

	if(IS_ON(NOEX) && Makecall == NO)
		return(0);

	if(metas(comstring))
		status = doshell(comstring,nohalt);
	else
		status = doexec(comstring);

	return(status);
}



metas(s)   /* Are there are any  Shell meta-characters? */
register CHARSTAR s;
{
	while(*s)
		if( funny[*s++] & META)
			return(YES);

	return(NO);
}

doshell(comstring,nohalt)
register CHARSTAR comstring;
register int nohalt;
{
	register CHARSTAR shell;
	register CHARSTAR shellname;

	setenv();
	if((waitpid = vfork()) == 0)
	{
#if 0
		/*
		 * This won't work with "vfork" since the signal vector is
		 * in user space on Suns.
		 */
		enbint(SIG_DFL);
#endif
		doclose();

		shell = varptr("SHELL")->varval;
		if(shell == 0 || shell[0] == CNULL)
			shell = SHELLCOM;
		if ((shellname = strrchr(shell, SLASH)) == NULL)
			shellname = shell;
		else
			shellname++;
		(void)execl(shell, shellname, (nohalt ? "-c" : "-ce"), comstring, (char *)0);
		fatal("Couldn't load Shell");
	}
	rstenv();
	dirflush();
	return( await() );
}




await()
{
	int intrupt();
	int status;
	int pid;

	enbint(intrupt);
	while( (pid = wait(&status)) != waitpid)
		if(pid == -1)
			fatal("bad wait code");
	waitpid = 0;
	return(status);
}



/*
 * Close open directory files before exec'ing
 * close everything but 0, 1, and 2
 * Can't call closedir due to vfork
 */
doclose()
{
	register int n, maxfd;

	maxfd = getdtablesize();
	for (n = 3; n < maxfd; n++)
		close(n);
}


#define	MAXARGV	500

doexec(str)
register CHARSTAR str;
{
	register CHARSTAR t;
	register CHARSTAR *p;
	CHARSTAR argv[MAXARGV];

	while( *str==BLANK || *str==TAB )
		++str;
	if( *str == CNULL )
		return(-1);	/* no command */

	p = &argv[1];		/* reserve argv[0] in case of execvp failure */
	for(t = str ; *t ; )
	{
		if (p >= &argv[MAXARGV])
			fatal1("%s: Too many arguments.", str);
		*p++ = t;
		while(*t!=BLANK && *t!=TAB && *t!=CNULL)
			++t;
		if(*t)
			for( *t++ = CNULL ; *t==BLANK || *t==TAB  ; ++t);
	}

	*p = NULL;

	setenv();
	if((waitpid = vfork()) == 0)
	{
#if 0
		/*
		 * This won't work with "vfork" since the signal vector is
		 * in user space on Suns.
		 */
		enbint(SIG_DFL);
#endif
		doclose();
		(void)execvp(str, &argv[1]);
		fatal1("Cannot load %s",str);
	}
	rstenv();
	dirflush();
	return( await() );
}

touch(force, name)
register int force;
register char *name;
{
	extern long lseek();
        struct stat stbuff;
        char junk[1];
        int fd;

        if( stat(name,&stbuff) < 0)
                if(force)
                        goto create;
                else
                {
                        (void)fprintf(stderr,"touch: file %s does not exist.\n",name);
                        return;
                }
        if(stbuff.st_size == 0)
                goto create;
        if( (fd = open(name, 2)) < 0)
                goto bad;
        if( read(fd, junk, 1) < 1)
        {
                (void)close(fd);
                goto bad;
        }
        (void)lseek(fd, 0L, 0);
        if( write(fd, junk, 1) < 1 )
        {
                (void)close(fd);
                goto bad;
        }
        (void)close(fd);
        return;
bad:
        (void)fprintf(stderr, "Cannot touch %s\n", name);
        return;
create:
        if( (fd = creat(name, 0666)) < 0)
                goto bad;
        (void)close(fd);
}
