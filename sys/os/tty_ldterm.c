#ifndef lint
static  char sccsid[] = "@(#)tty_ldterm.c 1.1 92/07/30 SMI";
#endif

/*
 * Standard tty streams module.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tty.h>
#include <sys/unistd.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/debug.h>

#define	IBSIZE	16		/* "standard" input data block size */
#define	OBSIZE	64		/* "standard" output data block size */
#define	EBSIZE	16		/* "standard" echo data block size */
#define	TTYHOG	CANBSIZ		/* from param.h; in case anyone uses it */
#define	TTXOHI	80
#define	TTXOLO	32

/*
 * Should be "void", not "int" - they return no values!
 */
static int ldtermopen(/*queue_t *q, int dev, int oflag, int sflag*/);
static int ldtermclose(/*queue_t *q, int flag*/);
static int ldtermrput(/*queue_t *q, mblk_t *mp*/);
static int ldtermrsrv(/*queue_t *q*/);
static int ldtermwput(/*queue_t *q, mblk_t *mp*/);
static int ldtermwsrv(/*queue_t *q*/);

/*
 * Since most of the buffering occurs either at the stream head or in
 * the "message currently being assembled" buffer, we have a relatively
 * small input queue, so that blockages above us get reflected fairly
 * quickly to the module below us.  We also have a small maximum packet
 * size, since you can put a message of that size on an empty queue no
 * matter how much bigger than the high water mark it is.
 */
static struct module_info ldtermmiinfo = {
	0x0bad,			/* to the bone */
	"ldterm",
	0,
	128,	/* never eat anything bigger than your head */
	500,
	200
};

static struct qinit ldtermrinit = {
	ldtermrput,
	ldtermrsrv,
	ldtermopen,
	ldtermclose,
	NULL,
	&ldtermmiinfo
};

/*
 * Write side flow control strategy:  We want the module to behave as much as
 * possible as if it had no write service procedure.  Thus, the write put
 * procedure will process incoming messages and hand them to the downstream
 * module whenever canput allows it.  The high water mark is set to one, so
 * that only a single message is queued between the time the downstream module
 * blocks and this module reflects that state upward.
 */
static struct module_info ldtermmoinfo = {
	0x0bad,			/* to the bone */
	"ldterm",
	0,
	INFPSZ,
	1,
	0
};

static struct qinit ldtermwinit = {
	ldtermwput,
	ldtermwsrv,
	ldtermopen,
	ldtermclose,
	NULL,
	&ldtermmoinfo
};

struct streamtab ldtrinfo = {
	&ldtermrinit,
	&ldtermwinit,
	NULL,
	NULL,
	NULL
};

typedef	struct {
	mblk_t	*t_savbp;	/* saved mblk that holds ld struct */
	struct termios t_modes;	/* modes */
	unsigned long t_state;	/* internal state of tty module */
	int	t_line;		/* output line of tty */
	int	t_col;		/* output column of tty */
	int	t_rocount;	/* number of chars echoed since last output */
	int	t_rocol;	/* column in which first such char appeared */
	mblk_t	*t_message;	/* pointer to 1st mblk in message being built */
	mblk_t	*t_endmsg;	/* pointer to last mblk in that message */
	int	t_msglen;	/* number of characters in that message */
	mblk_t	*t_echomp;	/* echoed output being assembled */
	int	t_iocid;	/* ID of ioctl reply being awaited */
	int	t_wbufcid;	/* ID of pending write-side bufcall */
} ldterm_state_t;

/*
 * Internal state bits.
 */
#define	TS_TTSTOP	0x00000001	/* output stopped by ^S */
#define	TS_TBLOCK	0x00000002	/* input stopped by IXOFF mode */
#define	TS_QUOT		0x00000004	/* last character input was \ */
#define	TS_ERASE	0x00000008	/* within a \.../ for PRTRUB */
#define	TS_SLNCH	0x00000010	/* next char svc rtn sees is literal */
#define	TS_PLNCH	0x00000020	/* next char put rtn sees is literal */

#define	TS_TTCR		0x00000040	/* mapping NL to CR-NL */
#define	TS_NOCANPUT	0x00000080	/* canonicalization done by */
					/*   somebody below us */
#define	TS_NOCANSRV	0x00000100
#define	TS_RESCAN	0x00000200	/* canonicalization mode changed, */
					/*   rescan input queue */

#define	TS_IOCWAIT	0x00000400	/* waiting for reply to ioctl message */

/*
 * Statistics counters, for tuning.
 *
 * They record, respectively, the number of regular priority messages
 * processed directly through the write put procedure and the number queued
 * and later processed through the write service procedure.
 *
 * XXX:	Is it worth retaining these counts?  Should they be moved into
 *	ldterm_state_t?
 */
struct ldterm_counts {
	int	ld_wputmsgs;
	int	ld_wsrvmsgs;
} ldterm_stats;
#define	ldterm_wputmsgs	ldterm_stats.ld_wputmsgs
#define	ldterm_wsrvmsgs	ldterm_stats.ld_wsrvmsgs

/*
 * Character types.
 */
#define	PRINTABLE	0		/* ordinary printable character */
#define	CONTROL		1		/* non-special control character */
#define	BACKSPACE	2		/* BS */
#define	NEWLINE		3		/* NL */
#define	TAB		4		/* TAB */
#define	VTAB		5		/* VT */
#define	RETURN		6		/* CR */

/*
 * Table indicating character classes to tty driver.  In particular,
 * if the class is PRINTABLE, then the character needs no special
 * processing on output.
 *
 * Characters in the C1 set are all considered CONTROL; this will
 * work with terminals that properly use the ANSI/ISO extensions,
 * but might cause distress with terminals that put graphics in
 * the range 0200-0237.  On the other hand, characters in that
 * range cause even greater distress to other UNIX terminal drivers....
 */

static char typetab[256] = {
/* 000 */ 	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 004 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 010 */	BACKSPACE,	TAB,		NEWLINE,	CONTROL,
/* 014 */	VTAB,		RETURN,		CONTROL,	CONTROL,
/* 020 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 024 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 030 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 034 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 040 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 044 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 050 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 054 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 060 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 064 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 070 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 074 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 100 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 104 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 110 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 114 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 120 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 124 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 130 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 134 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 140 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 144 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 150 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 154 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 160 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 164 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 170 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 174 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	CONTROL,
/* 200 */ 	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 204 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 210 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 214 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 220 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 224 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 230 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 234 */	CONTROL,	CONTROL,	CONTROL,	CONTROL,
/* 240 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 244 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 250 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 254 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 260 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 264 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 270 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 274 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 300 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 304 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 310 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 314 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 320 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 324 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 330 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 334 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 340 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 344 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 350 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 354 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 360 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 364 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 370 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
/* 374 */	PRINTABLE,	PRINTABLE,	PRINTABLE,	PRINTABLE,
};

/*
 * Translation table for output without OLCUC.  All PRINTABLE-class characters
 * translate to themselves.  All other characters have a zero in the table,
 * which stops the copying.
 */
static unsigned char notrantab[256] = {
/* 000 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 010 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 020 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 030 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 040 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/* 050 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 060 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/* 070 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 110 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 120 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 130 */	'X',	'Y',	'Z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 150 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 160 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 170 */	'x',	'y',	'z',	'{',	'|',	'}',	'~',	0,
/* 200 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 210 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 220 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 230 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */	'\300',	'\301',	'\302',	'\303',	'\304',	'\305',	'\306',	'\307',
/* 310 */	'\310',	'\311',	'\312',	'\313',	'\314',	'\315',	'\316',	'\317',
/* 320 */	'\320',	'\321',	'\322',	'\323',	'\324',	'\325',	'\326',	'\327',
/* 330 */	'\330',	'\331',	'\332',	'\333',	'\334',	'\335',	'\336',	'\337',
/* 340 */	'\340',	'\341',	'\342',	'\343',	'\344',	'\345',	'\346',	'\347',
/* 350 */	'\350',	'\351',	'\352',	'\353',	'\354',	'\355',	'\356',	'\357',
/* 360 */	'\360',	'\361',	'\362',	'\363',	'\364',	'\365',	'\366',	'\367',
/* 370 */	'\370',	'\371',	'\372',	'\373',	'\374',	'\375',	'\376',	'\377',
};

/*
 * Translation table for output with OLCUC.  All PRINTABLE-class characters
 * translate to themselves, except for lower-case letters which translate
 * to their upper-case equivalents.  All other characters have a zero in
 * the table, which stops the copying.
 */
static unsigned char lcuctab[256] = {
/* 000 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 010 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 020 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 030 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 040 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/* 050 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 060 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/* 070 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 110 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 120 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 130 */	'X',	'Y',	'Z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	0,
/* 200 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 210 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 220 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 230 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */	'\300',	'\301',	'\302',	'\303',	'\304',	'\305',	'\306',	'\307',
/* 310 */	'\310',	'\311',	'\312',	'\313',	'\314',	'\315',	'\316',	'\317',
/* 320 */	'\320',	'\321',	'\322',	'\323',	'\324',	'\325',	'\326',	'\327',
/* 330 */	'\330',	'\331',	'\332',	'\333',	'\334',	'\335',	'\336',	'\337',
/* 340 */	'\340',	'\341',	'\342',	'\343',	'\344',	'\345',	'\346',	'\347',
/* 350 */	'\350',	'\351',	'\352',	'\353',	'\354',	'\355',	'\356',	'\357',
/* 360 */	'\360',	'\361',	'\362',	'\363',	'\364',	'\365',	'\366',	'\367',
/* 370 */	'\370',	'\371',	'\372',	'\373',	'\374',	'\375',	'\376',	'\377',
};

/*
 * Input mapping table -- if an entry is non-zero, and XCASE is set,
 * when the corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
static char	imaptab[256] = {
/* 000 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 010 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 020 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 030 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 040 */	0,	'|',	0,	0,	0,	0,	0,	'`',
/* 050 */	'{',	'}',	0,	0,	0,	0,	0,	0,
/* 060 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 070 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 100 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 110 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 120 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 130 */	0,	0,	0,	0,	'\\',	0,	'~',	0,
/* 140 */	0,	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	0,	0,	0,	0,	0,
/* 200-377 aren't mapped */
};

/*
 * Output mapping table -- if an entry is non-zero, and XCASE is set,
 * the corresponding character is printed as "\" followed by the table
 * value.  Mostly used for upper-case only terminals.
 */
static char	omaptab[256] = {
/* 000 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 010 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 020 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 030 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 040 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 050 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 060 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 070 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 100 */	0,	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 110 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 120 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 130 */	'X',	'Y',	'Z',	0,	0,	0,	0,	0,
/* 140 */	'\'',	0,	0,	0,	0,	0,	0,	0,
/* 150 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 160 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 170 */	0,	0,	0,	'(',	'!',	')',	'^',	0,
/* 200-377 aren't mapped */
};

static int	ldtermopen_wakeup(/*caddr_t addr*/);
static mblk_t	*do_canon_input(/*unsigned char c, mblk_t *bpt, int ebsize,
    ldterm_state_t *tp*/);
static int	curline_unget(/*ldterm_state_t *tp*/);
static void	curline_trim(/*ldterm_state_t *tp*/);
static void	curline_rubout(/*unsigned char c, queue_t *q, int ebsize,
    ldterm_state_t *tp*/);
