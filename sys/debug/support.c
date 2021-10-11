#ident "@(#)support.c	1.1 7/30/92 SMI"

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include <sys/user.h>
#include <time.h>
#include <tzfile.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <debug/debugger.h>
#include <sys/ptrace.h>
#include <varargs.h>
#ifdef sparc
#include <machine/debug/allregs.h>
u_int npmgrps;
u_int segmask;
#endif

#ifdef	OPENPROMS
extern void	prom_exit_to_mon(), prom_enter_mon(), prom_putchar();
#else	OPENPROMS
#define	prom_exit_to_mon()	(*romp->v_exit_to_mon)()
#define	prom_enter_mon()	(*romp->v_abortent)()
#define	prom_putchar(c)		(*romp->v_putchar)((c))
#define	prom_mayget()		(*romp->v_mayget)()
#endif	OPENPROMS

_exit()
{
	prom_exit_to_mon();
}

kadb_done()
{
	prom_enter_mon();
}

int interrupted = 0;

getchar()
{
	register int c;

	while ((c = prom_mayget()) == -1)
		;
	if (c == '\r')
		c = '\n';
	if (c == 0177 || c == '\b') {
		putchar('\b');
		putchar(' ');
		c = '\b';
	}
	putchar(c);
	return (c);
}

/*
 * Read a line into the given buffer and handles
 * erase (^H or DEL), kill (^U), and interrupt (^C) characters.
 * This routine ASSUMES a maximum input line size of LINEBUFSZ
 * to guard against overflow of the buffer from obnoxious users.
 */
gets(buf)
	char buf[];
{
	register char *lp = buf;
	register c;

	for (;;) {
		c = getchar() & 0177;
		switch(c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case 'u'&037:			/* ^U */
			lp = buf;
			putchar('^');
			putchar('U');
			putchar('\n');
			continue;
		case 'c'&037:
			dointr(1);
			/*MAYBE REACHED*/
			/* fall through */
		default:
			if (lp < &buf[LINEBUFSZ-1]) {
				*lp++ = c;
			} else {
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			break;
		}
	}
}

dointr(doit)
{

	putchar('^');
	putchar('C');
	interrupted = 1;
	if (abort_jmp && doit) {
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}
}

/*
 * Check for ^C on input
 */
tryabort(doit)
{

	if (prom_mayget() == ('c' & 037)) {
		dointr(doit);
		/*MAYBE REACHED*/
	}
}

/*
 * Implement pseudo ^S/^Q processing along w/ handling ^C
 * We need to strip off high order bits as monitor cannot
 * reliably figure out if the control key is depressed when
 * prom_mayget is called in certain circumstances.
 * Unfortunately, this means that s/q will work as well
 * as ^S/^Q and c as well as ^C when this guy is called.
 */
trypause()
{
	register int c;

	c = prom_mayget() & 037;

	if (c == ('s' & 037)) {
		while ((c = prom_mayget() & 037) != ('q' & 037)) {
			if (c == ('c' & 037)) {
				dointr(1);
				/*MAYBE REACHED*/
			}
		}
	} else if (c == ('c' & 037)) {
		dointr(1);
		/*MAYBE REACHED*/
	}
}

/*
 * Scaled down version of C Library printf.
 */
/*VARARGS1*/
printf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list x1;

	tryabort(1);
	va_start(x1);
	prf(fmt, x1);
	va_end(x1);
}

prf(fmt, adx)
	register char *fmt;
	register va_list adx;
{
	register int b, c;
	register char *s;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		putchar(c);
	}
again:
	c = *fmt++;
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
		printn(va_arg(adx, u_long), b);
		break;
	case 'c':
		b = va_arg(adx, int);
		putchar(b);
		break;
	case 's':
		s = va_arg(adx, char *);
		while (c = *s++)
			putchar(c);
		break;
	}
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b)
	register u_long n;
	register short b;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp);
	while (cp > prbuf);
}

/*
 * Print a character on console.
 */
putchar(c)
	int c;
{
/* XXX - OPENPROMS?? */
#if defined (sun4c) || defined (sun4m)
	if (c == '\n')
		prom_putchar('\r');
#endif
	prom_putchar(c);
}

