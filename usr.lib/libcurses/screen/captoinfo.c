/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)captoinfo.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6.1.7 */
#endif

/*
    NAME
	captoinfo - convert a termcap description to a terminfo description

    SYNOPSIS
	captoinfo [-1vV] [-w width] [ filename ... ]

    AUTHOR
	Tony Hansen, January 22, 1984.
*/

#include "curses.h"
#include <ctype.h>
#include "otermcap.h"
#include "print.h"

#define trace stderr			/* send trace messages to stderr */

/* extra termcap variables no longer in terminfo */
char *oboolcodes[] =
    {
    "bs",	/* Terminal can backspace with "^H" */
    "nc",	/* No correctly working carriage return (DM2500,H2000) */
    "ns",	/* Terminal is a CRT but does not scroll. */
    "pt",	/* Has hardware tabs (may need to be set with "is") */
    "MT",	/* Has meta key, alternate code. */
    "xr",	/* Return acts like ce \r \n (Delta Data) */
    0
    };
int cap_bs = 0, cap_nc = 1, cap_ns = 2, cap_pt = 3, cap_MT = 4, cap_xr = 5;
char *onumcodes[] =
    {
    "dB",	/* Number of millisec of bs delay needed */
    "dC",	/* Number of millisec of cr delay needed */
    "dF",	/* Number of millisec of ff delay needed */
    "dN",	/* Number of millisec of nl delay needed */
    "dT",	/* Number of millisec of tab delay needed */
    "ug",	/* Number of blank chars left by us or ue */
/* Ignore the 'kn' number. It was ill-defined and never used. */
    "kn",	/* Number of "other" keys */
    0
    };
int cap_dB = 0, cap_dC = 1, cap_dF = 2, cap_dN = 3, cap_dT = 4, cap_ug = 5;

char *ostrcodes[] =
    {
    "bc",	/* Backspace if not "^H" */
    "ko",	/* Termcap entries for other non-function keys */
    "ma",	/* Arrow key map, used by vi version 2 only */
    "nl",	/* Newline character (default "\n") */
    "rs",	/* undocumented reset string, like is (info is2) */
/* Ignore the 'ml' and 'mu' strings. */
    "ml",	/* Memory lock on above cursor. */
    "mu",	/* Memory unlock (turn off memory lock). */
    0
    };
int cap_bc = 0, cap_ko = 1, cap_ma = 2, cap_nl = 3, cap_rs = 4;

#define numelements(x)	(sizeof(x)/sizeof(x[0]))
char oboolval [2] [numelements(oboolcodes)];
short onumval [2] [numelements(onumcodes)];
char *ostrval [2] [numelements(ostrcodes)];

/* externs from libcurses.a */
extern char *boolnames [], *boolcodes [];
extern char *numnames [], *numcodes [];
extern char *strnames [], *strcodes [];

/* externs from libc.a */
extern char *getenv ();
extern char *malloc ();
extern void exit ();
#if defined(SYSV) || defined(USG)  /* handle both Sys Vr2 and Vr3 curses */
extern char *getcwd ();
#else
extern char *getwd ();
#endif /* SYSV || USG */
extern int getopt ();
extern int optind;
extern char *optarg;
extern int strncmp(), strcmp();
extern char *strcpy();

/* globals for this file */
char *progname;			/* argv [0], the name of the program */
static char *term_name;		/* the name of the terminal being worked on */
static int uselevel;		/* whether we're dealing with use= info */
static int boolcount,	       	/* the maximum numbers of each name array */
	   numcount,
	   strcount;

/* globals dealing with the environment */
extern char **environ;
static char TERM [100];
#if defined(SYSV) || defined(USG)  /* handle both Sys Vr2 and Vr3 curses */
static char dirname [BUFSIZ];
#else
# include <sys/param.h>
static char dirname [MAXPATHLEN];
#endif /* SYSV || USG */
static char TERMCAP [BUFSIZ+15];
static char *newenviron [] = { &TERM [0], &TERMCAP [0], 0 };

/* dynamic arrays */
static char *boolval [2];	/* dynamic array of boolean values */
static short *numval [2];	/* dynamic array of numeric values */
static char **strval [2];	/* dynamic array of string pointers */

/* data buffers */
static char *capbuffer;		/* string table, pointed at by strval */
static char *nextstring;	/* pointer into string table */
static char *bp;		/* termcap raw string table */
static char *buflongname;	/* place to copy the long names */

/* flags */
static int verbose = 0;		/* debugging printing level */
static int copycomments = 0;	/* copy comments from tercap source */

#define ispadchar(c)	(isdigit(c) || (c) == '.' || (c) == '*')

/*
    Verify that the names given in the termcap entry are all valid.
*/

int capsearch (codes, ocodes, cap)
register char *codes[], *ocodes[], *cap;
{
    for ( ; *codes; codes++)
	if (((*codes)[0] == cap[0]) && ((*codes)[1] == cap[1]))
	    return 1;

    for ( ; *ocodes; ocodes++)
	if (((*ocodes)[0] == cap[0]) && ((*ocodes)[1] == cap[1]))
	    return 1;

    return 0;
}

