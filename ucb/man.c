/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static	char sccsid[] = "@(#)man.c 1.1 92/07/30 SMI"; /* from UCB 5.6 8/29/85 */
#endif not lint

/*
 * man
 * link also to apropos and whatis
 * This version uses more for underlining and paging.
 */

#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <strings.h>

/*
 * The default search path for man subtrees.  Note that
 * System V can be handled by changing this value to
 * "/usr/man/u_man:/usr/man/a_man:/usr/man/local".
 */
#define	MANDIR	"/usr/man"


/*
 * Names for formatting and display programs.  The values given
 * below are reasonable defaults, but sites with source may
 * wish to modify them to match the local environment.  The
 * value for TCAT is particularly problematic as there's no
 * accepted standard value available for it.  (The definition
 * below assumes C.A.T. troff output and prints it).
 */
#define	MORE	"more -s"	/* default paging filter */
#define	CAT_	"/bin/cat"	/* for when output is not a tty */
#define	CAT_S	"/bin/cat -s"	/* for '-' opt (no more) */

#define	TROFF	"troff"		/* local name for troff */
#define	TCAT	"lpr -t"	/* command to "display" troff output */


/*
 * The subsection values given below are the union of all known
 * from 4bsd-derived systems.  Sites missing some of these subsections
 * can speed things up a bit by pruning the unused ones.
 */
#define	ALLSECT	"1nl6823457po"	/* order to look through sections */
#define	SECT1	"1nlo"		/* sections to look at if 1 is specified */
#define	SUBSEC1	"bcgmprsvw"	/* subsections to try in section 1 */
#define	SUBSEC2	"bvw"
#define	SUBSEC3	"bcfjklmnprsvwx"
#define	SUBSEC4	"bfmnpsv"
#define	SUBSEC5	"bv"
#define	SUBSEC7	"bv"
#define	SUBSEC8	"bcvs"

#define	WHATIS	"whatis"

#define	SOLIMIT	10		/* maximum allowed .so chain length */

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
 * code in manual() is structured to expect there to be two
 * subdirectories apiece, the first for unformatted files
 * and the second for formatted ones.
 */
char	*nroffdirs[] = { "man", "cat", 0 };
char	*troffdirs[] = { "man", "fmt", 0 };

char	tmpname[15];		/* name of temporary file */

int	nomore;
int	troffit;
char	*CAT	= CAT_;
char	*manpath = MANDIR;
char	*macros	= "-man";
char	*pager;
char	*troffcmd;
char	*troffcat;
char	*calloc();
char	*getenv();
char	*trim();
char	*msuffix();
int	remove();
int	apropos();
int	whatis();

#define	eq(a,b)	(strcmp(a,b) == 0)

