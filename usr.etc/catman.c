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

#ifndef lint
static	char *sccsid = "@(#)catman.c 1.1 92/07/30 SMI"; /* from UCB 5.3 8/9/85 */
#endif not lint

/*
 * catman: update cat'able versions of manual pages
 *  (whatis database also)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <ctype.h>

#define	SYSTEM(str)	(pflag ? printf("%s\n", str) : system(str))

#define	TROFF		"troff"		/* default local name for troff */

#define	SOLIMIT		10		/* maximum allowed .so chain length */

/*
 * A list of known preprocessors to precede the formatter itself
 * in the formatting pipeline.  Preprocessors are specified by
 * starting a manual page with a line of the form:
 *	'\" X
 * where X is a string consisting of letters from the p_tag fields
 * below.
 */
struct preprocessor {
	char	p_tag;
	char	*p_nroff,
		*p_troff;
} preprocessors [] = {
	{'c',	"cw",				"cw"},
	{'e',	"neqn /usr/pub/eqnchar",	"eqn /usr/pub/eqnchar"},
	{'p',	"pic",				"pic"},
	{'r',	"refer",			"refer"},
	{'t',	"tbl",				"tbl"},
	{'v',	"vgrind -f",			"vgrind -f"},
	{0,	0,				0}
};

/*
 * Subdirectories to search for unformatted/formatted man page
 * versions, in nroff and troff variations.  The searching
 * code is structured to expect there to be two subdirectories
 * apiece, the first for unformatted files and the second for
 * formatted ones.
 */
char	*nroffdirs[] = { "man", "cat", 0 };
char	*troffdirs[] = { "man", "fmt", 0 };

char	pflag;
char	nflag;
char	wflag;
int	troffit;
char	buf[BUFSIZ];		/* buffer for first line of file */
char	cmdbuf[BUFSIZ];		/* buffer for reformatting command */
char	man[MAXPATHLEN+1];	/* unformatted entry or directory name */
char	cat[MAXPATHLEN+1];	/* formatted entry or directory name */
char	soman[MAXPATHLEN+1];	/* name of end of .so chain */
char	lncat[MAXPATHLEN+1];	/* formatted entry name to link to */
char	*mandir = "/usr/man";
char	*macros	= "-man";
char	*troffcmd;
char	*msuffix();
char	*getenv();
char	*index();
char	*rindex();