static int	curline_tabcols(/*ldterm_state_t *tp*/);
static void	curline_erase(/*queue_t *q, int ebsize, ldterm_state_t *tp*/);
static void	curline_werase(/*queue_t *q, int ebsize, ldterm_state_t *tp*/);
static void	curline_kill(/*queue_t *q, int ebsize, ldterm_state_t *tp*/);
static void	curline_reprint(/*queue_t *q, int ebsize,
    ldterm_state_t *tp*/);
static void	do_noncanon_input(/*mblk_t *bp, queue_t *q,
    ldterm_state_t *tp*/);
static int	echo_char(/*unsigned char c, queue_t *q, int ebsize,
    ldterm_state_t *tp*/);
static void	output_echo_char(/*unsigned char c, queue_t *q, int bsize,
    ldterm_state_t *tp*/);
static void	output_echo_string(/*unsigned char *cp, int len, queue_t *q,
    int ebsize, ldterm_state_t *tp*/);
static mblk_t	*newmsg(/*ldterm_state_t *tp*/);
static void	msg_upstream(/*queue_t *q, ldterm_state_t *tp*/);
static int	ldterm_wenable(/*long addr*/);
static int	ldtermwmsg(/*queue_t *q, mblk_t *mp*/);
static mblk_t	*do_output(/*queue_t *q, mblk_t *imp, mblk_t **omp,
    ldterm_state_t *tp, int echoing*/);
static void	do_flush_output(/*unsigned char c, queue_t *q,
    ldterm_state_t *tp*/);
static void	do_signal(/*queue_t *q, int sig, unsigned char c, int doecho, 
	int ignore_noflsh*/);
static void	do_ioctl(/*queue_t *q, mblk_t *mp*/);
static void	chgstropts(/*struct termios *oldmodep, ldterm_state_t *tp,
    queue_t *q*/);
static void	ioctl_reply(/*queue_t *q, mblk_t *mp*/);

static struct termios initmodes = {
	BRKINT|ICRNL|IXON|ISTRIP,	/* iflag */
	OPOST|ONLCR|XTABS,		/* oflag */
	0,				/* cflag */
	ISIG|ICANON|ECHO|IEXTEN,	/* lflag */
	0,				/* line */
	{	CINTR,
		CQUIT,
		CERASE,
		CKILL,
		CEOF,
		CEOL,
		CEOL2,
		CSWTCH,
		CSTART,
		CSTOP,
		CSUSP,
		CDSUSP,
		CRPRNT,
		CFLUSH,
		CWERASE,
		CLNEXT,
		0			/* nonexistent STATUS */
	}
};

/*
 * Line discipline open.
 */
/*ARGSUSED*/
static int
ldtermopen(q, dev, oflag, sflag)
	queue_t *q;
	int dev, oflag, sflag;
{
	register ldterm_state_t *tp;
	register mblk_t *bp;
	register int s;
	register struct stroptions *strop;
	int id;

	if (q->q_ptr != NULL)
		return (NEWCTTY);		/* already attached */

	if ((bp = allocb((int)sizeof (ldterm_state_t), BPRI_MED)) == NULL) {
		printf("ldtermopen: open fails, can't alloc state structure\n");
		return (OPENFAIL);
	}
	bp->b_wptr += sizeof (ldterm_state_t);
	tp = (ldterm_state_t *)bp->b_rptr;
	tp->t_savbp = bp;

	tp->t_modes = initmodes;

	tp->t_state = 0;

	tp->t_line = 0;
	tp->t_col = 0;

	tp->t_rocount = 0;
	tp->t_rocol = 0;

	tp->t_message = NULL;
	tp->t_endmsg = NULL;
	tp->t_msglen = 0;

	tp->t_echomp = NULL;

	tp->t_iocid = 0;
	tp->t_wbufcid = 0;

	q->q_ptr = (caddr_t)tp;
	WR(q)->q_ptr = (caddr_t)tp;

	/*
	 * Find out if the module below us does canonicalization; if so,
	 * we won't do it ourselves.
	 */
	while (!putctl1(WR(q)->q_next, M_CTL, MC_CANONQUERY)) {
		s = splstr();
		id = bufcall(1, BPRI_HI, ldtermopen_wakeup, (long)&q->q_ptr);
		if (sleep((caddr_t)&q->q_ptr, STIPRI|PCATCH)) {
			unbufcall(id);
			/* Dump the state structure, then unlink it */
			freeb(tp->t_savbp);
			q->q_ptr = NULL;
			(void) splx(s);
			u.u_error = EINTR;
			return (OPENFAIL);
		}
		(void) splx(s);
	}

	/*
	 * Set the high-water and low-water marks on the stream head to values
	 * appropriate for a terminal.  Also disable vmin and vtime
	 * processing, turn on message-nondiscard mode (as we're in ICANON
	 * mode), and turn on "old-style NODELAY" mode.
	 *
	 * N.B.: Disabling vmin and vtime is a bit tricky.  It turns out that
	 * we really want to send a boolean to the stream head that enables or
	 * disables them.  However, adding such a field to the M_SETOPTS
	 * message would break compatibility with previous releases.
	 * Fortunately, there's an out.  The so_vmin field is declared as
	 * ushort, but the only legal values are u_chars.  Thus we can use
	 * "(ushort) -1" as an out of band value to encode the boolean.  (This
	 * trick would have been cleaner had so_vmin been declared as short.)
	 */
	while ((bp =
	    allocb((int)sizeof (struct stroptions), BPRI_MED)) == NULL) {
		s = splstr();
		id = bufcall(sizeof (struct stroptions), BPRI_MED,
		    ldtermopen_wakeup, (long)&q->q_ptr);
		if (sleep((caddr_t)&q->q_ptr, STIPRI|PCATCH)) {
			unbufcall(id);
			/* Dump the state structure, then unlink it */
			freeb(tp->t_savbp);
			q->q_ptr = NULL;
			(void) splx(s);
			u.u_error = EINTR;
			return (OPENFAIL);
		}
		(void) splx(s);
	}
	strop = (struct stroptions *)bp->b_wptr;
	strop->so_flags =
	    SO_READOPT|SO_HIWAT|SO_LOWAT|SO_VMIN|SO_VTIME|SO_NDELON;
	strop->so_readopt = RMSGN;
	strop->so_hiwat = 300;
	strop->so_lowat = 200;
	strop->so_vmin = (ushort) -1;
	strop->so_vtime = 0;
	bp->b_wptr += sizeof (struct stroptions);
	bp->b_datap->db_type = M_SETOPTS;
	putnext(q, bp);

	return (NEWCTTY);	/* this can become a controlling TTY */
}

static int
ldtermopen_wakeup(addr)
	long addr;
{
	wakeup((caddr_t) addr);
}

/*
 * Close this module instance, coordinating with the driver below to preserve
 * the line discipline's semantics until all output has drained.  If a signal
 * occurs during this interval, give up and complete the close immediately.
 */
/*ARGSUSED*/
static int
ldtermclose(q, flag)
	register queue_t *q;
	int flag;
{
	ldterm_state_t *tp = (ldterm_state_t *)q->q_ptr;
	register mblk_t *bp;
	mblk_t *datap;
	register struct iocblk *iocb;
	register struct stroptions *strop;
	register int s;
	int id;

	/*
	 * Reset the high-water and low-water marks on the stream head (?),
	 * turn on byte-stream mode, and turn off "old-style NODELAY" mode.
	 */
	while ((bp =
	    allocb((int)sizeof (struct stroptions), BPRI_MED)) == NULL) {
		s = splstr();
		id = bufcall(sizeof (struct stroptions), BPRI_MED,
		    ldtermopen_wakeup, (long)&q->q_ptr);
		if (sleep((caddr_t)&q->q_ptr, STIPRI|PCATCH)) {
			unbufcall(id);
			(void) splx(s);
			goto tidy_up;
		}
		(void) splx(s);
	}
	strop = (struct stroptions *)bp->b_wptr;
	strop->so_flags = SO_READOPT|SO_NDELOFF;
	strop->so_readopt = RNORM;
	bp->b_wptr += sizeof (struct stroptions);
	bp->b_datap->db_type = M_SETOPTS;
	putnext(q, bp);

	/*
	 * Wait for output to drain completely from the modules and driver
	 * below us before completing the close.  Doing so guarantees that the
	 * proper tty semantics -- in particular, flow control processing --
	 * still apply for the remaining pending output.
	 *
	 * We do this by sending a TCSBRK ioctl downstream and waiting for the
	 * driver's response.  (When issued with a nonzero argument, this
	 * ioctl does nothing other than wait for output to drain, which is
	 * precisely what we want.)
	 */
	/*
	 * Set up the ioctl.
	 */
	while ((bp = allocb((int)sizeof (struct iocblk), BPRI_MED)) == NULL) {
		s = splstr();
		id = bufcall(sizeof (struct iocblk), BPRI_MED,
		    ldtermopen_wakeup, (long)&q->q_ptr);
		if (sleep((caddr_t)&q->q_ptr, STIPRI|PCATCH)) {
			unbufcall(id);
			(void) splx(s);
			goto tidy_up;
		}
		(void) splx(s);
	}
	while ((datap = allocb((int)sizeof (int), BPRI_MED)) == NULL) {
		s = splstr();
		id = bufcall(sizeof (int), BPRI_MED,
		    ldtermopen_wakeup, (long)&q->q_ptr);
		if (sleep((caddr_t)&q->q_ptr, STIPRI|PCATCH)) {
			unbufcall(id);
			(void) splx(s);
			freeb(bp);
			goto tidy_up;
		}
		(void) splx(s);
	}
	iocb = (struct iocblk *)bp->b_wptr;
	iocb->ioc_cmd = TCSBRK;
	iocb->ioc_uid = 0;
	iocb->ioc_gid = 0;
	iocb->ioc_id = getiocseqno();
	iocb->ioc_count = sizeof (int);
	iocb->ioc_error = 0;
	iocb->ioc_rval = 0;
	bp->b_wptr += (sizeof *iocb)/(sizeof *bp->b_wptr);
	bp->b_datap->db_type = M_IOCTL;
	*(int*)datap->b_wptr = 1;		/* arbitrary nonzero value */
	datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
	bp->b_cont = datap;
	/*
	 * Fire it off.
	 */
	tp->t_state |= TS_IOCWAIT;
	tp->t_iocid = iocb->ioc_id;
	putnext(WR(q), bp);
	/*
	 * Now wait for it.  Let our read queue put routine wake us up
	 * when it arrives.
	 */
	while (tp->t_state & TS_IOCWAIT) {
		if (sleep((caddr_t)&tp->t_iocid, STIPRI|PCATCH))
			break;
	}

tidy_up:
	/*
	 * From here to the end, the routine does not sleep, so it's
	 * guaranteed to run to completion.
	 */

	/*
	 * Restart output; since this module handles ^S and ^Q for most
	 * drivers, rather than those drivers handling it themselves, any
	 * subsequent ^Qs won't be able to restart it.
	 * Do this only if we stopped the output, and do it
	 * only after the output has drained, so we get flow
	 * control properly handled for all output that has gone
	 * before the module was popped.
	 */
	if (tp->t_state & TS_TTSTOP)
		(void) putctl(WR(q)->q_next, M_START);
	/*
	 * If we previously sent a M_STOPI (i.e. tandem mode flow control
	 * output to the other system), restart the other side.
	 */
	if (tp->t_state & TS_TBLOCK)
		(void) putctl(WR(q)->q_next, M_STARTI);

	/*
	 * Commit to the final stage of the close; beyond this point tp is
	 * invalid outside of this routine.  Arrange for messages arriving
	 * from below to be shunted off to our upstream neighbor.
	 */
	q->q_ptr = NULL;

	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (tp->t_wbufcid)
		unbufcall(tp->t_wbufcid);

	/*
	 * Dismantle state by freeing saved messages and then unlinking our
	 * per-instance state structure.
	 */
	if (tp->t_message != NULL)
		freemsg(tp->t_message);
	freeb(tp->t_savbp);
}

