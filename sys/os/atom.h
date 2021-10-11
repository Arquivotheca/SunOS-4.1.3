#ifndef	__ATOM_H__
#define	__ATOM_H__

/* @(#)atom.h 1.1 92/07/30 SMI
 *
 * atomic type for set and clear operations
 *
 * Defined semantics:
 *
 *   declare atoms as:
 *	atom_t	a;
 *
 *   initialization:
 *	atom_init(a);
 *
 *   allowed operations:
 *	void	atom_setup()		set up the atom package
 *	void	atom_init(atom_t)	initialize the atom
 *	void	atom_free(atom_t)	release atom storage
 *	void	atom_set(atom_t)	set atom
 *	void	atom_clr(atom_t)	clear atom
 *	int	atom_setp(atom_t)	return TRUE if atom is set
 *	int	atom_clrp(atom_t)	return TRUE if atom is clear
 *	int	atom_tas(atom_t)	set atom, return TRUE if atom was clear
 *	int	atom_tac(atom_t)	clear atom, return TRUE if atom was set
 *	void	atom_wfs(atom_t)	spin until atom is set
 *	void	atom_wfc(atom_t)	spin until atom is clear
 *	void	atom_wtas(atom_t)	spin until "tas" succeeds
 *	void	atom_wtac(atom_t)	spin until "tac" succeeds
 *
 * Please follow the requirements for using the atoms; pass them
 * through atom_init once if they are staticly allocated; if they
 * are automatic, pass them through atom_init before using them,
 * and through atom_free before letting them drop out of scope.
 * While you may think using static initialization to zero is
 * sufficient, bear in mind that this is an abstract data type
 * that may require lots and lots more fancy stuff on some platforms
 * or for some debug reasons.
 *
 * Please don't look below this line unless you are maintaining
 * the atom package itsself. Thanks.
 *
 *************************************************************************/

#ifndef	SEG_ATOMS
/* atom cell contains set or clr */
#define	ATOM_CLRV	(0)
#define	ATOM_SETV	(-1)
#else	SEG_ATOMS
/* atom cell contains null, or ptr to cell with set or clr */
#define	ATOM_CLRV	1
#define	ATOM_SETV	3
#define	ATOM_AVAIL	5
#define	ATOM_INUSE     	7
#endif	SEG_ATOMS

#ifndef	LOCORE
extern int	atoms_blown;
extern int	atoms_ready;

#ifndef	SEG_ATOMS
typedef unsigned atom_t[1];
#define	ATOM_NSOC(v)	(((v)+1)&~1)
#define	atom_at(a)	(a)
#else	SEG_ATOMS
extern int	seg_atoms;

struct s_atom {
	unsigned        lock[4];
	unsigned        busy[4];
};

#define	ATOM_NSOC(v)	(((v)-1)&~2)

#define	s_atom_at(a)	((a)->lock)
#define	s_atom_fr(a)	((a)->busy)

#define	atom_at(a)	(!seg_atoms ? ((unsigned *)(a)) : s_atom_at(*(a)))
#define	atom_fr(a)	(!seg_atoms ? ((unsigned *)(a)) : s_atom_fr(*(a)))

typedef struct s_atom *(atom_t[1]);

#endif	SEG_ATOMS

/* we need this to work, but only on atom_at(a) and atom_fr(a). */
extern unsigned	swapl();	/* (unsigned value, unsigned *vaddr) */

/* Macros, to define how we manipulate atoms. */

#define	ATOM_DECP(a)	register unsigned *p; p = atom_at(a);
#define	ATOM_DECC(a)	register unsigned *p, ct; p = atom_at(a); ct = 0;

#define	ATOM__SET(p)   { *(p) = ATOM_SETV; }
#define	ATOM__CLR(p)   { *(p) = ATOM_CLRV; }
#define	ATOM__SETP(p)	(*(p) == ATOM_SETV)
#define	ATOM__CLRP(p)	(*(p) == ATOM_CLRV)
#define	ATOM__TAS(p)	(swapl(ATOM_SETV, (p)) == ATOM_CLRV)
#define	ATOM__TAC(p)	(swapl(ATOM_CLRV, (p)) == ATOM_SETV)

