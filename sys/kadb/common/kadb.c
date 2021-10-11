#ifndef lint
static  char sccsid[] = "@(#)kadb.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * kadb - support glue for kadb
 */

/*
 * These include files come from the adb source directory.
 */
#include "adb.h"
#include "symtab.h"

#include "pf.h"
#include <debug/debugger.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ptrace.h>
#include <machine/cpu.h>

#define	reset()	_longjmp(abort_jmp, 1)

int infile;
int eof;
char **environ;			/* hack- needed for libc setlocale call 

int wtflag = 2;			/* pretend we opened for r/w */
int debugging = 0;		/* patchable value */
int dottysync = 1;		/* call monitor's initgetkey() routine */
int foundu = 0;			/* found uarea */
struct user *uunix;		/* address of uarea */
int print_cpu_done = 0;		/* Only need for MP, but kadb makes it tough */

char myname_default[] = "kadb";	/* default name for debugger */

#define	MAC_FD	0x80

debuginit(fd, exec, name)
	int fd;
	struct exec *exec;
	char *name;
{
	char *s, *index();

	s = index(name, '/');	/* used to be a ')', in 4c code, WHY? */
	if (s)
		s++;
	else
		s = name;

	symfil = malloc(strlen(s) + 1);
	if (symfil)
		strcpy(symfil, s);
	else {
		printf("malloc failed\n");
		symfil = "-";
	}
	corfil = "-";
	fsym = fd;		/* set fd for source file */
	fcor = -1;

	filhdr = *exec;		/* structure assignment */

	setsym();
	setvar();
	pid = 0xface;		/* fake pid as the process is `alive' */

	(void) lookup("_uunix");
	if (cursym != 0) {
		foundu = 1;
		uunix = (struct user *) cursym->s_value;
	}
}

static jmp_buf jb;

debugcmd()
{

	for (;;) {
		/*
		 * The next block of code is for first time through only so
		 * that when we are first entered everything looks good.
		 */
		bpwait(PTRACE_CONT);
		if (reg->r_pc != 0) {
			_printf("stopped at%16t");	/* use adb printf */
			printpc();
		}

		abort_jmp = jb;
		(void) _setjmp(jb);
		if (executing)
			delbp();
		executing = 0;
		for (;;) {
			killbuf();
			if (errflg) {
				printf("%s\n", errflg);
				errflg = 0;
			}
			if (interrupted) {
				interrupted = 0;
				lastcom = 0;
				printf("\n");
				doreset();
				/*NOTREACHED*/
			}
			if ((infile & MAC_FD) == 0)
				printf("%s> ", myname);
			lp = 0; (void) rdc(); lp--;
			if (eof) {
				if (infile) {
					iclose(-1, 0);
					eof = 0;
					reset();
					/*NOTREACHED*/
				} else
					printf("eof?");
			}
			(void) command((char *)0, lastcom);
			if (lp && lastc!='\n')
				error("newline expected");
		}
	}
}

chkerr()
{

	if (errflg || interrupted)
		error(errflg);
}

doreset()
{

	iclose(0, 1);
	oclose();
	reset();
	/*NOTREACHED*/
}

error(n)
	char *n;
{

	errflg = n;
	doreset();
	/*NOTREACHED*/
}


#define NMACFILES	10		/* number of max open macro files */

struct open_file {
	struct pseudo_file *of_f;
	char *of_pos;
} filetab[NMACFILES];

static
getfileslot()
{
	register struct open_file *fp;

	for (fp = filetab; fp < &filetab[NMACFILES]; fp++) {
		if (fp->of_f == NULL)
			return (fp - filetab);
	}
	return (-1);
}

_open(path, flags, mode)
	char *path;
	int flags, mode;
{
	register struct pseudo_file *pfp;
	register int fd;
	register char *name, *s;
	extern char *rindex();
	
	tryabort(1);
	if (flags != O_RDONLY) {
		errno = EROFS;
		return (-1);
	}
	/* find open file slot */
	if ((fd = getfileslot()) == -1) {
		errno = EMFILE;
		return (-1);
	}
	/*
	 * Scan ahead in the path past any directories
	 * and convert all '.' in file name to '_'.
	 */
	name = rindex(path, '/');
	if (name == NULL)
		name = path;
	else
		name++;
	while ((s = rindex(path, '.')) != NULL)
		*s = '_';
	/* try to find "file" in pseudo file list */
	for (pfp = pf; pfp < &pf[npf]; pfp++) {
		if (strcmp(name, pfp->pf_name) == 0)
			break;
	}
	if (pfp >= &pf[npf]) {
		errno = ENOENT;
		return (-1);
	}
	filetab[fd].of_f = pfp;
	filetab[fd].of_pos = pfp->pf_string;
	return (fd | MAC_FD);
}

