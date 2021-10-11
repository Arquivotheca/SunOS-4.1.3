/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tput.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

/* tput - print terminal attribute

   return codes:
	0	ok (if boolean capname -> TRUE)
	1	(for boolean capname -> FALSE)
	2	usage error
	3	bad terminal type given or no terminfo database
	4	unknown capname

   tput printfs (a value) if an INT capname was given (e.g. cols),
	putp's (a string) if a STRING capname was given (e.g. clear), and
	for BOOLEAN capnames (e.g. hard-copy) just returns the boolean value.
*/

#include <curses.h>
#include <term.h>
#include <fcntl.h>
#include <ctype.h>

/* externs from libcurses */
extern int tigetflag (), tigetnum ();
extern char *tigetstr (), *tparm ();

/* externs from libc */
extern void exit ();
extern char *getenv ();
extern int getopt ();
extern int optind;
extern char *optarg;

char *progname;		/* argv[0] */
int CurrentBaudRate;	/* current baud rate */
int reset = 0;		/* called as reset_term */
int fildes = 1;

main (argc, argv)
int argc;
char **argv;
{
    register int i;
    register char *term = getenv("TERM");
    register char *cap;
    int setuperr;

    progname = argv[0];

    while ((i = getopt (argc, argv, "T:")) != EOF)
	switch (i)
	    {
	    case 'T':
		fildes = -1;
		putenv("LINES=");
		putenv("COLUMNS=");
		term = optarg;
		break;
	    case '?':
	    usage:
		(void) fprintf (stderr,
		    "usage: \"%s [-T term] capname [tparm argument...]\"\n",
		    argv[0]);
		exit(2);
	    }

    if (!term || !*term)
	{
	(void) fprintf(stderr,"%s: No value for $TERM and no -T specified\n",
	    argv[0]);
	exit(2);
	}

    (void) setupterm (term, fildes, &setuperr);

    switch (setuperr)
	{
	case -2:
	    (void) fprintf(stderr,"%s: unreadable terminal descriptor \"%s\"\n",
		argv[0], term);
	    exit(3);
	case -1:
	    (void) fprintf(stderr,"%s: no terminfo database\n",
		argv[0]);
	    exit(3);
	case 0:
	    (void) fprintf(stderr,"%s: unknown terminal \"%s\"\n",
		argv[0], term);
	    exit(3);
	}

    reset_shell_mode();
    if (argc == optind)
	goto usage;

    cap = argv[optind++];

    if (strcmp(cap, "init") == 0)
        initterm();
    else if (strcmp(cap, "reset") == 0)
        reset_term();
    else if (strcmp(cap, "longname") == 0) {
        printf("%s\n",longname());
	exit(0);
    } else
	outputcap(cap, argc, argv);
    /* NOTREACHED */
}

static int parm[9] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

outputcap(cap, argc, argv)
register char *cap;
int argc;
char **argv;
{
    register int parmset = 0;
    char *thisstr;
    register int i;

    if ((i = tigetflag(cap)) >= 0)
	exit(1 - i);

    if ((i = tigetnum(cap)) >= -1)
	{
	(void) printf ("%d\n", i);
	exit (0);
	}

    if ((thisstr = tigetstr(cap)) != (char *)-1)
	{
	if (!thisstr) exit(1);
	for (parmset = 0; optind < argc; optind++, parmset++)
	    if (allnumeric(argv[optind]))
		parm[parmset] = atoi(argv[optind]);
	    else
		parm[parmset] = (int) argv[optind];

	if (parmset)
	    putp (tparm (thisstr,
		parm[0], parm[1], parm[2], parm[3], parm[4], parm[5],
		parm[6], parm[7], parm[8], parm[9]));
	else
	    putp (thisstr);
	exit (0);
	}

    (void) fprintf(stderr,"%s: unknown terminfo capability '%s'\n", argv[0], cap);
    exit (4);
}

/*
    The decision as to whether an argument is a number or not is to simply
    look at whether there are any non-digits in the string.
*/
int allnumeric(string)
register char *string;
{
    while (*string)
	if (!isdigit(*string++))
	    return 0;
    return 1;
}

/*
    SYSTEM DEPENDENT TERMINAL DELAY TABLES

	These tables maintain the correspondence between the delays
	defined in terminfo and the delay algorithms in the tty driver
	on the particular systems. For each type of delay, the bits used
	for that delay must be specified (in XXbits) and a table
	must be defined giving correspondences between delays and
	algorithms. Algorithms which are not fixed delays (such
	as dependent on current column or line number) must be
	kludged in some way at this time.

	Some of this was taken from tset(1).
*/

struct delay
{
    int d_delay;
    int d_bits;
};

