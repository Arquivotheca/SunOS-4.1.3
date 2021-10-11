#ifndef lint
static char     sccsid[] = "@(#)kern_lock.c 1.1 92/07/30 SMI";
#endif

#include <sys/param.h>

#ifdef	MULTIPROCESSOR

#include <sys/user.h>
#include <os/baton.h>
#include <os/dlyprf.h>

/*
 * Kernel Lock Stuff
 */

baton_t         baton;

extern int	cpuid;

/*
 * a key for the lock: who locked the baton and why
 */
#define	BATONTRACKLEVEL	32
#ifndef	BATON_SOLID
who_t		baton_who[BATONTRACKLEVEL];
why_t		baton_why[BATONTRACKLEVEL];
#endif	BATON_SOLID

/*
 * Number of procs that want the baton
 */
int		baton_want = 0;

/*
 * local functions that need pre-declaration
 */
static void baton_incr();
static void baton_decr();

/*
 * variables that need predeclaration
 */
atom_t		bpanic_buf_lock;

/************************************************************************
 *	EXPORTED ROUTINES
 ************************************************************************/

/*
 * baton_init: grab a baton out of the box.
 *
 * We need to clean the grease off it, but when we are done own it at depth 1
 * and nobody else is waiting for it yet.
 */
/*VARARGS*/
void
baton_init(who, why)
	who_t	who;
	why_t	why;
{
	atom_init(bpanic_buf_lock);
	mutex_init(baton.access, 15);
	mutex_enter(baton.access);
	baton.depth = 0;
	cond_init(baton.waiting, 15);
	baton_incr(who, why);
	mutex_exit(baton.access);
}

/*
 * baton_lock: grab the baton.
 */
void
baton_lock(who, why)
	who_t	who;
	why_t	why;
{
	mutex_enter(baton.access);
	baton_want ++;
	while ((baton.owner != cpuid) && (baton.depth > 0))
		(void)cond_wait(baton.waiting, baton.access, PZERO-1);
	baton_want --;
	baton_incr (who, why);
	mutex_exit(baton.access);
}

/*
 * baton_free: give away the baton.
 */
void
baton_free(who, why)
	who_t	who;
	why_t	why;
{
	mutex_enter(baton.access);
	baton_decr (who, why);
	mutex_exit(baton.access);
}

/*
 * baton_trylock: try to grab the baton. returns true if we got the lock.
 * this can in fact block trying to enter baton.access, so don't call it
 * from above "splmutex" (currently only unsafe for level 14 and 15 ints).
 */
int
baton_trylock(who, why)
	who_t	who;
	why_t	why;
{
	register int rv;
	
	mutex_enter(baton.access);
	if ((baton.depth < 1) || (baton.owner == cpuid)) {
		baton_incr(who, why);
		rv = 1;
	} else
		rv = 0;
	mutex_exit(baton.access);
	return rv;
}

/*
 * baton_chklock: try to grab the baton. returns true if we got the lock.
 * this can in fact block chking to enter baton.access, so don't call it
 * from above "splmutex" (currently only unsafe for level 14 and 15 ints).
 * very similar to baton_trylock, but fails if anyone is waiting for us.
 */
int
baton_chklock(who, why)
	who_t	who;
	why_t	why;
{
	register int rv;

	mutex_enter(baton.access);
	if ((baton_want < 1) &&
	    ((baton.depth < 1) ||
	     (baton.owner == cpuid))) {
		baton_incr (who, why);
		rv = 1;
	} else
		rv = 0;
	mutex_exit(baton.access);
	return rv;
}

/*
 * baton_relock: change the who and why of the current lock
 * to the specified who and why.
 */
