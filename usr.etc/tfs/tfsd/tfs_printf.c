#ifndef lint
static char sccsid[] = "@(#)tfs_printf.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifdef TFSDEBUG

#include <sys/param.h>
#include <sys/time.h>
#include <stdio.h>
#include <nse/util.h>

extern char	tfsd_log_name[];

FILE		*trace_file;
int		tfsdebug = 0;
int		print_to_screen = 1;
int		log_to_file = 1;

void		dprint();
void		trace_init();
void		trace_reset();

#ifdef notdef
void		debug_printf();
static void	dprf();
static void	dprintn();
static void	dputchar();
#endif notdef

void		log_start_req();
void		log_finish_req();
void		timeval_sum();
void		timeval_diff();


/*
 * Debugging printfs
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

/*VARARGS3*/
void
dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int             var;
	int             level;
	char           *str;
	int             a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (var == level || (var > 10 && (var - 10) >= level)) {
		if (print_to_screen) {
			xprintf(stdout, str, a1, a2, a3, a4, a5, a6, a7,
				a8, a9);
		} else if (trace_file != NULL) {
			xprintf(trace_file, str, a1, a2, a3, a4, a5, a6, a7,
				a8, a9);
		}
	}
}


void
trace_init()
{
	trace_file = fopen(tfsd_log_name, "w+");
	setlinebuf(trace_file);
	if (trace_file == NULL) {
		nse_log_message("panic: can't write to %s", tfsd_log_name);
		tfsd_perror("");
		exit(1);
	}
}


void
trace_reset()
{
	if (trace_file != NULL) {
		(void) fclose(trace_file);
	}
	trace_init();
}


/*VARARGS2*/
int
xprintf(file, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	FILE		*file;
	char           *str;
	int             a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
#ifdef notdef
	if (log_to_file) {
		fprintf(file, str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	} else {
		debug_printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}
#else
	fprintf(file, str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif notdef
}


#ifdef notdef
/*
 * Routines to printf to a circular buffer in memory.
 */

#define DEBUG_BSIZE	(16384 - 2 * sizeof (long))

struct debug_buf {
	long		debug_bufx;
	char		debug_bufc[DEBUG_BSIZE];
};

static struct debug_buf debug_buf;


/* VARARGS1 */
void
debug_printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	dprf(fmt, &x1);
}


static void
dprf(fmt, adx)
	register char *fmt;
	register u_int *adx;
{
	register int b, c, i;
	char *s;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		dputchar(c);
	}
again:
	c = *fmt++;
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
		dprintn((u_long)*adx, b);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				dputchar(c);
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			dputchar(c);
		break;

	case '%':
		dputchar('%');
		break;
	}
	adx++;
	goto loop;
}


/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
static void
dprintn(n, b)
	u_long n;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		dputchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		dputchar(*--cp);
	while (cp > prbuf);
}


static void
dputchar(c)
	int		c;
{
	debug_buf.debug_bufc[debug_buf.debug_bufx++] = c;
	if (debug_buf.debug_bufx < 0 || debug_buf.debug_bufx >= DEBUG_BSIZE)
		debug_buf.debug_bufx = 0;
	debug_buf.debug_bufc[debug_buf.debug_bufx] = '\0';
}
#endif notdef


/*
 * Routines to monitor the amount of time the TFS is busy/idle
 */

#define LOG_INTERVAL	100	/* Print busy & idle times after this #
				 * of requests */

static struct timeval idle_time;
static struct timeval busy_time;
static struct timeval last_time;
static int	num_reqs;


void
log_start_req()
{
	struct timeval	cur_time;
	struct timeval	interval;

	gettimeofday(&cur_time, (struct timezone *) NULL);
	if (last_time.tv_sec == 0) {
		dprint(tfsdebug, 7, "%d:     busy         idle\n",
			cur_time.tv_sec);
	} else {
		timeval_diff(cur_time, last_time, &interval);
		timeval_sum(idle_time, interval, &idle_time);
	}
	if (num_reqs == LOG_INTERVAL) {
		dprint(tfsdebug, 7, "%d: %4d.%06d  %4d.%06d\n",
			cur_time.tv_sec,
			busy_time.tv_sec, busy_time.tv_usec,
			idle_time.tv_sec, idle_time.tv_usec);
		bzero((char *) &busy_time, sizeof busy_time);
		bzero((char *) &idle_time, sizeof idle_time);
		num_reqs = 0;
	}
	num_reqs++;
	last_time.tv_sec = cur_time.tv_sec;
	last_time.tv_usec = cur_time.tv_usec;
}


void
log_finish_req()
{
	struct timeval	cur_time;
	struct timeval	interval;

	gettimeofday(&cur_time, (struct timezone *) NULL);
	timeval_diff(cur_time, last_time, &interval);
	timeval_sum(busy_time, interval, &busy_time);
	last_time.tv_sec = cur_time.tv_sec;
	last_time.tv_usec = cur_time.tv_usec;
}


#define MILLION		1000000
/*
 * c := a + b
 */
void
timeval_sum(a, b, c)
	struct timeval	a;
	struct timeval	b;
	struct timeval	*c;
{
	c->tv_sec = a.tv_sec + b.tv_sec;
	c->tv_usec = a.tv_usec + b.tv_usec;
	if (c->tv_usec > MILLION) {
		c->tv_sec += 1;
		c->tv_usec -= MILLION;
	}
}


/*
 * c := a - b
 */
void
timeval_diff(a, b, c)
	struct timeval	a;
	struct timeval	b;
	struct timeval	*c;
{
	if (a.tv_usec < b.tv_usec) {
		a.tv_sec -= 1;
		a.tv_usec += MILLION;
	}
	c->tv_sec = a.tv_sec - b.tv_sec;
	c->tv_usec = a.tv_usec - b.tv_usec;
}
#endif TFSDEBUG