main(ac, av)
	int ac;
	char *av[];
{
	register char *msp, *sp;
	register char *sections;
	register int exstat = 0;
	register char changed = 0;
	char **subdirs;
	int socount;

	ac--, av++;
	while (ac > 0 && av[0][0] == '-') {
		switch (av[0][1]) {

		case 'M':
		case 'P':
			if (ac < 1) {
				fprintf(stderr, "%s: missing directory\n",
				    av[0]);
				exit(1);
			}
			ac--, av++;
			mandir = *av;
			break;

		case 'n':
			nflag++;
			break;

		case 'p':
			pflag++;
			break;

		case 'T':	/* Respecify tmac.an */
			if (ac < 1) {
				fprintf(stderr, "%s: missing filename\n",
				    av[0]);
				exit(1);
			}
			ac--, av++;
			macros = *av;
			break;

		case 't':
			troffit++;
			break;

		case 'w':
			wflag++;
			break;

		default:
			goto usage;
		}
		ac--, av++;
	}
	if (ac > 1) {
usage:
		printf(
"usage:\tcatman [-p] [-n] [-w] [-t] [-M path] [-T tmac.an] [sections]\n");
		exit(-1);
	}

	if (wflag)
		goto whatis;

	/*
	 * Collect environment information.
	 */
	if (troffit) {
		if ((troffcmd = getenv("TROFF")) == NULL)
			troffcmd = TROFF;
	}

	/*
	 * Establish list of subdirectories to search
	 * for acceptable man page versions.
	 */
	subdirs = troffit ? troffdirs : nroffdirs;

	if (chdir(mandir) < 0) {
		fprintf(stderr, "catman: "), perror(mandir);
		exit(1);
	}
	umask(02);

	sections = (ac == 1) ? *av : "12345678ln";
	for (sp = sections; *sp; sp++) {
		register DIR *mdir;
		register struct direct *dir;
		struct stat sbuf;

		sprintf(man, "%s%c", subdirs[0], *sp);
		sprintf(cat, "%s%c", subdirs[1], *sp);

		/*
		 * Open the man directory and extend its
		 * recorded name in preparation for constructing
		 * paths for files in the directory.
		 */
		if ((mdir = opendir(man)) == NULL) {
			fprintf(stderr, "opendir:");
			perror(man);
			exstat = 1;
			continue;
		}
		msp = man + strlen(man);
		*msp++ = '/';

		/*
		 * If the cat directory doesn't exist, create it.
		 */
		if (stat(cat, &sbuf) < 0) {
			char buf[MAXNAMLEN + 6], *cp;

			strcpy(buf, cat);
			cp = rindex(buf, '/');
			if (cp && cp[1] == '\0')
				*cp = '\0';
			if (pflag)
				printf("mkdir %s\n", buf);
			else if (mkdir(buf, 0775) < 0) {
				sprintf(buf, "catman: mkdir: %s", cat);
				perror(buf);
				continue;
			}
			stat(cat, &sbuf);
		}
		if ((sbuf.st_mode & 0777) != 0775)
			chmod(cat, 0775);

		/*
		 * Process each entry in the man directory.
		 */
		while ((dir = readdir(mdir)) != NULL) {
			time_t time;
			char *tsp;
			int  makelink;

			if (dir->d_ino == 0 || dir->d_name[0] == '.')
				continue;

			/*
			 * Make sure this is a man file, i.e., that it
			 * ends in .[0-9] or .[0-9][a-z] or .[nlo]
			 */
			tsp = rindex(dir->d_name, '.');
			if (tsp == NULL)
				continue;
			tsp++;
			if (isdigit(*tsp)) {
				if (*(tsp+1)) {
					tsp++;
					if (!isalpha(*tsp))
						continue;
				}
			} else if (*tsp != 'n' && *tsp != 'l' && *tsp != 'o')
				continue;
			if (*++tsp)
				continue;

			/*
			 * Record name of current unformatted and
			 * formatted entries.
			 */
			strcpy(msp, dir->d_name);
			sprintf(cat, "%s%s", subdirs[1], msuffix(man));

			/*
			 * Resolve .so chains and get name and
			 * first line of file to be formatted.
			 */
			if ((makelink = sofollow(man, soman, buf)) < 0)
				continue;

			if (stat(cat, &sbuf) >= 0) {
				/*
				 * The formatted entry already exists.
				 * Blow it away if it's out of date wrt
				 * the corresponding unformatted entry
				 * (i.e., wrt the end of the .so chain).
				 */
				time = sbuf.st_mtime;
				(void) stat(soman, &sbuf);
				if (time >= sbuf.st_mtime)
					continue;
				if (pflag)
					printf("rm %s\n", cat);
				else
					(void) unlink(cat);
			}

			/* Create the cat file. */
			if (makelink) {
				/*
				 * Get relative pathname of cat wrt lncat
				 * to use as contents of symlink.  Note that
				 * we assume that both source and target
				 * of the link live exactly one directory
				 * down from mandir.
				 */
				sprintf(lncat, "../%s%s", subdirs[1],
						msuffix(soman));
				if (pflag)
					printf("ln -s %s %s\n", lncat, cat);
				else
					symlink(lncat, cat);
			}
			else {
				if (mkfmtcmd(cmdbuf, soman, cat, buf) < 0) {
					exstat = 1;
					continue;
				}
				SYSTEM(cmdbuf);
			}
			changed = 1;

		}
		closedir(mdir);
	}

	/* Rebuild the whatis database. */
	if (changed && !nflag) {
whatis:
		if (!pflag) {
			execl("/bin/sh", "/bin/sh",
			    "/usr/lib/makewhatis", mandir, 0);
			perror("/bin/sh /usr/lib/makewhatis");
			exstat = 1;
		} else
			printf("/bin/sh /usr/lib/makewhatis %s\n", mandir);
	}
	exit(exstat);
	/* NOTREACHED */
}

/*
 * Follow a chain of .so'ed manual pages to its end or until
 * encountering SOLIMIT links in the chain.  Fname points to
 * a buffer containing the manual page's name.  Soname is set
 * to contain the name of the final man page file in the chain.
 * Fbuf is a pointer to a buffer of length BUFSIZ that is set
 * to contain the first line of the final man page file.
 * Return -1 on error, else return the number of links in the chain.
 */