void
baton_relock(who, why)
	who_t	who;
	why_t	why;
{
#ifndef	BATON_SOLID
	mutex_enter(baton.access);
#ifdef	BATON_PARANOID
	if ((baton.owner == cpuid) && (baton.depth > 0)) {
#endif	BATON_PARANOID
		baton.depth --;
		baton_incr(who, why);
#ifdef	BATON_PARANOID
	}
#endif	BATON_PARANOID
	mutex_exit(baton.access);
#endif	BATON_SOLID
}

/*
 * baton_require: assert that the baton is held at a specific depth. Normally
 * used to assert that it is unheld or at precisely one; if held by another
 * cpu, consider it unheld for this purpose.
 */
void	/*ARGSUSED*/
baton_require (module, where, req)
	char *module;
	char *where;
	int req;
{
#ifdef	BATON_PARANOIA
	who_t	who = ANY_WHO;
	why_t	why = ANY_WHY;
	int	depth;

	depth = baton_held (&who, &why, 1);

	if ((req == -1) && (depth > 0))
		return;		/* req==-1: ok if held at any depth */

	if (depth != req)
		bpanic("%s %s: baton requirement (%d) failed",
		       module, where, req);
#endif	BATON_PARANOIA
}

/*
 * baton_held: report information on how the baton is held.
 * return value is the current depth; lists of "who" and "why"
 * are returned in "whop" and "whyp".
 */
int
baton_held(whop, whyp, asize)
	who_t  *whop;
	why_t  *whyp;
	int 	asize;
{
	int rv = 0;

	mutex_enter (baton.access);
	if (baton.owner == cpuid) {
#ifndef	BATON_SOLID
		for (rv = 0; rv < asize; ++rv) {
			if (rv < baton.depth) {
				whop[rv] = baton_who[rv];
				whyp[rv] = baton_why[rv];
			} else {
				whop[rv] = 0;
				whyp[rv] = 0;
			}
		}
#endif	BATON_SOLID
		rv = baton.depth;
	}
	mutex_exit (baton.access);

	return rv;
}

/*
 * baton_intlock: possibly acquire the kernel in response to
 * an interrupt, possibly forwarding the interrupt to the
 * current master. Returns "true" if the service should *NOT*
 * be called.
 *
 * NOTE: level is 1..15 for hardints, 17..31 for softints.
 */

static char	batn_intcode[] = "nHHHHHHHHHHHHHnnSSSSSSSSSSnnnnnn";

int
baton_intlock(service, level)
	who_t	service;
	why_t	level;
{
	register int	redirect_int;

	switch(batn_intcode[level]) {
	case 'S':		/* soft 1..9: forward to baton holder*/
		mutex_enter(baton.access);
		if ((baton.depth < 1) || (baton.owner == cpuid)) {
			baton_incr (service, level);
			redirect_int = 0;
		} else {
			send_dirint(baton.owner, (int)level&15);
			redirect_int = 1;
		}
		mutex_exit(baton.access);
		return redirect_int;

	case 'H':		/* hard 1..13: do it here */
		return !baton_trylock(service, level);
	}

	return 0;
}

/*
 * baton_intfree: corresponding free routine for baton_intlock.
 * Unlocks the baton if baton_intlock locked it.
 * Level is 1..15 for hardints, 17..31 for softints.
 * YES, WE MUST KNOW WHETHER THIS IS A HARD OR SOFT INT.
 */

void
baton_intfree(service, level)
	who_t	service;
	why_t	level;
{
	if (batn_intcode[level] != 'n')
		baton_free (service, level);
}

/*
 * baton_spin: acquire the baton without sleeping or swtching,
 * and without hogging the baton access lock
 */
/*VARARGS*/
void
baton_spin (who, why)
	who_t	who;
	why_t	why;
{
	while (!baton_trylock (who, why))
		while (baton.depth > 0)
			; /* spin until we see depth drop to zero */
}


/* ********************************************************************
 * baton_swtch: replacement for swtch that knows how and when
 * to release and reacquire the baton. Call this wherever
 * you call 'swtch' if you might hold the baton. Note that with
 * the possibility of taking a trap from inside the baton we can
 * have a nested baton during a swtch.
 */
