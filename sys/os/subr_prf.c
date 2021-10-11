/*	@(#)subr_prf.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*LINTLIBRARY*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/vm.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/debug.h>
#include <sys/syslog.h>

#ifdef vax
#include <vax/mtpr.h>
#endif
#ifdef sun
#include <sun/consdev.h>
#include <machine/pte.h>
#endif
#include <sys/varargs.h>

#define	TOCONS	0x1		/* to the console */
#define	TOTTY	0x2		/* to the user's tty */
#define	TOLOG	0x4		/* to the system log */

#ifdef	MULTIPROCESSOR
extern int	cpuid;
int		panic_cpu = 0;
#endif	MULTIPROCESSOR

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr = (char *)0;

/*
 * digits for use in numeric output
 */
static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/*
 * Place to save regs on panic
 * so adb can find them later.
 */
#ifdef	i386
struct	regs panic_regs;
#else
label_t	panic_regs;
#endif

int	noprintf = 0;	/* patch to non-zero to suppress kernel printf's */

void prf();
void fnprn();
void fnprf();
char *sprintf();

/*
 * On a Sun, we support redirection of console output to a pseudo-tty.
 * This routine checks whether this message should be sent to that pseudo-tty
 * or not.  If the console isn't redirected, or if this message is a panic
 * message, or if the console is redirected to a device that doesn't have an
 * active stream, or if that stream is over-full, we don't want to send output
 * there.
 *
 * On a non-Sun, we don't support it so we never send output there.
 */
#ifdef sun
static struct vnode *
getconsvp()
{
	register struct vnode *vp;

	/*
	 * If this is not a "panic" message, and the console is redirected to a
	 * pseudo-tty, and that pseudo-tty has an active stream, and the stream
	 * is not too much over full, get the vnode of the pseudo-tty it's
	 * redirected to and don't send it to the (real) console, but send it
	 * to that pseudo-tty.
	 */
	if (consdev != rconsdev && (panicstr == 0) &&
	    consvp->v_stream != NULL && strcheckoutq(consvp, 0))
		vp = consvp;
	else
		vp = NULL;

	return (vp);
}
#else
#define	getconsvp()	((struct vnode *)NULL)
#endif

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */
/*VARARGS1*/
printf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list x1;


	va_start(x1);
	prf(fmt, x1, TOCONS | TOLOG, getconsvp());
	logwakeup();
	va_end(x1);

}

/*
 * Uprintf prints to the current user's terminal.
 * It may block if the tty queue is overfull.
 * No message is printed if the queue does not clear
 * in a reasonable time.
 * Should determine whether current terminal user is related
 * to this process.
 */
/*VARARGS1*/
uprintf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	register struct vnode *vp;
	va_list x1;

	va_start(x1);
	if ((vp = u.u_procp->p_sessp->s_vp) == NULL) {
		prf(fmt, x1, TOCONS, getconsvp());
		/* no controlling tty - send it to the console */
	} else {
#ifdef sun
		/*
		 * XXX - if the process' controlling tty is the "real"
		 * console, send the output to the current console, in case
		 * it was redirected after the process was assigned its
		 * controlling tty.  This whole console redirection
		 * mechanism is a bit of a mess.
		 */
		if (vp == rconsvp)
			vp = consvp;
#endif
		if (vp->v_stream != NULL && strcheckoutq(vp, 1))
			prf(fmt, x1, TOTTY, vp);
	}
	va_end(x1);
}

/*
 * tprintf prints on the specified terminal (console if none)
 * and logs the message.  It is designed for error messages from
 * single-open devices, and may be called from interrupt level
 * (does not sleep).
 */
/*VARARGS2*/
tprintf(vp, fmt, va_alist)
	register struct vnode *vp;
	char *fmt;
	va_dcl
{
	int flags = TOTTY | TOLOG;
	va_list x1;

	va_start(x1);
	logpri(LOG_INFO);
	if (vp == NULL)
		vp = consvp;
#ifdef sun
	else {
		/*
		 * XXX - if the terminal in question is the "real" console,
		 * send the output to the current console, in case it was
		 * redirected after the terminal in question was specified.
		 * This whole console redirection mechanism is a bit of a
		 * mess.
		 */
		if (vp == rconsvp)
			vp = consvp;
	}
#endif
	if (strcheckoutq(vp, 0) == 0)
		flags = TOLOG;
	prf(fmt, x1, flags, vp);
	logwakeup();
	va_end(x1);
}