/*
 * Fake getpagesize() system call
 */
getpagesize()
{

	return (NBPG);
}

/*
 * Fake gettimeofday call
 * Needed for ctime - we are lazy and just
 * give a bogus approximate answer
 */
gettimeofday(tp, tzp)
	struct timeval *tp;
	struct timezone *tzp;
{

	tp->tv_sec = (1989 - 1970) * 365 * 24 * 60 * 60;	/* ~1989 */
	tzp->tz_minuteswest = 8 * 60;	/* PDT: California ueber alles */
	tzp->tz_dsttime = DST_USA;
}

int errno;

caddr_t
sbrk(incr)
	int incr;
{
	extern char end[];
	static caddr_t lim;
	caddr_t val;
	register int i;
#ifdef sun3x
	union {
		struct pte pte;
		int ipte;
	} tmppte;
#endif sun3x
#ifdef sun4m
	union ptes tmppte;
#endif sun4m
#ifdef OPENPROMS
	register int size;
	register caddr_t addr;
#endif OPENPROMS

	if (nobrk) {
		printf("sbrk:  late call\n");
		errno = ENOMEM;
		return ((caddr_t)-1);
	}
	if (lim == 0) {
		lim = (caddr_t)roundup((u_int)end, NBPG);
#ifdef OPENPROMS
		/*
		 * Workaround for prom_alloc() call in machdep.c
		 * to allocate the page for L1PTs.
		 */
		lim += 0x1000;
#endif OPENPROMS
	}
	incr = btoc(incr);
#ifdef OPENPROMS
	size = ctob(incr);
#endif OPENPROMS
	if ((lim + ctob(incr)) >= (caddr_t)DEBUGEND) {
		printf("sbrk:  lim %x + %x exceeds %x\n", lim,
		    ctob(incr), DEBUGEND);
		errno = EINVAL;
		return ((caddr_t)-1);
	}

	val = lim;
	pagesused += incr;

#ifdef OPENPROMS
#define BETA_PROM_BUG_FIXED
#ifdef BETA_PROM_BUG_FIXED
	if (romp->op_romvec_version > 0) {
		lim += size;
		addr = (caddr_t)prom_alloc(val, size);
		if (addr != val) {
			errno = EINVAL;
			return ((caddr_t)-1);
		}
		return (val);
	}
#else BETA_PROM_BUG_FIXED
	/*
	 * XXX - A bounds-checking bug in the alloc() routine can
	 * cause the invalid pmeg to get allocated pages.
	 * Make life easier for the prom by allocating one page at a time.
	 * This can go away when the V.2 Open Prom ships.
	 * Putting this code in for Galaxy here, even though not sure
	 * if we have the above mentioned bug in our code.  Will check on.
	 */
	if (romp->op_romvec_version > 0) {
		caddr_t a;

		lim += size;
		for (a = val; a < val + size; a += PAGESIZE) {
			addr = (caddr_t)prom_alloc(a, PAGESIZE);
			if (addr != a) {
				errno = EINVAL;
				return ((caddr_t)-1);
			}
		}
		return (val);
	}
#endif BETA_PROM_BUG_FIXED
#endif OPENPROMS

	for (i = 0; i < incr; i++, lim += NBPG) {
#ifdef sun3x
		tmppte.ipte = MMU_INVALIDPTE;
		tmppte.pte.pte_vld = PTE_VALID;
		tmppte.pte.pte_pfn = --lastpg;
		Setpgmap(lim, tmppte.ipte);
		atc_flush();
#elif sun4m
		tmppte.pte_int = PTEOF(0, --lastpg, MMU_STD_SRWX, 0);
		Setpgmap(lim, tmppte.pte_int);
		mmu_flushall();
#else /* !sun3x && !sun4m */
		if (getsegmap(lim) == PMGRP_INVALID) {
			register int j = (int)lim & ~PMGRPOFFSET;
			int last = j + NPMENTPERPMGRP * NBPG;

			setsegmap(lim, --lastpm);

			for (; j < last; j += NBPG)
				Setpgmap(j, 0);
		}
		Setpgmap(lim, PG_V | PG_KW | PGT_OBMEM | getapage());
#endif
	}
	return (val);
}

