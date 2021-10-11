#ifndef lint
static	char sccsid[] = "@(#)gmon.c 1.1 92/07/30 SMI";
#endif

#ifdef DEBUG
#include <stdio.h>
#endif DEBUG

/*
 *	Environment variable PROFDIR added such that:
 *		If PROFDIR doesn't exist, "gmon.out" is produced as before.
 *		If PROFDIR = NULL, no profiling output is produced.
 *		If PROFDIR = string, "string/pid.progname" is produced,
 *		  where name consists of argv[0] suitably massaged.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include "gmon.h"
#define PROFDIR	"PROFDIR"

extern int creat(), write(), close(), getpid();
extern void profil(), perror();
extern char *getenv(), *strcpy(), *strrchr();

void monitor(), moncontrol();

char **___Argv = NULL; /* initialized to argv array by mcrt0 (if loaded) */

    /*
     *	froms is actually a bunch of unsigned shorts indexing tos
     */
extern char		profiling;
static unsigned short	*froms;
static struct tostruct	*tos = 0;
static long		tolimit = 0;
static int		s_lowpc = 0;
static char		*s_highpc = 0;
static unsigned long	s_textsize = 0;
static int		s_scale;
    /* see profil(2) where this is described (incorrectly) */
#define	SCALE_1_TO_1	0x10000L


static int	ssiz;
static int	*sbuf;

#define	MSG "No space for monitor buffer(s)\n"

static char gmon_out[MAXPATHLEN];
static char progname[MAXNAMLEN];

monstartup(lowpc, highpc)
    char	*lowpc;
    char	*highpc;
{
    int			monsize, fromsize, tosize;
    char		*buffer;
    char		*_alloc_profil_buf();

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
    lowpc = (char *)
	    ROUNDDOWN((unsigned)lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_lowpc = (int) lowpc;
    highpc = (char *)
	    ROUNDUP((unsigned)highpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_highpc = highpc;
    s_textsize = highpc - lowpc;

    monsize = ROUNDUP((s_textsize / HISTFRACTION) + sizeof(struct phdr), sizeof(long));
    fromsize = ROUNDUP(s_textsize / HASHFRACTION, sizeof(long));
    tolimit = s_textsize * ARCDENSITY / 100;
    if ( tolimit < MINARCS ) {
		tolimit = MINARCS;
    } else if ( tolimit > 65534 ) {
		tolimit = 65534;
    }
    tosize = ROUNDUP( tolimit * sizeof( struct tostruct ), sizeof(long) );
	buffer = _alloc_profil_buf(monsize+fromsize+tosize);
	if(buffer == (char*) -1) {
		write( 2 , MSG , sizeof(MSG) );
		froms = 0;
		tos = 0;
		return;
	}
	froms = (unsigned short*) (buffer + monsize);
	tos = (struct tostruct*) (buffer + monsize + fromsize);

    tos[0].link = 0;
    monitor( lowpc , highpc , buffer , monsize , tolimit );
}

_mcleanup()
{
    int			fd;
    int			fromindex;
    int			endfrom;
    int			frompc;
    int			toindex;
    struct rawarc	rawarc;
    char		*name = gmon_out;

    moncontrol(0);
    if (sbuf != NULL) {
	register int pid, n;

	if (progname[0] != '\0') { /* finish constructing
				    "PROFDIR/pid.progname" */
	    /* set name to end of PROFDIR */
	    name = strrchr(gmon_out, '\0');  
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
	    (void)strcpy(name, progname);
	}

	fd = creat( gmon_out , 0666 );
	if ( fd < 0 ) {
	    perror( gmon_out );
	    return;
	}
#   ifdef DEBUG
	    fprintf( stderr , "[mcleanup] sbuf 0x%x ssiz %d\n" , sbuf , ssiz );
#   endif DEBUG
	if (write( fd , sbuf , ssiz ) == -1) {
	    perror( gmon_out );
	    return;
	}
    }
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
#	    ifdef DEBUG
		fprintf( stderr ,
			"[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			frompc , tos[toindex].selfpc , tos[toindex].count );
#	    endif DEBUG
	    rawarc.raw_frompc = (unsigned long) frompc;
	    rawarc.raw_selfpc = (unsigned long) tos[toindex].selfpc;
	    rawarc.raw_count = tos[toindex].count;
	    write( fd , &rawarc , sizeof rawarc );
	}
    }
    close( fd );
}

/*VARARGS1*/
void
monitor( lowpc , highpc , buf , bufsiz , nfunc )
    char	*lowpc;
    char	*highpc;
    int		*buf, bufsiz;
    int		nfunc;	/* not used, available for compatability only */
{
    register o;
    register char *s, *name = gmon_out;

    if ( lowpc == NULL ) {		/* true only at the end */
	moncontrol(0);
	_mcleanup();
	return;
    }
#if 0
    if (ssiz >= bufsize || lowpc >= highpc)
	return;		/* buffer too small or PC range bad */
#endif
    if ((s = getenv(PROFDIR)) == NULL) /* PROFDIR not in environment */
	(void)strcpy(name, GMON_OUT); /* use default "gmon.out" */
    else if (*s == '\0') /* value of PROFDIR is NULL */
	return; /* no profiling on this run */
    else { /* set up mon_out and progname to construct
	      "PROFDIR/pid.progname" when done profiling */

	while (*s != '\0') /* copy PROFDIR value (path-prefix) */
	    *name++ = *s++;
	*name++ = '/'; /* two slashes won't hurt */
	if (___Argv != NULL) /* mcrt0.s executed */
	    if ((s = strrchr(___Argv[0], '/')) != NULL)
		strcpy(progname, s + 1);
	    else
		strcpy(progname, ___Argv[0]);
    }
    sbuf = buf;			/* for writing buffer at the wrapup */
    ssiz = bufsiz;
    ( (struct phdr *) buf ) -> lpc = lowpc;	/* initialize the first */
    ( (struct phdr *) buf ) -> hpc = highpc;	/* region of the buffer */
    ( (struct phdr *) buf ) -> ncnt = ssiz;
    o = sizeof(struct phdr);
    buf = (int *) ( ( (int) buf ) + o );
    bufsiz -= o;
    if ( bufsiz <= 0 )
	return;
    o = ( ( (char *) highpc - (char *) lowpc) );
    if( bufsiz < o )
	s_scale = ( (float) bufsiz / o ) * SCALE_1_TO_1;
    else
	s_scale = SCALE_1_TO_1;
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
    if (mode) {
	/* start */
	profil((char*) sbuf + sizeof(struct phdr), ssiz - sizeof(struct phdr),
		s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}