void checktermcap()
{
    register char *tbuf = bp;
    enum { tbool, tnum, tstr, tcancel, tunknown } type;

    for (;;)
	{
	tbuf = tskip(tbuf);
	while (*tbuf == '\t' || *tbuf == ' ' || *tbuf == ':')
	    tbuf++;

	if (*tbuf == 0)
	    return;

	/* commented out entry? */
	if (*tbuf == '.')
	    {
	    if (verbose)
		(void) fprintf (trace, "termcap string '%c%c' commented out.\n",
		    tbuf[1], tbuf[2]);
	    if (!capsearch (boolcodes, oboolcodes, tbuf+1) &&
		!capsearch (numcodes, onumcodes, tbuf+1) &&
		!capsearch (strcodes, ostrcodes, tbuf+1))
		(void) fprintf (stderr,
		    "%s: TERM=%s: commented out code '%.2s' is unknown.\n",
		    progname, term_name, tbuf+1);
	    continue;
	    }

	if (verbose)
	    (void) fprintf (trace, "looking at termcap string '%.2s'.\n",
		tbuf);

	switch (tbuf[2])
	    {
	    case ':': case '\0':	type = tbool;	break;
	    case '#':			type = tnum;	break;
	    case '=':			type = tstr;	break;
	    case '@':			type = tcancel;	break;
	    default:
		(void) fprintf (stderr,
		    "%s: TERM=%s: unknown type given for the termcap code '%.2s'.\n",
		    progname, term_name, tbuf);
		type = tunknown;
	    }

	if (verbose > 1)
	    (void) fprintf (trace, "type of '%.2s' is %s.\n", tbuf,
		(type == tbool) ? "boolean" :
		(type == tnum) ? "numeric" :
		(type = tstr) ? "string" :
		(type = tcancel) ? "canceled" : "unknown");

	/* look for the name in bools */
	if (capsearch (boolcodes, oboolcodes, tbuf))
	    {
	    if (type != tbool && type != tcancel)
		(void) fprintf (stderr,
		    "%s: TERM=%s: wrong type given for the boolean termcap code '%.2s'.\n",
		    progname, term_name, tbuf);
	    continue;
	    }

	/* look for the name in nums */
	if (capsearch (numcodes, onumcodes, tbuf))
	    {
	    if (type != tnum && type != tcancel)
		(void) fprintf (stderr,
		    "%s: TERM=%s: wrong type given for the numeric termcap code '%.2s'.\n",
		    progname, term_name, tbuf);
	    continue;
	    }

	/* look for the name in strs */
	if (capsearch (strcodes, ostrcodes, tbuf))
	    {
	    if (type != tstr && type != tcancel)
		(void) fprintf (stderr,
		    "%s: TERM=%s: wrong type given for the string termcap code '%.2s'.\n",
		    progname, term_name, tbuf);
	    continue;
	    }

	(void) fprintf (stderr,
	    "%s: TERM=%s: the %s termcap code '%.2s' is not a valid name.\n",
	    progname, term_name,
	    (type == tbool) ? "boolean" :
	    (type == tnum) ? "numeric" :
	    (type = tstr) ? "string" :
	    (type = tcancel) ? "canceled" : "(unknown type)",
	    tbuf);
	}
}

/*
    Fill up the termcap tables.
*/
int filltables ()
{
    register int i, tret;

    /* Retrieve the termcap entry. */
    if ((tret = otgetent (bp, term_name)) != 1)
	{
	(void) fprintf (stderr,
	    "%s: TERM=%s: tgetent failed with return code %d (%s).\n",
	    progname, term_name, tret,
	    (tret == 0) ? "non-existent or invalid entry" :
	    (tret == -1) ? "cannot open $TERMCAP" :
	    "unknown reason");
	return 0;
	}

    if (verbose)
	{
	(void) fprintf (trace, "bp=");
	(void) cpr (trace, bp);
	(void) fprintf (trace, ".\n");
	}

    if (uselevel == 0)
	checktermcap();

    /* Retrieve the values that are in terminfo. */

    /* booleans */
    for (i = 0; boolcodes [i]; i++)
	{
	boolval [uselevel] [i] = otgetflag (boolcodes [i]);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "boolcodes=%s, ", boolcodes [i]);
	    (void) fprintf (trace, "boolnames=%s, ", boolnames [i]);
	    (void) fprintf (trace, "flag=%d.\n", boolval [uselevel] [i]);
	    }
	}

    /* numbers */
    for (i = 0; numcodes [i]; i++)
	{
	numval [uselevel] [i] = otgetnum (numcodes [i]);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "numcodes=%s, ", numcodes [i]);
	    (void) fprintf (trace, "numnames=%s, ", numnames [i]);
	    (void) fprintf (trace, "num=%d.\n", numval [uselevel] [i]);
	    }
	}

    if (uselevel == 0)
	nextstring = capbuffer;

    /* strings */
    for (i = 0; strcodes [i]; i++)
	{
	strval [uselevel] [i] = otgetstr (strcodes [i], &nextstring);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "strcodes=%s, ", strcodes [i]);
	    (void) fprintf (trace, "strnames=%s, ", strnames [i]);
	    if (strval [uselevel] [i])
		{
		(void) fprintf (trace, "str=");
		tpr (trace, strval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}
	    else
		(void) fprintf (trace, "str=NULL.\n");
	    }
	/* remove zero length strings */
	if (strval[uselevel][i] && (strval[uselevel][i][0] == '\0') )
	    {
	    (void) fprintf (stderr,
		"%s: TERM=%s: cap %s (info %s) is NULL: REMOVED\n",
		progname, term_name, strcodes[i], strnames[i]);
	    strval [uselevel] [i] = NULL;
	    }
	}

    /* Retrieve the values not found in terminfo anymore. */

    /* booleans */
    for (i = 0; oboolcodes [i]; i++)
	{
	oboolval [uselevel] [i] = otgetflag (oboolcodes [i]);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "oboolcodes=%s, ", oboolcodes[i]);
	    (void) fprintf (trace, "flag=%d.\n", oboolval [uselevel] [i]);
	    }
	}

    /* numbers */
    for (i = 0; onumcodes [i]; i++)
	{
	onumval [uselevel] [i] = otgetnum (onumcodes [i]);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "onumcodes=%s, ", onumcodes [i]);
	    (void) fprintf (trace, "num=%d.\n", onumval [uselevel] [i]);
	    }
	}

    /* strings */
    for (i = 0; ostrcodes [i]; i++)
	{
	ostrval [uselevel] [i] = otgetstr (ostrcodes [i], &nextstring);
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "ostrcodes=%s, ", ostrcodes [i]);
	    if (ostrval [uselevel] [i])
		{
		(void) fprintf (trace, "ostr=");
		tpr (trace, ostrval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}
	    else
		(void) fprintf (trace, "ostr=NULL.\n");
	    }
	/* remove zero length strings */
	if (ostrval[uselevel][i] && (ostrval[uselevel][i][0] == '\0') )
	    {
	    (void) fprintf (stderr,
		"%s: TERM=%s: cap %s (no terminfo name) is NULL: REMOVED\n",
		progname, term_name, ostrcodes[i]);
	    ostrval [uselevel] [i] = NULL;
	    }
	}
    return 1;
}

