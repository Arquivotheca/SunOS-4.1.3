#ident	"@(#)kern_atom.c 1.1 92/07/30 SMI"

/*LINTLIBRARY*/

#include <os/atom.h>

extern int      cpuid;

int             atoms_blown = 0;
int             atoms_ready = 0;
int             atoms_verbose = 0;

#ifdef	SEG_ATOMS
int             seg_atoms = 1;		/* 0=inplace 1=segregated */
#endif	SEG_ATOMS

#ifdef	SEG_ATOMS
#ifndef	PAGESIZE
#define	PAGESIZE	4096
#endif	PAGESIZE

#define	ATOMS_PER_PAGE	(PAGESIZE / sizeof (struct s_atom))
#define	MINATOMS	ATOMS_PER_PAGE
#define	PADATOMS	(MINATOMS + ATOMS_PER_PAGE)

struct s_atom   atompool[PADATOMS];
struct s_atom  *atoms = atompool;
struct s_atom  *eatoms = atompool + PADATOMS;

#endif	SEG_ATOMS

/*
 * hidden implementation details for the atom package.
 * note that currently everything is really spelled out
 * in the .h file so using atoms is as fast as possible,
 * so many details that should be hidden in here are
 * in fact exported to the world. Can we trust the
 * world not to look at atom.h? <sigh>
 *
 * Since we produce a bunch of functions here that may
 * or may not be used at any given time, declare this
 * to be LINTLIBRARY.
 */

#define	APANIC(msg)	atom_panic(msg, __FILE__, __LINE__)

/*ARGSUSED*/
atom_panic(msg, fn, ln)
	char           *msg;
	char           *fn;
	int             ln;
{
	if (atoms_blown)
		return;
	atoms_blown = 1;
#ifdef	PROM_PARANOIA
	mpprom_eprintf("atom_panic: %s(%d): %s\n",
		  fn, ln, msg);
#endif	PROM_PARANOIA
	panic(msg);
}

int	/*ARGSUSED*/
atom_check(a)
	atom_t          a;
{
#ifdef	ATOM_PARANOIA
	register unsigned *p;
	register unsigned v;

	if (atoms_blown)
		return 1;
	if (!a)
		APANIC("null atom");
	p = atom_at(a);
	if (!p)
		APANIC("uninit atom");
	v = *p;
	if ((v != ATOM_SETV) && (v != ATOM_CLRV))
		APANIC("junk data");
#endif	ATOM_PARANOIA
	return 0;
}

/*
 * define this before we start fiddling
 * with "#undef".
 */
char *
atom_dump(a)
	atom_t		a;
{
	return (atom_setp(a) ? "set" :
		atom_clrp(a) ? "clear" :
		"junk");
}

#ifdef	TLBLOCK_ATOMS
int             tlblock_atoms = 1;
int             tlblock_atoms_verbose = 0;

void
atom_tlblock()
{
	extern int      cpuid;
	extern int      mmu_ltic();
	register int    lc;

	if (!tlblock_atoms)
		return;
	if (!seg_atoms)
		return;
	if (tlblock_atoms_verbose)
		printf("atom_tlblock: locking down %x..%x\n", atoms, eatoms);
	lc = mmu_ltic(atoms, eatoms);
	if (tlblock_atoms_verbose)
		if (lc)
			printf("atom_tlblock: cpu%d locked %d pages of atoms in tlb\n",
				  cpuid, lc);
		else
			printf("atom_tlblock: cpu%d unable to lock atoms in tlb\n",
				  cpuid);
}
#endif	TLBLOCK_ATOMS

/*
 * always provide function equivalents,
 * but keep their actions in sync with
 * the macros.
 */

#undef	atom_setup
void
atom_setup()
{
#ifdef	SEG_ATOMS
	register struct s_atom *p;

	if (seg_atoms) {
#ifndef	lint
		if (atoms_verbose)
			printf("cpu%d/atom_setup: atom pool in %x..%x\n",
				  cpuid, atoms, eatoms);
		atoms = (struct s_atom *) /* round up to page */
			(((unsigned) atoms + 0xFFF) & ~0xFFF);
		eatoms = (struct s_atom *) /* round down to page */
			(((unsigned) eatoms) & ~0xFFF);
#endif	lint
		if (atoms_verbose)
			printf("cpu%d/atom_setup: atom pool is %x..%x via pte=%x\n",
				  cpuid, atoms, eatoms, mmu_probe(atoms));
		for (p = atoms; p < eatoms; ++p)
			(void) swapl(ATOM_AVAIL, s_atom_fr(p));
	}
#endif	SEG_ATOMS
	atoms_ready = 1;
}

