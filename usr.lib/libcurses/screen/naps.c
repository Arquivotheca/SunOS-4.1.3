#ifndef lint
static	char sccsid[] = "@(#)naps.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */

#include "curses.ext"
#include <signal.h>

#define NAPINTERVAL 100
#define HZ 60

/* From early specs - this may change by 4.2BSD */
struct _timeval {
	long tv_sec;
	long tv_usec;
};

/*
 * Delay the output for ms milliseconds.
 * Note that this is NOT the same as a high resolution sleep.  It will
 * cause a delay in the output but will not necessarily suspend the
 * processor.  For applications needing to sleep for 1/10th second,
 * this is not a usable substitute.  It causes a pause in the displayed
 * output, for example, for the eye wink in snake.  It is disrecommended
 * for "delay" to be much more than 1/2 second, especially at high
 * baud rates, because of all the characters it will output.  Note
 * that due to system delays, the actual pause could be even more.
 * Some games won't work decently with this routine.
 */
delay_output(ms)
int ms;
{
	extern int _outchar();		/* it's in putp.c */

	return _delay(ms*10, _outchar);
}

/*
 * napms.  Sleep for ms milliseconds.  We don't expect a particularly good
 * resolution - 60ths of a second is normal, 10ths might even be good enough,
 * but the rest of the program thinks in ms because the unit of resolution
 * varies from system to system.  (In some countries, it's 50ths, for example.)
 *
 * Here are some reasonable ways to get a good nap.
 *
 * (1) Use the select system call in Berkeley 4.2BSD.
 *
 * (2) Use the 1/10th second resolution wait in the UNIX 3.0 tty driver.
 *     It turns out this is hard to do - you need a tty line that is
 *     always unused that you have read permission on to sleep on.
 *
 * (3) Install the ft (fast timer) device in your kernel.
 *     This is a psuedo-device to which an ioctl will wait n ticks
 *     and then send you an alarm.
 *
 * (4) Install the nap system call in your kernel.
 *     This system call does a timeout for the requested number of ticks.
 *
 * (5) Write a routine that busy waits checking the time with ftime.
 *     Ftime is not present on USG systems, and since this busy waits,
 *     it will drag down response on your system.  But it works.
 */
#ifdef TIOCREMOTE
/* on 4.2BSD, use select */
napms(ms)
int ms;
{
	struct _timeval t;

	/*
	 * If your 4.2BSD select still rounds up to the next higher second,
	 * you should remove this code and install the ft driver.
	 * This routine was written under the assumption that the problem
	 * would be corrected by 4.2BSD.
	 */
	t.sec = ms/1000;
	t.usec = 1000 * (ms % 1000);
	select(0, 0, 0, 0, &t);
	return OK;
}
#else
/*
 * Pause for ms milliseconds.  Convert to ticks and wait that long.
 * Call nap, which is either defined below or a system call.
 */
napms(ms)
int ms;
{
	int ticks;
	int rv;

	ticks = ms / (1000 / HZ);
	if (ticks <= 0)
		ticks = 1;
	rv = nap(ticks);  /* call either the code below or nap system call */
	return rv;
}
#endif

#ifdef FTIOCSET
#define HASNAP
/*
 * The following code is adapted from the sleep code in libc.
 * It uses the "fast timer" device posted to USENET in Feb 1982.
 * nap is like sleep but the units are ticks (e.g. 1/60ths of
 * seconds in the USA).
 */
#include <setjmp.h>
static jmp_buf jmp;
static int ftfd;