/*
    This routine copies the set of names from the termcap entry into
    a separate buffer, getting rid of the old obsolete two character
    names.
*/
getlongname ()
{
    register char *b = &bp [0],  *l = buflongname;

    /* Skip the two character name */
    if (bp [2] == '|')
	b = &bp [3];

    /* Copy the rest of the names */
    while (*b && *b != ':')
	*l++ = *b++;
    *l = '\0';

    if (b != &bp[0])
	{
	(void) fprintf (stderr, "%s: obsolete 2 character name '%2.2s' removed.\n",
	    progname, bp);
	(void) fprintf (stderr, "\tsynonyms are: '%s'\n", buflongname);
	}
}

/*
    Return the value of the termcap string 'capname' as stored in our list.
*/
char *getcapstr (capname)
register char *capname;
{
    register int i;

    if (verbose > 1)
	(void) fprintf (trace, "looking for termcap value of %s.\n",
	    capname);

    /* Check the old termcap list. */
    for (i = 0; ostrcodes [i]; i++)
	if (strcmp (ostrcodes [i], capname) == 0)
	    {
	    if (verbose > 1)
		{
		(void) fprintf (trace, "\tvalue is:");
		tpr (trace, ostrval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}
	    return ostrval [uselevel] [i];
	    }

    if (verbose > 1)
	(void) fprintf (trace, "termcap name '%s' not found in ostrcodes.\n",
	    capname);

    /* Check the terminfo list. */
    for (i = 0; strcodes [i]; i++)
	if (strcmp (strcodes [i], capname) == 0)
	    {
	    if (verbose > 1)
		{
		(void) fprintf (trace, "\tvalue is:");
		tpr (trace, strval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}
	    return strval [uselevel] [i];
	    }

    (void) fprintf (stderr, "%s: TERM=%s: termcap name '%s' not found.\n",
	progname, term_name, capname);

    return (char *) NULL;
}

/*
    Search for a name in the given table and return the index.
    Someday I'll redo this to use bsearch().
*/
/* ARGSUSED */
int search (names, max, infoname)
char *names[], *infoname;
int max;
{
#ifndef BSEARCH
    register int i;
    for (i = 0; names [i] != NULL; i++)
	if (strcmp (names [i], infoname) == 0)
	    return i;
    return -1;
#else				/* this doesn't work for some reason */
    extern char *bsearch();
    register char **bret;

    bret = (char **) bsearch ( infoname, (char *) names, max, sizeof (char *), strcmp);
    (void) fprintf (trace, "search looking for %s.\n", infoname);
    (void) fprintf (trace, "base=%#x, bret=%#x, nel=%d.\n", names, bret, max);
    (void) fprintf (trace, "returning %d.\n", bret == NULL ? -1 : bret - names);
    if (bret == NULL)
	return -1;
    else
	return bret - names;
#endif /* OLD */
}

/*
    return the value of the terminfo string 'infoname'
*/
char *getinfostr (infoname)
register char *infoname;
{
    register int i;

    if (verbose > 1)
	(void) fprintf (trace, "looking for terminfo value of %s.\n",
	    infoname);

    i = search (strnames, strcount, infoname);
    if (i != -1)
	{
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "\tvalue is:");
	    tpr (trace, strval [uselevel] [i]);
	    (void) fprintf (trace, ".\n");
	    }
	return strval [uselevel] [i];
	}

    if (verbose > 1)
	(void) fprintf (trace, "terminfo name '%s' not found.\n", infoname);

    return (char *) NULL;
}

/*
    Replace the value stored for the terminfo boolean
    capability 'infoname' with the newvalue.
*/
void putbool (infoname, newvalue)
register char *infoname;
register int newvalue;
{
    register int i;

    if (verbose > 1)
	(void) fprintf (trace, "changing value for %s to %d.\n",
	    infoname, newvalue);

    i = search (boolnames, boolcount, infoname);
    if (i != -1)
	{
	if (verbose > 1)
	    (void) fprintf (trace, "value was: %d.\n", boolval[uselevel][i]);

	boolval [uselevel] [i] = newvalue;
	return;
	}

    (void) fprintf (stderr, "%s: TERM=%s: the boolean name '%s' was not found!\n",
	progname, term_name, infoname);
}

/*
    Replace the value stored for the terminfo number
    capability 'infoname' with the newvalue.
*/
void putnum (infoname, newvalue)
register char *infoname;
register int newvalue;
{
    register int i;

    if (verbose > 1)
	(void) fprintf (trace, "changing value for %s to %d.\n",
	    infoname, newvalue);

    i = search (numnames, numcount, infoname);
    if (i != -1)
	{
	if (verbose > 1)
	    (void) fprintf (trace, "value was: %d.\n",
		numval [uselevel] [i]);

	numval [uselevel] [i] = newvalue;
	return;
	}

    (void) fprintf (stderr, "%s: TERM=%s: the numeric name '%s' was not found!\n",
	progname, term_name, infoname);
}

