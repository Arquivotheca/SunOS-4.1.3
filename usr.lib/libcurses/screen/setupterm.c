/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)setupterm.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.44 */
#endif

#ifndef	NOBLIT
#include	<sys/jioctl.h>
#endif	/* NOBLIT */
#include	"curses_inc.h"
#include	"uparm.h"
#ifndef	_TERMPATH
#define	_TERMPATH(file)	"/usr/lib/terminfo/file"
#endif	/* _TERMPATH */
#include	<errno.h>

chtype	bit_attributes[NUM_ATTRIBUTES] =
	{
	    A_STANDOUT,
	    A_UNDERLINE,
	    A_ALTCHARSET,
	    A_REVERSE,
	    A_BLINK,
	    A_DIM,
	    A_BOLD,
	    A_INVIS,
	    A_PROTECT
	};

extern	char	*strncpy();
char	*Def_term = "unknown",	/* default terminal type */
	term_parm_err[32], ttytype[128], _frst_tblstr[1400];

static int stringsneq();


TERMINAL		_first_term, *cur_term = &_first_term;
struct	_bool_struct	_frst_bools, *cur_bools = &_frst_bools;
struct	_num_struct	_frst_nums, *cur_nums = &_frst_nums;
struct	_str_struct	_frst_strs, *cur_strs = &_frst_strs;

/* _called_before is used/cleared by delterm.c and restart.c */
char	_called_before = 0;
short	term_errno = -1;

#ifdef	DUMPTI
extern	char	*boolfnames[], *boolnames[], *boolcodes[],
		*numfnames[], *numnames[], *numcodes[],
		*strfnames[], *strnames[], *strcodes[];

main(argc, argv)	/* FOR DEBUG ONLY */
int	argc;
char	**argv;
{
    if (argc > 1)
	setupterm(argv[1], 1, (int *)0);
    else
	setupterm((char *)0, 1, (int *)0);
    return (0);
}

_Pr(ch)	/* FOR DEBUG ONLY */
register	int	ch;
{
    if (ch >= 0200)
    {
	printf("M-");
	ch -= 0200;
    }
    if ( (ch < ' ') || (ch == 0177) )
	printf("^%c", ch ^ 0100);
    else
	printf("%c", ch);
}

_Sprint(n, string)	/* FOR DEBUG ONLY */
int	n;
register	char	*string;
{
    register	int	ch;

    if (n == -1)
    {
	printf(".\n");
	return;
    }
    printf(", string = '");
    while (ch = *string++)
	_Pr(ch&0377);

    printf("'.\n");
}

_Mprint(n, memory)	/* FOR DEBUG ONLY */
register	int	n;
register	char	*memory;
{
    register	unsigned	char	ch;

    while (ch = *memory++, n-- > 0)
	_Pr(ch&0377);
}

#define	_Vr2getshi()	_Vr2getsh(ip-2)

#if	vax || pdp11 || i386
#define	_Vr2getsh(ip)	(* (short *) (ip))
#endif	/* vax || pdp11 || i386 */

#ifndef	_Vr2getsh
/*
 * Here is a more portable version, which does not assume byte ordering
 * in shorts, sign extension, etc.
 */
_Vr2getsh(p)
register	char	*p;
{
    register	int	rv;

    if (*p == '\377')
	return (-1);
    rv = (unsigned char) *p++;
    rv += (unsigned char) *p * 256;
    return (rv);
}
#endif	/* _Vr2getsh */

#endif	/* DUMPTI */

#define	_Getshi()	_Getsh(ip) ; ip += 2

/*
 * "function" to get a short from a pointer.  The short is in a standard
 * format: two bytes, the first is the low order byte, the second is
 * the high order byte (base 256).  The only negative number allowed is
 * -1, which is represented as 255, 255.  This format happens to be the
 * same as the hardware on the pdp-11 and vax, making it fast and
 * convenient and small to do this on a pdp-11.
 */

#if	vax || pdp11 || i386
#define	_Getsh(ip)	(* (short *) ip)
#endif	/* vax || pdp11 || i386 */
/*
 * The following macro is partly due to Mike Laman, laman@sdcsvax
 *	NCR @ Torrey Pines.		- Tony Hansen
 */
