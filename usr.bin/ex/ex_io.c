/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_io.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.19 */
#endif

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * File input/output, source, preserve and recover
 */

/*
 * Following remember where . was in the previous file for return
 * on file switching.
 */
int	altdot;
int	oldadot;
bool	wasalt;
short	isalt;

long	cntch;			/* Count of characters on unit io */
#ifndef VMUNIX
short	cntln;			/* Count of lines " */
#else
int	cntln;
#endif
long	cntnull;		/* Count of nulls " */
long	cntodd;			/* Count of non-ascii characters " */

/*
 * Parse file name for command encoded by comm.
 * If comm is E then command is doomed and we are
 * parsing just so user won't have to retype the name.
 */
filename(comm)
	int comm;
{
	register int c = comm, d;
	register int i;

	d = getchar();
	if (endcmd(d)) {
		if (savedfile[0] == 0 && comm != 'f')
			error("No file|No current filename");
		CP(file, savedfile);
		wasalt = (isalt > 0) ? isalt-1 : 0;
		isalt = 0;
		oldadot = altdot;
		if (c == 'e' || c == 'E')
			altdot = lineDOT();
		if (d == EOF)
			ungetchar(d);
	} else {
		ungetchar(d);
		getone();
		eol();
		if (savedfile[0] == 0 && c != 'E' && c != 'e') {
			c = 'e';
			edited = 0;
		}
		wasalt = strcmp(file, altfile) == 0;
		oldadot = altdot;
		switch (c) {

		case 'f':
			edited = 0;
			/* fall into ... */

		case 'e':
			if (savedfile[0]) {
				altdot = lineDOT();
				CP(altfile, savedfile);
			}
			CP(savedfile, file);
			break;

		default:
			if (file[0]) {
				if (c != 'E')
					altdot = lineDOT();
				CP(altfile, file);
			}
			break;
		}
	}
	if (hush && comm != 'f' || comm == 'E')
		return;
	if (file[0] != 0) {
		lprintf("\"%s\"", file);
		if (comm == 'f') {
			if (value(vi_READONLY))
				printf(" [Read only]");
			if (!edited)
				printf(" [Not edited]");
			if (tchng)
				printf(" [Modified]");
		}
		flush();
	} else
		printf("No file ");
	if (comm == 'f') {
		if (!(i = lineDOL()))
			i++;
		printf(" line %d of %d --%ld%%--", lineDOT(), lineDOL(),
		    (long) 100 * lineDOT() / i);
	}
}

/*
 * Get the argument words for a command into genbuf
 * expanding # and %.
 */
getargs()
{
	register int c;
	register char *cp, *fp;
	static char fpatbuf[32];	/* hence limit on :next +/pat */

	pastwh();
	if (peekchar() == '+') {
		for (cp = fpatbuf;;) {
			c = getchar();
			*cp++ = c;
			if (cp >= &fpatbuf[sizeof(fpatbuf)])
				error("Pattern too long");
			if (c == '\\' && isspace(peekchar()))
				c = getchar();
			if (c == EOF || isspace(c)) {
				ungetchar(c);
				*--cp = 0;
				firstpat = &fpatbuf[1];
				break;
			}
		}
	}
	if (skipend())
		return (0);
	CP(genbuf, "echo "); cp = &genbuf[5];
	for (;;) {
		c = getchar();
		if (endcmd(c)) {
			ungetchar(c);
			break;
		}
		switch (c) {

		case '\\':
			if (any(peekchar(), "#%|"))
				c = getchar();
			/* fall into... */

		default:
			if (cp > &genbuf[LBSIZE - 2])
flong:
				error("Argument buffer overflow");
			*cp++ = c;
			break;

		case '#':
			fp = altfile;
			if (*fp == 0)
				error("No alternate filename@to substitute for #");
			goto filexp;

		case '%':
			fp = savedfile;
			if (*fp == 0)
				error("No current filename@to substitute for %%");
filexp:
			while (*fp) {
				if (cp > &genbuf[LBSIZE - 2])
					goto flong;
				*cp++ = *fp++;
			}
			break;
		}
	}
	*cp = 0;
	return (1);
}