#ifndef	BATON_SOLID
#define	BATON_SWTCH_MAX_NEST	3
#endif	BATON_SOLID

void
baton_swtch()
{
	int locked, ix;
#ifndef	BATON_SOLID
	who_t	who[BATON_SWTCH_MAX_NEST];
	why_t	why[BATON_SWTCH_MAX_NEST];
#endif	BATON_SOLID

	mutex_enter(baton.access);
	locked = baton.depth;
	if ((baton.owner == cpuid) && (locked > 0)) {
#ifdef	BATON_PARANOIA
		if (baton.depth > BATON_SWTCH_MAX_NEST) {
			bpanic("baton: too deeply nested");
			real_swtch();
			return;
		}
#endif	BATON_PARANOIA
#ifndef	BATON_SOLID
		for (ix = 0; ix < locked; ++ix) {
			who[ix] = baton_who[ix];
			why[ix] = baton_why[ix];
		}
#endif	BATON_SOLID
		baton.depth = 0;
		if (baton_want)
			cond_signal(baton.waiting);
	} else
		locked = 0;
	mutex_exit(baton.access);

	if (!locked) {
		real_swtch();
		return;
	}
	real_swtch();

	mutex_enter(baton.access);
	baton_want ++;
	while ((baton.owner != cpuid) && (baton.depth > 0))
		(void)cond_wait(baton.waiting, baton.access, PZERO - 1);
	if (baton.owner != cpuid) {
		take_interrupt_target();
		baton.owner = cpuid;
	}
#ifndef	BATON_SOLID
	for (ix = 0; ix < locked; ++ix) {
		baton_who[ix] = who[ix];
		baton_why[ix] = why[ix];
	}
#endif	BATON_SOLID
	baton.depth = locked;
	baton_want --;
	mutex_exit(baton.access);
}

/*
 * baton_idle: the current processor is idle.
 * A future optimisation might involve moving
 * the interrupt target register to the idle
 * processor; when this is done, take care not
 * to move it too fast: if it gets to oscillating
 * between two processors, an interrupt may never
 * actually get processed.
 */
int
baton_idle()
{
#ifdef	BATON_PARANOID
	ccheck(__FILE__, __LINE__);
#endif	BATON_PARANOID
}

/************************************************************************
 *	PRIVATE ROUTINES
 ************************************************************************/
void
baton_dump()
{
	extern void mutex_dump();
	extern void cond_dump();
	register int	i;

	romprintf("baton state information:\n");
	romprintf("baton owned by %d at depth %d; %d reservations\n",
	       baton.owner, baton.depth, baton_want);
	mutex_dump("baton.access", baton.access);
	cond_dump("baton.waiting", baton.waiting);

#ifndef	BATON_SOLID
	romprintf("CURRENT BATON HOLD STACK (including stale data):\n");
	for  (i=0; i<BATONTRACKLEVEL; ++i)
		if (baton_who[i])
			romprintf("%3d. 0x%x 0x%x\n", i, baton_who[i], baton_why[i]);
#endif	BATON_SOLID
}

#ifdef	PROM_PARANOIA
extern void prom_enter_mon();
#endif	PROM_PARANOIA

char		bpanic_buf[4096];

/*
 * bpanic: shut down kernel baton lock, maybe bpanic.
 * If no bpanic, leave the baton code in a transparent state.
 */