sofollow(fname, soname, fbuf)
	char *fname, *soname, *fbuf;
{
	register FILE *inf;
	register int socount = 0;
	register char *cp, *slp;

	/* Set up the final name. */
	strcpy(soname, fname);

	for ( ;; ) {
		/* Get first line of current file. */
		if ((inf = fopen(soname, "r")) == NULL) {
			/*
			 * If we've actually done a .so, name both
			 * the head of the chain and the current
			 * position in the error message.
			 */
			fflush(stdout);
			if (socount > 0) {
				fprintf(stderr, "%s (.so'ed from %s): ",
					soname, fname);
				perror ("");
			}
			else
				perror(soname);
			return (-1);
		}
		if (fgets(fbuf, BUFSIZ - 1, inf) == NULL) {
			fclose(inf);
			return (-1);
		}
		fclose(inf);

		if (strncmp(fbuf, ".so ", sizeof ".so " - 1) != 0)
			return (socount);

		if (++socount > SOLIMIT) {
			fflush(stdout);
			fprintf(stderr, ".so chain leading from %s too long\n",
				fname);
			return (-1);
		}

		cp = fbuf + sizeof ".so " - 1;

		/*
		 * Verify that the target of the .so is well-formed.
		 * It must be relative to the base of the manual subtree.
		 * For this implementation, we can deal with exactly
		 * one level of subdirectory from the base.
		 */
		slp = index(cp, '/');
		if (slp == 0 || slp == cp || slp != rindex(cp, '/')) {
			fflush(stdout);
			fprintf(stderr,
"target of .so in %s must be relative to %s\n",
				soname, mandir);
			return (-1);
		}

		/* Replace the file name with its referent. */
		strcpy(soname, cp);
		cp = index(soname, '\n');
		if (cp)
			*cp = '\0';
		/*
		 * Compensate for sloppy typists by stripping
		 * trailing white space.
		 */
		cp = soname + strlen(soname);
		while (--cp >= soname && (*cp == ' ' || *cp == '\t'))
			*cp = '\0';
	}
}

/*
 * Set cbuf to contain a command line to reformat the man page whose
 * unformatted source is named by rawp, placing the results in the file
 * named by fmtp.  Check the contents of the line pointed to by linep
 * for preprocessor directives, using them to construct a suitable command
 * line.  If troffit is set, format with troffcmd, otherwise with "nroff".
 * Return 0 on success, -1 on error.
 */
mkfmtcmd(cbuf, rawp, fmtp, linep)
	char *cbuf, *rawp, *fmtp, *linep;
{
	register int pipestage = 0;
	register int needcol = 0;
	register char *cbp = cbuf;

	/*
	 * Look for preprocessor directives.
	 */
	if (strncmp(linep, "'\\\" ", sizeof "'\\\" " - 1) == 0) {
		register char *ptp = linep + sizeof "'\\\" " - 1;

		while (*ptp && *ptp != '\n') {
			register struct preprocessor *pp;

			/*
			 * Check for a preprocessor we know about.
			 */
			for (pp = preprocessors; pp->p_tag; pp++) {
				if (pp->p_tag == *ptp)
					break;
			}
			if (pp->p_tag == 0) {
				fprintf(stderr,
					"unknown preprocessor specifier %c\n",
					*ptp);
				fflush(stderr);
				return (-1);
			}

			/*
			 * Add it to the pipeline.
			 */
			sprintf(cbp, "%s %s | ",
				troffit ? pp->p_troff : pp->p_nroff,
				pipestage++ == 0 ? rawp : "-");
			cbp += strlen(cbp);

			/*
			 * Special treatment: if tbl is among the
			 * preprocessors and we'll process with
			 * nroff, we have to pass things through
			 * col at the end of the pipeline.
			 */
			if (pp->p_tag == 't' && !troffit)
				needcol++;

			ptp++;
		}
	}

	/*
	 * Tack on the formatter specification.
	 */
	sprintf(cbp, "%s%s %s %s%s > %s",
		troffit ? troffcmd : "nroff -Tman",
		troffit ? " -t" : "",
		macros,
		pipestage == 0 ? rawp : "-",
		troffit ? "" : " | col",
		fmtp);
}

/*
 * Given a string of the form `xn/y', with n a single character
 * return a pointer to the substring starting at `n'.  If the
 * input string is ill-formed, return it as is.
 */
char *
msuffix(cp)
	char *cp;
{
	register char *sp = index(cp, '/');

	return (sp && sp > cp ? sp - 1 : cp);
}