/*
    replace the value stored for the terminfo string capability 'infoname'
    with the newvalue.
*/
void putstr (infoname, newvalue)
register char *infoname, *newvalue;
{
    register int i;

    if (verbose > 1)
	{
	(void) fprintf (trace, "changing value for %s to ", infoname);
	tpr (trace, newvalue);
	(void) fprintf (trace, ".\n");
	}

    i = search (strnames, strcount, infoname);
    if (i != -1)
	{
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "value was:");
	    tpr (trace, strval [uselevel] [i]);
	    (void) fprintf (trace, ".\n");
	    }
	strval [uselevel] [i] = nextstring;
	while (*newvalue)
	    *nextstring++ = *newvalue++;
	*nextstring++ = '\0';
	return;
	}

    (void) fprintf (stderr, "%s: TERM=%s: the string name '%s' was not found!\n",
	progname, term_name, infoname);
}

/*
    Add in extra delays if they are not recorded already.
    This is done before the padding information has been modified by
    changecalculations() below, so the padding information, if there
    already, is still at the beginning of the string in termcap format.
*/
void addpadding (cappadding, infostr)
register int cappadding;
register char *infostr;
{
    register char *cap;
    char tempbuffer [100];

    /* Is there padding to add? */
    if (cappadding > 0)
	/* Is there a string to add it to? */
	if (cap = getinfostr (infostr))
	    /* Is there any padding info already? */
	    if (ispadchar(*cap))
		{
		/* Assume that the padding info that is there is correct. */
		}
	    else
		{
		/* Add the padding at the end of the present string. */
		(void) sprintf(tempbuffer, "%s$<%d>", cap, cappadding);
		putstr (infostr, tempbuffer);
		}
	else
	    {
	    /* Create a new string that only has the padding. */
	    (void) sprintf(tempbuffer, "$<%d>", cappadding);
	    putstr (infostr, tempbuffer);
	    }
}

struct
    {
    char *capname;
    char *keyedinfoname;
    } ko_map[] =
	{
	"al",		"kil1",
	"bs",		"kbs",		/* special addition */
	"bt",		"kcbt",
	"cd",		"ked",
	"ce",		"kel",
	"cl",		"kclr",
	"ct",		"ktbc",
	"dc",		"kdch1",
	"dl",		"kdl1",
	"do",		"kcud1",
	"ei",		"krmir",
	"ho",		"khome",
	"ic",		"kich1",
	"im",		"kich1",	/* special addition */
	"le",		"kcub1",
	"ll",		"kll",
	"nd",		"kcuf1",
	"sf",		"kind",
	"sr",		"kri",
	"st",		"khts",
	"up",		"kcuu1",
/*	"",		"kctab",	*/
/*	"",		"knp",		*/
/*	"",		"kpp",		*/
	0,		0
	};

/*
    Work with the ko string. It is a comma separated list of keys for which
    the keyboard has a key by the same name that emits the same sequence.
    For example, ko=dc,im,ei means that there are keys called
    delete-character, enter-insert-mode and exit-insert-mode on the keyboard,
    and they emit the same sequences as specified in the dc, im and ei
    capabilities.
*/
handleko()
{
    char capname[3];
    register char *capstr;
    register int i, j, found;
    register char *infostr;

    if (verbose > 1)
	(void) fprintf (trace, "working on termcap ko string.\n");

    if (ostrval[uselevel][cap_ko] == NULL)
	return;

    capname[2] = '\0';
    for (i = 0; ostrval[uselevel][cap_ko][i] != '\0'; )
	{
	/* isolate the termcap name */
	capname[0] = ostrval[uselevel][cap_ko][i++];
	if (ostrval[uselevel][cap_ko][i] == '\0')
	    break;
	capname[1] = ostrval[uselevel][cap_ko][i++];
	if (ostrval[uselevel][cap_ko][i] == ',')
	    i++;

	if (verbose > 1)
	    {
	    (void) fprintf (trace, "key termcap name is '");
	    tpr (trace, capname);
	    (void) fprintf (trace, "'.\n");
	    }

	/* match it up into our list */
	found = 0;
	for (j = 0; !found && ko_map[j].keyedinfoname != NULL; j++)
	    {
	    if (verbose > 1)
		(void) fprintf (trace, "looking at termcap name %s.\n",
		    ko_map[j].capname);
	    if (capname[0] == ko_map[j].capname[0] &&
		capname[1] == ko_map[j].capname[1])
		{
		/* add the value to our database */
		if ((capstr = getcapstr (capname)) != NULL)
		    {
		    infostr = getinfostr (ko_map[j].keyedinfoname);
		    if (infostr == NULL)
			{
			/* skip any possible padding information */
			while (ispadchar(*capstr))
			    capstr++;
			putstr (ko_map[j].keyedinfoname, capstr);
			}
		    else
			if (strcmp(capstr, infostr) != 0)
			    {
			    (void) fprintf (stderr,
				"%s: TERM=%s: a function key for '%s' was specified with the value ",
				progname, term_name, capname);
			    tpr (stderr, capstr);
			    (void) fprintf (stderr, ", but it already has the value '");
			    tpr (stderr, infostr);
			    (void) fprintf (stderr, "'.\n");
			    }
		    }
		found = 1;
		}
	    }

	if (!found)
	    {
	    (void) fprintf (stderr, "%s: TERM=%s: the unknown termcap name '%s' was\n",
		progname, term_name, capname);
	    (void) fprintf (stderr, "specified in the 'ko' termcap capability.\n");
	    }
	}
}

#undef CTRL
#define CTRL(x)		((x) & 037)
struct
    {
    char vichar;
    char *keyedinfoname;
    } ma_map[] =
	{
	CTRL('J'),	"kcud1",	/* down */
	CTRL('N'),	"kcud1",
	'j',		"kcud1",
	CTRL('P'),	"kcuu1",	/* up */
	'k',		"kcuu1",
	'h',		"kcub1",	/* left */
	CTRL('H'),	"kcub1",
	' ',		"kcuf1",	/* right */
	'l',		"kcuf1",
	'H',		"khome",	/* home */
	CTRL('L'),	"kclr",		/* clear */
	0,		0
	};