main(argc, argv)
	int argc;
	char *argv[];
{
	char section, subsec;
	char *mp;
	char *cmdname;

	/* If defined, use environment value as default manual path list. */
	if ((mp = getenv("MANPATH")) != NULL)
		manpath = mp;

	(void) sprintf(tmpname, "/tmp/man%d", getpid());
	(void) umask(0);

	/* get base part of command name */
	if ((cmdname = rindex(argv[0], '/')) != NULL)
		cmdname++;
	else
		cmdname = argv[0];

	if (eq(cmdname, "apropos") || eq(cmdname, "whatis")) {
		runpath(argc-1, argv+1,
			cmdname[0] == 'a' ? apropos : whatis);
		exit(0);
	}

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch(argv[0][1]) {

		case 0:
			nomore++;
			CAT = CAT_S;
			break;

		case 'f':
			whatis(argc-1, argv+1);
			exit(0);

		case 'k':
			apropos(argc-1, argv+1);
			exit(0);

		case 'P':	/* Backwards compatibility */
		case 'M':	/* Respecify path for man pages. */
			if (argc < 2) {
				(void) fprintf(stderr, "%s: missing path\n",
					*argv);
				exit(1);
			}
			argc--, argv++;
			manpath = *argv;
			break;

		case 'T':	/* Respecify tmac.an */
			argc--, argv++;
			macros = *argv;
			break;

		case 't':
			troffit++;
			break;

		default:
			goto usage;
		}
		argc--, argv++;
	}
	if (argc <= 0) {
usage:
		(void) fprintf(stderr, "\
Usage:\t%s [-] [-t] [-M path] [-T tmac.an] [ section ] name ...\n\
\t%s -k keyword ...\n\
\t%s -f file ...\n",
			cmdname, cmdname, cmdname);
		exit(1);
	}

	if (troffit == 0 && nomore == 0 && !isatty(fileno(stdout)))
		nomore++;

	/*
	 * Collect environment information.
	 */
	if (troffit) {
		if ((troffcmd = getenv("TROFF")) == NULL)
			troffcmd = TROFF;
		if ((troffcat = getenv("TCAT")) == NULL)
			troffcat = TCAT;
	}
	else {
		if (((pager = getenv("PAGER")) == NULL) ||
		    (*pager == NULL))
			pager = MORE;
	}

	/*
	 * The manual routine contains windows during which
	 * termination would leave a temp file behind.  Thus
	 * we blanket the whole thing with a clean-up routine.
	 */
	if (signal(SIGINT, SIG_IGN) == SIG_DFL) {
		(void) signal(SIGINT, remove);
		(void) signal(SIGQUIT, remove);
		(void) signal(SIGTERM, remove);
	}

	section = 0;
	do {
		if (eq(argv[0], "l") || (eq(argv[0], "local"))) {
			section = 'l';
			goto sectin;
		} else if (eq(argv[0], "n") || (eq(argv[0], "new"))) {
			section = 'n';
			goto sectin;
		} else if (eq(argv[0], "o") || (eq(argv[0], "old"))) {
			section = 'o';
			goto sectin;
		} else if (eq(argv[0], "p") || (eq(argv[0], "public"))) {
			section = 'p';
			goto sectin;
		} else if (argv[0][0] >= '1' && argv[0][0] <= '8' &&
		    (argv[0][1] == 0 || argv[0][2] == 0)) {
			section = argv[0][0];
			subsec = argv[0][1];
sectin:
			argc--, argv++;
			if (argc == 0) {
				(void) fprintf(stderr,
					"But what do you want from section %s?\n",
					argv[-1]);
				exit(1);
			}
			continue;
		}
		manual(section, subsec, argv[0]);
		argc--, argv++;
	} while (argc > 0);
	exit(0);
	/* NOTREACHED */
}

runpath(ac, av, f)
	int ac;
	char *av[];
	int (*f)();
{
	if (ac > 0 && (strcmp(av[0], "-M") == 0 || strcmp(av[0], "-P") == 0)) {
		if (ac < 2) {
			(void) fprintf(stderr, "%s: missing path\n", av[0]);
			exit(1);
		}
		manpath = av[1];
		ac -= 2, av += 2;
	}
	(*f)(ac, av);
	exit(0);
}

/*
 * Process the title specified by the arguments.
 */