/* The appropriate speeds for various termio settings. */
int speeds[] =
    {
    0,		/*  B0, */
    50,		/*  B50, */
    75,		/*  B75, */
    110,	/*  B110, */
    134,	/*  B134, */
    150,	/*  B150, */
    200,	/*  B200, */
    300,	/*  B300, */
    600,	/*  B600, */
    1200,	/*  B1200, */
    1800,	/*  B1800, */
    2400,	/*  B2400, */
    4800,	/*  B4800, */
    9600,	/*  B9600, */
    19200,	/*  EXTA, */
    38400,	/*  EXTB, */
    0,
    };

#if defined(SYSV) || defined(USG)
/*	Unix 3.0 on up */

/*    Carriage Return delays	*/

int	CRbits = CRDLY;
struct delay	CRdelay[] =
{
	0,	CR0,
	80,	CR1,
	100,	CR2,
	150,	CR3,
	-1
};

/*	New Line delays	*/

int	NLbits = NLDLY;
struct delay	NLdelay[] =
{
	0,	NL0,
	100,	NL1,
	-1
};

/*	Back Space delays	*/

int	BSbits = BSDLY;
struct delay	BSdelay[] =
{
	0,	BS0,
	50,	BS1,
	-1
};

/*	TaB delays	*/

int	TBbits = TABDLY;
struct delay	TBdelay[] =
{
	0,	TAB0,
	11,	TAB1,		/* special M37 delay */
	100,	TAB2,
				/* TAB3 is XTABS and not a delay */
	-1
};

/*	Form Feed delays	*/

int	FFbits = FFDLY;
struct delay	FFdelay[] =
{
	0,	FF0,
	2000,	FF1,
	-1
};

#else	/* BSD */

/*	Carriage Return delays	*/

int	CRbits = CRDELAY;
struct delay	CRdelay[] =
{
	0,	CR0,
	9,	CR3,
	80,	CR1,
	160,	CR2,
	-1
};

/*	New Line delays	*/

int	NLbits = NLDELAY;
struct delay	NLdelay[] =
{
	0,	NL0,
	66,	NL1,		/* special M37 delay */
	100,	NL2,
	-1
};

/*	Tab delays	*/

int	TBbits = TBDELAY;
struct delay	TBdelay[] =
{
	0,	TAB0,
	11,	TAB1,		/* special M37 delay */
	-1
};

/*	Form Feed delays	*/

int	FFbits = VTDELAY;
struct delay	FFdelay[] =
{
	0,	FF0,
	2000,	FF1,
	-1
};
#endif	/* BSD */

/*
    Initterm (reset_term) does terminal specific initialization. In
    particular, the init_strings from terminfo are output and tabs are
    set (if they aren't hardwired in). Much of this stuff was done by
    the tset(1) program.
*/

/*
   Figure out how many milliseconds of padding the capability cap
   needs and return that number. Padding is stored in the string as $<n>,
   where n is the number of milliseconds of padding. More than one
   padding string is allowed within the string, although this is unlikely.
*/

static int getpad (cap)
register char *cap;
{
    register int padding = 0;

    /* No padding needed at speeds below padding_baud_rate */
    if (padding_baud_rate > CurrentBaudRate)
	return 0;

    /* If no such capability, no padding */
    if (cap == 0)
	return 0;

    while (*cap)
	if ( (cap[0] == '$') && (cap[1] == '<') )
	    {
	    cap++;
	    cap++;
	    padding += atoi (cap);
	    while ( isdigit (*cap) )
		cap++;
	    while (*cap == '.' || *cap == '/' || *cap == '*' || isdigit(*cap))
		cap++;
	    while (*cap == '>')
		cap++;
	    }
	else
	    cap++;
    return padding;
}

/*
    Set the appropriate delay bits in the termio structure for
    the given delay.
*/
static void setdelay (delay, delaytable, bits, flags)
register int delay;
struct delay delaytable[];
int bits;
unsigned short *flags;
{
    register struct delay  *p;
    register struct delay  *lastdelay;

    /* Clear out the bits, replace with new ones */
    *flags &= ~bits;

    /* Scan the delay table for first entry with adequate delay */
    for (lastdelay = p = delaytable;
	 (p -> d_delay >= 0) && (p -> d_delay < delay);
	 p++)
	lastdelay = p;

    /* use last entry if none will do */
    *flags |= lastdelay -> d_bits;
}

/*
 * Set the hardware tabs on the terminal, using clear_all_tabs,
 * set_tab, and column_address capabilities. Cursor_address and cursor_right
 * may also be used, if necessary.
 * This is done before the init_file and init_3string, so they can patch in
 * case we blow this.
 */