/*
 * Put procedure for input from driver end of stream (read queue).
 */
static int
ldtermrput(q, mp)
	queue_t *q;
	mblk_t *mp;
{
	register ldterm_state_t *tp;
	register unsigned char c;
	queue_t *wrq = WR(q);		/* write queue of tty mod */
	queue_t *nextq = q->q_next;	/* queue below us */
	mblk_t *bp;
	register unsigned char *readp;
	register unsigned char *writep;
	struct iocblk *iocp;

	tp = (ldterm_state_t *)q->q_ptr;
	if (tp == NULL) {
		/*
		 * The message has arrived in the window between ldtermclose
		 * committing to dismantle our per-instance state and the
		 * STREAMS framework completing the close.  Since we're
		 * effectively closed, pass it along for the stream head to
		 * worry about.
		 */
		putnext(q, mp);
		return;
	}

	switch (mp->b_datap->db_type) {

	default:
		putq(q, mp);
		return;

	case M_IOCACK:
	case M_IOCNAK:
		/*
		 * If we are doing an "ioctl" ourselves, check if this
		 * is the reply to that code.  If so, wake up the
		 * "close" routine, and toss the reply, otherwise just
		 * pass it up.
		 */
		iocp = (struct iocblk *)mp->b_rptr;
		if (!(tp->t_state & TS_IOCWAIT) ||
		    iocp->ioc_id != tp->t_iocid) {
			/*
			 * This isn't the reply we're looking for.  Pass it
			 * on.
			 */
			putq(q, mp);
		} else {
			tp->t_state &= ~TS_IOCWAIT;
			wakeup((caddr_t)&tp->t_iocid);
			freemsg(mp);
		}
		return;

	case M_BREAK:
		if (!(tp->t_modes.c_iflag & IGNBRK)) {
			if (tp->t_modes.c_iflag & BRKINT) {
				/* 
				 * Needed for POSIX compliance:
				 * Ignore the NOFLSH flag if on.
				 * NOFLSH does not stop break-related flushing.
				 */
				do_signal(q, SIGINT, '\0', 0, 1);
				freemsg(mp);
			} else {
				/*
				 * Convert the message into an M_DATA message
				 * containing a byte sequence matching the
				 * PARMRK setting.
				 *
				 * XXX:	Sleazy code here; we take advantage of
				 *	compatibility code in allocb that
				 *	forces all requests to a minimum of 4
				 *	bytes.
				 */
				mp->b_datap->db_type = M_DATA;
				mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
				if (tp->t_modes.c_iflag & PARMRK) {
					/*
					 * Report the break as the sequence
					 * '\377' '\0' '\0'.
					 */
					*mp->b_wptr++ = '\377';
					*mp->b_wptr++ = '\0';
				} else {
					/*
					 * Report as '\0'.  (Fall through...)
					 */
				}
				*mp->b_wptr++ = '\0';
				break;
			}
		} else
			freemsg(mp);
		return;

	case M_CTL:
		switch (*mp->b_rptr) {

		case MC_NOCANON:
			tp->t_state |= TS_NOCANPUT;
			break;

		case MC_DOCANON:
			tp->t_state &= ~TS_NOCANPUT;
			break;
		}
		putq(q, mp);
		return;

	case M_DATA:
		break;
	}

	/*
	 * Flow control: send "start input" message if blocked and
	 * our queue is below its low water mark.
	 */
	if ((tp->t_modes.c_iflag & IXOFF) && (tp->t_state & TS_TBLOCK) &&
	    q->q_count <= TTXOLO) {
		tp->t_state &= ~TS_TBLOCK;
		(void) putctl(wrq->q_next, M_STARTI);
	}

	/*
	 * If somebody below us ("intelligent" communications board,
	 * pseudo-tty controlled by an editor) is doing
	 * canonicalization, don't scan it for special characters.
	 */
	if (tp->t_state & TS_NOCANPUT) {
		tk_nin += msgdsize(mp);
		putq(q, mp);
		return;
	}

	bp = mp;

	do {
		readp = bp->b_rptr;
		writep = readp;
		tk_nin += bp->b_wptr - readp;
		if (tp->t_modes.c_iflag & (INLCR|IGNCR|ICRNL|IUCLC|IXON) ||
		    tp->t_modes.c_lflag & (ISIG|ICANON)) {
			/*
			 * We're doing some sort of non-trivial processing
			 * of input; look at every character.
			 */
			while (readp < bp->b_wptr) {
				c = *readp++;

				if (tp->t_modes.c_iflag & ISTRIP)
					/*
					 * For POSIX compliance, \377 must
					 * pass thru unmolested
					 * for the case of PARMRK set.
					 */
					if ((!(tp->t_modes.c_iflag &
						PARMRK)) || (c != 0377) || 
						(readp >= bp->b_wptr) ||
						(*readp != 0)) {
						c &= 0177;
					}
				/*
				 * First, check that this hasn't been escaped
				 * with the "literal next" character.
				 */
				if (tp->t_state & TS_PLNCH) {
					tp->t_state &= ~TS_PLNCH;
					tp->t_modes.c_lflag &= ~FLUSHO;
					*writep++ = c;
					continue;
				}

				/*
				 * Setting a special character to VDISABLE
				 * disables  it, so if this character is
				 * VDISABLE, it should  not be compared with
				 * any of the special characters.
				 * It should, however, restart frozen output if
				 * IXON and IXANY are set.
				 */
				if (c == VDISABLE) {
					if (tp->t_modes.c_iflag & IXON &&
					    tp->t_state & TS_TTSTOP &&
					    tp->t_modes.c_iflag & IXANY) {
						tp->t_state  &= ~TS_TTSTOP;
						(void) putctl(wrq->q_next,
						    M_START);
					}
					tp->t_modes.c_lflag &= ~FLUSHO;
					*writep++ = c;
					continue;
				}

				/*
				 * If stopped, start if you can; if running,
				 * stop if you must.
				 */
				if (tp->t_modes.c_iflag & IXON) {
					if (tp->t_state & TS_TTSTOP) {
						if (c == tp->t_modes.c_cc[VSTART] ||
						    tp->t_modes.c_iflag & IXANY) {
							tp->t_state &= ~TS_TTSTOP;
							(void) putctl(wrq->q_next, M_START);
						}
					} else {
						if (c == tp->t_modes.c_cc[VSTOP]) {
							tp->t_state |= TS_TTSTOP;
							(void) putctl(wrq->q_next, M_STOP);
						}
					}
					if (c == tp->t_modes.c_cc[VSTOP] ||
					    c == tp->t_modes.c_cc[VSTART])
						continue;
				}

				/*
				 * Check for "literal next" character and
				 * "flush output" character.  Note that we
				 * omit checks for ISIG and ICANON, since
				 * the IEXTEN setting subsumes them.
				 */
				if (tp->t_modes.c_lflag & IEXTEN) {
					if (c == tp->t_modes.c_cc[VLNEXT]) {
						/*
						 * Remember that we saw a
						 * "literal next" while
						 * scanning input, but leave
						 * it in the message so that
						 * the service routine can see
						 * it too.
						 */
						tp->t_state |= TS_PLNCH;
						tp->t_modes.c_lflag &= ~FLUSHO;
						*writep++ = c;
						continue;
					}
					if (c == tp->t_modes.c_cc[VDISCARD]) {
						do_flush_output(c, wrq, tp);
						continue;
					}
				}

				tp->t_modes.c_lflag &= ~FLUSHO;

				/*
				 * Check for signal-generating characters.
				 */
				if (tp->t_modes.c_lflag & ISIG) {
					if (c == tp->t_modes.c_cc[VINTR]) {
						do_signal(q, SIGINT, c, 1, 0);
						continue;
					}
					if (c == tp->t_modes.c_cc[VQUIT]) {
						do_signal(q, SIGQUIT, c, 1, 0);
						continue;
					}
					if (c == tp->t_modes.c_cc[VSUSP]) {
						do_signal(q, SIGTSTP, c, 1, 0);
						continue;
					}
					if (c == tp->t_modes.c_cc[VSWTCH]) {
						/*
						 * Shouldn't ever see this;
						 * ignore it.
						 */
						continue;
					}
				}

				/*
				 * Throw away CR if IGNCR set, or turn
				 * it into NL if ICRNL set.
				 */
				if (c == '\r') {
					if (tp->t_modes.c_iflag & IGNCR)
						continue;
					if (tp->t_modes.c_iflag & ICRNL)
						c = '\n';
				} else {
					/*
					 * Turn NL into CR if INLCR set.
					 */
					if (c == '\n' &&
					    tp->t_modes.c_iflag & INLCR)
						c = '\r';
				}

				/*
				 * Map upper case input to lower case if
				 * IUCLC flag set.
				 */
				if ((tp->t_modes.c_iflag & IUCLC) &&
				    c >= 'A' && c <= 'Z')
					c += 'a' - 'A';

				/*
				 * Put the possibly-transformed character
				 * back in the message.
				 */
				*writep++ = c;
			}

			/*
			 * If we didn't copy some characters because
			 * we were ignoring them, fix the size of the
			 * data block by adjusting the write pointer.
			 * XXX This may result in a zero-length block;
			 * will this cause anybody gastric distress?
			 */
			bp->b_wptr -= (readp - writep);
		} else {
			/*
			 * We won't be doing anything other than possibly
			 * stripping the input.
			 */
			if (tp->t_modes.c_iflag & ISTRIP) {
				while (readp < bp->b_wptr) {
					/*
					 * For POSIX compliance, \377 must
					 * pass thru unmolested
					 * for the case of PARMRK set.
					 */
					if (!((!(tp->t_modes.c_iflag &
						PARMRK)) || (*readp != 0377) || 
						(readp >= bp->b_wptr) ||
						(*(readp + 1) != 0)))
						*writep++ = *readp++;
					else
						*writep++ = *readp++ & 0177;
			
				}
			}
			tp->t_modes.c_lflag &= ~FLUSHO;
		}

		/*
		 * Flow control: send "stop input" message if our queue is
		 * approaching its high-water mark.
		 *
		 * Set QWANTW to ensure that the read queue service
		 * procedure gets run when nextq empties up again, so that
		 * it can unstop the input.
		 */
		if ((tp->t_modes.c_iflag & IXOFF) &&
		    !(tp->t_state & TS_TBLOCK) &&
		    q->q_count >= TTXOHI) {
			nextq->q_flag |= QWANTW;
			tp->t_state |= TS_TBLOCK;
			(void) putctl(wrq->q_next, M_STOPI);
		}
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	/*
	 * Queue the message for service procedure.
	 */
	putq(q, mp);
}

/*
 * Line discipline input server processing.  Erase/kill and escape ('\')
 * processing, gathering into messages, upper/lower case input mapping.
 */
static int
ldtermrsrv(q)
	register queue_t *q;
{
	register ldterm_state_t *tp;
	mblk_t *mp;
	register mblk_t *bp;
	register mblk_t *bpt;
	register unsigned char c;
	int ebsize;

	tp = (ldterm_state_t *)q->q_ptr;

	if (tp->t_state & TS_RESCAN) {
		/*
		 * Canonicalization was turned on or off.
		 * Put the message being assembled back in the input queue,
		 * so that we rescan it.
		 */
		if (tp->t_message != NULL) {
			putbq(q, tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
		}
		tp->t_state &= ~TS_RESCAN;
	}

	bpt = NULL;

	while ((mp = getq(q)) != NULL) {
		if (mp->b_datap->db_type <= QPCTL && !canput(q->q_next)) {
			putbq(q, mp);
			goto out;	/* read side is blocked */
		}
		switch (mp->b_datap->db_type) {

		default:
			putnext(q, mp);	/* pass it on */
			continue;

		case M_FLUSH:
			/*
			 * Flush everything we haven't looked at yet.
			 */
			flushq(q, FLUSHDATA);

			/*
			 * Flush everything we have looked at.
			 */
			freemsg(tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
			tp->t_rocount = 0;	/* if it hasn't been typed, */
			tp->t_rocol = 0;	/* it hasn't been echoed :-) */
			putnext(q, mp);		/* pass it on */
			continue;

		case M_HANGUP:
			/*
			 * Flush everything we haven't looked at yet.
			 */
			flushq(q, FLUSHDATA);

			/*
			 * Flush everything we have looked at.
			 */
			freemsg(tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
			tp->t_rocount = 0;	/* if it hasn't been typed, */
			tp->t_rocol = 0;	/* it hasn't been echoed :-) */

			/*
			 * Restart output, since it's probably got nowhere to
			 * go anyway, and we're probably not going to see
			 * another ^Q for a while.
			 */
			tp->t_state &= ~TS_TTSTOP;
			(void) putctl(WR(q)->q_next, M_START);

			/*
			 * This guy will travel up the read queue, flushing
			 * as it goes, get turned around at the stream head,
			 * and travel back down the write queue, flushing as
			 * it goes.
			 */
			(void) putctl1(q->q_next, M_FLUSH, FLUSHRW);

			/*
			 * This guy will travel down the write queue, flushing
			 * as it goes, get turned around at the driver,
			 * and travel back up the read queue, flushing as
			 * it goes.
			 */
			(void) putctl1(WR(q), M_FLUSH, FLUSHRW);

			/*
			 * Now that that's done, we send a SIGCONT upstream,
			 * followed by the M_HANGUP.
			 */
			(void) putctl1(q->q_next, M_PCSIG, SIGCONT);
			(void) putnext(q, mp);
			continue;

		case M_IOCACK:
			/*
			 * Augment whatever information the driver is
			 * returning  with the information we supply.
			 */
			ioctl_reply(q, mp);
			continue;

		case M_CTL:
			switch (*mp->b_rptr) {

			case MC_NOCANON:
				/*
				 * Note: this is very nasty.  It's not
				 * clear what the right thing to do
				 * with a partial message is; we throw
				 * it out.
				 */
				if (tp->t_message != NULL) {
					freemsg(tp->t_message);
					tp->t_message = NULL;
					tp->t_endmsg = NULL;
					tp->t_msglen = 0;
					tp->t_rocount = 0;
					tp->t_rocol = 0;
				}
				tp->t_state |= TS_NOCANSRV;
				break;

			case MC_DOCANON:
				tp->t_state &= ~TS_NOCANSRV;
				break;
			}
			putnext(q, mp);
			continue;

		case M_DATA:
			break;
		}

		/*
		 * This is an M_DATA message.
		 */

		/*
		 * If somebody below us ("intelligent" communications board,
		 * pseudo-tty controlled by an editor) is doing
		 * canonicalization, don't scan it for special characters.
		 */
		if (tp->t_state & TS_NOCANSRV) {
			putnext(q, mp);
			continue;
		}

		if (tp->t_modes.c_lflag & ICANON) {
			bp = mp;

			ebsize = bp->b_wptr - bp->b_rptr;
			if (ebsize > EBSIZE)
				ebsize = EBSIZE;

			if ((bpt = newmsg(tp)) != NULL) {
				mblk_t *bcont;

				do {
					bcont = bp->b_cont;
					while (bp->b_rptr < bp->b_wptr) {
						c = *bp->b_rptr++;
						if ((bpt =
						    do_canon_input(c, bpt,
						    ebsize, q, tp)) == NULL)
							break;
					}
					/*
					 * Release this block.
					 */
					freeb(bp);
					if (bpt == NULL) {
						printf("ldtermrsrv: out of blocks\n");
						freemsg(bcont);
						break;
					}
				} while ((bp = bcont) != NULL);
			}
		} else
			do_noncanon_input(mp, q, tp);

		/*
		 * Send whatever we echoed downstream.
		 */
		if (tp->t_echomp != NULL) {
			putnext(WR(q), tp->t_echomp);
			tp->t_echomp = NULL;
		}
	}

out:
	/*
	 * Flow control: send start message if blocked and
	 * our queue is below its low water mark.
	 */
	if ((tp->t_modes.c_iflag & IXOFF) && (tp->t_state & TS_TBLOCK) &&
		q->q_count <= TTXOLO) {
		tp->t_state &= ~TS_TBLOCK;
		(void) putctl(WR(q), M_STARTI);
	}
}

/*
 * Do canonical mode input; check whether this character is to be treated as a
 * special character - if so, check whether it's equal to any of the special
 * characters and handle it accordingly.  Otherwise, just add it to the current
 * line.
 */
static mblk_t *
do_canon_input(c, bpt, ebsize, q, tp)
	register unsigned char c;
	register mblk_t *bpt;
	int ebsize;
	queue_t *q;
	register ldterm_state_t *tp;
{
	register queue_t *wrq = WR(q);
	int i;

	/*
	 * If the previous character was the "literal next" character, treat
	 * this character as regular input.
	 */
	if (tp->t_state & TS_SLNCH)
		goto escaped;

	/*
	 * Setting a special character to VDISABLE disables it, so if this
	 * character is VDISABLE, it should not be compared with any of the
	 * special characters.
	 */
	if (c == VDISABLE) {
		tp->t_state &= ~TS_QUOT;
		goto escaped;
	}

	/*
	 * If this character is the literal next character, echo it as '^',
	 * backspace over it, and record that fact.
	 */
	if ((tp->t_modes.c_lflag & IEXTEN) && c == tp->t_modes.c_cc[VLNEXT]) {
		if (tp->t_modes.c_lflag & ECHO)
			output_echo_string((unsigned char *)"^\b", 2, wrq,
			    ebsize, tp);
		tp->t_state &= ~TS_QUOT;	/* cannot be escaped with \ */
		tp->t_state |= TS_SLNCH;
		goto out;
	}

	/*
	 * Check for the editing characters.
	 */
	if (c == tp->t_modes.c_cc[VERASE]) {
		if (tp->t_state & TS_QUOT) {
			/*
			 * Get rid of the backslash, and put the erase
			 * character in its place.
			 */
			curline_erase(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
			goto escaped;
		} else {
			/*
			 * Erase the most-recently-typed character.
			 */
			curline_erase(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
			goto out;
		}
	}

	if ((tp->t_modes.c_lflag & IEXTEN) && c == tp->t_modes.c_cc[VWERASE]) {
		tp->t_state &= ~TS_QUOT;	/* cannot be escaped with \ */
		curline_werase(wrq, ebsize, tp);
		bpt = tp->t_endmsg;
		goto out;
	}

	if (c == tp->t_modes.c_cc[VKILL]) {
		if (tp->t_state & TS_QUOT) {
			/*
			 * Get rid of the backslash, and put the kill character
			 * in its place.
			 */
			curline_erase(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
			goto escaped;
		} else {
			/*
			 * Erase the entire line.
			 */
			curline_kill(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
			goto out;
		}
	}

	if ((tp->t_modes.c_lflag & IEXTEN) && c == tp->t_modes.c_cc[VREPRINT]) {
		curline_reprint(wrq, ebsize, tp);
		goto out;
	}

	/*
	 * If the preceding character was a backslash:
	 *	if the current character is an EOF, get rid of the backslash
	 *	and treat the EOF as data;
	 *	if we're in XCASE mode and the current character is part
	 *	of a backslash-X escape sequence, process it;
	 *	otherwise, just treat the current character normally.
	 */
	if (tp->t_state & TS_QUOT) {
		tp->t_state &= ~TS_QUOT;
		if (c == tp->t_modes.c_cc[VEOF]) {
			/*
			 * EOF character.
			 * Since it's escaped, get rid of the backslash and put
			 * the EOF character in its place.
			 */
			curline_erase(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
		} else {
			/*
			 * If we're in XCASE mode, and the current character
			 * is part of a backslash-X sequence, get rid of the
			 * backslash and replace the current character with
			 * what that sequence maps to.
			 */
			if ((tp->t_modes.c_lflag & XCASE) &&
				imaptab[c] != '\0') {
				curline_erase(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
				c = imaptab[c];
			}
		}
	} else {
		/*
		 * Previous character wasn't backslash; check whether this
		 * was the EOF character.
		 */
		if (c == tp->t_modes.c_cc[VEOF]) {
			/*
			 * EOF character.
			 * Don't echo it unless ECHOCTL is set, don't stuff it
			 * in the current line, but send the line up the
			 * stream.
			 */
			if ((tp->t_modes.c_lflag & ECHOCTL) &&
				(tp->t_modes.c_lflag & ECHO)) {
				i = echo_char(c, wrq, ebsize, tp);
				while (i > 0) {
					output_echo_char('\b', wrq, ebsize,
					    tp);
					i--;
				}
			}
			bpt->b_datap->db_type = M_DATA;
			msg_upstream(q, tp);
			bpt = newmsg(tp);
			goto out;
		}
	}

escaped:
	if (tp->t_msglen == TTYHOG) {
		/*
		 * Character will cause line to overflow.
		 * Ring the bell or discard all input, and don't save the
		 * character away.
		 */
		if (tp->t_modes.c_iflag & IMAXBEL)
			output_echo_char(_CTRL(g), wrq, ebsize, tp);
		else
			(void) putctl1(q, M_FLUSH, FLUSHR);
		tp->t_state &= ~TS_SLNCH;
		goto out;
	}

	/*
	 * Add the character to the current line.
	 */
	if (bpt->b_wptr >= bpt->b_datap->db_lim) {
		/*
		 * No more room in this mblk; save this one away, and
		 * allocate a new one.
		 */
		bpt->b_datap->db_type = M_DATA;
		if ((bpt = allocb(IBSIZE, BPRI_MED)) == NULL)
			goto out;

		/*
		 * Chain the new one to the end of the old one, and
		 * mark it as the last block in the current line.
		 */
		tp->t_endmsg->b_cont = bpt;
		tp->t_endmsg = bpt;
	}
	*bpt->b_wptr++ = c;
	tp->t_msglen++;

	if (!(tp->t_state & TS_SLNCH) &&
	    (c == '\n' || (c != VDISABLE && (c == tp->t_modes.c_cc[VEOL] ||
		((tp->t_modes.c_lflag & IEXTEN) &&
		c == tp->t_modes.c_cc[VEOL2]))))) {
		/*
		 * It's a line-termination character; send the line
		 * up the stream.
		 */
		bpt->b_datap->db_type = M_DATA;
		msg_upstream(q, tp);
		if ((bpt = newmsg(tp)) == NULL)
			goto out;
	} else {
		/*
		 * Character was escaped with LNEXT.
		 */
		if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_col;
		tp->t_state &= ~(TS_SLNCH|TS_QUOT);
		if (c == '\\')
			tp->t_state |= TS_QUOT;
	}

	/*
	 * Echo it.
	 */
	if (tp->t_state & TS_ERASE) {
		tp->t_state &= ~TS_ERASE;
		if (tp->t_modes.c_lflag & ECHO)
			output_echo_char('/', wrq, ebsize, tp);
	}

	if (tp->t_modes.c_lflag & ECHO)
		(void) echo_char(c, wrq, ebsize, tp);
	else {
		/*
		 * Echo NL when ECHO turned off, if ECHONL flag is set.
		 */
		if (c == '\n' && (tp->t_modes.c_lflag & ECHONL))
			output_echo_char(c, wrq, ebsize, tp);
	}

out:
	return (bpt);
}

/*
 * Get the character at the end of the current line, and remove it from the
 * current line.  Return -1 if the current line is empty.
 */
static int
curline_unget(tp)
	register ldterm_state_t *tp;
{
	register mblk_t *bpt;

	if ((bpt = tp->t_endmsg) == NULL)
		return (-1);	/* no buffers */
	if (bpt->b_rptr == bpt->b_wptr)
		return (-1);	/* zero-length record */
	tp->t_msglen--;	/* one fewer character */
	return (*--bpt->b_wptr);
}

/*
 * Trim any zero-length mblks from the end of the current line (they may have
 * been left there by erasing characters from the line).
 */
static void
curline_trim(tp)
	register ldterm_state_t *tp;
{
	register mblk_t *bpt;
	register mblk_t *bp;

	if ((bpt = tp->t_endmsg) == NULL)
		panic("curline_trim called with no message");
	if (bpt->b_rptr == bpt->b_wptr) {
		/*
		 * This mblk is now empty.
		 * Find the previous mblk; throw this one away, unless
		 * it's the first one.
		 */
		bp = tp->t_message;
		if (bp != bpt) {
			while (bp->b_cont != bpt) {
				if ((bp = bp->b_cont) == NULL)
					panic("curline_trim: current input line mislinked");
			}
			bp->b_cont = NULL;
			freeb(bpt);
			tp->t_endmsg = bp;	/* point to that mblk */
		}
	}
}


/*
 * Rubout one character from the current line being built for tp
 * as cleanly as possible.  q is the write queue for tp.
 */
static void
curline_rubout(c, q, ebsize, tp)
	register unsigned char c;
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register int tabcols;
	static unsigned char crtrubout[] = "\b \b\b \b";
#define	RUBOUT1	&crtrubout[3]	/* rub out one position */
#define	RUBOUT2	&crtrubout[0]	/* rub out two positions */

	if (!(tp->t_modes.c_lflag & ECHO))
		return;
	if (tp->t_modes.c_lflag & ECHOE) {
		/*
		 * "CRT rubout"; try erasing it from the screen.
		 */
		if (tp->t_rocount == 0) {
			/*
			 * After the character being erased was echoed,
			 * some data was written to the terminal; we
			 * can't erase it cleanly, so we just reprint the
			 * whole line as if the user had typed the
			 * reprint character.
			 */
			curline_reprint(q, ebsize, tp);
			return;
		} else {
			/*
			 * XXX what about escaped characters?
			 */
			switch (typetab[c]) {

			case PRINTABLE:
				if ((tp->t_modes.c_lflag & XCASE) &&
					omaptab[c])
					output_echo_string(RUBOUT1, 3, q,
					    ebsize, tp);
				output_echo_string(RUBOUT1, 3, q, ebsize, tp);
				break;

			case VTAB:
			case BACKSPACE:
			case CONTROL:
			case RETURN:
			case NEWLINE:
				if (tp->t_modes.c_lflag & ECHOCTL)
					output_echo_string(RUBOUT2, 6, q,
					    ebsize, tp);
				break;

			case TAB:
				if (tp->t_rocount < tp->t_msglen) {
					/*
					 * While the tab being erased was
					 * expanded, some data was written
					 * to the terminal; we can't erase it
					 * cleanly, so we just reprint the
					 * whole line as if the user had typed
					 * the reprint character.
					 */
					curline_reprint(q, ebsize, tp);
					return;
				}
				tabcols = curline_tabcols(tp);
				while (--tabcols >= 0)
					output_echo_char('\b', q, ebsize, tp);
				break;
			}
		}
	} else if (tp->t_modes.c_lflag & ECHOPRT) {
		/*
		 * "Printing rubout"; echo it between \ and /.
		 */
		if (!(tp->t_state & TS_ERASE)) {
			output_echo_char('\\', q, ebsize, tp);
			tp->t_state |= TS_ERASE;
		}
		(void) echo_char(c, q, ebsize, tp);
	} else
		(void) echo_char(tp->t_modes.c_cc[VERASE], q, ebsize, tp);
	tp->t_rocount--;	/* we "unechoed" this character */
}

/*
 * Find the number of characters the tab we just deleted took up by
 * zipping through the current line and recomputing the column number.
 */
static int
curline_tabcols(tp)
	register ldterm_state_t *tp;
{
	register int col;
	register mblk_t *bp;
	register unsigned char *readp;
	register unsigned char c;

	col = tp->t_rocol;
	bp = tp->t_message;
	do {
		readp = bp->b_rptr;
		while (readp < bp->b_wptr) {
			c = *readp++;
			if (tp->t_modes.c_lflag & ECHOCTL) {
				if (c <= 037 && c != '\t' && c != '\n' ||
					c == 0177) {
					/*
					 * One column for '^' and one for
					 * (c + ' ').
					 */
					col += 2;
					continue;
				}
			}

			/*
			 * Column position calculated here.
			 */
			switch (typetab[c]) {

			/* Ordinary characters; advance by one. */
			case PRINTABLE:
				col++;
				break;

			/* Non-printing characters; nothing happens. */
			case CONTROL:
				break;

			/* Backspace */
			case BACKSPACE:
				if (col != 0)
					col--;
				break;

			/* Newline; column depends on flags. */
			case NEWLINE:
				if (tp->t_modes.c_oflag & ONLRET)
					col = 0;
				break;

			/* tab */
			case TAB:
				col |= 07;
				col++;
				break;

			/* vertical motion */
			case VTAB:
				break;

			/* carriage return */
			case RETURN:
				col = 0;
				break;
			}
		}
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	/*
	 * "col" is now the column number before the tab.
	 * "tp->t_col" is still the column number after the tab,
	 * since we haven't erased the tab yet.
	 * Thus "tp->t_col - col" is the number of positions the tab
	 * moved.
	 */
	col = tp->t_col - col;
	if (col > 8)
		col = 8;		/* overflow screw */
	return (col);
}

/*
 * Erase a single character from the current line; remove the character and
 * properly echo the erase character.
 */
static void
curline_erase(q, ebsize, tp)
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register int c;

	if ((c = curline_unget(tp)) != -1) {
		curline_rubout((unsigned char)c, q, ebsize, tp);
		curline_trim(tp);
	}
}

/*
 * Erase an entire word from the current line; remove the characters and, for
 * each one, echo an erase character.
 */
static void
curline_werase(q, ebsize, tp)
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register int c;

	/*
	 * Erase trailing white space, if any.
	 */
	while ((c = curline_unget(tp)) == ' ' || c == '\t') {
		curline_rubout((unsigned char)c, q, ebsize, tp);
		curline_trim(tp);
	}

	/*
	 * Erase non-white-space characters, if any.
	 */
	while (c != -1 && c != ' ' && c != '\t') {
		curline_rubout((unsigned char)c, q, ebsize, tp);
		curline_trim(tp);
		c = curline_unget(tp);
	}
	if (c != -1) {
		/*
		 * We removed one too many characters; put the last one
		 * back.
		 */
		tp->t_endmsg->b_wptr++;	/* put 'c' back */
		tp->t_msglen++;
	}
}

/*
 * Kill the entire current line; remove the characters and, if ECHOKE is set,
 * echo an erase character for each one, otherwise echo the kill character and,
 * if ECHOK is set, a newline.
 */
static void
curline_kill(q, ebsize, tp)
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register int c;

	if ((tp->t_modes.c_lflag & ECHOKE) && tp->t_msglen == tp->t_rocount) {
		while ((c = curline_unget(tp)) != -1) {
			curline_rubout((unsigned char)c, q, ebsize, tp);
			curline_trim(tp);
		}
	} else {
		(void) echo_char(tp->t_modes.c_cc[VKILL], q, ebsize, tp);
		if (tp->t_modes.c_lflag & ECHOK)
			(void) echo_char('\n', q, ebsize, tp);
		while (curline_unget(tp) != -1)
			curline_trim(tp);
		tp->t_rocount = 0;
	}
	tp->t_state &= ~(TS_QUOT|TS_ERASE|TS_SLNCH);
}

/*
 * Reprint the current input line.  First, echo the reprint character, if it
 * hasn't been disabled.
 * XXX just the current line, not the whole queue?
 * What about DEFECHO mode?
 */
static void
curline_reprint(q, ebsize, tp)
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register mblk_t *bp;
	register unsigned char *readp;

	if (tp->t_modes.c_cc[VREPRINT] != (unsigned char)0)
		(void) echo_char(tp->t_modes.c_cc[VREPRINT], q, ebsize, tp);
	output_echo_char('\n', q, ebsize, tp);

	bp = tp->t_message;
	do {
		readp = bp->b_rptr;
		while (readp < bp->b_wptr)
			(void) echo_char(*readp++, q, ebsize, tp);
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	tp->t_state &= ~TS_ERASE;
	tp->t_rocount = tp->t_msglen;	/* we reechoed the entire line */
	tp->t_rocol = 0;
}

/*
 * Do non-canonical mode input.
 */
static void
do_noncanon_input(mp, q, tp)
	register mblk_t *mp;
	queue_t *q;
	register ldterm_state_t *tp;
{
	queue_t *wrq = WR(q);
	int ebsize;
	register mblk_t *bp, *prevbp;
	mblk_t *savebp;
	register unsigned char *rptr;

	for (bp = mp, prevbp = NULL; bp != NULL;
	    prevbp = bp, bp = bp->b_cont) {
		while (bp->b_rptr == bp->b_wptr) {
			/*
			 * Zero-length block.  Throw it away.
			 */
			if (prevbp == NULL)
				mp = bp->b_cont;
			else
				prevbp->b_cont = bp->b_cont;
			savebp = bp;
			bp = bp->b_cont;
			savebp->b_cont = NULL;
			freeb(savebp);
			if (bp == NULL)
				return;	/* entire message gone */
		}
		if (tp->t_modes.c_lflag & (ECHO|ECHONL)) {
			/*
			 * Echo the data in this message.
			 */
			if (tp->t_modes.c_lflag & ECHO) {
				ebsize = bp->b_wptr - bp->b_rptr;
				if (ebsize > EBSIZE)
					ebsize = EBSIZE;
				rptr = bp->b_rptr;
				while (rptr < bp->b_wptr)
					(void) echo_char(*rptr++, wrq,
					    ebsize, tp);
			} else {
				/*
				 * Echo NL, even though ECHO is not
				 * set.
				 */
				rptr = bp->b_rptr;
				while (rptr < bp->b_wptr) {
					if (*rptr++ == '\n')
						output_echo_char('\n',
						    wrq, 1, tp);
				}
			}
		}
	}
	putnext(q, mp);

	/*
	 * Send whatever we echoed downstream.
	 */
	if (tp->t_echomp != NULL) {
		putnext(wrq, tp->t_echomp);
		tp->t_echomp = NULL;
	}
}

/*
 * Echo a typed character to the terminal.
 * Returns the number of characters printed.
 */
static int
echo_char(c, q, ebsize, tp)
	register unsigned char c;
	register queue_t *q;
	int ebsize;
	register ldterm_state_t *tp;
{
	register int i;

	if (!(tp->t_modes.c_lflag & ECHO))
		return (0);
	i = 0;
	if (tp->t_modes.c_lflag & ECHOCTL) {
		if (c <= 037 && c != '\t' && c != '\n') {
			output_echo_char('^', q, ebsize, tp);
			i++;
			if (tp->t_modes.c_oflag & OLCUC)
				c += 'a' - 1;
			else
				c += 'A' - 1;
		} else if (c == 0177) {
			output_echo_char('^', q, ebsize, tp);
			i++;
			c = '?';
		}
	}
	output_echo_char(c, q, ebsize, tp);
	return (i + 1);
}

/*
 * Put a character generated as part of an echo operation on the output queue.
 */
static void
output_echo_char(c, q, bsize, tp)
	register unsigned char c;
	register queue_t *q;
	int bsize;
	register ldterm_state_t *tp;
{
	register mblk_t *curbp;

	/*
	 * Don't even look at the characters unless we
	 * have something useful to do with them.
	 */
	if ((tp->t_modes.c_oflag & OPOST) ||
		((tp->t_modes.c_lflag & XCASE) &&
		(tp->t_modes.c_lflag & ICANON))) {
		register mblk_t *mp;

		if ((mp = allocb(4, BPRI_HI)) == NULL) {
			printf("output_echo_char: out of blocks\n");
			return;
		}
		*mp->b_wptr++ = c;
		mp = do_output(q, mp, &tp->t_echomp, tp, bsize, 1);
		if (mp != NULL)
			freemsg(mp);
	} else {
		/*
		 * No fancy processing required; just glue the character onto
		 * the end of the to-be-echoed message, allocating or
		 * extending it if necessary.
		 */
		if ((curbp = tp->t_echomp) != NULL) {
			while (curbp->b_cont != NULL)
				curbp = curbp->b_cont;
			if (curbp->b_datap->db_lim == curbp->b_wptr) {
				register mblk_t *newbp;

				if ((newbp = allocb(bsize, BPRI_HI)) == NULL) {
					printf("output_echo_char: out of blocks\n");
					return;
				}
				curbp->b_cont = newbp;
				curbp = newbp;
			}
		} else {
			if ((curbp = allocb(bsize, BPRI_HI)) == NULL) {
				printf("output_echo_char: out of blocks\n");
				return;
			}
			tp->t_echomp = curbp;
		}
		*curbp->b_wptr++ = c;
	}
}

/*
 * Copy a string, of length len, of characters generated as part of an echo
 * operation to the output queue.
 */
static void
output_echo_string(cp, len, q, bsize, tp)
	register unsigned char *cp;
	register int len;
	register queue_t *q;
	int bsize;
	register ldterm_state_t *tp;
{

	while (len > 0) {
		output_echo_char(*cp++, q, bsize, tp);
		len--;
	}
}

static mblk_t *
newmsg(tp)
	register ldterm_state_t *tp;
{
	register mblk_t *bp;

	/*
	 * If no current message, allocate a block
	 * for it.
	 */
	if ((bp = tp->t_endmsg) == NULL) {
		if ((bp = allocb(IBSIZE, BPRI_MED)) == NULL) {
			printf("newmsg: out of blocks\n");
			return (bp);
		}
		tp->t_message = bp;
		tp->t_endmsg = bp;
	}
	return (bp);
}

static void
msg_upstream(q, tp)
	register queue_t *q;
	register ldterm_state_t *tp;
{
	putnext(q, tp->t_message);
	tp->t_message = NULL;
	tp->t_endmsg = NULL;
	tp->t_msglen = 0;
	tp->t_rocount = 0;
}

/*
 * Re-enable the write-side service procedure.  When an allocation failure
 * causes write-side processing to stall, we disable the write side and
 * arrange to call this function when allocation once again becomes possible.
 */
static int
ldterm_wenable(addr)
	long	addr;
{
	register queue_t *q = (queue_t *)addr;
	register ldterm_state_t *tp;

	if ((tp = (ldterm_state_t *)q->q_ptr) == NULL)
		return;
	/*
	 * The bufcall is no longer pending.
	 */
	tp->t_wbufcid = 0;
	enableok(q);
	qenable(q);
}

/*
 * Line discipline output queue put procedure.  Attempts to process the
 * message directly and send it on downstream, queueing it only if there's
 * already something pending or if its downstream neighbor is clogged.
 */
static int
ldtermwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	unsigned char type = mp->b_datap->db_type;

	/*
	 * Always process priority messages, regardless of whether or not our
	 * queue is nonempty.
	 */
	if (type >= QPCTL) {
		switch (type) {

		case M_FLUSH:
			/*
			 * This is coming from above, so we only handle the
			 * write queue here.  If FLUSHR is set, it will get
			 * turned around at the driver, and the read procedure
			 * will see it eventually.
			 */
			if (*mp->b_rptr & FLUSHW)
				flushq(q, FLUSHDATA);
			putnext(q, mp);
			break;

		default:
			/* Pass it through unmolested. */
			putnext(q, mp);
			break;
		}
		return;
	}

	/*
	 * If our queue is nonempty or there's a traffic jam downstream, this
	 * message must get in line.
	 */
	if (q->q_first != NULL || !canput(q->q_next)) {
		/*
		 * Exception: ioctls, except for those defined to take effect
		 * after output has drained, should be processed immediately.
		 */
		if (type == M_IOCTL) {
			register struct iocblk *iocp;

			iocp = (struct iocblk *)mp->b_rptr;
			switch (iocp->ioc_cmd) {

			/*
			 * Queue these.
			 */
			case TCSETSW:
			case TCSETSF:
			case TCSETAW:
			case TCSETAF:
			case TCSBRK:
				break;

			/*
			 * Handle all others immediately.
			 */
			default:
				ldterm_wputmsgs++;
				(void) ldtermwmsg(q, mp);
				return;
			}
		}
		putq(q, mp);
		return;
	}

	/*
	 * We can take the fast path through, by simply calling ldtermwmsg to
	 * dispose of mp.
	 */
	ldterm_wputmsgs++;
	(void) ldtermwmsg(q, mp);
}

/*
 * Line discipline output queue service procedure.
 */
static int
ldtermwsrv(q)
	register queue_t *q;
{
	register mblk_t *mp;

	/*
	 * We expect this loop to iterate at most once, but must be prepared
	 * for more in case our upstream neighbor isn't paying strict
	 * attention to what canput tells it.
	 */
	while ((mp = getq(q)) != NULL) {
		ldterm_wsrvmsgs++;
		/*
		 * N.B.: ldtermwput has already handled high-priority
		 * messages, so we don't have to worry about them here.
		 * Hence, the putbq call is safe.
		 */
		if (!canput(q->q_next)) {
			putbq(q, mp);
			break;
		}
		if (!ldtermwmsg(q, mp)) {
			/*
			 * Couldn't handle the whole thing; give up for now
			 * and wait to be rescheduled.
			 */
			break;
		}
	}
}

/*
 * Process the write-side message denoted by mp.  If mp can't be processed
 * completely (due to allocation failures), put the residual unprocessed part
 * on the front of the write queue, disable the queue, and schedule a bufcall
 * to arrange to complete its processing later.
 *
 * Return 1 if the message was processed completely and 0 if not.
 *
 * This routine is called from both ldtermwput and ldtermwsrv to do the actual
 * work of dealing with mp.  ldtermwput will have already dealt with high
 * priority messages.
 */
static int
ldtermwmsg(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register ldterm_state_t *tp;
	register mblk_t *residmp = NULL;
	register u_int size;

	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		do_ioctl(q, mp);
		break;

	case M_DATA: {
		mblk_t *omp = NULL;

		tp = (ldterm_state_t *)q->q_ptr;
		tp->t_rocount = 0;

		/*
		 * Don't even look at the characters unless we
		 * have something useful to do with them.
		 */
		if ((tp->t_modes.c_oflag & OPOST) ||
		    ((tp->t_modes.c_lflag & XCASE) &&
		    (tp->t_modes.c_lflag & ICANON))) {
			residmp = do_output(q, mp,  &omp, tp, OBSIZE, 0);
			if ((mp = omp) == NULL)
				break;
		}
		if (!(tp->t_modes.c_lflag & FLUSHO)) {
			tk_nout += msgdsize(mp);
			putnext(q, mp);
		} else
			freemsg(mp);	/* drop on floor */
		break;
	    }

	default:
		putnext(q, mp);		/* pass it through unmolested */
		break;
	}

	if (residmp == NULL)
		return (1);

	/*
	 * An allocation failure occurred that prevented the message from
	 * being completely processed.  First, disable our queue, since it's
	 * pointless to attempt further processing until the allocation
	 * situation is resolved.  (This must precede the putbq call below,
	 * which would otherwise mark the queue to be serviced.)
	 */
	noenable(q);
	/*
	 * Stuff the remnant on our write queue so that we can complete it
	 * later when times become less lean.  Note that this sets QFULL, so
	 * that our upstream neighbor will be blocked by flow control.
	 */
	putbq(q, residmp);
	/*
	 * Schedule a bufcall to re-enable the queue.  The failure won't have
	 * been for an allocation of more than OBSIZE bytes, so don't ask for
	 * more than that from bufcall.
	 */
	size = msgdsize(residmp);
	if (size > OBSIZE)
		size = OBSIZE;
	if (tp->t_wbufcid)
		unbufcall(tp->t_wbufcid);
	tp->t_wbufcid = bufcall(size, BPRI_MED, ldterm_wenable, (long)q);

	return (0);
}

/*
 * Perform output processing on the message denoted by imp, adding the output
 * characters to the message denoted by *omp (allocating it if necessary).
 * Return the unprocessed residual input, freeing those blocks of the input
 * that were completely processed.
 *
 * Normally, the return value will be NULL.  If an allocation failure prevents
 * the routine from having enough space to continue accumulating output, it
 * stops where it is and hands back what it's done, leaving the caller to
 * decide whether to free the remaining input or to arrange for a bufcall to
 * try again later.
 */
static mblk_t *
do_output(q, imp, omp, tp, bsize, echoing)
	register queue_t *q;
	mblk_t *imp;		/* head of input message we're examining */
	mblk_t **omp;		/* addr of head of output we're constructing */
	register ldterm_state_t *tp;
	int bsize;
	int echoing;
{
	register mblk_t *ibp;	/* block we're examining from input message */
	register mblk_t *obp;	/* block we're filling in output message */
	mblk_t *oobp;		/* old value of obp; valid if NEW_BLOCK fails */
	mblk_t **contpp;	/* where to stuff ptr to newly-allocated blk */
	register unsigned char c;
	register int count, ctype;
	register int chars_left;

	/*
	 * Allocate a new block into which to put characters.  If we can't,
	 * set oobp to the previous value of obp and indicate failure.
	 */
#define	NEW_BLOCK() \
	(oobp = obp, !(obp = allocb(bsize, BPRI_MED)) ? 0 : \
	    (*contpp = obp, contpp = &obp->b_cont, \
		chars_left = obp->b_datap->db_lim - obp->b_wptr, 1))

	ibp = imp;

	/*
	 * When we allocate the first block of a message, we should stuff
	 * the pointer to it in "*omp".  All subsequent blocks should
	 * have the pointer to them stuffed into the "b_cont" field of the
	 * previous block.  "contpp" points to the place where we should
	 * stuff the pointer.
	 *
	 * If we already have a message we're filling in, continue doing
	 * so.
	 */
	if ((obp = *omp) != NULL) {
		/* Find end of previously accumulated output. */
		for (; obp->b_cont != NULL; obp = obp->b_cont)
			;
		contpp = &obp->b_cont;
		chars_left = obp->b_datap->db_lim - obp->b_wptr;
	} else {
		contpp = omp;
		chars_left = 0;
	}

	for (;;) {
		mblk_t	*cibp = ibp->b_cont;

		while (ibp->b_rptr < ibp->b_wptr) {
			/*
			 * Make sure there's room for one more
			 * character.
			 */
			if (chars_left == 0 && !NEW_BLOCK())
				goto outofbufs;

			/*
			 * If doing XCASE processing (not very likely,
			 * in this day and age), look at each character
			 * individually.
			 */
			if ((tp->t_modes.c_lflag & XCASE) &&
				(tp->t_modes.c_lflag & ICANON)) {
				c = *ibp->b_rptr++;

				/*
				 * If character is mapped on output, put out
				 * a backslash followed by what it is
				 * mapped to.
				 */
				if (omaptab[c] != 0 &&
					(!echoing || c != '\\')) {
					/*
					 * Backslash is an ordinary character.
					 */
					tp->t_col++;
					*obp->b_wptr++ = '\\';
					chars_left--;
					if (chars_left == 0 && !NEW_BLOCK()) {
						/*
						 * Make state consistent.
						 */
						ibp->b_rptr--;
						tp->t_col--;
						oobp->b_wptr--;
						goto outofbufs;
					}
					c = omaptab[c];
				}

				/*
				 * If no other output processing is required,
				 * push the character into the block and
				 * get another.
				 */
				if (!(tp->t_modes.c_oflag & OPOST)) {
					tp->t_col++;
					*obp->b_wptr++ = c;
					chars_left--;
					continue;
				}

				/*
				 * OPOST output flag is set.
				 * Map lower case to upper case if OLCUC flag
				 * is set.
				 */
				if ((tp->t_modes.c_oflag & OLCUC) &&
					c >= 'a' && c <= 'z')
					c -= 'a' - 'A';
			} else {
				/*
				 * Copy all the ordinary characters,
				 * possibly mapping upper case
				 * to lower case.
				 */
				register int chars_to_move;
				register int chars_moved;

				chars_to_move = ibp->b_wptr - ibp->b_rptr;
				if (chars_to_move > chars_left)
					chars_to_move = chars_left;
				chars_moved = movtuc(chars_to_move,
				    ibp->b_rptr, obp->b_wptr,
				    (tp->t_modes.c_oflag & OLCUC ?
					lcuctab : notrantab));
				tp->t_col += chars_moved;
				ibp->b_rptr += chars_moved;
				obp->b_wptr += chars_moved;
				chars_left -= chars_moved;
				if (ibp->b_rptr >= ibp->b_wptr) {
					/* moved all of block */
					continue;
				}
				if (chars_left == 0 && !NEW_BLOCK())
					goto outofbufs;
				c = *ibp->b_rptr++;	/* stopper */
			}

			/*
			 * Map <CR> to <NL> on output if OCRNL flag set.
			 */
			if (c == '\r' && (tp->t_modes.c_oflag & OCRNL))
				c = '\n';

			ctype = typetab[c];

			/*
			 * Map <NL> to <CR><NL> on output if ONLCR flag is set.
			 */
			if (c == '\n' && (tp->t_modes.c_oflag & ONLCR)) {
				register int s;
				
			        s = splstr();
				if (!(tp->t_state & TS_TTCR)) {
					tp->t_state |= TS_TTCR;
					c = '\r';
					ctype = typetab['\r'];
					--ibp->b_rptr;
				} else
					tp->t_state &= ~TS_TTCR;
			     (void) splx(s);
			}

			/*
			 * Delay values and column position calculated here.
			 */
			count = 0;
			switch (ctype) {

			case PRINTABLE:
				tp->t_col++;
				*obp->b_wptr++ = c;
				chars_left--;
				break;

			case CONTROL:
				*obp->b_wptr++ = c;
				chars_left--;
				break;

			case BACKSPACE:
				if (tp->t_col)
					tp->t_col--;
				if (tp->t_modes.c_oflag & BSDLY) {
					if (tp->t_modes.c_oflag & OFILL)
						count = 2;
					else
						count = 3;
				}
				*obp->b_wptr++ = c;
				chars_left--;
				break;

			case NEWLINE:
				if (tp->t_modes.c_oflag & ONLRET)
					goto cr;
				if ((tp->t_modes.c_oflag & NLDLY) == NL1)
					count = 2;
				*obp->b_wptr++ = c;
				chars_left--;
				break;

			case TAB:
				/*
				 * Map '\t' to spaces if XTABS flag is set.
				 */
				if ((tp->t_modes.c_oflag & TABDLY) == XTABS) {
					for (;;) {
						*obp->b_wptr++ = ' ';
						chars_left--;
						tp->t_col++;
						if ((tp->t_col & 07) == 0)
							break;	/* every 8th */
						/*
						 * If we don't have room to
						 * fully expand this tab in
						 * this block, back up to
						 * continue expanding it
						 * into the next block.
						 */
						if (obp->b_wptr >=
						    obp->b_datap->db_lim) {
							ibp->b_rptr--;
							break;
						}
					}
				} else {
					tp->t_col |= 07;
					tp->t_col++;
					if (tp->t_modes.c_oflag & OFILL) {
						if (tp->t_modes.c_oflag&TABDLY)
							count = 2;
					} else {
						switch (tp->t_modes.c_oflag&TABDLY) {
						case TAB2:
							count = 6;
							break;

						case TAB1:
							count = 1 +
							    (tp->t_col | ~07);
							if (count < 5)
								count = 0;
							break;
						}
					}
					*obp->b_wptr++ = c;
					chars_left--;
				}
				break;

			case VTAB:
				if ((tp->t_modes.c_oflag & VTDLY) &&
					!(tp->t_modes.c_oflag & OFILL))
					count = 127;
				*obp->b_wptr++ = c;
				chars_left--;
				break;

			case RETURN:
				/*
				 * Ignore <CR> in column 0 if ONOCR flag set.
				 */
				if (tp->t_col == 0 &&
					(tp->t_modes.c_oflag & ONOCR))
					break;

			cr:
				switch (tp->t_modes.c_oflag & CRDLY) {

				case CR1:
					if (tp->t_modes.c_oflag & OFILL)
						count = 2;
					else
						count = tp->t_col % 2;
					break;

				case CR2:
					if (tp->t_modes.c_oflag & OFILL)
						count = 4;
					else
						count = 6;
					break;

				case CR3:
					if (tp->t_modes.c_oflag & OFILL)
						count = 0;
					else
						count = 9;
					break;
				}
				tp->t_col = 0;
				*obp->b_wptr++ = c;
				chars_left--;
				break;
			}

			if (count != 0) {
				/*
				 * At this point, the characters we've added
				 * to the output message are consistent with
				 * those we've processed from the input
				 * message.  If an allocation failure occurs
				 * while handling the delay, it's too hard to
				 * back up to undo the input and output
				 * characters associated with the delay.  This
				 * leaves us with two choices: punt (and drop
				 * the remaining delay) or add more state to
				 * the ldterm_state structure that records the
				 * value of count.
				 *
				 * In the second alternative, we would have to
				 * check the saved count value on entry to
				 * this routine (and quite likely in lots of
				 * other places, too) and jump here if it's
				 * nonzero (after suitable preparation).
				 *
				 * The second alternative looks too ugly for
				 * too little benefit, so we choose the
				 * first.
				 */
				if (tp->t_modes.c_oflag & OFILL) {
					do {
						if (chars_left == 0 &&
						    !NEW_BLOCK())
							goto outofbufs;
						if (tp->t_modes.c_oflag & OFDEL)
							*obp->b_wptr++ = CDEL;
						else
							*obp->b_wptr++ = CNUL;
						chars_left--;
					} while (--count != 0);
				} else {
					if (!(tp->t_modes.c_lflag & FLUSHO)) {
						tk_nout += msgdsize(*omp);
						putnext(q, *omp);
						(void) putctl1(q->q_next,
						    M_DELAY, count);
					} else {
						/* Flush it. */
						freemsg(*omp);
					}
					chars_left = 0;
					/*
					 * We have to start a new message;
					 * the delay introduces a break
					 * between messages.
					 */
					*omp = NULL;
					contpp = omp;
				}
			}
		}

		/*
		 * Free the current input block and advance to the next.
		 */
		freeb(ibp);
		ibp = cibp;
		if (ibp == NULL)
			break;
	}

outofbufs:
	return (ibp);
#undef	NEW_BLOCK
}

#if	!defined(vax) && !defined(sun)
movtuc(size, from, origto, table)
	register int size;
	register unsigned char *from;
	unsigned char *origto;
	register unsigned char *table;
{
	register unsigned char *to = origto;
	register unsigned char c;

	while (size != 0 && (c = table[*from++]) != 0) {
		*to++ = c;
		size--;
	}
	return (to - origto);
}
#endif

/*
 * Respond to the "flush output" character; if output is being flushed, stop
 * flushing it, otherwise flush our write queue and the write queues below us,
 * echo the "flush output" character, and start flushing subsequent output.
 */
static void
do_flush_output(c, q, tp)
	unsigned char c;
	register queue_t *q;
	register ldterm_state_t *tp;
{

	if (tp->t_modes.c_lflag & FLUSHO)
		tp->t_modes.c_lflag &= ~FLUSHO;
	else {
		flushq(q, FLUSHDATA);	/* flush our write queue */
		/* flush the ones below us */
		(void) putctl1(q->q_next, M_FLUSH, FLUSHW);
		if ((tp->t_echomp = allocb(EBSIZE, BPRI_HI)) != NULL) {
			(void) echo_char(c, q, 1, tp);
			if (tp->t_msglen != 0)
				curline_reprint(q, EBSIZE, tp);
			if (tp->t_echomp != NULL) {
				putnext(q, tp->t_echomp);
				tp->t_echomp = NULL;
			}
		}
		tp->t_modes.c_lflag |= FLUSHO;
	}
}

/*
 * Respond to a signal-generating character or an M_BREAK message by echoing
 * the signal-generating character (if "doecho" is set) and sending an M_PCSIG
 * and M_FLUSH message upstream.
 */
static void
do_signal(q, sig, c, doecho, ignore_noflsh)
	register queue_t *q;
	int sig;
	unsigned char c;
	int doecho;
	int ignore_noflsh;
{
	register ldterm_state_t *tp = (ldterm_state_t *)q->q_ptr;

	/*
	 * If ignore_noflsh, do the flushing regardless of NOFLSH.
 	 */
	if (ignore_noflsh || (!(tp->t_modes.c_lflag & NOFLSH))) {
		/*
		 * Put it on our read queue, so the service procedure
		 * will see it.
		 */
		if (sig != SIGTSTP) {
			/*
			 * Flush our read and write queues, and send a
			 * "flush read and write" message downstream and
			 * upstream.
			 */
			flushq(WR(q), FLUSHDATA);
			flushq(q, FLUSHDATA);

			(void) putctl1(WR(q)->q_next, M_FLUSH, FLUSHRW);
			(void) putctl1(q->q_next, M_FLUSH, FLUSHRW);
		} else {
			/*
			 * Flush our read queue, and send a "flush read"
			 * message upstream and downstream.
			 */
			flushq(q, FLUSHDATA);
			(void) putctl1(WR(q)->q_next, M_FLUSH, FLUSHR);
			(void) putctl1(q->q_next, M_FLUSH, FLUSHR);
		}
	}
	tp->t_state &= ~TS_QUOT;
	/* PCSIG, not SIG - do it NOW */
	(void) putctl1(q->q_next, M_PCSIG, sig);

	if (doecho) {
		if ((tp->t_echomp = allocb(4, BPRI_HI)) != NULL) {
			(void) echo_char(c, WR(q), 4, tp);
			putnext(WR(q), tp->t_echomp);
			tp->t_echomp = NULL;
		}
	}
}

/*
 * Called when an M_IOCTL message is seen on the write queue; does whatever
 * we're supposed to do with it, and either replies immediately or passes it
 * to the next module down.
 */
static void
do_ioctl(q, mp)
	queue_t *q;
	register mblk_t *mp;
{
	register ldterm_state_t *tp;
	register struct iocblk *iocp;

	iocp = (struct iocblk *)mp->b_rptr;
	tp = (ldterm_state_t *)q->q_ptr;

	switch (iocp->ioc_cmd) {

	case TCSETS:
	case TCSETSW:
	case TCSETSF: {
		/*
		 * Set current parameters and special characters.
		 */
		register struct termios *cb =
		    (struct termios *)mp->b_cont->b_rptr;
		struct termios oldmodes;

		oldmodes = tp->t_modes;
		tp->t_modes = *cb;
		if (tp->t_modes.c_lflag & PENDIN) {
			/*
			 * Yuk.  The C shell file completion code actually
			 * uses this "feature", so we have to support it.
			 */
			if (tp->t_message != NULL) {
				tp->t_state |= TS_RESCAN;
				qenable(RD(q));
			}
			tp->t_modes.c_lflag &= ~PENDIN;
		}

		chgstropts(&oldmodes, tp, RD(q));

		/*
		 * The driver may want to know about the following iflags:
		 * IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK.
		 */
		break;
	    }

	case TCSETA:
	case TCSETAW:
	case TCSETAF: {
		/*
		 * Old-style "ioctl" to set current parameters and
		 * special characters.
		 * Don't clear out the unset portions, leave them as
		 * they are.
		 */
		register struct termio *cb =
		    (struct termio *)mp->b_cont->b_rptr;
		struct termios oldmodes;

		oldmodes = tp->t_modes;
		tp->t_modes.c_iflag =
		    (tp->t_modes.c_iflag & 0xffff0000 | cb->c_iflag);
		tp->t_modes.c_oflag =
		    (tp->t_modes.c_oflag & 0xffff0000 | cb->c_oflag);
		tp->t_modes.c_cflag =
		    (tp->t_modes.c_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_modes.c_lflag =
		    (tp->t_modes.c_lflag & 0xffff0000 | cb->c_lflag);

		tp->t_modes.c_cc[VINTR] = cb->c_cc[VINTR];
		tp->t_modes.c_cc[VQUIT] = cb->c_cc[VQUIT];
		tp->t_modes.c_cc[VERASE] = cb->c_cc[VERASE];
		tp->t_modes.c_cc[VKILL] = cb->c_cc[VKILL];
		tp->t_modes.c_cc[VEOF] = cb->c_cc[VEOF];
		tp->t_modes.c_cc[VEOL] = cb->c_cc[VEOL];
		tp->t_modes.c_cc[VEOL2] = cb->c_cc[VEOL2];
		tp->t_modes.c_cc[VSWTCH] = cb->c_cc[VSWTCH];

		chgstropts(&oldmodes, tp, RD(q));

		/*
		 * The driver may want to know about the following iflags:
		 * IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK.
		 */
		break;
	    }

	case TCFLSH:
		/*
		 * Do the flush on the write queue immediately, and queue
		 * up any flush on the read queue for the service procedure
		 * to see.  Then turn it into the appropriate M_FLUSH message,
		 * so that the module below us doesn't have to know about
		 * TCFLSH.
		 */
		ASSERT(mp->b_datap != NULL);
		switch (*(int *)mp->b_cont->b_rptr) {

		default:
			u.u_error = EINVAL;
			break;

		case TCIFLUSH:
			(void) putctl1(q->q_next, M_FLUSH, FLUSHR);
			(void) putctl1(RD(q), M_FLUSH, FLUSHR);
			break;

		case TCOFLUSH:
			flushq(q, FLUSHDATA);
			(void) putctl1(q->q_next, M_FLUSH, FLUSHW);
			(void) putctl1(RD(q)->q_next, M_FLUSH, FLUSHW);
			break;

		case TCIOFLUSH:
			flushq(q, FLUSHDATA);
			(void) putctl1(q->q_next, M_FLUSH, FLUSHRW);
			(void) putctl1(RD(q), M_FLUSH, FLUSHRW);
			break;
		}
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		qreply(q, mp);
		return;

	case TCXONC:
		switch (*(int *)mp->b_cont->b_rptr) {

		case TCOOFF:
			(void) putctl(q->q_next, M_STOP);
			tp->t_state |= TS_TTSTOP;
			break;

		case TCOON:
			(void) putctl(q->q_next, M_START);
			tp->t_state &= ~TS_TTSTOP;
			break;

		case TCIOFF:
			(void) putctl(q->q_next, M_STOPI);
			tp->t_state |= TS_TBLOCK;
			break;

		case TCION:
			(void) putctl(q->q_next, M_STARTI);
			tp->t_state &= ~TS_TBLOCK;
			break;
		}
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		qreply(q, mp);
		return;
	}

	putnext(q, mp);
}

/*
 * Send an M_SETOPTS message upstream if any mode changes are being made
 * that affect the stream head options.
 */
static void
chgstropts(oldmodep, tp, q)
	register struct termios *oldmodep;
	register ldterm_state_t *tp;
	queue_t *q;
{
	struct stroptions optbuf;
	register mblk_t *bp;

	optbuf.so_flags = 0;
	if ((oldmodep->c_lflag ^ tp->t_modes.c_lflag) & ICANON) {
		/*
		 * Canonical mode is changing state; switch the stream head
		 * to message-nondiscard or byte-stream mode.  Also, rerun
		 * the service procedure so it can change its mind about
		 * whether to send data upstream or not.
		 */
		optbuf.so_flags = SO_READOPT|SO_VMIN|SO_VTIME;
		if (tp->t_modes.c_lflag & ICANON) {
			optbuf.so_readopt = RMSGN;
			optbuf.so_vmin = (ushort) -1;
			optbuf.so_vtime = 0;
		} else {
			optbuf.so_readopt = RNORM;
			optbuf.so_vmin = tp->t_modes.c_cc[VMIN];
			optbuf.so_vtime = tp->t_modes.c_cc[VTIME];
		}
		if (tp->t_message != NULL) {
			tp->t_state |= TS_RESCAN;
			qenable(q);
		}
	} else if (!(tp->t_modes.c_lflag & ICANON) &&
		(oldmodep->c_cc[VMIN] != tp->t_modes.c_cc[VMIN] ||
		oldmodep->c_cc[VTIME] != tp->t_modes.c_cc[VTIME])) {
		/*
		 * Canonical mode is off, and the VMIN and VTIME values are
		 * changing; let the stream head know.
		 */
		optbuf.so_flags = SO_VMIN|SO_VTIME;
		optbuf.so_vmin = tp->t_modes.c_cc[VMIN];
		optbuf.so_vtime = tp->t_modes.c_cc[VTIME];
	}

	if ((oldmodep->c_lflag ^ tp->t_modes.c_lflag) & TOSTOP) {
		/*
		 * The "stop on background write" bit is changing.
		 */
		optbuf.so_flags |= SO_TOSTOP;
		if (tp->t_modes.c_lflag & TOSTOP)
			optbuf.so_tostop = 1;
		else
			optbuf.so_tostop = 0;
	}

	if (optbuf.so_flags != 0) {
		if ((bp = allocb(sizeof (struct stroptions), BPRI_HI)) == NULL)
			panic("chgstropts: can't allocate stroptions message");
			/* XXX - should probably do bufcall */
		*(struct stroptions *)bp->b_wptr = optbuf;
		bp->b_wptr += sizeof (struct stroptions);
		bp->b_datap->db_type = M_SETOPTS;
		putnext(q, bp);
	}
}

/*
 * Called when an M_IOCACK message is seen on the read queue; modifies
 * the data being returned, if necessary, and passes the reply up.
 */
static void
ioctl_reply(q, mp)
	queue_t *q;
	register mblk_t *mp;
{
	register ldterm_state_t *tp;
	register struct iocblk *iocp;

	iocp = (struct iocblk *)mp->b_rptr;
	tp = (ldterm_state_t *)q->q_ptr;

	switch (iocp->ioc_cmd) {

	case TCGETS: {
		/*
		 * Get current parameters and return them to stream head
		 * eventually.
		 */
		register struct termios *cb =
		    (struct termios *)mp->b_cont->b_rptr;
		register unsigned long cflag = cb->c_cflag;

		*cb = tp->t_modes;
		if (cflag != 0)
			cb->c_cflag = cflag;	/* set by driver */
		break;
	    }

	case TCGETA: {
		/*
		 * Old-style "ioctl" to get current parameters and
		 * return them to stream head eventually.
		 */
		register struct termio *cb =
		    (struct termio *)mp->b_cont->b_rptr;

		cb->c_iflag = tp->t_modes.c_iflag;	/* all except the */
		cb->c_oflag = tp->t_modes.c_oflag;	/* cb->c_cflag */
		cb->c_lflag = tp->t_modes.c_lflag;

		if (cb->c_cflag == 0)	/* not set by driver */
			cb->c_cflag = tp->t_modes.c_cflag;

		cb->c_line = 0;
		cb->c_cc[VINTR] = tp->t_modes.c_cc[VINTR];
		cb->c_cc[VQUIT] = tp->t_modes.c_cc[VQUIT];
		cb->c_cc[VERASE] = tp->t_modes.c_cc[VERASE];
		cb->c_cc[VKILL] = tp->t_modes.c_cc[VKILL];
		cb->c_cc[VEOF] = tp->t_modes.c_cc[VEOF];
		cb->c_cc[VEOL] = tp->t_modes.c_cc[VEOL];
		cb->c_cc[VEOL2] = tp->t_modes.c_cc[VEOL2];
		cb->c_cc[VSWTCH] = tp->t_modes.c_cc[VSWTCH];
		break;
	    }
	}
	putnext(q, mp);
}