manual(sec, subsec, name)
	char sec, subsec;
	char *name;
{
	char section = sec;
	char *sp = ALLSECT;
	char **subdirs;
	int last, ss;
	int socount, regencat, updatedcat, catonly;
	struct stat mansb, catsb;
	char manbuf[BUFSIZ];
	char title[200], path[200];
	char manpname[MAXPATHLEN+1], catpname[MAXPATHLEN+1];

	/*
	 * Establish list of subdirectories to search
	 * for acceptable man page versions.
	 */
	subdirs = troffit ? troffdirs : nroffdirs;

	/*
	 * Note the side-effect in this definition: if the cat page
	 * exists, but the man page doesn't, catonly will be set nonzero.
	 */
#define	checkpage() ((catonly = pathfind(title, subdirs, path)) >= 0)

	/*
	 * Set title to contain the (relative) pathname
	 * of the unformatted manual page.
	 */
	(void) sprintf(title, "manx/%s.x", name);
	last = strlen(title) - 1;
	if (section == '1') {
		sp = SECT1;
		section = 0;
	}
	if (section == 0) {
		/* Any section will do... */

		ss = 0;
		for (section = *sp++; section; section = *sp++) {
			title[3] = section;
			title[last] = section;
			title[last+1] = 0;
			title[last+2] = 0;
			if (checkpage())
				break;
			if (title[last] >= '1' && title[last] <= '8') {
				register char *cp;
search:
				switch (title[last]) {
				case '1': cp = SUBSEC1; break;
				case '2': cp = SUBSEC2; break;
				case '3': cp = SUBSEC3; break;
				case '4': cp = SUBSEC4; break;
				case '5': cp = SUBSEC5; break;
				case '7': cp = SUBSEC7; break;
				case '8': cp = SUBSEC8; break;
				default:  cp = ""; break;
				}
				while (*cp) {
					title[last+1] = *cp++;
					if (checkpage()) {
						ss = title[last+1];
						goto found;
					}
				}
				if (ss == 0)
					title[last+1] = 0;
			}
		}
		if (section == 0) {
			if (sec == 0)
				(void) printf("No manual entry for %s.\n",
					name);
			else
				(void) printf("No entry for %s in section %c of the manual.\n",
					name, sec);
			return;
		}
	} else {
		/* Explicit section specified. */
		title[3] = section;
		title[last] = section;
		title[last+1] = subsec;
		title[last+2] = 0;
		if (!checkpage()) {
			if ((section >= '1' && section <= '8') && subsec == 0) {
				sp = "\0";
				goto search;
			}
			else if (section == 'o') {
				char *cp;
				char sec;

				for (sec = '1'; sec <= '8'; sec++) {
					title[last] = sec;
					if (checkpage())
						goto found;
					switch (title[last]) {
					case '1': cp = SUBSEC1; break;
					case '2': cp = SUBSEC2; break;
					case '3': cp = SUBSEC3; break;
					case '4': cp = SUBSEC4; break;
					case '5': cp = SUBSEC5; break;
					case '7': cp = SUBSEC7; break;
					case '8': cp = SUBSEC8; break;
					default:  cp = ""; break;
					}
					while (*cp) {
						title[last+1] = *cp++;
						if (checkpage()) {
							ss = title[last+1];
							goto found;
						}
					}
					if (ss == 0)
						title[last+1] = 0;
				}
			}
			(void) printf("No entry for %s in section %c", name,
				section);
			if (subsec)
				(void) printf("%c", subsec);
			(void) printf(" of the manual.\n");
			return;
		}
	}

found:
	/*
	 * At this point we've found a match for the man page specified
	 * by our arguments.  However, it may be an indirect reference
	 * to another man page.
	 */

	/*
	 * Take care of indirect references to other man pages;
	 * i.e., resolve files containing only ".so manx/file.x".
	 * We follow .so chains, replacing title with the .so'ed
	 * file at each stage, and keeping track of how many times
	 * we've done so, so that we can avoid looping.
	 */
	for (socount = 0; ; ) {
		register FILE *md;
		register char *cp;

		if (catonly)
			break;

		/*
		 * Construct the title's corresponding man page
		 * pathname.
		 */
		(void) sprintf(manpname, "%s/%s%s", path, subdirs[0],
			msuffix(title));

		/*
		 * Grab manpname's first line, stashing it in manbuf.
		 */
		if ((md = fopen(manpname, "r")) == NULL) {
			perror(manpname);
			return;
		}
		if (fgets(manbuf, BUFSIZ-1, md) == NULL) {
			(void) fclose(md);
			(void) fprintf(stderr, "%s: null file\n", manpname);
			(void) fflush(stderr);
			return;
		}
		(void) fclose(md);

		if (strncmp(manbuf, ".so ", sizeof ".so " - 1))
			break;
		if (++socount > SOLIMIT) {
			(void) fprintf(stderr, ".so chain too long\n");
			(void) fflush(stderr);
			return;
		}
		(void) strcpy(title, manbuf + sizeof ".so " - 1);
		cp = rindex(title, '\n');
		if (cp)
			*cp = '\0';
		/*
		 * Compensate for sloppy typists by stripping
		 * trailing white space.
		 */
		cp = title + strlen(title);
		while (--cp >= title && (*cp == ' ' || *cp == '\t'))
			*cp = '\0';

		/*
		 * Go off and find the next link in the chain.
		 */
		if (!checkpage()) {
			(void) fprintf(stderr,
				"Can't find referent of .so in %s\n",
				manpname);
			(void) fflush(stderr);
			return;
		}
	}

#undef	checkpage

	/*
	 * Get pathnames for the formatted and unformatted
	 * versions of title.
	 */
	(void) sprintf(catpname, "%s/%s%s", path, subdirs[1], msuffix(title));
	if (!catonly)
		(void) sprintf(manpname, "%s/%s%s", path, subdirs[0],
			msuffix(title));

#ifdef DEBUG
	(void) printf("title: %s, cat page: %s, man page: %s\n", title,
		catpname, catonly ? "" : manpname);
#endif DEBUG

	/*
	 * Obtain the cat page that corresponds to the man page.
	 * If it already exists, is up to date, and if we haven't
	 * been told not to use it, use it as it stands.
	 */
	regencat = updatedcat = 0;
	if (!catonly && stat(manpname, &mansb) >= 0 &&
			(stat(catpname, &catsb) < 0 ||
			 catsb.st_mtime < mansb.st_mtime)) {
		/*
		 * Construct a shell command line for formatting manpname.
		 * The resulting file goes initially into /tmp.  If possible,
		 * it will later be moved to catpname.
		 */

		int pipestage = 0;
		char cmdbuf[200];
		char *cbp = cmdbuf;

		regencat = updatedcat = 1;

		(void) fprintf(stderr, "Reformatting page.  Wait...");
		(void) fflush(stderr);

		/*
		 * Recover the relevant component of manpath and
		 * add a cd to it at the front of the command line.
		 * Doing so allows embedded relative .so commands
		 * to work correctly.
		 *
		 * This code is uglier than it should be, but that's
		 * what comes of twisting the design around to add
		 * afterthoughts.  It's yet another place that knows
		 * that there's exactly one level of subdirectory in
		 * each man subtree.
		 */
		{
			char	*slashp = rindex(manpname, '/');

			/* Back up to the previous slash. */
			while (slashp && --slashp >= manpname && *slashp != '/')
				continue;
			if (slashp > manpname) {
				int len = slashp - manpname;

				(void) sprintf(cbp, "cd %.*s; ",
					len, manpname);
				cbp += strlen(cbp);
			}
		}

		/*
		 * Check for special formatting requirements by examining
		 * manpname's first line (which we read earlier) for
		 * preprocessor specifications.
		 */
		if (strncmp(manbuf, "'\\\" ", sizeof "'\\\" " - 1) == 0) {
			register char *ptp = manbuf + sizeof "'\\\" " - 1;

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
					(void) fprintf(stderr,
						"unknown preprocessor specifier %c\n",
						*ptp);
					(void) fflush(stderr);
					return;
				}

				/*
				 * Add it to the pipeline.
				 */
				(void) sprintf(cbp, "%s %s | ",
					troffit ? pp->p_troff : pp->p_nroff,
					pipestage++ == 0 ? manpname : "-");
				cbp += strlen(cbp);

				ptp++;
			}
		}

		/*
		 * Tack on the formatter specification.
		 */
		(void) sprintf(cbp, "%s%s %s %s%s > %s",
			troffit ? troffcmd : "nroff -Tman",
			troffit ? " -t" : "",
			macros,
			pipestage == 0 ? manpname : "-",
			troffit ? "" : " | col",
			tmpname);