#define	ATOM_SET(a)    { ATOM_DECP(a); ATOM__SET(p);}
#define	ATOM_CLR(a)    { ATOM_DECP(a); ATOM__CLR(p);}
#define	ATOM_SETP(a)	(ATOM__SETP(atom_at(a)))
#define	ATOM_CLRP(a)	(ATOM__CLRP(atom_at(a)))
#define	ATOM_TAS(a)	(ATOM__TAS(atom_at(a)))
#define	ATOM_TAC(a)	(ATOM__TAC(atom_at(a)))

#define	ATOM_WFS(a)    { ATOM_DECP(a); while(!ATOM__SETP(p));}
#define	ATOM_WFC(a)    { ATOM_DECP(a); while(!ATOM__CLRP(p));}
#define	ATOM_WSC(a)    { ATOM_DECP(a); while(!ATOM__SETP(p) || !ATOM__TAC(p));}
#define	ATOM_WCS(a)    { ATOM_DECP(a); while(!ATOM__CLRP(p) || !ATOM__TAS(p));}

#define	ATOM_WFSI(a,c) { ATOM_DECC(a); while(!ATOM__SETP(p)) ct++; *(c) += ct;}
#define	ATOM_WFCI(a,c) { ATOM_DECC(a); while(!ATOM__CLRP(p)) ct++; *(c) += ct;}
#define	ATOM_WSCI(a,c) { ATOM_DECC(a); while(!ATOM__SETP(p) || !ATOM__TAC(p)) ct++; *(c) += ct;}
#define	ATOM_WCSI(a,c) { ATOM_DECC(a); while(!ATOM__CLRP(p) || !ATOM__TAS(p)) ct++; *(c) += ct;}

#ifndef	ATOM_PARANOIA
#define	ATOM_BAD(a)	(0)
#else	ATOM_PARANOIA
#define	ATOM_SOCP(a)	(!ATOM_NSOC(atom_at(a)[0]))
#define	ATOM_AOK(a)	(((a) && atom_at(a)) || atom_panic("naok", __FILE__, __LINE__))
#define	ATOM_DOK(a)	(ATOM_SOCP(a) || atom_panic("ndok", __FILE__, __LINE__))
#define	ATOM_RDY	(atoms_ready || atom_panic("nrdy", __FILE__, __LINE__))
#define	ATOM_BAD(a)	(atoms_blown || !ATOM_RDY || !ATOM_AOK(a) || !ATOM_DOK(a))
#endif	ATOM_PARANOIA

/* declearations for the function entries */
extern int	atom_panic();

extern void	atom_setup();
extern void	atom_init();
extern void	atom_free();

extern int	atom_setp();
extern int	atom_clrp();
extern int	atom_tas();
extern int	atom_tac();
extern void	atom_set();
extern void	atom_clr();

extern void	atom_wfs();
extern void	atom_wfc();
extern void	atom_wsc();
extern void	atom_wcs();

extern void	atom_wfsi();
extern void	atom_wfci();
extern void	atom_wsci();
extern void	atom_wcsi();

#ifndef	lint
/*
 * macro implementation for raw speed
 * note: always lint against using
 * the real functions, to avoid lots
 * of lint blather about constants in
 * conditional contexts.
 */
#define	atom_setp(a)	(ATOM_BAD(a) || ATOM_SETP(a))
#define	atom_clrp(a)	(ATOM_BAD(a) || ATOM_CLRP(a))
#define	atom_tas(a)	(ATOM_BAD(a) || ATOM_TAS(a))
#define	atom_tac(a)	(ATOM_BAD(a) || ATOM_TAC(a))
#define	atom_set(a)	if(ATOM_BAD(a));else ATOM_SET(a)
#define	atom_clr(a)	if(ATOM_BAD(a));else ATOM_CLR(a)
#define	atom_wfs(a)	if(ATOM_BAD(a));else ATOM_WFS(a)
#define	atom_wfc(a)	if(ATOM_BAD(a));else ATOM_WFC(a)
#define	atom_wsc(a)	if(ATOM_BAD(a));else ATOM_WSC(a)
#define	atom_wcs(a)	if(ATOM_BAD(a));else ATOM_WCS(a)
#define	atom_wfsi(a,c)	if(ATOM_BAD(a));else ATOM_WFSI(a,c)
#define	atom_wfci(a,c)	if(ATOM_BAD(a));else ATOM_WFCI(a,c)
#define	atom_wsci(a,c)	if(ATOM_BAD(a));else ATOM_WSCI(a,c)
#define	atom_wcsi(a,c)	if(ATOM_BAD(a));else ATOM_WCSI(a,c)
#endif
#endif LOCORE

#endif	/* __ATOM_H__ */