/*
 * Log writes to the log buffer,
 * and guarantees not to sleep (so can be called by interrupt routines).
 * If there is no process reading the log yet, it writes to the console also.
 */
/*VARARGS2*/
log(level, fmt, va_alist)
	char *fmt;
	va_dcl
{
	register s = splhigh();
	extern int log_open;
	va_list x1;

	va_start(x1);
	logpri(level);
	prf(fmt, x1, TOLOG, (struct vnode *)0);
	(void) splx(s);
	if (!log_open)
		prf(fmt, x1, TOCONS, getconsvp());
	logwakeup();
	va_end(x1);
}

logpri(level)
	int level;
{
	char logbuf[13];

	(void) sprintf(logbuf, "<%d>", level);
	writekmsg(logbuf, strlen(logbuf), TOLOG, (struct vnode *)0);
}

#define	PRF_LINEBUF_SIZE	128
struct prf_context {
	char	linebuf[PRF_LINEBUF_SIZE];
	int	lbi;
	struct vnode *vp;
	int	flags;
};

static int 
prf_putchar(cp, ch)
	register struct prf_context *cp;
	register int	ch;
{
	register int lbi = cp->lbi;
	register char *lbp = cp->linebuf;

	if (lbi >= PRF_LINEBUF_SIZE) {
		writekmsg(lbp, lbi, cp->flags, cp->vp);
		lbi = 0;
	}
	lbp[lbi] = ch;
	cp->lbi = lbi + 1;
}

void
prf(fmt, adx, flags, vp)
	register char *fmt;
	register va_list adx;
	struct vnode *vp;
{
	struct prf_context cp[1];

	cp->lbi = 0;
	cp->vp = vp;
	cp->flags = flags;

	fnprf(prf_putchar, (caddr_t) cp, fmt, adx);

	writekmsg(cp->linebuf, cp->lbi, cp->flags, cp->vp);
}

struct cprintf_context {
	int	count;
};

/*ARGSUSED*/
static int
cprintf_putchar(cp, ch)
	struct cprintf_context *cp;
	int	ch;
{
	cp->count++;
}

/*
 * cprintf(): printf, with output discarded. Returns
 * the number of characters that printf will emit.
 * Useful for figuring out the size for sprintf
 * buffers.
 */
/*VARARGS1*/
int
cprintf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list	ap;
	struct cprintf_context cp[1];
	cp->count = 0;
	va_start(ap);
	fnprf(cprintf_putchar, (caddr_t) cp, fmt, ap);
	va_end(ap);
	return cp->count;
}

struct sprintf_context {
	char   *here;		/* where it goes next */
};

static int
sprintf_putchar(cp, ch)
	struct sprintf_context *cp;
	int	ch;
{
	cp->here[0] = ch;
	cp->here ++;
	cp->here[0] = '\0';
}

/*
 * sprintf(): printf, with output sent to sequential
 * memory starting at the specified location. It is
 * up to the caller to figure out how big the data
 * is going to be. [[cprintf() is useful for this]]
 */

/*VARARGS2*/
char *
sprintf(str, fmt, va_alist)
	char *str;
	char *fmt;
	va_dcl
{
	va_list	ap;
	struct sprintf_context cp[1];
	cp->here = str;
	va_start(ap);
	fnprf(sprintf_putchar, (caddr_t) cp, fmt, ap);
	va_end(ap);
	return str;
}

/*
 * Generic PRINTF driver: construct and emit characters
 * according to the normal "printf" formatting rules,
 * via a specified function. The function is called with
 * a value passed in as its first parameter, and the
 * character to be emitted as its second parameter.
 * This routine can be used as the central point in
 * any number of printf-like functions. There is no
 * a-priori limit to the number of characters to be
 * emitted or the number of parameters.
 */
void
fnprf(fn, cp, fmt, adx)
	int (*fn)();
	caddr_t cp;
	char *fmt;
	va_list adx;
{
	register int b, c, i;
	register char *s;
	int any;
	int width;

#define	PUTCHAR(c)	(*fn)(cp, c)

loop:
	width = 0;
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		if (c == '\n')
			PUTCHAR('\r');
		PUTCHAR(c);
	}
