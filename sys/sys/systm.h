/*	@(#)systm.h 1.1 92/07/30 SMI; from UCB 4.35 83/05/22	*/

/*
 * Random set of variables
 * used by more than one
 * routine.
 */

#ifndef _sys_systm_h
#define _sys_systm_h

#include <sys/types.h>
#include <sys/kmem_alloc.h>

extern	char version[];		/* system version */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch.
 * Used in bounds checking on major
 * device numbers.
 */
int	nblkdev;

/*
 * Number of character switch entries.
 */
int	nchrdev;

char	runin;			/* scheduling flag */
char	runout;			/* scheduling flag */
int	runrun;			/* scheduling flag */
char	curpri;			/* more scheduling */

int	maxmem;			/* actual max memory per process */
int	physmem;		/* physical memory (to be) used by system */
int	physmax;		/* highest numbered physical page present */

extern	int intstack[];		/* stack for interrupts */
dev_t	rootdev;		/* device of the root */
struct	vnode *rootvp;		/* vnode of root filesystem */
struct	vnode  *dumpvp;		/* vnode to dump on */
long	dumplo;			/* offset into dumpvp */

u_int	max();
u_int	min();
struct	vnode *bdevvp();
struct	vnode *specvp();

/*
 * Structure of the system-entry table
 */
extern	struct sysent {
	short	sy_narg;	/* total number of arguments */
	int	(*sy_call)();	/* handler */
} sysent[];

int	noproc;			/* no one is running just now */
char	*panicstr;
int	wantin;
int	boothowto;		/* reboot flags, from console subsystem */
int	selwait;

extern	char vmmap[];		/* poor name! */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)

/*
 * TRUE if the pageout daemon. This process can't sleep to get memory
 * else deadlock will result.
 */
#define	NOMEMWAIT() (u.u_procp == &proc[2])

#endif /*!_sys_systm_h*/