/*
 * Fake ptrace - ignores pid and signals
 * Otherwise it's about the same except the "child" never runs,
 * flags are just set here to control action elsewhere.
 */
ptrace(request, pid, addr, data, addr2)
	enum ptracereq request;
	char *addr, *addr2;
{
	int rv = 0;
	register int i, val;
	register int *p;
#ifdef sparc
	extern struct allregs adb_regs;
#endif

	switch (request) {
	case PTRACE_TRACEME:	/* do nothing */
		break;

	case PTRACE_PEEKTEXT:
	case PTRACE_PEEKDATA:
#ifdef sparc
		/*
		 * Use two peek's instead of a peekl to handle
		 * misaligned word transfers on sparc.
		 */
		rv = peek(addr);
		if (errno) {
			errno = EFAULT;
		} else {
			/*
			 * Now read the 2nd half in sparc word wording fashion
			 */
			int tmp;

			tmp = peek(addr + sizeof (short));
			if (errno) {
				rv = -1;
				errno = EFAULT;
			} else {
				rv = (rv << (sizeof (short) * NBBY)) | 
				(tmp & 0xffff);
			}
		}
#else sparc
		rv = peekl(addr);
#endif sparc
		break;

	case PTRACE_PEEKUSER:
		i = (int)addr;
		if (i < 0 || i > (sizeof(struct user) - sizeof(int))) {
			rv = -1;
			errno = EIO;
		} else {
			i = (i + sizeof(int) - 1) & ~(sizeof(int) - 1);
			p = (int *)((int)uunix + i);
			rv = peekl(p);
		}
		break;

	case PTRACE_POKEUSER:
		i = (int)addr;
		if (i < 0 || i > (sizeof(struct user) - sizeof(int))) {
			rv = -1;
			errno = EIO;
		} else {

			i = (i + sizeof(int) - 1) & ~(sizeof(int) - 1);
			p = (int *)((int)uunix + i);
			rv = pokel(p, data);
		}
		break;

	case PTRACE_POKETEXT:
		rv = poketext(addr, data);
		break;

	case PTRACE_POKEDATA:
		rv = pokel(addr, data);
		break;

	case PTRACE_SINGLESTEP:
		dotrace = 1;
		/* fall through to ... */
	case PTRACE_CONT:
		dorun = 1;
		if ((int)addr != 1)
			reg->r_pc = (int)addr;
		break;

	case PTRACE_SETREGS:
#ifdef sparc
		rv = scopy(addr, (caddr_t)reg, sizeof (struct allregs));
#else
		rv = scopy(addr, (caddr_t)reg, sizeof (struct regs));
#endif
		break;

	case PTRACE_GETREGS:
#ifdef sparc
		rv = scopy((caddr_t)reg, addr, sizeof (struct allregs));
#else
		rv = scopy((caddr_t)reg, addr, sizeof (struct regs));
#endif
		break;

	case PTRACE_WRITETEXT:
	case PTRACE_WRITEDATA:
		rv = scopy(addr2, addr, data);
		break;

	case PTRACE_READTEXT:
	case PTRACE_READDATA:
		rv = scopy(addr, addr2, data);
		break;

	case PTRACE_KILL:
	case PTRACE_ATTACH:
	case PTRACE_DETACH:
	default:
		errno = EINVAL;
		rv = -1;
		break;
	}
	return (rv);
}

/*
 * This localtime is a modified version of offtime from libc, which does not
 * bother to figure out the time zone from the kernel, from environment
 * varaibles, or from Unix files.  We just return things in GMT format.
 */

static int mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

struct tm *
localtime(clock)
	time_t *clock;
{
	register struct tm *tmp;
	register long days;
	register long rem;
	register int y;
	register int yleap;
	register int *ip;
	static struct tm tm;

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	tmp->tm_zone = "GMT";
	tmp->tm_gmtoff = 0;
	return (tmp);
}