#ifdef	DEBUG
		(void) printf("\n%s\n", cmdbuf);
#endif	DEBUG

		/* Reformat the page. */
		if (system(cmdbuf)) {
			(void) fprintf(stderr, " aborted (sorry)\n");
			(void) fflush(stderr);
			(void) unlink(tmpname);
			return;
		}

		/*
		 * Attempt to move the cat page to its proper home.
		 */
		(void) sprintf(cmdbuf,
			"trap '' 1 15; /bin/mv -f %s %s 2> /dev/null",
			tmpname,
			catpname);
		if (system(cmdbuf))
			updatedcat = 0;

		(void) fprintf(stderr, " done\n");
		(void) fflush(stderr);
	}

	/*
	 * Reset catpname to name the formatted output,
	 * wherever it may be.
	 */
	if (regencat && !updatedcat)
		(void) strcpy(catpname, tmpname);

	/*
	 * Dispose of the formatted page,
	 * presenting it to our invoker, etc.
	 */
	if (troffit)
		troff(catpname, nomore);
	else
		nroff(catpname, nomore);

	/*
	 * Get rid of dregs.
	 */
	(void) unlink(tmpname);
}

/*
 * Generate and examine paths of the form `manpath[i]/subdirs[j]/nametail',
 * performing a depth-first search (varying j most rapidly) for the
 * first existent file.  Here, `name' is assumed to be of the form
 * `xxxn/tail', where `n' is a single character.
 *
 * Return -1 if not found.  Otherwise, copy manpath[i] into respath
 * and return j.
 *
 */
