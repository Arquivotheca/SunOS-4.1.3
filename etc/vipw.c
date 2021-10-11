#ifndef lint
static	char *sccsid = "@(#)vipw.c 1.1 92/07/30 SMI"; /* from UCB 4.3 */
#endif
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>

char	*sprintf();

/*
 * Password file editor with locking.
 */
char	*temp = "/etc/ptmp";
char	*passwd = "/etc/passwd";
char	buf[BUFSIZ];
char	*getenv();
char	*index();
extern	int errno;

main()
{
	int fd;
	FILE *ft, *fp;
	char *editor;
	int ok;

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	setbuf(stderr, (char *)NULL);
	(void)umask(0);
	fd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		if (errno == EEXIST) {
			(void)fprintf(stderr, "vipw: password file busy\n");
			exit(1);
		}
		(void)fprintf(stderr, "vipw: "); perror(temp);
		exit(1);
	}
	ft = fdopen(fd, "w");
	if (ft == NULL) {
		(void)fprintf(stderr, "vipw: "); perror(temp);
		goto bad;
	}
	fp = fopen(passwd, "r");
	if (fp == NULL) {
		(void)fprintf(stderr, "vipw: "); perror(passwd);
		goto bad;
	}
	while (fgets(buf, sizeof (buf) - 1, fp) != NULL)
		fputs(buf, ft);
	(void)fclose(ft);
	(void)fclose(fp);
	editor = getenv("VISUAL");
	if (editor == 0)
		editor = getenv("EDITOR");
	if (editor == 0)
		editor = "vi";
	(void)sprintf(buf, "%s %s", editor, temp);

	ok = 0;				/* not sanity checked yet */
	if (system(buf) == 0) {
		struct stat sbuf;

		/* sanity checks */
		if (stat(temp, &sbuf) < 0) {
			(void)fprintf(stderr,
			    "vipw: can't stat temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}
		if (sbuf.st_size == 0) {
			(void)fprintf(stderr, "vipw: bad temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}
		ft = fopen(temp, "r");
		if (ft == NULL) {
			(void)fprintf(stderr,
			    "vipw: can't reopen temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}

		while (fgets(buf, sizeof (buf) - 1, ft) != NULL) {
			register char *cp;

			cp = index(buf, '\n');
			if (cp == 0)
				continue;	/* ??? allow very long lines
						 * and passwd files that do
						 * not end in '\n' ???
						 */
			*cp = '\0';

			cp = index(buf, ':');
			if (cp == 0)		/* lines without colon
						 * separated fields
						 */
				continue;
			*cp = '\0';

			if (strcmp(buf, "root"))
				continue;

			/* root password */
			*cp = ':';
			cp = index(cp + 1, ':');
			if (cp == 0)
				goto bad_root;

			/* root uid */
			if (atoi(cp + 1) != 0) {
				(void)fprintf(stderr, "root UID != 0:\n%s\n",
				    buf);
				break;
			}
			cp = index(cp + 1, ':');
			if (cp == 0)
				goto bad_root;

			/* root's gid */
			cp = index(cp + 1, ':');
			if (cp == 0)
				goto bad_root;

			/* root's gecos */
			cp = index(cp + 1, ':');
			if (cp == 0) {
bad_root:			(void)fprintf(stderr,
				    "Missing fields in root entry:\n%s\n", buf);
				break;
			}

			/* root's login directory */
			if (strncmp(++cp, "/:", 2)) {
				(void)fprintf(stderr,
				    "Root login directory != \"/\" or%s\n%s\n",
				    " default shell missing:", buf);
				break;
			}

			/* root's login shell */
			cp += 2;
			if (*cp && ! validsh(cp)) {
				(void)fprintf(stderr,
				    "Invalid root shell:\n%s\n", buf);
				break;
			}
			ok++;
		}
		(void)fclose(ft);
		if (ok) {
			if (chmod(temp, 0644) < 0) {
				(void)fprintf(stderr, "vipw: "); perror("chmod");
				goto bad;
			}
			if (rename(temp, passwd) < 0)
				(void)fprintf(stderr, "vipw: "), perror("rename");
		} else
			(void)fprintf(stderr,
			    "vipw: you mangled the temp file, %s unchanged\n",
			    passwd);
	}
bad:
	(void)unlink(temp);
	exit( ok ? 0 : 1 );
	/* NOTREACHED */
}

validsh(rootsh)
	char	*rootsh;
{
	char	*sh, *getusershell();
	int	ret = 0;

	setusershell();
	while( (sh = getusershell()) != NULL ) {
		if( strcmp( rootsh, sh) == 0 ) {
			ret = 1;
			break;
		}
	}
	endusershell();
	return(ret);
}