#if	u3b || u3b15 || u3b2 || mc68000 || sparc
#define	_Getsh(ip)	((short) (*((unsigned char *) ip) | (*(ip+1) << 8)))
#endif	/* u3b || u3b15 || u3b2 || mc68000 || sparc */

#ifndef	_Getsh
/*
 * Here is a more portable version, which does not assume byte ordering
 * in shorts, sign extension, etc. It does assume that the C preprocessor
 * does sign-extension the same as on the machine being compiled for.
 * When ANSI C comes along, this should be changed to check <limits.h>
 * to see if the low character value is negative.
 */
static
_Getsh(p)
register	char	*p;
{
    register	int	rv, rv2;

#if	-1 == '\377'			/* sign extension occurs */
    rv = (*p++) & 0377;
    rv2 = (*p) & 0377;
#else	/* -1 == '\377'			/* no sign extension */
    rv = *p++;
    rv2 = *p;
#endif	/* -1 == '\377' */
    if ((rv2 == 0377) && ((rv == 0377) || (rv == 0376)))
	return (-1);
    return (rv + (rv2 * 256));
}
#endif	/* _Getsh */

/*
 * setupterm: low level routine to dig up terminfo from database
 * and read it in.  Parms are terminal type (0 means use getenv("TERM"),
 * file descriptor all output will go to (for ioctls), and a pointer
 * to an int into which the error return code goes (0 means to bomb
 * out with an error message if there's an error).  Thus,
 * setupterm((char *)0, 1, (int *)0) is a reasonable way for a simple
 * program to set up.
 */
