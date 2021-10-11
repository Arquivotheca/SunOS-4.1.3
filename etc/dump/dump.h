/*	@(#)dump.h 1.1 92/07/30 SMI; from UCB 5.2 5/28/86	*/

#define	NI		16
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define MAXNINDIR	(MAXBSIZE / sizeof(daddr_t))

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/wait.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <protocols/dumprestore.h>
#include <ufs/fsdir.h>
#include <utmp.h>
#include <signal.h>
#include <fstab.h>

#define	MWORD(m,i)	(m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)		(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

u_int	msiz;
char	*clrmap;
char	*dirmap;
char	*nodmap;

/*
 *	All calculations done in 0.1" units!
 */

char	*disk;		/* name of the disk file */
char	*tape;		/* name of the tape file */
char	*increm;	/* name of the file containing incremental information*/
char	*temp;		/* name of the file for doing rewrite of increm */
char	lastincno;	/* increment number of previous dump */
char	incno;		/* increment number */
int	uflag;		/* update flag */
int	fi;		/* disk file descriptor */
int	to;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
ino_t	ino;		/* current inumber; used globally */
int	nsubdir;
int	newtape;	/* new tape flag */
int	nadded;		/* number of added sub directories */
int	dadded;		/* directory added flag */
int	density;	/* density in 0.1" units */
long	tsize;		/* tape size in 0.1" units */
long	esize;		/* estimated tape size, blocks */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */

int	verify;		/* verify each volume */
int	doingverify;	/* true => doing a verify pass */
int	archive;	/* true => saving a archive in archivefile */
char	*archivefile;	/* name of archivefile */
int	notify;		/* notify operator flag */
int	writing_eom;	/* true if writing end of media record */
int	diskette;	/* true if dumping to a diskette */

int	blockswritten;	/* number of blocks written on current tape */
int	tapeno;		/* current tape number */
time_t	telapsed;	/* time spent writing previous tapes */
time_t	tstart_writing;	/* when we started writing the latest tape */
struct fs *sblock;	/* the file system super block */
char	buf[MAXBSIZE];

time_t	time();
char 	*malloc(), *calloc();
char	*ctime();
char	*prdate();
long	atol();
int	mark();
int	add();
int	dirdump();
int	dump();
int	tapsrec();
int	dmpspc();
int	dsrch();
int	nullf();
char	*getsuffix();
char	*rawname();
struct dinode *getino();

void	interrupt();		/* in case operator bangs on console */
void	dumpabort();

#define	HOUR	(60L*60L)
#define	DAY	(24L*HOUR)
#define	YEAR	(365L*DAY)

/*
 *	Exit status codes
 */
#define	X_FINOK		0	/* normal exit */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort all of dump; don't attempt checkpointing */
#define	X_VERIFY	4	/* verify the reel just written */

#define	NINCREM	"/etc/dumpdates"	/*new format incremental info*/
#define	TEMP	"/etc/dtmp"		/*output temp file*/

#define	TAPE	"/dev/rmt8"		/* default tape device */
#define	RTAPE	"dumphost:/dev/rmt8"	/* default tape device */
#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

/*
 * diskette size == sides * sectors per cylinder side * cylinders * sector size
 */
#ifdef i386
/* 1 cylinder is reserved for bad block information */
#define	DISKETTE_DSIZE	(2*18*(80-1)*512)
#else i386
#define	DISKETTE_DSIZE	(2*18*(80)*512)
#endif i386
#define	DISKETTE	"/dev/rfd0c"


struct	fstab	*fstabsearch();	/* search in fs_file and fs_spec */

/*
 *	The contents of the file NINCREM is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
struct	idates {
	char	id_name[MAXNAMLEN+3];
	char	id_incno;
	time_t	id_ddate;
};

struct	itime{
	struct	idates	it_value;
	struct	itime	*it_next;
};

struct	itime	*ithead;	/* head of the list version */
int	nidates;		/* number of records (might be zero) */
int	idates_in;		/* we have read the increment file */
struct	idates	**idatev;	/* the arrayfied version */
#define	ITITERATE(i, ip) for (i = 0,ip = idatev[0]; i < nidates; i++, ip = idatev[i])

/*
 *	We catch these interrupts
 */
void	sighup();
void	sigquit();
void	sigill();
void	sigtrap();
void	sigfpe();
void	sigkill();
void	sigbus();
void	sigsegv();
void	sigsys();
void	sigalrm();
void	sigterm();
