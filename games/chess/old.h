/*	@(#)old.h 1.1 92/07/30 SMI */

#define	uleft	04040
#define	uright	04004
#define	dleft	00440
#define	dright	00404
#define	left	00040
#define	right	00004
#define	up	04000
#define	down	00400
#define	u2r1	06004
#define	u1r2	04006
#define	d1r2	00406
#define	d2r1	00604
#define	d2l1	00640
#define	d1l2	00460
#define	u1l2	04060
#define	u2l1	06040
#define	rank2	00200
#define	rank7	02000

short	attacv[64];
short	center[64];
short	(*wheur[])();
short	(*bheur[])();
short	control[64];
short	clktim[2];
short	testf;
short	qdepth;
short	mdepth;
int	bookf;
short	bookp;
short	manflg;
short	matflg;
short	intrp;
short	moveno;
short	gval;
short	game;
short	abmove;
short	*lmp;
short	*amp;
char	*sbufp;
short	lastmov;
short	mantom;
short	ply;
short	value;
short	ivalue;
short	mfmt;
short	depth;
short	flag;
short	eppos;
short	bkpos;
short	wkpos;
short	column;
short	edge[8];
short	pval[13];
short	ipval[13];
short	dir[64];
short	board[64];
short	lmbuf[1000];
/*short	ambuf[1200];*/
short	ambuf[3000];
char	sbuf[100];

short *done(),*statl();