setupterm(term, filenum, errret)
char	*term;
int	filenum;	/* This is a UNIX file descriptor, not a stdio ptr. */
int	*errret;
{
    short	tiebuf[2048];
    char	fname[1025];	/* MAXPATHLEN + 1, sigh */
    register	char	*ip;
    register	char	*cp;
    int		n, tfd;
    char	*lcp, *ccp, **on_sequences, **str_array;
    int		snames, nbools, nints, nstrs, sstrtab;
    char	*strtab;
#ifdef	DUMPTI
    int		Vr2val;
#endif	/* DUMPTI */

    if (term == NULL)
	term = getenv("TERM");
    if (term == NULL || *term == '\0')
	term = Def_term;
    tfd = -1;
    if (errret != 0)
	*errret = -1;
    if ((cp = getenv("TERMINFO")) && *cp)
    {
	if (strlen(cp) + 3 + strlen(term) > 1024)
	{
	    term_errno = TERMINFO_TOO_LONG;
	    goto out_err;
	}
	(void) strcpy(fname, cp);
	cp = fname + strlen(fname);
	*cp++ = '/';
	*cp++ = *term;
	*cp++ = '/';
	(void) strcpy(cp, term);
	tfd = open(fname, 0);
#ifdef	DUMPTI
	printf("looking in file %s\n", fname);
#endif	/* DUMPTI */
	if ((tfd < 0) && (errno == EACCES))
	    goto cant_read;
    }
    if (tfd < 0)
    {
	(void) strcpy(fname, _TERMPATH(a/));
	cp = fname + strlen(fname);
	cp[-2] = *term;
	(void) strcpy(cp, term);
	tfd = open(fname, 0);
#ifdef	DUMPTI
	printf("looking in file %s\n", fname);
#endif	/* DUMPTI */

    }

    if (tfd < 0)
    {
	if (errno == EACCES)
	{
cant_read:
	    term_errno = NOT_READABLE;
	}
	else
	{
	    if (access(_TERMPATH(.), 0))
		term_errno = UNACCESSIBLE;
	    else
	    {
		term_errno = NO_TERMINAL;
		if (errret != 0)
		    *errret = 0;
	    }
	}
	(void) strcpy(term_parm_err, term);
	goto out_err;
    }

    n = read(tfd, (char *)tiebuf, sizeof tiebuf);
    (void) close(tfd);
    if (n <= 0)
    {
corrupt:
	term_errno = CORRUPTED;
	goto out_err;
    }
    else
	if (n == sizeof (tiebuf))
	{
	    term_errno = ENTRY_TOO_LONG;
	    goto out_err;
	}
    cp = ttytype;
    ip = (char *)tiebuf;

    /* Pick up header */
    snames = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Magic number = %d, %#o [%d, %#o].\n", snames, snames, Vr2val, Vr2val);
#endif	/* DUMPTI */
    if (snames != MAGNUM)
	goto corrupt;
    snames = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Size of names = %d, %#o [%d, %#o].\n", snames, snames, Vr2val, Vr2val);
#endif	/* DUMPTI */

    nbools = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Number of bools = %d, %#o [%d, %#o].\n", nbools, nbools, Vr2val, Vr2val);
#endif	/* DUMPTI */

    nints = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Number of ints = %d, %#o [%d, %#o].\n", nints, nints, Vr2val, Vr2val);
#endif	/* DUMPTI */

    nstrs = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Number of strings = %d, %#o [%d, %#o].\n", nstrs, nstrs, Vr2val, Vr2val);
#endif	/* DUMPTI */

    sstrtab = _Getshi();
#ifdef	DUMPTI
    Vr2val = _Vr2getshi();
    printf("Size of string table = %d, %#o [%d, %#o].\n", sstrtab, sstrtab, Vr2val, Vr2val);
    printf("Names are: %.*s.\n", snames, ip);
#endif	/* DUMPTI */

    /* allocate all of the space */
    strtab = NULL;
    if (_called_before)
    {
	/* 2nd or more times through */
	if ((cur_term = (TERMINAL *) malloc(sizeof (TERMINAL))) == NULL)
	    goto badmalloc;
	if ((cur_bools = (struct _bool_struct *) malloc(sizeof (struct _bool_struct))) == NULL)
	    goto freeterminal;
	if ((cur_nums = (struct _num_struct *) malloc(sizeof (struct _num_struct))) == NULL)
	    goto freebools;
	if ((cur_strs = (struct _str_struct *) malloc(sizeof (struct _str_struct))) == NULL)
	{
freenums:
	    free((char *) cur_nums);
freebools:	
	    free((char *) cur_bools);
freeterminal:
	    free((char *) cur_term);
badmalloc:
	    term_errno = TERM_BAD_MALLOC;
#ifdef	DEBUG
	    strcpy(term_parm_err, "setupterm");
#endif	/* DEBUG */
out_err:
	    if (errret == 0)
	    {
		termerr();
		exit(-term_errno);
	    }
	    else
		return (ERR);
	}
    }
    else
    {
	/* First time through */
	_called_before = TRUE;
	cur_term = &_first_term;
	cur_bools = &_frst_bools;
	cur_nums = &_frst_nums;
	cur_strs = &_frst_strs;
	if (sstrtab < sizeof(_frst_tblstr))
	    strtab = _frst_tblstr;
    }

    if (strtab == NULL)
    {
	if ((strtab = (char *) malloc((unsigned) sstrtab)) == NULL)
	{
	    if (cur_strs != &_frst_strs)
		free((char *)cur_strs);
	    goto freenums;
	}
    }

    /* no more catchable errors */
    if (errret)
	*errret = 1;

    (void) strncpy(cur_term->_termname, term, 50);
    /* In case the name is exactly 51 characters */
    cur_term->_termname[50] = '\0';
    cur_term->_bools = cur_bools;
    cur_term->_nums = cur_nums;
    cur_term->_strs = cur_strs;
    cur_term->_strtab = strtab;
    cur_term->sgr_mode = cur_term->sgr_faked = A_NORMAL;

    if (filenum == 1 && !isatty(filenum))
	filenum = 2;	/* Allow output redirect */
    cur_term->Filedes = filenum;
    _blast_keys(cur_term);
    cur_term->_iwait = cur_term->fl_typeahdok = cur_term->_chars_on_queue =
	cur_term->_fl_rawmode = cur_term->_ungotten = 0;
    cur_term->_cursorstate = 1;
    cur_term->_delay = cur_term->_inputfd = cur_term->_check_fd = -1;
    (void) memset((char*)cur_term->_regs, 0, 26 * sizeof(short));

#ifndef	DUMPTI
    def_shell_mode();
    /* This is a useful default for PROGTTY, too */
    PROGTTY = SHELLTTY;
#endif	/* DUMPTI */

    /* Skip names of terminals */
    (void) memcpy((char *) cp, (char *) ip, (int) (snames * sizeof(*cp)));
    ip += snames;

    /*
     * Pull out the booleans.
     * The for loop below takes care of a new curses with an old tic
     * file and visa-versa.  nbools says how many bools the tic file has.
     * So, we only loop for as long as there are bools to read.
     * However, if this is an old curses that doesn't have all the
     * bools that this new tic has dumped, then the extra if
     * "if (cp < fp)" says that if we are going to read into our structure
     * passed its size don't do it but we still need to keep bumping
     * up the pointer of what we read in from the terminfo file.
     */
    {
	char		*fp = &cur_bools->Sentinel;
	register	char	s;
#ifdef	DUMPTI
	register	int	tempindex = 0;
#endif	/* DUMPTI */
	cp = &cur_bools->_auto_left_margin;
	while (nbools--)
	{
	    s = *ip++;
#ifdef	DUMPTI
	    printf("Bool %s [%s] (%s) = %d.\n", boolfnames[tempindex],
		boolnames[tempindex], boolcodes[tempindex], s);
	    tempindex++;
#endif	/* DUMPTI */
	    if (cp < fp)
		*cp++ = s & 01;
	}
	if (cp < fp)
	    (void) memset(cp, 0, (int) ((fp - cp) * sizeof(bool)));
    }

    /* Force proper alignment */
    if (((unsigned int) ip) & 1)
	ip++;

    /*
     * Pull out the numbers.
     */
    {
	register	short	*sp = &cur_nums->_columns;
	short		*fp = &cur_nums->Sentinel;
	register	int	s;
#ifdef	DUMPTI
	register	int	tempindex = 0;
#endif	/* DUMPTI */

	while (nints--)
	{
	    s = _Getshi();
#ifdef	DUMPTI
	    Vr2val = _Vr2getshi();
	    printf("Num %s [%s] (%s) = %d [%d].\n", numfnames[tempindex],
		numnames[tempindex], numcodes[tempindex], s, Vr2val);
	    tempindex++;
#endif	/* DUMPTI */
	    if (sp < fp)
		if (s < 0)
		    *sp++ = -1;
		else
		    *sp++ = s;
	}
	if (sp < fp)
	    (void) memset((char *) sp, '\377', (int) ((fp - sp) * sizeof(short)));
    }

#ifdef	TIOCGWINSZ
    /*
     * ioctls for 4.3BSD and other systems that have adopted TIOCGWINSZ.
     */
    {
	struct	winsize	w;

	if (ioctl(filenum, TIOCGWINSZ, &w) != -1 && w.ws_row != 0 && w.ws_col != 0)
	{
	    cur_nums->_lines = (unsigned char) w.ws_row;
	    cur_nums->_columns = (unsigned char) w.ws_col;
#ifdef	DUMPTI
	    printf("ioctl TIOCGWINSZ override: (lines, columns)=(%d, %d)\n", w.ws_row, w.ws_col);
#endif	/* DUMPTI */
	}
    }
#else	/* TIOCGWINSZ */
# ifdef	JWINSIZE
    /*
     * ioctls for Blit - you may need to #include <jioctl.h>
     * This ioctl defines the window size and overrides what
     * it says in terminfo.
     */
    {
	struct	jwinsize	w;

	if (ioctl(filenum, JWINSIZE, &w) != -1 && w.bytesy != 0 && w.bytesx != 0)
	{
	    cur_nums->_lines = (unsigned char) w.bytesy;
	    cur_nums->_columns = (unsigned char) w.bytesx;
#  ifdef	DUMPTI
	    printf("ioctl JWINSIZE override: (lines, columns)=(%d, %d)\n", w.bytesy, w.bytesx);
#  endif	/* DUMPTI */
	}
    }
# endif	/* JWINSIZE */

# ifdef	TIOCGSIZE
    /*
     * ioctls for Sun.
     */
    {
	struct	ttysize	w;

	if (ioctl(filenum, TIOCGSIZE, &w) != -1 && w.ts_lines > 0 && w.ts_cols > 0)
	{
	    cur_nums->_lines = w.ts_lines;
	    cur_nums->_columns = w.ts_cols;
#  ifdef	DUMPTI
	    printf("ioctl TIOCGSIZE override: (lines, columns)=(%d, %d)\n", w.ts_lines, w.ts_cols);
#  endif	/* DUMPTI */
	}
    }
# endif	/* TIOCGSIZE */
#endif	/* TIOCGWINSZ */

    /*
     * Check $LINES and $COLUMNS.
     */
    {
	int	ilines = 0, icolumns = 0;

	lcp = getenv("LINES");
	ccp = getenv("COLUMNS");
	if (lcp)
	    if ((ilines = atoi(lcp)) > 0)
	    {
		cur_nums->_lines = ilines;
#ifdef	DUMPTI
		printf("$LINES override: lines=%d\n", ilines);
#endif	/* DUMPTI */
	    }
	    if (ccp)
		if ((icolumns = atoi(ccp)) > 0)
		{
		    cur_nums->_columns = icolumns;
#ifdef	DUMPTI
		    printf("$COLUMNS override: columns=%d\n", icolumns);
#endif	/* DUMPTI */
		}
    }

    /* Pull out the strings. */
    {
	register	char	**pp = &cur_strs->strs._back_tab;
	char		**fp = &cur_strs->strs3.Sentinel;
#ifdef	DUMPTI
	register	int	tempindex = 0;
	register	char	*startstr = ip + sizeof(short) * nstrs;

	printf("string table = '");
	_Mprint(sstrtab, startstr);
	printf("'\n");
#endif	/* DUMPTI */

	while (nstrs--)
	{
	    n = _Getshi();
#ifdef	DUMPTI
	    Vr2val = _Vr2getshi();
	    printf("String %s [%s] (%s) offset = %d [%d]", strfnames[tempindex],
		strnames[tempindex], strcodes[tempindex], n, Vr2val);
	    tempindex++;
#endif	/* DUMPTI */
	    if (pp < fp)
	    {
#ifdef	DUMPTI
		_Sprint(n, startstr+n);
#endif	/* DUMPTI */
		if (n < 0)
		    *pp++ = NULL;
		else
		    *pp++ = strtab + n;
	    }
#ifdef	DUMPTI
	    else
		_Sprint(-1, (char *) 0);
#endif	/* DUMPTI */
	}
	if (pp < fp)
	    (void) memset((char *) pp, 0, (int) ((fp - pp) * sizeof(charptr)));
    }

    (void) memcpy(strtab, ip, sstrtab);

#ifndef	DUMPTI

    /*
     * If tabs are being expanded in software, turn this off
     * so output won't get messed up.  Also, don't use tab
     * or backtab, even if the terminal has them, since the
     * user might not have hardware tabs set right.
     */
#ifdef	SYSV
    if ((PROGTTY.c_oflag & TABDLY) == TAB3)
    {
	PROGTTY.c_oflag &= ~TABDLY;
	if (PROGTTY.c_oflag == OPOST)
	    PROGTTY.c_oflag = 0;
	reset_prog_mode();
	goto next;
    }
#else	/* SYSV */
    if ((PROGTTY.sg_flags & XTABS) == XTABS)
    {
	PROGTTY.sg_flags &= ~XTABS;
	reset_prog_mode();
	goto next;
    }
#endif	/* SYSV */
    if (dest_tabs_magic_smso)
    {
next:
	cur_strs->strs2._tab = cur_strs->strs._back_tab = NULL;
    }

#ifdef	LTILDE
    ioctl(cur_term -> Filedes, TIOCLGET, &n);
#endif	/* LTILDE */
#endif	/* DUMPTI */

#ifdef	_VR2_COMPAT_CODE
    (void) memcpy(&cur_term->_b1, &cur_bools->_auto_left_margin,
	    (char *) &cur_term->_c1 - (char *) &cur_term->_b1);
    (void) memcpy((char *) &cur_term->_c1, (char *) &cur_nums->_columns,
	    (char *) &cur_term->_Vr2_Astrs._s1 - (char *) &cur_term->_c1);
    (void) memcpy((char *) &cur_term->_Vr2_Astrs._s1, (char *) &cur_strs->strs._back_tab,
	    (char *) &cur_term->Filedes - (char *) &cur_term->_Vr2_Astrs._s1);
#endif	/* _VR2_COMPAT_CODE */

    on_sequences = cur_term->turn_on_seq;
    str_array = (char **) cur_strs;
    {
	static	char	offsets[] =
			{
			    35,	/* enter_standout_mode, */
			    36,	/* enter_underline_mode, */
			    25,	/* enter_alt_charset_mode, */
			    34,	/* enter_reverse_mode, */
			    26,	/* enter_blink_mode, */
			    30,	/* enter_dim_mode, */
			    27,	/* enter_bold_mode, */
			    32,	/* enter_secure_mode, */
			    33,	/* enter_protected_mode, */
			};

	for (n = 0; n < NUM_ATTRIBUTES; n++)
	{
	    if (on_sequences[n] = str_array[offsets[n]])
		cur_term->bit_vector |= bit_attributes[n];
	}
    }

    if (!(set_attributes))
    {
	static	char	faked_attrs[] = { 1, 3, 4, 6 },
			offsets[] =
			{
			    43,	/* exit_standout_mode, */
			    44,	/* exit_underline_mode, */
			    38,	/* exit_alt_charset_mode, */
			};
	char		**off_sequences = cur_term->turn_off_seq;
	int		i;

	if ((max_attributes == -1) && (ceol_standout_glitch || (magic_cookie_glitch >= 0)))
	    max_attributes = 1;

	/* Figure out what attributes need to be faked.  See vidupdate.c */

	for (n = 0; n < sizeof(faked_attrs); n++)
	{
	    if ((!on_sequences[i = faked_attrs[n]]) ||
		(stringsneq(on_sequences[i], on_sequences[0]) == 0))
	    {
		cur_term->sgr_faked |= bit_attributes[i];
	    }
	}

	cur_term->check_turn_off = A_STANDOUT | A_UNDERLINE | A_ALTCHARSET;

	for (n = 0; n < sizeof(offsets); n++)
	{
	    if ((!(off_sequences[n] = str_array[offsets[n]])) ||
		((n > 0) && (stringsneq(off_sequences[n], off_sequences[0]) == 0)) ||
		((n == 2) && (stringsneq(exit_attribute_mode, off_sequences[n]) == 0)))
	    {
		cur_term->check_turn_off &= ~bit_attributes[n];
	    }
	}
    }
    cur_term->cursor_seq[0] = cursor_invisible;
    cur_term->cursor_seq[1] = cursor_normal;
    cur_term->cursor_seq[2] = cursor_visible;

    return (OK);
}