/*
 * Glob the argument words in genbuf, or if no globbing
 * is implied, just split them up directly.
 */
glob(gp)
	struct glob *gp;
{
	int pvec[2];
	register char **argv = gp->argv;
	register char *cp = gp->argspac;
	register int c;
	unsigned char ch;
	int nleft = NCARGS;

	gp->argc0 = 0;
	if (gscan() == 0) {
		register char *v = genbuf + 5;		/* strlen("echo ") */

		for (;;) {
			while (isspace((unsigned char)*v))
				v++;
			if (!*v)
				break;
			*argv++ = cp;
			while (*v && !isspace((unsigned char)*v))
				*cp++ = *v++;
			*cp++ = 0;
			gp->argc0++;
		}
		*argv = 0;
		return;
	}
	if (pipe(pvec) < 0)
		error("Can't make pipe to glob");
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error("Can't fork to do glob");
	}
	if (pid == 0) {
		int oerrno;

		close(1);
		dup(pvec[1]);
		close(pvec[0]);
		close(2);	/* so errors don't mess up the screen */
		open("/dev/null", 1);
		execlp(svalue(vi_SHELL), "sh", "-c", genbuf, (char *)0);
		oerrno = errno; close(1); dup(2); errno = oerrno;
		filioerr(svalue(vi_SHELL));
	}
	close(pvec[1]);
	do {
		*argv = cp;
		for (;;) {
			if (read(io, (char *)&ch, 1) != 1) {
				close(io);
				c = -1;
			} else
				c = ch;
			if (c <= 0 || isspace(c))
				break;
			*cp++ = c;
			if (--nleft <= 0)
				error("Arg list too long");
		}
		if (cp != *argv) {
			--nleft;
			*cp++ = 0;
			gp->argc0++;
			if (gp->argc0 >= NARGS)
				error("Arg list too long");
			argv++;
		}
	} while (c >= 0);
	waitfor();
	if (gp->argc0 == 0)
		error("No match");
}

/*
 * Scan genbuf for shell metacharacters.
 * Set is union of v7 shell and csh metas.
 */
gscan()
{
	register char *cp;

	for (cp = genbuf; *cp; cp++)
		if (any(*cp, "~{[*?$`'\"\\"))
			return (1);
	return (0);
}

/*
 * Parse one filename into file.
 */
struct glob G;
getone()
{
	register char *str;

	if (getargs() == 0)
		error("Missing filename");
	glob(&G);
	if (G.argc0 > 1)
		error("Ambiguous|Too many file names");
	str = G.argv[G.argc0 - 1];
	if (strlen(str) > FNSIZE - 4)
		error("Filename too long");
samef:
	CP(file, str);
}

/*
 * Read a file from the world.
 * C is command, 'e' if this really an edit (or a recover).
 */
rop(c)
	int c;
{
	register int i;
	struct stat stbuf;
	static int ovro;	/* old value(vi_READONLY) */
	static int denied;	/* 1 if READONLY was set due to file permissions */

	io = open(file, 0);
	if (io < 0) {
		if (c == 'e' && errno == ENOENT) {
			edited++;
			/*
			 * If the user just did "ex foo" he is probably
			 * creating a new file.  Don't be an error, since
			 * this is ugly, and it messes up the + option.
			 */
			if (!seenprompt) {
				printf(" [New file]");
				noonl();
				return;
			}
		}
		syserror(0);
	}
	if (fstat(io, &stbuf))
		syserror(0);
	switch (FTYPE(stbuf) & S_IFMT) {

	case S_IFBLK:
		error(" Block special file");

	case S_IFCHR:
		if (isatty(io))
			error(" Teletype");
		if (samei(&stbuf, "/dev/null"))
			break;
		error(" Character special file");

	case S_IFDIR:
		error(" Directory");

	}
	if (c != 'r') {
		if (value(vi_READONLY) && denied) {
			value(vi_READONLY) = ovro;
			denied = 0;
		}
		if ((FMODE(stbuf) & 0222) == 0 || access(file, 2) < 0) {
			ovro = value(vi_READONLY);
			denied = 1;
			value(vi_READONLY) = 1;
		}
	}
	if (hush == 0 && value(vi_READONLY)) {
		printf(" [Read only]");
		flush();
	}
	if (c == 'r')
		setdot();
	else
		setall();

	/* If it is a read command, then we must set dot to addr1
	 * (value of N in :Nr ).  In the default case, addr1 will
	 * already be set to dot.
	 * 
	 * Next, it is necessary to mark the beginning (undap1) and
	 * ending (undap2) addresses affected (for undo).  Note that
	 * rop2() and rop3() will adjust the value of undap2.
	 */
	if (FIXUNDO && inopen && c == 'r') {
		dot = addr1;
		undap1 = undap2 = dot + 1;
	}
	rop2();
	rop3(c);
}

