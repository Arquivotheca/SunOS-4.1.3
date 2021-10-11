/*	@(#)ttychars.h 1.1 92/07/30 SMI; from UCB 4.6 83/07/01	*/

/*
 * User visible structures and constants
 * related to terminal handling.
 */

#ifndef	_sys_ttychars_h
#define	_sys_ttychars_h

struct	ttychars {
	char	tc_erase;	/* erase last character */
	char	tc_kill;	/* erase entire line */
	char	tc_intrc;	/* interrupt */
	char	tc_quitc;	/* quit */
	char	tc_startc;	/* start output */
	char	tc_stopc;	/* stop output */
	char	tc_eofc;	/* end-of-file */
	char	tc_brkc;	/* input delimiter (like nl) */
	char	tc_suspc;	/* stop process signal */
	char	tc_dsuspc;	/* delayed stop process signal */
	char	tc_rprntc;	/* reprint line */
	char	tc_flushc;	/* flush output (toggles) */
	char	tc_werasc;	/* word erase */
	char	tc_lnextc;	/* literal next character */
};

#ifndef	CTRL
#define	CTRL(c)	('c'&037)
#endif

/*
 * default special characters.
 * guarded because termio[s].h also define these.
 */
#ifndef	CERASE
#define	CERASE	0177
#define	CKILL	CTRL(u)
#define	CINTR	CTRL(c)
#define	CQUIT	034		/* FS, ^\ */
#define	CSTART	CTRL(q)
#define	CSTOP	CTRL(s)
#define	CEOF	CTRL(d)
#define	CEOT	CEOF
#define	CBRK	0377
#define	CSUSP	CTRL(z)
#define	CDSUSP	CTRL(y)
#define	CRPRNT	CTRL(r)
#define	CFLUSH	CTRL(o)
#define	CWERASE	CTRL(w)
#define	CLNEXT	CTRL(v)
#endif	/* !CERASE */

#endif	/* !_sys_ttychars_h */