/*
    Work with the ma string. This is a list of pairs of characters.
    The first character is the what a function key sends. The second
    character is the equivalent vi function that should be done when
    it receives that character. Note that only function keys that send
    a single character could be defined by this list.
*/

void prchar (stream, c)
register FILE *stream;
register int c;
{
    char xbuf[2];
    xbuf[0] = c;
    xbuf[1] = '\0';
    (void) fprintf (stream, "%s", iexpand (xbuf));
}

handlema()
{
    register char vichar;
    char cap[2];
    register int i, j, found;
    register char *infostr;

    if (verbose > 1)
	(void) fprintf (trace, "working on termcap ma string.\n");

    if (ostrval[uselevel][cap_ma] == NULL)
	return;

    cap[1] = '\0';
    for (i = 0; ostrval[uselevel][cap_ma][i] != '\0'; )
	{
	/* isolate the key's value */
	cap[0] = ostrval[uselevel][cap_ma][i++];
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "key value is '");
	    tpr (trace, cap);
	    (void) fprintf (trace, "'.\n");
	    }

	if (ostrval[uselevel][cap_ma][i] == '\0')
	    break;

	/* isolate the vi key name */
	vichar = ostrval[uselevel][cap_ma][i++];
	if (verbose > 1)
	    {
	    (void) fprintf (trace, "the vi key is '");
	    prchar (trace, vichar);
	    (void) fprintf (trace, "'.\n");
	    }

	/* match up the vi name in our list */
	found = 0;
	for (j = 0; !found && ma_map[j].keyedinfoname != NULL; j++)
	    {
	    if (verbose > 1)
		{
		(void) fprintf (trace, "looking at vi character '");
		prchar (trace, ma_map[j].vichar);
		(void) fprintf (trace, "'\n");
		}
	    if (vichar == ma_map[j].vichar)
		{
		infostr = getinfostr (ma_map[j].keyedinfoname);
		if ( infostr == NULL )
		    putstr (ma_map[j].keyedinfoname, cap);
		else if (strcmp (cap, infostr) != 0)
		    {
		    (void) fprintf (stderr, "%s: TERM=%s: the vi character '",
			progname, term_name);
		    prchar (stderr, vichar);
		    (void) fprintf (stderr,
			"' (info '%s') has the value '",
			ma_map[j].keyedinfoname);
		    tpr (stderr, infostr);
		    (void) fprintf (stderr, "', but 'ma' gives '");
		    prchar (stderr, cap[0]);
		    (void) fprintf (stderr, "'.\n");
		    }
		found = 1;
		}
	    }

	if (!found)
	    {
	    (void) fprintf (stderr, "%s: the unknown vi key '", progname);
	    prchar (stderr, vichar);
	    (void) fprintf (stderr, "' was\n");
	    (void) fprintf (stderr, "specified in the 'ma' termcap capability.\n");
	    }
	}
}

/*
    Many capabilities were defaulted in termcap which must now be explicitly
    given. We'll assume that the defaults are in effect for this terminal.
*/
void adddefaults ()
{
    register char *cap;
    register int sg;

    if (verbose > 1)
	(void) fprintf (trace, "assigning defaults.\n");

    /* cr was assumed to be ^M, unless nc was given, */
    /* which meant it could not be done. */
    /* Also, xr meant that ^M acted strangely. */
    if ( (getinfostr ("cr") == NULL) && !oboolval[uselevel][cap_nc] &&
	 !oboolval[uselevel][cap_xr])
	if ( (cap = getcapstr ("cr")) == NULL)
	    putstr ("cr", "\r");
	else
	    putstr ("cr", cap);

    /* cursor down was assumed to be ^J if not specified by nl */
    if (getinfostr ("cud1") == NULL)
	if (ostrval[uselevel][cap_nl] != NULL)
	    putstr ("cud1", ostrval[uselevel][cap_nl]);
	else
	    putstr ("cud1", "\n");

    /* ind was assumed to be ^J, unless ns was given, */
    /* which meant it could not be done. */
    if ( (getinfostr ("ind") == NULL) && !oboolval[uselevel][cap_ns] )
	if ( ostrval[uselevel][cap_nl] == NULL)
	    putstr ("ind", "\n");
	else
	    putstr ("ind", ostrval[uselevel][cap_nl]);

    /* bel was assumed to be ^G */
    if (getinfostr ("bel") == NULL)
	putstr ("bel", "\07");

    /* if bs, then could do backspacing, */
    /* with value of bc, default of ^H */
    if ( (getinfostr ("cub1") == NULL) && oboolval[uselevel][cap_bs] )
	if (ostrval[uselevel][cap_bc] != NULL)
	    putstr ("cub1", ostrval[uselevel][cap_bc]);
	else
	    putstr ("cub1", "\b");

    /* default xon to true */
    if ( !otgetflag ("xo") )
	putbool ("xon", 1);

    /* if pt, then hardware tabs are allowed, */
    /* with value of ta, default of ^I */
    if ( (getinfostr ("ht") == NULL) && oboolval[uselevel][cap_pt] )
	if ( (cap = getcapstr ("ta")) == NULL)
	    putstr ("ht", "\t");
	else
	    putstr ("ht", cap);

    /* The dX numbers are now stored as padding */
    /* in the appropriate terminfo string. */
    addpadding (onumval[uselevel][cap_dB], "cub1");
    addpadding (onumval[uselevel][cap_dC], "cr");
    addpadding (onumval[uselevel][cap_dF], "ff");
    addpadding (onumval[uselevel][cap_dN], "cud1");
    addpadding (onumval[uselevel][cap_dT], "ht");

    /* The ug and sg caps were essentially identical, */
    /* so ug almost never got used. We set sg from ug */
    /* if it hasn't already been set. */
    if (onumval[uselevel][cap_ug] >= 0 && (sg = otgetnum ("sg")) < 0)
	putnum ("xmc", onumval[uselevel][cap_ug]);
    else if ((onumval[uselevel][cap_ug] >= 0) &&
	     (sg >= 0) &&
	     (onumval[uselevel][cap_ug] != sg))
	(void) fprintf (stderr,
	    "%s: TERM=%s: Warning: termcap sg and ug had different values (%d<->%d).\n",
	    progname, term_name, sg, onumval[uselevel][cap_ug]);

    /* The MT boolean was never really part of termcap, */
    /* but we can check for it anyways. */
    if (oboolval[uselevel][cap_MT] && !otgetflag ("km"))
	putbool ("km", 1);

    /* the rs string was renamed r2 (info rs2) */
    if ( (ostrval[uselevel][cap_rs] != NULL) &&
	 (ostrval[uselevel][cap_rs][0] != NULL) )
	putstr ("rs2", ostrval[uselevel][cap_rs]);

    handleko();
    handlema();
}