again:
	c = *fmt++;
	if (c >= '2' && c <= '9') {
		width = c - '0';
		c = *fmt++;
	}
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		fnprn(fn, cp, va_arg(adx, u_long), b, width);
		break;
	case 'c':
		b = va_arg(adx, int);
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f) {
				if (c == '\n')
					PUTCHAR('\r');
				PUTCHAR(c);
			}
		break;
	case 'b':
		b = va_arg(adx, int);
		s = va_arg(adx, char*);
		if ((s == NULL) || (*s == 0)) /* sanity check. */
			s = "\020";
		fnprn(fn, cp, (u_long)b, *s++, 0);
		any = 0;
		if (b) {
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					PUTCHAR(any? ',' : '<');
					any = 1;
					for (; (c = *s) > 32; s++)
						PUTCHAR(c);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				PUTCHAR('>');
		}
		break;

	case 's':
		s = va_arg(adx, char*);
		if (s == 0)
			s = "(null)";
		while (c = *s++) {
			if (c == '\n')
				PUTCHAR('\r');
			PUTCHAR(c);
		}
		break;

	case 'e':		/* HACK: six octets, ethernet style. */
		s = va_arg(adx, char*);
		if (width<1) width=6;
		while (width-->0) {
			i = c = *s++;
			i >>= 4;
			i &= 15;
			c &= 15;
			PUTCHAR(digits[i]);
			PUTCHAR(digits[c&15]);
			if (width>0)
				PUTCHAR(':');
		}
		break;

	case '%':
		PUTCHAR('%');
		break;
	}
	goto loop;
#undef	PUTCHAR
}

/*
 * fnprn prints a number n in base b, using the
 * specified putchar function and context.
 * We don't use recursion to avoid deep kernel stacks.
 * Allowable bases are 2 through 36. Bases outside
 * this range are converted to decimal.
 * %%% all decimal numbers are signed.
 * %%% all non-decimal numbers are unsigned.
 */

static void
fnprn(fn, cp, n, b, width)
	int (*fn)();
	caddr_t cp;
	u_long n;
{
	char prbuf[33];		/* wide enough for BINARY longword */
	register char *bp;

#define	PUTCHAR(c)	(*fn)(cp, c)

	if ((b < 2) || (b >= sizeof digits))
		b = 10;

	if (b == 10 && (int)n < 0) {
		PUTCHAR('-');
		n = (unsigned)(-(int)n);
	}

	bp = prbuf;
	do {
		*bp++ = digits[n%b];
		n /= b;
		width--;
	} while (n);
	while (width-- > 0)
		*bp++ = '0';
	while (bp > prbuf)
		PUTCHAR(*--bp);
#undef	PUTCHAR
}

#ifdef	MULTIPROCESSOR
panic_slave()
{
	extern label_t panic_label;
	flush_windows();
	(void)setjmp(&panic_label);
}
#endif	MULTIPROCESSOR

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */

int panic_opt = RB_DUMP | RB_AUTOBOOT;

panic(s)
	char *s;
{
#ifdef sun4m
	trigger_logan();
#endif
#ifdef	PROM_PARANOIA
	mpprom_eprintf("panic on %d: %s\n", cpuid, s);
#endif	PROM_PARANOIA
#ifdef sun386
	(void) spl7();		/* ZZZ */
	getpregs();
	/* XXX this is a guess XXX */
	panic_regs.r_reg[OESP] = *(long *)&u.u_procp->p_stack[KERNSTACK-8];
#endif
	if (panicstr)
		panic_opt |= RB_NOSYNC;
	else {
		panicstr = s;
		noprintf = 0;	/* turn printfs on */
#ifdef	MULTIPROCESSOR
		xc_attention();				/* pull everyone's chain */
		xc_callstart(0,0,0,0,0,panic_slave);	/* flush their windows & setjmp */
		panic_slave();				/* flush our windows & setjmp */
		xc_callsync();				/* wait for it all to finish */
#endif	MULTIPROCESSOR
		if (u.u_procp) {
#if defined(sun2) || defined(sun3) || defined(sun3x) || defined(sun4) || defined(sun4c) || defined(sun4m)
			(void) setjmp(&u.u_pcb.pcb_regs);
			panic_regs = u.u_pcb.pcb_regs;
#endif sun2 sun3 sun3x sun4 sun4c sun4m
		}
	}
#ifndef	MULTIPROCESSOR
	printf("panic: %s\n", s);
#else	MULTIPROCESSOR
	panic_cpu = cpuid;
	printf("panic on cpu %d: %s\n", cpuid, s);
#endif	MULTIPROCESSOR
	boot(panic_opt);
}