pathfind(name, subdirs, respath)
	char *name, **subdirs, respath[];
{
	register char *mp, *tp, *ntp, *ep;
	char **sdp;
	struct stat statb;

	/*
	 * Set ntp to point immediately before the slash preceding nametail.
	 * Thus, ntp will be of the form `n/tail'.
	 */
	if ((ntp = msuffix(name)) == name)
		return (-1);

	for (mp = manpath; mp && *mp; mp = tp) {
		/* Obtain position of end of current manpath component. */
		tp = index(mp, ':');

		for (sdp = subdirs; *sdp; sdp++) {
			if (tp) {
				if (tp == mp) {
					/* Null manpath component mapped to . */
					(void) sprintf(respath, "./%s%s", *sdp,
						ntp);
					ep = respath + 1;
				}
				else {
					/* Embedded manpath component. */
					(void) sprintf(respath, "%.*s/%s%s",
						tp - mp, mp, *sdp, ntp);
					ep = respath + (tp - mp);
				}
			}
			else {
				/* Last component in manpath. */
				(void) sprintf(respath, "%s/%s%s", mp, *sdp,
					ntp);
				ep = respath + strlen(mp);
			}

			/* Check the file. */
			if (stat(respath, &statb) >= 0) {
				*ep = '\0';
				return (sdp - subdirs);
			}
		}

		/* Move to beginning of next manpath component. */
		if (tp)
			tp++;
	}

	return (-1);
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

/*
 * Arrange to display the nroffed man page we just generated.
 * If plain is true, use CAT instead of the pager.
 */
nroff(filep, plain)
	char *filep;
	int plain;
{
	char cmdbuf[BUFSIZ];

	(void) sprintf(cmdbuf, "%s %s", plain ? CAT : pager, filep);
	(void) system(cmdbuf);
}

/*
 * Arrange to display the troffed man page we just generated.
 * If plain is true, don't bother.
 */
troff(filep, plain)
	char *filep;
	int plain;
{
	char cmdbuf[BUFSIZ];

	if (!plain) {
		(void) sprintf(cmdbuf, "%s %s", troffcat, filep);
		(void) system(cmdbuf);
	}
}

any(c, sp)
	register int c;
	register char *sp;
{
	register int d;

	while (d = *sp++)
		if (c == d)
			return (1);
	return (0);
}

/*
 * Clean things up after receiving a signal.
 */
remove()
{
	(void) unlink(tmpname);
	exit(1);
}

apropos(argc, argv)
	int argc;
	char **argv;
{
	char buf[BUFSIZ], file[MAXPATHLEN+1];
	char *gotit, *cp, *tp;
	register char **vp;

	if (argc == 0) {
		(void) fprintf(stderr, "apropos what?\n");
		exit(1);
	}
	gotit = (char *) calloc(1, blklen(argv));
	for (cp = manpath; cp; cp = tp) {
		tp = index(cp, ':');
		if (tp) {
			if (tp == cp)
				(void) strcpy(file, WHATIS);
			else
				(void) sprintf(file, "%.*s/%s", tp-cp, cp,
					WHATIS);
			tp++;
		} else
			(void) sprintf(file, "%s/%s", cp, WHATIS);
		if (freopen(file, "r", stdin) == NULL) {
			perror(file);
			continue;
		}
		while (fgets(buf, sizeof buf, stdin) != NULL)
			for (vp = argv; *vp; vp++)
				if (match(buf, *vp)) {
					(void) printf("%s", buf);
					gotit[vp - argv] = 1;
					for (vp++; *vp; vp++)
						if (match(buf, *vp))
							gotit[vp - argv] = 1;
					break;
				}
	}
	for (vp = argv; *vp; vp++)
		if (gotit[vp - argv] == 0)
			(void) printf("%s: nothing appropriate\n", *vp);
}

match(buf, str)
	char *buf, *str;
{
	register char *bp;

	bp = buf;
	for (;;) {
		if (*bp == 0)
			return (0);
		if (amatch(bp, str))
			return (1);
		bp++;
	}
}

amatch(cp, dp)
	register char *cp, *dp;
{

	while (*cp && *dp && lmatch(*cp, *dp))
		cp++, dp++;
	if (*dp == 0)
		return (1);
	return (0);
}

lmatch(c, d)
	char c, d;
{

	if (c == d)
		return (1);
	if (!isalpha(c) || !isalpha(d))
		return (0);
	if (islower(c))
		c = toupper(c);
	if (islower(d))
		d = toupper(d);
	return (c == d);
}

blklen(ip)
	register int *ip;
{
	register int i = 0;

	while (*ip++)
		i++;
	return (i);
}

whatis(argc, argv)
	int argc;
	char **argv;
{
	register char *gotit, **vp;
	char buf[BUFSIZ], file[MAXPATHLEN+1], *cp, *tp;

	if (argc == 0) {
		(void) fprintf(stderr, "whatis what?\n");
		exit(1);
	}
	for (vp = argv; *vp; vp++)
		*vp = trim(*vp);
	gotit = (char *)calloc(1, blklen(argv));
	for (cp = manpath; cp; cp = tp) {
		tp = index(cp, ':');
		if (tp) {
			if (tp == cp)
				(void) strcpy(file, WHATIS);
			else
				(void) sprintf(file, "%.*s/%s", tp-cp, cp,
					WHATIS);
			tp++;
		} else
			(void) sprintf(file, "%s/%s", cp, WHATIS);
		if (freopen(file, "r", stdin) == NULL) {
			perror(file);
			continue;
		}
		while (fgets(buf, sizeof buf, stdin) != NULL)
			for (vp = argv; *vp; vp++)
				if (wmatch(buf, *vp)) {
					(void) printf("%s", buf);
					gotit[vp - argv] = 1;
					for (vp++; *vp; vp++)
						if (wmatch(buf, *vp))
							gotit[vp - argv] = 1;
					break;
				}
	}
	for (vp = argv; *vp; vp++)
		if (gotit[vp - argv] == 0)
			(void) printf("man: man entry for %s not found\n", *vp);
}

wmatch(buf, str)
	char *buf, *str;
{
	register char *bp, *cp;

	bp = buf;
again:
	cp = str;
	while (*bp && *cp && lmatch(*bp, *cp))
		bp++, cp++;
	if (*cp == 0 && (*bp == '(' || *bp == ',' || *bp == '\t' || *bp == ' '))
		return (1);
	while (*bp != '(' && *bp != ',' && *bp != '\t' && *bp != ' ' && *bp)
		bp++;
	if (*bp != ',')
		return (0);
	bp++;
	while (isspace(*bp))
		bp++;
	goto again;
}

char *
trim(cp)
	register char *cp;
{
	register char *dp;

	for (dp = cp; *dp; dp++)
		if (*dp == '/')
			cp = dp + 1;
	if (cp[0] != '.') {
		if (cp + 3 <= dp && dp[-2] == '.' &&
		    any(dp[-1], "cosa12345678npP"))
			dp[-2] = 0;
		if (cp + 4 <= dp && dp[-3] == '.' &&
		    any(dp[-2], "13") && isalpha(dp[-1]))
			dp[-3] = 0;
	}
	return (cp);
}