rop2()
{
	line *first, *last, *a;
	struct stat statb;

	deletenone();
	clrstats();
	first = addr2 + 1;
	if (fstat(io, &statb) < 0)
		bsize = LBSIZE;
	else {
		bsize = min(statb.st_blksize, MAXBSIZE);
		if (bsize <= 0)
			bsize = LBSIZE;
	}
	ignore(append(getfile, addr2));
	last = dot;
	/*
	 *	if the modeline variable is set,
	 *	check the first and last five lines of the file
	 *	for a mode line.
	 */
	if (value(vi_MODELINE))	{	/* was vi_MODELINES */
		for (a=first; a<=last; a++) {
			if (a==first+5 && last-first > 10)
				a = last - 4;
			getline(*a);
			checkmodeline(linebuf);
		}
	}
}

rop3(c)
	int c;
{

	if (iostats() == 0 && c == 'e')
		edited++;
	if (c == 'e') {
		if (wasalt || firstpat) {
			register line *addr = zero + oldadot;

			if (addr > dol)
				addr = dol;
			if (firstpat) {
				globp = (*firstpat) ? firstpat : "$";
				commands(1,1);
				firstpat = 0;
			} else if (addr >= one) {
				if (inopen)
					dot = addr;
				markpr(addr);
			} else
				goto other;
		} else
other:
			if (dol > zero) {
				if (inopen)
					dot = one;
				markpr(one);
			}
		if(FIXUNDO)
			undkind = UNDNONE;
		if (inopen) {
			vcline = 0;
			vreplace(0, lines, lineDOL());
		}
	}
	if (laste) {
#ifdef VMUNIX
		tlaste();
#endif
		laste = 0;
		sync();
	}
}

/*
 * Are these two really the same inode?
 */
samei(sp, cp)
	struct stat *sp;
	char *cp;
{
	struct stat stb;

	if (stat(cp, &stb) < 0) 
		return (0);
	return (IDENTICAL((*sp), stb));
}

/* Returns from edited() */
#define	EDF	0		/* Edited file */
#define	NOTEDF	-1		/* Not edited file */
#define	PARTBUF	1		/* Write of partial buffer to Edited file */

/*
 * Write a file.
 */