/*VARARGS1*/
int
bpanic(fmt, a, b, c, d)
char *fmt;
{
	extern char *sprintf();

	trigger_logan();
#ifdef	DLYPRF
	dlyprf("baton bpanic %08X.%08X %d+%d",
	       baton_who[0],baton_why[0],baton.owner,baton.depth);
	dlyprf(fmt, a, b, c, d);

	dpenable = 0;
#endif
	atom_wcs(bpanic_buf_lock);
	romprintf("bpanic: '%s' %x %x %x %x\n",
		  fmt, a, b, c, d);
	(void)sprintf(bpanic_buf, fmt, a, b, c, d);
	romprintf("bpanic on %d: %s\n", cpuid, bpanic_buf);
	atom_clr(bpanic_buf_lock);
#ifdef	PROM_PARANOIA
romprintf("stack may be at %x or %x\n",
stackpointer(), framepointer());
prom_enter_mon();
#endif	PROM_PARANOIA

	baton_dump();
	panic(fmt);
	cond_broadcast (baton.waiting);
}

/*
 * baton_incr: increment the depth of the baton. handles setting the
 * processor number correctly, incrementing the depth, and logging
 * who and why the baton is being locked.
 */
static void
baton_incr (who, why)
	who_t	who;
	why_t	why;
{
#ifdef	BATON_PARANOIA
	if (baton.depth >= BATONTRACKLEVEL)
		bpanic("baton_incr: too deep");

	if ((baton.owner != cpuid) && (baton.depth > 0))
		bpanic("baton_incr: not owner");
#endif	BATON_PARANOIA

	if (baton.owner != cpuid) {
		take_interrupt_target();
		baton.owner = cpuid;
	}
	baton.depth++;
#ifndef	BATON_SOLID
	baton_who[baton.depth-1] = who;
	baton_why[baton.depth-1] = why;
#endif	BATON_SOLID
}

/*
 * baton_decr: decrement the depth of the baton. handles decrementing
 * the depth, and checking that the correct incarnation of the lock
 * is being released (ie. check who and why)
 */
static void
baton_decr (who, why)
	who_t	who;
	why_t	why;
{
#ifdef	BATON_PARANOIA
	if (baton.owner != cpuid)
		bpanic("baton_decr: not owner (i am %d, held by %d)",
		       cpuid, baton.owner);
	if (baton.depth < 1)
		bpanic("baton_decr: not held");

	if (baton.depth <= BATONTRACKLEVEL) {
#endif	BATON_PARANOIA
#ifndef	BATON_SOLID
		if ((who != ANY_WHO) &&
		    (baton_who[baton.depth-1] != who))
			bpanic("baton: who mismatch (exp=%x, act=%x)",
			       baton_who[baton.depth-1], who);
		if ((why != ANY_WHY) &&
		    (baton_why[baton.depth-1] != why))
			bpanic("baton: why mismatch (exp=%x, act=%x)",
			       baton_why[baton.depth-1], why);
#endif	BATON_SOLID
#ifdef	BATON_PARANOIA
	}
#endif	BATON_PARANOIA

	baton.depth--;
	if (baton_want && (baton.depth < 1))
		cond_signal(baton.waiting);
}


/*
 * baton_steal: grab the baton, no matter who owns it.
 * only called from panic after all cpus are safely
 * and permanently tucked away in xc_serv. Once the
 * baton is acquired with this, it should not be
 * released (although we may release acquisitions
 * nested on top of this acquire).
 */
baton_t         broken_baton;
#ifndef	BATON_SOLID
who_t		broken_baton_who[BATONTRACKLEVEL];
why_t		broken_baton_why[BATONTRACKLEVEL];
#endif	BATON_SOLID

baton_steal(who, why)
	who_t	who;
	why_t	why;
{
#ifndef	BATON_SOLID
	register int ix;
	for (ix=0; ix<BATONTRACKLEVEL; ++ix) {
		broken_baton_who[ix] = baton_who[ix];
		broken_baton_why[ix] = baton_why[ix];
	}
#endif	BATON_SOLID
	broken_baton = baton;

	baton.depth = 0;				/* jimmy the lock */
	baton_incr(who, why);				/* get the lock */
	atom_clr(baton.access->atom);			/* prevent deadlocks */
}
#endif	MULTIPROCESSOR
