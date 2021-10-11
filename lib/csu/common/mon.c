#ifndef lint
static	char sccsid[] = "@(#)mon.c 1.1 92/07/30 SMI";
#endif

/*
 *	Environment variable PROFDIR added such that:
 *		If PROFDIR doesn't exist, "mon.out" is produced as before.
 *		If PROFDIR = NULL, no profiling output is produced.
 *		If PROFDIR = string, "string/pid.progname" is produced,
 *		  where name consists of argv[0] suitably massaged.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include "mon.h"
#define PROFDIR	"PROFDIR"

extern int creat(), write(), close(), getpid();
extern void profil(), perror();
extern char *getenv(), *strcpy(), *strrchr();

void monitor(), moncontrol();

char **___Argv = NULL; /* initialized to argv array by mcrt0 (if loaded) */

struct cnt	*countbase;
int		numctrs;
int		profiling;

static struct mondata {
	char	*s_sbuf;
	int 	s_bufsiz;
	int	s_scale;
	int	s_lowpc;
	char	mon_out[MAXPATHLEN];
	char	progname[MAXNAMLEN];
} *mondata, *_mondata();

#define	MSG "No space for monitor buffer(s)\n"

static struct mondata *
_mondata()
{
	register struct mondata *d = mondata;

	if (d == 0) {
		if ((d = (struct mondata *)
			calloc(1, sizeof(struct mondata))) == NULL) {
				return (NULL);
		}
		mondata = d;
	}
	return (d);
}

monstartup(lowpc, highpc)
	char *lowpc;
	char *highpc;
{
	int monsize;
	char *buffer;
	int cntsiz;
	char *_alloc_profil_buf();

	cntsiz = (highpc - lowpc) * ARCDENSITY / 100;
	if (cntsiz < MINARCS)
		cntsiz = MINARCS;
	monsize = (highpc - lowpc + HISTFRACTION - 1) / HISTFRACTION
		+ sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	buffer = _alloc_profil_buf(monsize);
	if (buffer == (char *)-1) {
		write(2, MSG, sizeof(MSG));
		return;
	}
	monitor(lowpc, highpc, buffer, monsize, cntsiz);
}

void
monitor(lowpc, highpc, buf, bufsiz, cntsiz)
	char *lowpc, *highpc;	/* boundaries of text to be monitored */
	char *buf;		/* ptr to space for monitor data (WORDs) */
	int bufsiz;		/* size of above space (in WORDs) */
	int cntsiz;		/* max no. of functions whose calls are counted */
{
	register struct mondata *d = _mondata();
	register int o;
	struct phdr *php;
	static int ssiz;
	static char *sbuf;
	register char *s, *name;

	name = d->mon_out;

	if (lowpc == NULL) {		/* true only at the end */
		moncontrol(0);
		if (sbuf != NULL) {
			register int pid, n;

			if (d->progname[0] != '\0') { /* finish constructing
						    "PROFDIR/pid.progname" */
			    /* set name to end of PROFDIR */
			    name = strrchr(d->mon_out, '\0');  
			    if ((pid = getpid()) <= 0) /* extra test just in case */
				pid = 1; /* getpid returns something inappropriate */
			    for (n = 10000; n > pid; n /= 10)
				; /* suppress leading zeros */
			    for ( ; ; n /= 10) {
				*name++ = pid/n + '0';
				if (n == 1)
				    break;
				pid %= n;
			    }
			    *name++ = '.';
			    (void)strcpy(name, d->progname);
			}

			if ((o = creat(d->mon_out, 0666)) < 0 ||
			    write(o, sbuf, (unsigned)ssiz) == -1)
				perror(d->mon_out);
			if (o >= 0)
				close(o);
		}
		return;
	}
	countbase = (struct cnt *)(buf + sizeof(struct phdr));
	sbuf = NULL;
	o = sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	if (ssiz >= bufsiz || lowpc >= highpc)
		return;		/* buffer too small or PC range bad */
	if ((s = getenv(PROFDIR)) == NULL) /* PROFDIR not in environment */
		(void)strcpy(name, MON_OUT); /* use default "mon.out" */
	else if (*s == '\0') /* value of PROFDIR is NULL */
		return; /* no profiling on this run */
	else { /* set up mon_out and progname to construct
		  "PROFDIR/pid.progname" when done profiling */

		while (*s != '\0') /* copy PROFDIR value (path-prefix) */
			*name++ = *s++;
		*name++ = '/'; /* two slashes won't hurt */
		if (___Argv != NULL) /* mcrt0.s executed */
			if ((s = strrchr(___Argv[0], '/')) != NULL)
			    strcpy(d->progname, s + 1);
			else
			    strcpy(d->progname, ___Argv[0]);
	}
	sbuf = buf;		/* for writing buffer at the wrapup */
	ssiz = bufsiz;
	php = (struct phdr *)&buf[0];
	php->lpc = (char *)lowpc;	/* initialize the first */
	php->hpc = (char *)highpc;	/* region of the buffer */
	php->ncnt = cntsiz;
	numctrs = cntsiz;
	buf += o;
	bufsiz -= o;
	if (bufsiz <= 0)
		return;
	o = (highpc - lowpc);
	if(bufsiz < o)
		o = ((float) bufsiz / o) * 65536;
	else
		o = 65536;
	d->s_scale = o;
	d->s_sbuf = buf;
	d->s_bufsiz = bufsiz;
	d->s_lowpc = (int) lowpc;
	moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
void
moncontrol(mode)
    int mode;
{
    register struct mondata *d = _mondata();

    if (mode) {
	/* start */
	profil(d->s_sbuf, d->s_bufsiz, d->s_lowpc, d->s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}