wop(dofname)
bool dofname;	/* if 1 call filename, else use savedfile */
{
	register int c, exclam, nonexist;
	line *saddr1, *saddr2;
	struct stat stbuf;

	c = 0;
	exclam = 0;
	if (dofname) {
		if (peekchar() == '!')
			exclam++, ignchar();
		ignore(skipwh());
		while (peekchar() == '>')
			ignchar(), c++, ignore(skipwh());
		if (c != 0 && c != 2)
			error("Write forms are 'w' and 'w>>'");
		filename('w');
	} else {
		if (savedfile[0] == 0)
			error("No file|No current filename");
		saddr1=addr1;
		saddr2=addr2;
		addr1=one;
		addr2=dol;
		CP(file, savedfile);
		if (inopen) {
			vclrech(0);
			splitw++;
		}
		lprintf("\"%s\"", file);
	}
	nonexist = stat(file, &stbuf);
	switch (c) {

	case 0:
		if (!exclam && (!value(vi_WRITEANY) || value(vi_READONLY)))
		switch (edfile()) {
		
		case NOTEDF:
			if (nonexist)
				break;
			if (ISCHR(stbuf)) {
				if (samei(&stbuf, "/dev/null"))
					break;
				if (samei(&stbuf, "/dev/tty"))
					break;
			}
			io = open(file, 1);
			if (io < 0)
				syserror(0);
			if (!isatty(io))
				serror(" File exists| File exists - use \"w! %s\" to overwrite", file);
			close(io);
			break;

		case EDF:
			if (value(vi_READONLY))
				error(" File is read only");
			break;

		case PARTBUF:
			if (value(vi_READONLY))
				error(" File is read only");
			error(" Use \"w!\" to write partial buffer");
		}
cre:
/*
		synctmp();
*/
		io = creat(file, 0666);
		if (io < 0)
			syserror(0);
		writing = 1;
		if (hush == 0)
			if (nonexist)
				printf(" [New file]");
			else if (value(vi_WRITEANY) && edfile() != EDF)
				printf(" [Existing file]");
		break;

	case 2:
		io = open(file, 1);
		if (io < 0) {
			if (exclam || value(vi_WRITEANY))
				goto cre;
			syserror(0);
		}
		lseek(io, 0l, 2);
		break;
	}
	if (write_quit && inopen && (argc == 0 || morargc == argc))
		setty(normf);
	putfile(0);
	if (fsync(io) < 0)
		wrerror();
	ignore(iostats());
	if (c != 2 && addr1 == one && addr2 == dol) {
		if (eq(file, savedfile))
			edited = 1;
		sync();
	}
	if (!dofname) {
		addr1 = saddr1;
		addr2 = saddr2;
	}
	writing = 0;
}

/*
 * Is file the edited file?
 * Work here is that it is not considered edited
 * if this is a partial buffer, and distinguish
 * all cases.
 */
edfile()
{

	if (!edited || !eq(file, savedfile))
		return (NOTEDF);
	return (addr1 == one && addr2 == dol ? EDF : PARTBUF);
}

/*
 * Extract the next line from the io stream.
 */
char *nextip;

getfile()
{
	register short c;
	register char *lp; 
	register char *fp;

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			ninbuf = read(io, genbuf, bsize) - 1;
			if (ninbuf < 0) {
				if (lp != linebuf) {
					lp++;
					printf(" [Incomplete last line]");
					break;
				}
				return (EOF);
			}
			fp = genbuf;
			cntch += ninbuf+1;
		}
		if (lp >= &linebuf[LBSIZE]) {
			error(" Line too long");
		}
		c = *fp++;
		if (c == 0) {
			cntnull++;
			continue;
		}
		*lp++ = c;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	cntln++;
	return (0);
}

/*
 * Write a range onto the io stream.
 */