/*
 * Warn that a system table is full.
 */
tablefull(tab)
	char *tab;
{

	log(LOG_ERR, "%s: table is full\n", tab);
	printf("%s: table is full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk transfers.
 */
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{

	printf("%s%d%c: hard error sn%d ", cp,
	    dkunit(bp), 'a'+(minor(bp->b_dev)&07), bp->b_blkno);
}

#ifdef DEBUG
/*
 * Called by the ASSERT macro in debug.h when an assertion fails.
 */
assfail(a, f, l)
	char *a, *f;
	int l;
{

	noprintf = 0;
	panicstr = "";
	printf("assertion failed: %s, file: %s, line: %d\n", a, f, l);
	panicstr = 0;
	panic("assertion failed");
}
#endif

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
/*ARGSUSED*/
putchar(c, flags, vp)
	int c, flags;
	struct vnode *vp;
{
	char cc = c;

	if (c == '\n')
		writekmsg("\r", 1, flags, vp);
	writekmsg(&cc, 1, flags, vp);
}

/*
 * Write a block of characters to: the user's terminal, the log, and either
 * the real console or the redirected console.
 * We don't want to do this one character at a time because we don't want
 * to chew up all the stream buffers with one-character writes.
 */
writekmsg(msgp, count, flags, vp)
	register char *msgp;
	int count;
	register int flags;
	register struct vnode *vp;
{
	register int c;
#ifdef sun
	extern int msgbufinit;
#endif

	if (!noprintf) {
		if (vp != NULL) {
			if (stroutput(vp, msgp, count))
				flags &= ~TOCONS;
		}
	}
#ifdef	MULTIPROCESSOR
	/*
	 * MULTIPROCESSORs may incur additional overhead when
	 * we dive into and out of prom services, so try to
	 * do the entire write in one call.
	 */
	if ((flags & TOCONS) && !noprintf)
		mpprom_write_nl7(msgp, count);
#endif
	while (count > 0) {
		count--;
		if ((c = *msgp++) == '\0')
			continue;	/* don't print NULs */
		if ((flags & TOLOG) &&
#ifdef vax
		    mfpr(MAPEN) &&
#endif
#ifdef sun
		    msgbufinit &&
#endif
		    c != '\r' && c != 0177) {
			if (msgbuf.msg_magic != MSG_MAGIC ||
			    msgbuf.msg_size != MSG_BSIZE) {
				register int i;

				msgbuf.msg_magic = MSG_MAGIC;
				msgbuf.msg_size = MSG_BSIZE;
				msgbuf.msg_bufx = msgbuf.msg_bufr = 0;
				for (i=0; i < MSG_BSIZE; i++)
					msgbuf.msg_bufc[i] = 0;
			}
			msgbuf.msg_bufc[msgbuf.msg_bufx++] = c;
			if (msgbuf.msg_bufx < 0 ||
			    msgbuf.msg_bufx >= MSG_BSIZE)
				msgbuf.msg_bufx = 0;
		}
#ifndef	MULTIPROCESSOR
		if ((flags & TOCONS) && !noprintf)
			cnputc(c);
#endif
	}
}

/*
 * Print out the message buffer.
 */
int	prtmsgbuflines = 12;

prtmsgbuf()
{
	register int c, l;
	static int ndx;

	for (l = 0; l < prtmsgbuflines; l++)
		do {
			if (ndx < 0 || ndx >= MSG_BSIZE) {
				char *msg = "\n<top of msgbuf>\n";

				while (*msg)
					cnputc(*msg++);
				ndx = 0;
			}
			cnputc(c = msgbuf.msg_bufc[ndx++]);
		} while (c != '\n');
}