#undef	atom_init
void
atom_init(a)
	atom_t          a;
{
	if (atoms_blown)
		return;
#ifdef	SEG_ATOMS
	if (seg_atoms) {
		register struct s_atom *p;

		p = atoms;
		while (1) {
			register unsigned *fp;
			register unsigned rv;

			fp = s_atom_fr(p);

			rv = *fp;
			if (rv == ATOM_AVAIL) {
				rv = swapl(ATOM_INUSE, s_atom_fr(p));
				if (rv == ATOM_AVAIL) {
					*a = p;
#endif	SEG_ATOMS
					if (atoms_verbose)
						printf("cpu%d/atom at %x gets hunk at %x\n",
							  cpuid, a, atom_at(a));
#ifdef	SEG_ATOMS
					break;
				}
			}
#ifdef	ATOM_PARANOIA
			if (rv != ATOM_INUSE)
				APANIC("atom_init: junk");
#endif	ATOM_PARANOIA
			++p;
#ifdef	ATOM_PARANOIA
			if (p >= eatoms)
				APANIC("atom_init: overflow");
#endif	ATOM_PARANOIA
		}
	}
#endif	SEG_ATOMS
	atom_at(a)[0] = ATOM_CLRV;
}

#undef	atom_free
void
atom_free(a)
	atom_t          a;
{
	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	if (atoms_verbose)
		printf("cpu%d/atom at %x frees hunk at %x\n",
			  cpuid, a, atom_at(a));
#ifdef	SEG_ATOMS
	if (seg_atoms) {
		register unsigned *p;
#ifdef	ATOM_PARANOIA
		register unsigned rv;
#endif	ATOM_PARANOIA
		p = atom_fr(a);	/* locate "free" word */
		*a = 0;		/* detach hunk from atom */
#ifndef	ATOM_PARANOIA
		*p = ATOM_AVAIL; /* mark hunk as free */
#else	ATOM_PARANOIA
		rv = swapl(ATOM_AVAIL, p); /* mark hunk as free */
		if (rv == ATOM_AVAIL)
			APANIC("atom_free: dup");
		if (rv != ATOM_INUSE)
			APANIC("atom_free: junk");
#endif	ATOM_PARANOIA
	}
#endif	SEG_ATOMS
}

#undef	atom_setp
int
atom_setp(a)
	atom_t          a;
{
	register unsigned *p;
	register int    rv;

	if (atoms_blown)
		return 1;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return 1;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	rv = *p;
	if (rv == ATOM_SETV)
		return 1;
#ifdef	ATOM_PARANOIA
	if (rv != ATOM_CLRV)
		APANIC("atom_setp: junk");
#endif	ATOM_PARANOIA
	return 0;
}

#undef	atom_clrp
int
atom_clrp(a)
	atom_t          a;
{
	register unsigned *p;
	register int    rv;

	if (atoms_blown)
		return 1;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return 1;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	rv = *p;
	if (rv == ATOM_CLRV)
		return 1;
#ifdef	ATOM_PARANOIA
	if (rv != ATOM_SETV)
		APANIC("atom_clrp: junk");
#endif	ATOM_PARANOIA
	return 0;
}

#undef	atom_tas
int
atom_tas(a)
	atom_t          a;
{
	register unsigned *p;
	register int    rv;

	if (atoms_blown)
		return 1;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return 1;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	rv = swapl(ATOM_SETV, p);
	if (rv == ATOM_CLRV)
		return 1;
#ifdef	ATOM_PARANOIA
	if (rv != ATOM_SETV)
		APANIC("atom_tas: junk");
#endif	ATOM_PARANOIA
	return 0;
}

#undef	atom_tac
int
atom_tac(a)
	atom_t          a;
{
	register unsigned *p;
	register int    rv;

	if (atoms_blown)
		return 1;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return 1;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	rv = swapl(ATOM_CLRV, p);
	if (rv == ATOM_SETV)
		return 1;
#ifdef	ATOM_PARANOIA
	if (rv != ATOM_SETV)
		APANIC("atom_tac: junk");
#endif	ATOM_PARANOIA
	return 0;
}

#undef	atom_set
void
atom_set(a)
	atom_t          a;
{
	register unsigned *p;
#ifdef	ATOM_PARANOIA
	register unsigned rv;
#endif	ATOM_PARANOIA

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
#ifndef	ATOM_PARANOIA
	*p = ATOM_SETV;
#else	ATOM_PARANOIA
	rv = swapl(ATOM_SETV, p);
	if ((rv != ATOM_SETV) && (rv != ATOM_CLRV))
		APANIC("atom_set: junk");
#endif	ATOM_PARANOIA
}