putfile(isfilter)
int isfilter;
{
	line *a1;
	register char *lp;
	register char *fp;
	register int nib;
	register bool ochng = chng;
	struct stat statb;

	chng = 1;		/* set to force file recovery procedures in */
				/* the event of an interrupt during write   */
	a1 = addr1;
	clrstats();
	cntln = addr2 - a1 + 1;
	if (fstat(io, &statb) < 0)
		bsize = LBSIZE;
	else {
		bsize = min(statb.st_blksize, MAXBSIZE);
		if (bsize <= 0)
			bsize = LBSIZE;
	}
	nib = bsize;
	fp = genbuf;
	do {
		getline(*a1++);
		lp = linebuf;
		for (;;) {
			if (--nib < 0) {
				nib = fp - genbuf;
				if (write(io, genbuf, nib) != nib) {
					wrerror();
				}
				cntch += nib;
				nib = bsize - 1;
				fp = genbuf;
			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	nib = fp - genbuf;
	if ((cntch == 0) && (nib == 1)) {
		cntln = 0;
		return;
	}
	if (write(io, genbuf, nib) != nib) {
		wrerror();
	}
	cntch += nib;
	chng = ochng;			/* reset chng to original value */
}

/*
 * A write error has occurred;  if the file being written was
 * the edited file then we consider it to have changed since it is
 * now likely scrambled. Ensure the display can never be corrupted in
 * vi mode even if the message is going to be multi-line by forcing the
 * "hit return" message thru a call to pofix.
 */
wrerror()
{
        pofix();
        if (eq(file, savedfile) && edited)
                change();
        syserror(1);
}


/*
 * Source command, handles nested sources.
 * Traps errors since it mungs unit 0 during the source.
 */
short slevel;
short ttyindes;

source(fil, okfail)
	char *fil;
	bool okfail;
{
	jmp_buf osetexit;
	register int saveinp, ointty, oerrno;
	char *saveglobp;
	short savepeekc;

	signal(SIGINT, SIG_IGN);
	saveinp = dup(0);
	savepeekc = peekc;
	saveglobp = globp;
	peekc = 0; globp = 0;
	if (saveinp < 0)
		error("Too many nested sources");
	if (slevel <= 0)
		ttyindes = saveinp;
	close(0);
	if (open(fil, 0) < 0) {
		oerrno = errno;
		setrupt();
		dup(saveinp);
		close(saveinp);
		errno = oerrno;
		if (!okfail)
			filioerr(fil);
		return;
	}
	slevel++;
	ointty = intty;
	intty = isatty(0);
	oprompt = value(vi_PROMPT);
	value(vi_PROMPT) &= intty;
	getexit(osetexit);
	setrupt();
	if (setexit() == 0)
		commands(1, 1);
	else if (slevel > 1) {
		close(0);
		dup(saveinp);
		close(saveinp);
		slevel--;
		resexit(osetexit);
		reset();
	}
	intty = ointty;
	value(vi_PROMPT) = oprompt;
	close(0);
	dup(saveinp);
	close(saveinp);
	globp = saveglobp;
	peekc = savepeekc;
	slevel--;
	resexit(osetexit);
}

/*
 * Clear io statistics before a read or write.
 */
clrstats()
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;
	cntodd = 0;
}

/*
 * Io is finished, close the unit and print statistics.
 */
iostats()
{

	close(io);
	io = -1;
	if (hush == 0) {
		if (value(vi_TERSE))
			printf(" %d/%D", cntln, cntch);
		else
			printf(" %d line%s, %D character%s", cntln, plural((long) cntln),
			    cntch, plural(cntch));
		if (cntnull || cntodd) {
			printf(" (");
			if (cntnull) {
				printf("%D null", cntnull);
				if (cntodd)
					printf(", ");
			}
			if (cntodd)
				printf("%D non-ASCII", cntodd);
			putchar(')');
		}
		noonl();
		flush();
	}
	return (cntnull != 0 || cntodd != 0);
}

#ifdef USG
#define index strchr
#define rindex strrchr
#endif

checkmodeline(line)
char *line;
{
	char *beg, *end;
	char cmdbuf[1024];
	char *index(), *rindex();
	bool savetty;
	int  savepeekc;

	beg = index(line, ':');
	if (beg == NULL)
		return;
	if (&beg[-3] < line)
		return;
	if (!(((beg[-3] == ' ' || beg[-3] == '\t')
	        && beg[-2] == 'e' && beg[-1] == 'x')
	     || ((beg[-3] == ' ' || beg[-3] == '\t')
	        && beg[-2] == 'v' && beg[-1] == 'i')))
		return;

	strncpy(cmdbuf, beg+1, sizeof cmdbuf);
	end = rindex(cmdbuf, ':');
	if (end == NULL)
		return;
	*end = 0;
	globp = cmdbuf;
	savepeekc = peekc;
	peekc = 0;
	savetty = intty;
	intty = 0;
	commands(1, 1);
	peekc = savepeekc;
	globp = 0;
	intty = savetty;
}