_lseek(d, offset, whence)
	int d, offset, whence;
{
	register char *se;
	register int r;
	register struct pseudo_file *pfp;
	char *pos;

	tryabort(1);
	if (d & MAC_FD) {
		d &= ~MAC_FD;
		if ((pfp = filetab[d].of_f) == NULL) {
			r = -1;
			errno = EBADF;
			goto out;
		}	
		se = pfp->pf_string + strlen(pfp->pf_string);
		switch (whence) {
		case L_SET:
			pos = pfp->pf_string + offset;
			break;
		case L_INCR:
			pos = filetab[d].of_pos + offset;
			break;
		case L_XTND:
			pos = se + offset;
			break;
		default:
			r = -1;
			errno = EINVAL;
			goto out;
		}
		if (pos < pfp->pf_string || pos > se) {
			r = -1;
			errno = EINVAL;
			goto out;
		}
		filetab[d].of_pos = pos;
		r = filetab[d].of_pos - pfp->pf_string;
	} else {
		r = lseek(d, offset, whence);
		if (r == -1)
			errno = EINVAL;
	}
out:
	return (r);
}

_read(d, buf, nbytes)
	int d, nbytes;
	char *buf;
{
	static char line[LINEBUFSZ];
	static char *p;
	register struct pseudo_file *pfp;
	register struct open_file *ofp;
	register char *s, *t;
	register int r;

	if (d & MAC_FD) {
		d &= ~MAC_FD;
		ofp = &filetab[d];
		if ((pfp = ofp->of_f) == NULL || ofp->of_pos == NULL) {
			r = -1;
			errno = EBADF;
			goto out;
		}
		for (r = 0, t = buf, s = ofp->of_pos; *s && r < nbytes; r++) {
			if (*s == '\n')
				tryabort(1);
			*t++ = *s++;
		}
		ofp->of_pos = s;
	} else if (d != 0) {
		tryabort(1);
		r = read(d, buf, nbytes);
		if (r == -1)
			errno = EFAULT;
	} else {
		/*
		 * Reading from stdin (keyboard).
		 * Call gets() to read buffer (thus providing
		 * erase, kill, and interrupt functions), then
		 * return the characters as needed from buffer.
		 */
		for (r = 0; r < nbytes; ) {
			if (p == NULL)
				gets(p = line);
			else
				tryabort(1);
			if (*p == '\0') {
				buf[r++] = '\n';
				p = NULL;
				break;
			} else
				buf[r++] = *p++;
		}
	}
out:
	return (r);
}

int max_write = 20;

_write(d, buf, nbytes)
	int d, nbytes;
	char *buf;
{
	register int r, e, sz;

	if (d & MAC_FD) {
		r = -1;
		errno = EBADF;
	} else {
		for (r = 0; r < nbytes; r += e) {
			sz = nbytes - r;
			if (sz > max_write)
				sz = max_write;
			trypause();
			(void) putchar(*(buf+r));
			e = 1;
			if (e == -1) {
				r = -1;
				errno = EFAULT;
				break;
			}
		}
	}
	return (r);
}

_close(d)
	int d;
{
	int r;

	tryabort(1);
	if (d & MAC_FD) {
		d &= ~MAC_FD;
		if (filetab[d].of_f == NULL) {
			r = -1;
		} else {
			filetab[d].of_f = NULL;
			filetab[d].of_pos = NULL;
		}
		return (0);
	} else {
		r = close(d);
	}
	if (r == -1)
		errno = EBADF;
	return (r);
}

creat(path, mode)
	char *path;
{

	return (_open(path, O_RDWR | O_CREAT | O_TRUNC, mode));
}

#define	NCOL	5
#define	SIZECOL	16

printmacros()
{
	register struct pseudo_file *pfp;
	register int j, i = 0;

	for (pfp = pf; pfp < &pf[npf]; pfp++) {
		/*
		 * Don't print out macro names with '_' in
		 * them as they are normally secondary macros
		 * called by the "primary" macros.
		 */
		if (index(pfp->pf_name, '_') == NULL) {
			printf("%s", pfp->pf_name);
			if ((++i % NCOL) == 0)
				printf("\n");
			else for (j = strlen(pfp->pf_name); j < SIZECOL; j++) {
				printf(" ");
			}
		}
	}
	printf("\n");
}

/*
 * This routine is an attempt to resync the tty (to avoid getting
 * into repeat mode when it shouldn't be).  Because of a bug
 * in the sun2 PROM when dealing w/ certain devices, we skip
 * calling initgetkey() if dottysync is not set.
 * Because v_initgetkey doesn't even exist in the Open Boot Proms,
 * we skip it then, too.
 */
ttysync()
{
#ifndef OPENPROMS
	if (dottysync)
		(*romp->v_initgetkey)();
#endif
}

/*
 * Stubs from ttycontrol.c
 */
newsubtty()
{
}

subtty()
{
}

adbtty()
{
}