static int settabs ()
{
    register int c;

    /* Do not set tabs if they power up properly. */
    if (init_tabs == 8)
	return (0);

    if (set_tab)
	{
	/* Force the cursor to be at the left margin. */
	if (carriage_return)
	    putp (carriage_return);
	else
	    (void) putchar ('\r');

	/* Clear any current tab settings. */
	if (clear_all_tabs)
	    putp (clear_all_tabs);

	/* Set the tabs. */
	for (c = 8; c < columns; c += 8)
	    {
	    /* Get to that column. */
	    (void) fputs ("        ", stdout);

	    /* Set the tab. */
	    putp (set_tab);
	    }

	/* Get back to the left column. */
	if (carriage_return)
	    putp (carriage_return);
	else
	    (void) putchar ('\r');

	return 1;
	}
    return 0;
}

/*
    Copy "file" onto standard output.
*/

static void cat (file)
char *file;				/* File to copy. */
{
    register int fd;			/* File descriptor. */
    register int i;			/* Number characters read. */
    char buf[BUFSIZ];			/* Buffer to read into. */

    fd = open (file, O_RDONLY);
    if (fd < 0)
	(void) fprintf (stderr, "%s: Cannot open initialization file %s.\n",
	    progname, file);
    else
	{
	while ((i = read (fd, buf, BUFSIZ)) > 0)
	    (void) write (fileno (stdout), buf, (unsigned) i);
	(void) close (fd);
	}
}

/*
    Initialize the terminal.
    Send the initialization strings to the terminal.
*/

initterm ()
{
    register int filedes;		/* File descriptor for ioctl's. */
#if defined(SYSV) || defined(USG)
    struct termio termmode;		/* To hold terminal settings. */
# define GTTY(fd,mode)	ioctl(fd, TCGETA, mode)
# define STTY(fd,mode)	ioctl(fd, TCSETAW, mode)
# define SPEED(mode)	(mode.c_cflag & CBAUD)
# define OFLAG(mode)	mode.c_oflag
#else	/* BSD */
    struct sgttyb termmode;		/* To hold terminal settings. */
# define GTTY(fd,mode)	gtty(fd, mode)
# define STTY(fd,mode)	stty(fd, mode)
# define SPEED(mode)	(mode.sg_ospeed & 017)
# define OFLAG(mode)	mode.sg_flags
# define TAB3		XTABS
#endif

    /* Get the terminal settings. */
    /* First try standard output, then standard error, */
    /* then standard input, then /dev/tty. */
    if ( (filedes = 1, GTTY (filedes, &termmode) == -1) ||
	 (filedes = 2, GTTY (filedes, &termmode) == -1) ||
	 (filedes = 0, GTTY (filedes, &termmode) == -1) ||
	 (filedes = open ("/dev/tty", O_RDWR),
	     GTTY (filedes, &termmode) == -1) )
	{
	filedes = -1;
	CurrentBaudRate = speeds[B1200];
	}
    else
	CurrentBaudRate = speeds[SPEED(termmode)];

    if (xon_xoff)
	{
	OFLAG(termmode) &= ~(NLbits | CRbits | BSbits | FFbits | TBbits);
	}
    else
	{
	setdelay(getpad(carriage_return), CRdelay, CRbits, &OFLAG(termmode));
	setdelay(getpad(scroll_forward), NLdelay, NLbits, &OFLAG(termmode));
	setdelay(getpad(cursor_left), BSdelay, BSbits, &OFLAG(termmode));
	setdelay(getpad(form_feed), FFdelay, FFbits, &OFLAG(termmode));
	setdelay(getpad(tab), TBdelay, TBbits, &OFLAG(termmode));
	}

    /* If tabs can be sent to the tty, turn off their expansion. */
    if (tab && (set_tab || init_tabs == 8))
	OFLAG(termmode) &= ~(TAB3);
    else
	OFLAG(termmode) |= TAB3;

    /* Do the changes to the terminal settings */
    (void) STTY (filedes, &termmode);

    /* Send first initialization strings. */
    if (init_prog)
	(void) system (init_prog);

    if (reset && reset_1string)
	putp (reset_1string);
    else if (init_1string)
	putp (init_1string);

    if (reset && reset_2string)
	putp (reset_2string);
    else if (init_2string)
	putp (init_2string);

    /* Set up the tabs stops. */
    settabs ();

    /* Send out initializing file. */
    if (reset && reset_file)
	cat (reset_file);
    else if (init_file)
	cat (init_file);

    /* Send final initialization strings. */
    if (reset && reset_3string)
	putp (reset_3string);
    else if (init_3string)
	putp (init_3string);

    if (carriage_return)
        putp (carriage_return);
    else
	(void) putchar ('\r');

    /* Let the terminal settle down. */
    (void) fflush (stdout);
    (void) sleep (1);

    exit(0);
}

reset_term()
{
    reset++;
    initterm();
}