#define caddch(x) *to++ = (x)

/*
    add the string to the string table
*/
char *caddstr (to, str)
register char *to, *str;
{
    while (*str)
	*to++ = *str++;
    return to;
}

/* If there is no padding info or parmed strings, */
/* then we do not need to copy the string. */
int needscopying (string)
register char *string;
{
    /* any string at all? */
    if (string == NULL)
	return 0;

    /* any padding info? */
    if (ispadchar (*string))
	return 1;

    /* any parmed info? */
    while (*string)
	if (*string++ == '%')
	    return 1;

    return 0;
}

/*
    Certain manipulations of the stack require strange manipulations of the
    values that are on the stack. To handle these, we save the values of the
    parameters in registers at the very beginning and make the changes in
    the registers. We don't want to do this in the general case because of the
    potential performance loss.
*/
int fancycap (string)
register char *string;
{
    register int parmset = 0;

    while (*string)
	if (*string++ == '%')
	    {
	    switch (*string)
		{
		/* These manipulate just the top value on the stack, so we */
		/* only have to do something strange if a %r follows. */
		case '>': case 'B': case 'D':
		    parmset = 1;
		    break;
		/* If the parm has already been been pushed onto the stack */
		/* by %>, then we can not reverse the parms and must get */
		/* them from the registers. */
		case 'r':
		    if (parmset)
			return 1;
		    break;
		/* This manipulates both parameters, so we cannot */
		/* just do one and leave the value on the stack */
		/* like we can with %>, %B or %D. */
		case 'n':
		    return 1;
		}
	    string++;
	    }
    return 0;
}

