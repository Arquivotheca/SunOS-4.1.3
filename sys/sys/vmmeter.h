/*      @(#)vmmeter.h 1.1 92/07/30 SMI; from UCB 4.7 5/30/83        */

/*
 * Virtual memory related instrumentation
 */

#ifndef _sys_vmmeter_h
#define _sys_vmmeter_h

/*
 * Note that all the vmmeter entries between v_first and v_last
 *  must be unsigned [int], as they are used as such in vmmeter().
 */
struct vmmeter {
#define	v_first	v_swtch
	unsigned v_swtch;	/* context switches */
	unsigned v_trap;	/* calls to trap */
	unsigned v_syscall;	/* calls to syscall() */
	unsigned v_intr;	/* device interrupts */
	unsigned v_pdma;	/* pseudo-dma interrupts XXX: VAX only */
	unsigned v_pswpin;	/* pages swapped in */
	unsigned v_pswpout;	/* pages swapped out */
	unsigned v_pgin;	/* pageins */
	unsigned v_pgout;	/* pageouts */
	unsigned v_pgpgin;	/* pages paged in */
	unsigned v_pgpgout;	/* pages paged out */
	unsigned v_intrans;	/* intransit blocking page faults */
	unsigned v_pgrec;	/* total page reclaims (includes pageout) */
	unsigned v_xsfrec;	/* found in free list rather than on swapdev */
	unsigned v_xifrec;	/* found in free list rather than in filsys */
	unsigned v_exfod;	/* pages filled on demand from executables */
			/* XXX: above entry currently unused */
	unsigned v_zfod;	/* pages zero filled on demand */
	unsigned v_vrfod;	/* fills of pages mapped by vread() */
			/* XXX: above entry currently unused */
	unsigned v_nexfod;	/* number of exfod's created */
			/* XXX: above entry currently unused */
	unsigned v_nzfod;	/* number of zfod's created */
			/* XXX: above entry currently unused */
	unsigned v_nvrfod;	/* number of vrfod's created */
			/* XXX: above entry currently unused */
	unsigned v_pgfrec;	/* page reclaims from free list */
	unsigned v_faults;	/* total page faults taken */
	unsigned v_scan;	/* page examinations in page out daemon */
	unsigned v_rev;		/* revolutions of the paging daemon hand */
	unsigned v_seqfree;	/* pages taken from sequential programs */
			/* XXX: above entry currently unused */
	unsigned v_dfree;	/* pages freed by daemon */
	unsigned v_fastpgrec;	/* fast reclaims in locore XXX: VAX only */
#define	v_last v_fastpgrec
	unsigned v_swpin;	/* swapins */
	unsigned v_swpout;	/* swapouts */
};
#ifdef KERNEL
/*
 *	struct	v_first to v_last		v_swp*
 *	------	-----------------		------
 *	cnt	1 second interval accum		5 second interval accum
 *	rate	5 second average		previous interval
 *	sum			free running counter
 */
struct	vmmeter cnt, rate, sum;
#endif

/*
 * Systemwide totals computed every five seconds.
 * All these are snapshots, except for t_free.
 */
struct vmtotal {
	short	t_rq;		/* length of the run queue */
	short	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	short	t_pw;		/* jobs in page wait */
	short	t_sl;		/* ``active'' jobs sleeping in core */
	short	t_sw;		/* swapped out ``active'' jobs */
	int	t_vm;		/* total virtual memory */
			/* XXX: above entry currently unused */
	int	t_avm;		/* active virtual memory */
			/* XXX: above entry currently unused */
	int	t_rm;		/* total real memory in use */
	int	t_arm;		/* active real memory */
	int	t_vmtxt;	/* virtual memory used by text */
			/* XXX: above entry currently unused */
	int	t_avmtxt;	/* active virtual memory used by text */
			/* XXX: above entry currently unused */
	int	t_rmtxt;	/* real memory used by text */
			/* XXX: above entry currently unused */
	int	t_armtxt;	/* active real memory used by text */
			/* XXX: above entry currently unused */
	int	t_free;		/* free memory pages (60 second average) */
};
#ifdef KERNEL
struct	vmtotal total;
#endif

/*
 * Optional instrumentation.
 */
#ifdef PGINPROF

#define	NDMON	128
#define	NSMON	128

#define	DRES	20
#define	SRES	5

#define	PMONMIN	20
#define	PRES	50
#define	NPMON	64

#define	RMONMIN	130
#define	RRES	5
#define	NRMON	64

/* data and stack size distribution counters */
unsigned int	dmon[NDMON+1];
unsigned int	smon[NSMON+1];

/* page in time distribution counters */
unsigned int	pmon[NPMON+2];

/* reclaim time distribution counters */
unsigned int	rmon[NRMON+2];

int	pmonmin;
int	pres;
int	rmonmin;
int	rres;

unsigned rectime;		/* accumulator for reclaim times */
unsigned pgintime;		/* accumulator for page in times */
#endif

/*
 * Virtual Address Cache flush instrumentation.
 *
 * Everything from f_first to f_last must be unsigned [int].
 */
struct flushmeter {
#define f_first f_ctx
	unsigned f_ctx;		/* No. of context flushes */
	unsigned f_segment;	/* No. of segment flushes */
	unsigned f_page;	/* No. of complete page flushes */
	unsigned f_partial;	/* No. of partial page flushes */
	unsigned f_usr;		/* No. of non-supervisor flushes */
	unsigned f_region;	/* No. of region flushes */
#define f_last	f_region
};

#ifdef KERNEL
#ifdef VAC
/* cnt is 1 sec accum; rate is 5 sec avg; sum is grand total */
struct flushmeter	flush_cnt, flush_rate, flush_sum;
#endif VAC
#endif KERNEL

#endif /*!_sys_vmmeter_h*/