void
_blast_keys(terminal)
TERMINAL	*terminal;
{
    terminal->_keys = NULL;
    terminal->internal_keys = NULL;
    terminal->_ksz = terminal->_first_macro = 0;
    terminal->_lastkey_ordered = terminal->_lastmacro_ordered = -1;
    (void) memset((char*)terminal->funckeystarter, 0, 0400 * sizeof(bool));
}

static int
stringsneq(str1, str2)
char *str1, *str2;
{
        if (str1 == NULL)
                return (str2 != NULL);
        if (str2 == NULL)
                return (1);
        return (strcmp(str1, str2));
}


#ifndef	DUMPTI

reset_prog_mode()
{
    if (_BR(PROGTTY))
#ifdef	SYSV
	(void) ioctl(cur_term -> Filedes, TCSETAW, &PROGTTY);
#else	/* SYSV */
	(void) ioctl(cur_term -> Filedes, TIOCSETN, &PROGTTY);
#endif	/* SYSV */

#ifdef	LTILDE
    ioctl(cur_term -> Filedes, TIOCLGET, &cur_term -> oldlmode);
    cur_term -> newlmode = cur_term -> oldlmode & ~LTILDE;
    if (cur_term -> newlmode != cur_term -> oldlmode)
	ioctl(cur_term -> Filedes, TIOCLSET, &cur_term -> newlmode);
#endif	/* LTILDE */
#ifdef	DIOCSETT
    if (cur_term -> old.st_termt == 0)
	ioctl(cur_term->Filedes, DIOCGETT, &cur_term -> old);
    cur_term -> new = cur_term -> old;
    cur_term -> new.st_termt = 0;
    cur_term -> new.st_flgs |= TM_SET;
    ioctl(cur_term->Filedes, DIOCSETT, &cur_term -> new);
#endif	/* DIOCSETT */
    return (OK);
}

def_shell_mode()
{
#ifdef	SYSV
    (void) ioctl(cur_term -> Filedes, TCGETA, &SHELLTTY);
#else	/* SYSV */
    (void) ioctl(cur_term -> Filedes, TIOCGETP, &SHELLTTY);
#endif	/* SYSV */
    return (OK);
}

#endif	/* DUMPTI */