/*
    Change old style of doing calculations to the new stack style.
    Note that this will not necessarily produce the most efficient string,
    but it will work.
*/
void changecalculations ()
{
    register int i, currentparm;
    register char *from, *to = nextstring;
    int ch;
    int parmset, parmsaved;
    char padding [100], *saveto;

    for (i = 0; strnames [i]; i++)
	if (needscopying (strval [uselevel] [i]))
	    {
	    if (verbose)
		{
		(void) fprintf (trace, "%s needs copying, was:", strnames [i]);
		tpr (trace, strval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}

	    from = strval [uselevel] [i];
	    strval [uselevel] [i] = to;
	    currentparm = 1;
	    parmset = 0;

	    /* Handle padding information. Save it so that it can be */
	    /* placed at the end of the string where it should */
	    /* have been in the first place. */
	    if (ispadchar (*from))
		{
		saveto = to;
		to = padding;
		to = caddstr (to, "$<");
		while (isdigit (*from) || *from == '.')
		    caddch (*from++);
		if (*from == '*')
		    caddch (*from++);
		caddch ('>');
		caddch ('\0');
		to = saveto;
		}
	    else
		padding [0] = '\0';

	    if (fancycap (from))
		{
		to = caddstr (to, "%p1%Pa%p2%Pb");
		parmsaved = 1;
		(void) fprintf (stderr,
		    "%s: TERM=%s: Warning: the string produced for '%s' may be inefficient.\n",
		    progname, term_name, strnames[i]);
		(void) fprintf (stderr, "It should be looked at by hand.\n");
		}
	    else
		parmsaved = 0;

	    while (ch = *from++)
		if (ch != '%')
		    caddch (ch);
		else
		    switch (ch = *from++)
			{
			case '.':	/* %.  -> %p1%c */
			case 'd':	/* %d  -> %p1%d */
			case '2':	/* %2  -> %p1%2.2d */
			case '3':	/* %3  -> %p1%3.3d */
			case '+':	/* %+x -> %p1%'x'%+%c */

			case '>':	/* %>xy -> %p1%Pc%?%'x'%>%t%gc%'y'%+ */
					/* if current value > x, then add y. */
					/* No output. */

			case 'B':	/* %B: BCD (16*(x/10))+(x%10) */
					/* No output. */
					/* (Adds Regent 100) */

			case 'D':	/* %D: Reverse coding (x-2*(x%16)) */
					/* No output. */
					/* (Delta Data) */

			    if (!parmset)
				if (parmsaved)
				    {
				    to = caddstr (to, "%g");
				    if (currentparm == 1)
					caddch ('a');
				    else
					caddch ('b');
				    }
				else
				    {
				    to = caddstr (to, "%p");
				    if (currentparm == 1)
					caddch ('1');
				    else
					caddch ('2');
				    }
			    currentparm = 3 - currentparm;
			    parmset = 0;
			    switch (ch)
				{
				case '.':
				    to = caddstr (to, "%c");
				    break;
				case 'd':
				    to = caddstr (to, "%d");
				    break;
				case '2': case '3':
#ifdef USG	/* Vr2==USG, Vr3==SYSV. Use %02d for Vr2, %2.2d for Vr3 */
				    caddch('%'); caddch ('0');
#else
				    caddch('%'); caddch (ch); caddch ('.');
#endif /* USG vs. SYSV */
				    caddch (ch); caddch ('d');
				    break;
				case '+':
				    to = caddstr (to, "%'");
				    caddch (*from++);
				    to = caddstr (to, "'%+%c");
				    break;
				case '>':
				    to = caddstr (to, "%Pc%?%'");
				    caddch (*from++);
				    to = caddstr (to, "'%>%t%gc%'");
				    caddch (*from++);
				    to = caddstr (to, "'%+");
				    parmset = 1;
				    break;
				case 'B':
				    to = caddstr (to, "%Pc%gc%{10}%/%{16}%*%gc%{10}%m%+");
				    parmset = 1;
				    break;

				case 'D':
				    to = caddstr (to, "%Pc%gc%gc%{16}%m%{2}%*%-");
				    parmset = 1;
				    break;
				}
			    break;

			/* %r reverses current parameter */
			case 'r':
			    currentparm = 3 - currentparm;
			    break;

			/* %n: exclusive-or row AND column */
			/* with 0140, 96 decimal, no output */
			/* (Datamedia 2500, Exidy Sorceror) */
			case 'n':
			    to = caddstr (to, "%ga%'`'%^%Pa");
			    to = caddstr (to, "%gb%'`'%^%Pb");
			    break;

			/* assume %x means %x */
			/* this includes %i and %% */
			default:
			    caddch ('%');
			    caddch (ch);
			}
	    to = caddstr (to, padding);
	    caddch ('\0');

	    if (verbose)
		{
		(void) fprintf (trace, "and has become:");
		tpr (trace, strval [uselevel] [i]);
		(void) fprintf (trace, ".\n");
		}
	    }
    nextstring = to;
}

print_no_use_entry()
{
    register int i;

    pr_heading ("", buflongname);
    pr_bheading();

    for (i = 0; boolcodes [i]; i++)
	if (boolval [0] [i])
	    pr_boolean (boolnames[i], (char *)0, (char *)0, 1);

    pr_bfooting();
    pr_sheading();

    for (i = 0; numcodes [i]; i++)
	if (numval [0] [i] > -1)
	    pr_number(numnames[i], (char *)0, (char *)0, numval[0][i]);

    pr_nfooting();
    pr_sheading();

    for (i = 0; strcodes [i]; i++)
	if (strval [0] [i])
	    pr_string (strnames[i], (char *)0, (char *)0, strval[0][i]);

    pr_sfooting();
}

print_use_entry (usename)
char *usename;
{
    register int i;

    pr_heading ("", buflongname);
    pr_bheading();

    for (i = 0; boolcodes [i]; i++)
	if (boolval [0] [i] && !boolval [1] [i])
	    pr_boolean (boolnames[i], (char *)0, (char *)0, 1);
	else if (!boolval [0] [i] && boolval [1] [i])
	    pr_boolean (boolnames[i], (char *)0, (char *)0, -1);

    pr_bfooting();
    pr_nheading();

    for (i = 0; numcodes [i]; i++)
	if ((numval [0] [i] > -1) && (numval [0] [i] != numval [1] [i]))
	    pr_number (numnames[i], (char *)0, (char *)0, numval[0][i]);
	else if ((numval [0] [i] == -1) && (numval [1] [i] > -1))
	    pr_number (numnames[i], (char *)0, (char *)0, -1);

    pr_nfooting();
    pr_sheading();

    for (i = 0; strcodes [i]; i++)
	/* print out str[0] if: */
	/* str[0] != NULL and str[1] == NULL, or str[0] != str[1] */
	if (strval [0] [i] &&
	    ((strval[1][i] == NULL) ||
	     (strcmp(strval [0] [i],strval [1] [i]) != 0)) )
	    pr_string (strnames[i], (char *)0, (char *)0, strval[0][i]);
	/* print out @ if str[0] == NULL and str[1] != NULL */
	else if (strval [0] [i] == NULL && strval [1] [i] != NULL)
	    pr_string (strnames[i], (char *)0, (char *)0, (char *)0);

    pr_sfooting();

    (void) printf ("\tuse=%s,\n", usename);
}

captoinfo ()
{
    char usename[512];
    char *sterm_name;

    if (term_name == NULL)
	{
	(void) fprintf (stderr, "%s: Null term_name given.\n", progname);
	return;
	}

    if (verbose)
	(void) fprintf (trace, "changing cap to info, TERM=%s.\n", term_name);

    uselevel = 0;
    if (filltables () == 0)
	return;
    getlongname ();
    adddefaults ();
    changecalculations ();
    if (TLHtcfound != 0)
	{
	uselevel = 1;
	if (verbose)
	    (void) fprintf (trace, "use= found, %s uses %s.\n",
		term_name, TLHtcname);
	(void) strcpy (usename, TLHtcname);
	sterm_name = term_name;
	term_name = usename;
	if (filltables () == 0)
	    return;
	adddefaults ();
	changecalculations ();
	term_name = sterm_name;
	print_use_entry (usename);
	}
    else
	print_no_use_entry ();
}

use_etc_termcap ()
{
    if (verbose)
	(void) fprintf (trace, "reading from /etc/termcap\n");
    term_name = getenv ("TERM");
    captoinfo ();
}

initdirname ()
{
#if defined(SYSV) || defined(USG)  /* handle both Sys Vr2 and Vr3 curses */
    (void) getcwd (dirname, BUFSIZ-2);
#else
    (void) getwd (dirname);
#endif /* SYSV || USG */
    if (verbose)
	(void) fprintf (trace, "current directory name=%s.\n", dirname);
    environ = newenviron;
}

setfilename (capfile)
register char *capfile;
{
    if (capfile [0] == '/')
	(void) sprintf (TERMCAP, "TERMCAP=%s", capfile);
    else
	(void) sprintf (TERMCAP, "TERMCAP=%s/%s", dirname, capfile);
    if (verbose)
	(void) fprintf (trace, "setting the environment for %s.\n", TERMCAP);
}

setterm_name ()
{
    if (verbose)
	(void) fprintf (trace, "setting the environment for TERM=%s.\n", term_name);
    (void) sprintf (TERM, "TERM=%s", term_name);
}

/* Look at the current line to see if it is a list of names. */
/* If it is, return the first name in the list, else NULL. */
/* As a side-effect, comment lines and blank lines */
/* are copied to standard output. */
char *getterm_name (line)
register char *line;
{
    register char *lineptr = line;

    if (verbose)
	(void) fprintf (trace, "extracting name from '%s'.\n", line);

    /* Copy comment lines out. */
    if (*line == '#')
	{
	if (copycomments)
	    (void) printf ("%s", line);
	}
    /* Blank lines get copied too. */
    else if (isspace (*line))
	{
	if (copycomments)
	    {
	    for ( ; *lineptr ; lineptr++)
		if (!isspace(*lineptr))
		    break;
	    if (*lineptr == '\0')
		(void) printf ("\n");
	    }
	}
    else
	for ( ; *lineptr ; lineptr++)
	    if (*lineptr == '|' || *lineptr == ':')
		{
		*lineptr = '\0';
		if (verbose)
		    (void) fprintf (trace, "returning %s.\n", line);
		return line;
		}
    if (verbose)
	(void) fprintf (trace, "returning NULL.\n");
    return NULL;
}

use_file (filename)
register char *filename;
{
    register FILE *termfile;
    char buffer [BUFSIZ];

    if (verbose)
	(void) fprintf (trace, "reading from %s.\n", filename);

    if ( (termfile = fopen (filename, "r")) == NULL)
	{
	(void) fprintf (stderr, "%s: cannot open %s for reading.\n",
	    progname, filename);
	return;
	}

    copycomments++;
    setfilename (filename);

    while (fgets (buffer, BUFSIZ, termfile) != NULL)
	{
	if ((term_name = getterm_name (buffer)) != NULL)
	    {
	    setterm_name ();
	    captoinfo ();
	    }
	}
}

/*
    Sort a name and code table pair according to the name table.
    Use a simple bubble sort for now. Too bad I can't call qsort(3).
    At least I only have to do it once for each table.
*/
sorttable (nametable, codetable)
char *nametable [], *codetable [];
{
    register int i, j;
    register char *c;

    for (i = 0; nametable [i]; i++)
	for (j = 0; j < i; j++)
	    if (strcmp (nametable [i], nametable [j]) < 0)
		{
		c = nametable [i];
		nametable [i] = nametable [j];
		nametable [j] = c;
		c = codetable [i];
		codetable [i] = codetable [j];
		codetable [j] = c;
		}
}

/*
    Initialize and sort the name and code tables. Allocate space for the
    value tables.
*/
inittables ()
{
    register unsigned int i;

    for (i = 0; boolnames [i]; i++)
	;
    boolval[0] = (char *) malloc (i * sizeof (char));
    boolval[1] = (char *) malloc (i * sizeof (char));
    boolcount = i;
    sorttable (boolnames, boolcodes);

    for (i = 0; numcodes [i]; i++)
	;
    numval [0] = (short *) malloc (i * sizeof (short));
    numval [1] = (short *) malloc (i * sizeof (short));
    numcount = i;
    sorttable (numnames, numcodes);

    for (i = 0; strcodes [i]; i++)
	;
    strval [0] = (char **) malloc (i * sizeof (char *));
    strval [1] = (char **) malloc (i * sizeof (char *));
    strcount = i;
    sorttable (strnames, strcodes);
}

main (argc, argv)
int argc;
char **argv;
{
    int c;
    char _capbuffer [8192];
    char _bp [TBUFSIZE];
    char _buflongname [128];

    capbuffer = &_capbuffer[0];
    bp = &_bp[0];
    buflongname = &_buflongname[0];
    progname = argv [0];

    while ( (c = getopt (argc, argv, "1vVw:")) != EOF)
	switch (c)
	    {
	    case '1':
		pr_onecolumn (1);
		break;
	    case 'w':
		pr_width (atoi(optarg));
		break;
	    case 'v':
		verbose++;
		break;
	    case 'V':
		printf("@(#)curses:screen/captoinfo.c	1.6.1.7\n");
		fflush(stdout);
		exit (0);
	    case '?':
		(void) fprintf (stderr,
		    "usage: %s [-1Vv] [-w width] [filename ...]\n", progname);
		(void) fprintf (stderr, "\t-1\tsingle column output\n");
		(void) fprintf (stderr, "\t-v\tverbose debugging output\n");
		(void) fprintf (stderr, "\t-V\tprint program version\n");
		exit (-1);
	    }

    /* initialize */
    pr_init (pr_terminfo);
    inittables ();

    if (optind >= argc)
	use_etc_termcap ();
    else
	{
	initdirname ();
	for ( ; optind < argc ; optind++)
	    use_file (argv [optind]);
	}

    return 0;
}

/* fake out the modules in print.c so we don't have to load in */
/* cexpand.c and infotocap.c */
/* ARGSUSED */
int cpr(stream, string) FILE *stream; char *string; { return 0; }
/* ARGSUSED */
char *cexpand(string) char *string; { return string; }
/* ARGSUSED */
char *infotocap(value, err) char *value; int *err; { return value; }