/* don't call nap directly, you should call napms instead */
static int
nap(n)
unsigned n;
{
	int napx();
	unsigned altime;
	int (*alsig)() = SIG_DFL;
	char *ftname;
	struct requestbuf {
		short time;
		short signo;
	} rb;

	if (n==0)
		return OK;
	if (ftfd <= 0) {
		ftname = "/dev/ft0";
		while (ftfd <= 0 && ftname[7] <= '~') {
			ftfd = open(ftname, 0);
			if (ftfd <= 0)
				ftname[7] ++;
		}
	}
	if (ftfd <= 0) {	/* Couldn't open a /dev/ft? */
		sleepnap(n);
		return ERR;
	}
	altime = alarm(1000);	/* time to maneuver */
	if (setjmp(jmp)) {
		signal(SIGALRM, alsig);
		alarm(altime);
		return OK;
	}
	if (altime) {
		if (altime > n)
			altime -= n;
		else {
			n = altime;
			altime = 1;
		}
	}
	alsig = signal(SIGALRM, napx);
	rb.time = n;
	rb.signo = SIGALRM;
	ioctl(ftfd, FTIOCSET, &rb);
	for(;;)
		pause();
	/*NOTREACHED*/
}

static
napx()
{
	longjmp(jmp, 1);
}
#endif

#ifdef USG
#ifndef HASNAP
#define HASNAP
#define IDLETTY "/dev/idletty"
/*
 * Do it with the timer in the tty driver.  Resolution is only 1/10th
 * of a second.  Problem is, if the user types something while we're
 * sleeping, we wake up immediately, and have no way to tell how long
 * we should sleep again.  So we're sneaky and use a tty which we are
 * pretty sure nobody is using.
 *
 * Note that we should be able to do this by setting VMIN to 100 and VTIME
 * to the proper number of ticks.  But due to a bug in the USG tty driver
 * (this bug was still there in 5.0) this hangs until VMIN chars are typed
 * no matter how much time elapses.
 *
 * This requires some care.  If you choose a tty that is a dialup or
 * which otherwise can show carrier, it will hang and you won't get
 * any response from the keyboard.  You can use /dev/tty if you have
 * no such tty, but response will feel funny as described above.
 * To find a suitable tty, try "stty > /dev/ttyxx" for various ttyxx's
 * that look unused.  If it hangs, you can't use it.  You might try
 * connecting a cable to your port that raises carrier to keep it from hanging.
 *
 * To use this feature on USG, you must
 *	ln /dev/ttyxx /dev/idletty,
 * where /dev/ttyxx is one of your tty lines that is never used but
 * won't hang on open.  Otherwise we always return ERR.
 *
 * THIS USG CODE IS UNSUPPORTED AND ON A USE-AT-YOUR-OWN-RISK BASIS.
 */
static int
nap(ticks)
int ticks;
{
	struct termio t, ot;
	static int ttyfd;
	int n, tenths;
	char c;

	if (ttyfd == 0)
		ttyfd = open(IDLETTY, 2);
	if (ttyfd < 0) {
		sleepnap(ticks);
		return ERR;
	}
	tenths = (ticks+(HZ/10)/2) / (HZ/10); /* Round to nearest 10th second */
	ioctl(ttyfd, TCGETA, &t);
	ot = t;
	t.c_lflag &= ~ICANON;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = tenths;
	ioctl(ttyfd, TCSETAW, &t);
	n = read(ttyfd, &c, 1);
	ioctl(ttyfd, TCSETAW, &ot);
	/*
	 * Now we potentially have a character in c that somebody's going
	 * to want.  We just hope and pray they use getch, because there
	 * is no reasonable way to push it back onto the tty.
	 */
	if (n > 0) {
		for (n=0; SP->input_queue[n] >= 0; n++)
			;
		SP->input_queue[n++] = c;
		SP->input_queue[n++] = -1;
	}
	return OK;
}
#endif
#endif

/* If you have some other externally supplied nap(), add -DHASNAP to cflags */
#ifndef HASNAP
static	int
nap(ms)
int ms;
{
	sleep((ms+999)/1000);
	return ERR;
}
#endif

/*
 * Nothing better around, so we have to simulate nap with sleep.
 */
static
sleepnap(ticks)
{
	sleep((ticks+(HZ-1))/HZ);
}