#undef	atom_clr
void
atom_clr(a)
	atom_t          a;
{
	register unsigned *p;
#ifdef	ATOM_PARANOIA
	register unsigned rv;
#endif	ATOM_PARANOIA

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
#ifndef	ATOM_PARANOIA
	*p = ATOM_CLRV;
#else	ATOM_PARANOIA
	rv = swapl(ATOM_CLRV, p);
	if ((rv != ATOM_SETV) && (rv != ATOM_CLRV))
		APANIC("atom_set: junk");
#endif	ATOM_PARANOIA
}

#undef	atom_wfs
void
atom_wfs(a)
	atom_t          a;
{
	register unsigned *p;
	register unsigned rv;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	while (1) {
		rv = *p;
		if (rv == ATOM_SETV)
			break;
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_CLRV)
			APANIC("atom_wfs: junk");
#endif	ATOM_PARANOIA
	}
}

#undef	atom_wfc
void
atom_wfc(a)
	atom_t          a;
{
	register unsigned *p;
	register unsigned rv;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	while (1) {
		rv = *p;
		if (rv == ATOM_CLRV)
			break;
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_SETV)
			APANIC("atom_wfc: junk");
#endif	ATOM_PARANOIA
	}
}

#undef	atom_wsc
void
atom_wsc(a)
	atom_t          a;
{
	register unsigned *p;
	register unsigned rv;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	while (1) {
		rv = *p;
		if (rv == ATOM_SETV) {
			rv = swapl(ATOM_CLRV, p);
			if (rv == ATOM_SETV)
				break;
		}
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_CLRV)
			APANIC("atom_wsc: junk");
#endif	ATOM_PARANOIA
	}
}

#undef	atom_wcs
void
atom_wcs(a)
	atom_t          a;
{
	register unsigned *p;
	register unsigned rv;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	while (1) {
		rv = *p;
		if (rv == ATOM_CLRV) {
			rv = swapl(ATOM_SETV, p);
			if (rv == ATOM_CLRV)
				break;
		}
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_SETV)
			APANIC("atom_wcs: junk");
#endif	ATOM_PARANOIA
	}
}

#undef	atom_wfsi
void
atom_wfsi(a, c)
	atom_t          a;
	int            *c;
{
	register unsigned *p;
	register unsigned rv;
	register int    ct;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	ct = 0;
	while (1) {
		rv = *p;
		if (rv == ATOM_SETV)
			break;
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_CLRV)
			APANIC("atom_wfsi: junk");
#endif	ATOM_PARANOIA
		ct++;
	}
	*c += ct;
}

#undef	atom_wfci
void
atom_wfci(a, c)
	atom_t          a;
	int            *c;
{
	register unsigned *p;
	register unsigned rv;
	register int    ct;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	ct = 0;
	while (1) {
		rv = *p;
		if (rv == ATOM_CLRV)
			break;
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_SETV)
			APANIC("atom_wfci: junk");
#endif	ATOM_PARANOIA
		ct++;
	}
	*c += ct;
}

#undef	atom_wsci
void
atom_wsci(a, c)
	atom_t          a;
	int            *c;
{
	register unsigned *p;
	register unsigned rv;
	register int    ct;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	ct = 0;
	while (1) {
		rv = *p;
		if (rv == ATOM_SETV) {
			rv = swapl(ATOM_CLRV, p);
			if (rv == ATOM_SETV)
				break;
		}
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_CLRV)
			APANIC("atom_wsci: junk");
#endif	ATOM_PARANOIA
		ct++;
	}
	*c += ct;
}

#undef	atom_wcsi
void
atom_wcsi(a, c)
	atom_t          a;
	int            *c;
{
	register unsigned *p;
	register unsigned rv;
	register int    ct;

	if (atoms_blown)
		return;
#ifdef	ATOM_PARANOIA
	if (atom_check(a))
		return;
#endif	ATOM_PARANOIA
	p = atom_at(a);
	ct = 0;
	while (1) {
		rv = *p;
		if (rv == ATOM_CLRV) {
			rv = swapl(ATOM_SETV, p);
			if (rv == ATOM_CLRV)
				break;
		}
#ifdef	ATOM_PARANOIA
		if (rv != ATOM_SETV)
			APANIC("atom_wsci: junk");
#endif	ATOM_PARANOIA
		ct++;
	}
	*c += ct;
}
