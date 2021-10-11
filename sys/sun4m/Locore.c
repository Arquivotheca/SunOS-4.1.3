/*LINTLIBRARY*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * @(#)Locore.c 1.1 92/07/30 SMI
 */

#include <machine/fpu/fpu_simulator.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/clist.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_u.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/scb.h>
#include <machine/buserr.h>

#include <sun/autoconf.h>
#include <sundev/mbvar.h>
#include <sys/stream.h>
#include <sys/ttold.h>
#include <sys/tty.h>
#include <sundev/zsreg.h>

#include <scsi/scsi.h>

#ifdef LWP
#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/cntxt.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
extern void     __lwpkill();
extern void     __full_swtch();
extern stkalign_t *lwp_datastk();
extern stkalign_t *lwp_newstk();
#endif					/* LWP */

#ifdef NFS
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif					/* NFS */

#include <pixrect/pixrect.h>
static void     pixrect_resolve();

#ifdef IOMMU
#include <machine/iommu.h>
#endif

#ifdef	DLYPRF
#include <os/dlyprf.h>
#endif	DLYPR


#ifdef MULTIPROCESSOR
#include "percpu_def.h"
#endif


/************************************************************************
 *	forward declarations for stuff in this file
 */
extern void     sys_rtt();
extern void     sys_rtt_ast();
extern void     sys_rtt_continue();
extern void     ASM_interrupt();
extern void     int_rtt();
extern void     resume();
extern void     set_intmask();
extern void     set_diagled();
extern void     l15_async_fault();
extern void     flush_writebuffers();
extern void     flush_writebuffers_to();
extern void     flush_all_writebuffers();
extern void     flush_poke_writebuffers();
extern void     mmu_sys_unf();

/************************************************************************
 *	locore.s
 */

char            DVMA[1 << 20];
struct msgbuf   msgbuf;

int		intstack[INTSTACKSIZE];

#ifndef MULTIPROCESSOR
char            kernstack[KERNSTACK];
struct seguser  ubasic;
struct user     intu;
#else	MULTIPROCESSOR
#define	extern
#include "percpu.h"
#undef	extern
struct PerCPU   cpu0[NCPU];
#endif	MULTIPROCESSOR

int             kptstart[1];
trapvec         kclock14_vec;

struct pte      Heapptes[1];
char            Heapbase[1];
char            Heaplimit[1];
struct pte      Bufptes[1];
char            Bufbase[1];
char            Buflimit[1];
struct pte      Sysmap[1];
char            Sysbase[1];
char            CADDR2[1];
char            vmmap[1];
struct pte      ESysmap[1];
char            Syslimit[1];

char            CADDR1[1];
struct pte      CMAP1[1];
struct pte      CMAP2[1];
struct pte      Mbmap[1];
struct mbuf	mbutl[1];	/* virtual address of net free mem */
struct pte	mmap[1];

#ifdef	NOPROM
int             availmem;

#endif					/* NOPROM */

#ifndef MULTIPROCESSOR
struct user    *uunix = 0;

#endif					/* MULTIPROCESSOR */

char            start[1];

#ifndef	PC_scb
struct scb      scb;

#else					/* PC_scb */
struct scb      real_scb;

#endif					/* PC_scb */

struct sunromvec *romp;
struct debugvec *dvec;

int             nwindows = 8;
int		fpu_exists;
trapvec	kadb_tcode, trap_ff_tcode, trap_fe_tcode;	/* kadb trap vectors */
int	kadb_defer, kadb_want;

extern u_int    load_tmpptes();
extern void     early_startup();
extern void     module_setup();

trapvec         mon_clock14_vec;
trapvec         mon_breakpoint_vec;

extern int      cpudelay;
extern int nkeytables;
extern int rcache_enable;
extern int rcacheinit;

#ifdef	LWP
static struct cv_t cvnull = {CHARNULL, 0};

#ifdef	MULTIPROCESSOR
int	force_lwps;
#endif	/* MULTIPROCESSOR */

#endif	/* LWP */

int bcopy_cnt, bzero_cnt;

unsigned cpuX_startup[] = {0,};

trigger_logan(){}

int
entry(iromp, idvec, indir)
	struct sunromvec *iromp;
	struct debugvec *idvec;
	void            (*indir) ();
{
	struct scb     *tbr;
	trapvec         tv;

	(*indir) ();

	romp = iromp;
	dvec = idvec;

#ifdef MULTIPROCESSOR
        xc_serv();
        (void) xc_sync(0,0,0,0,0, (func_t) 0 );
#endif
	module_setup(0);
#ifdef IOMMU
        (void) iom_choose_map(0);
        iom_pteload(0,0,0, (struct map *) 0);
        iom_dvma_unload((u_int)0);
        iom_pagesync((union iommu_pte *)0);
#endif
#ifdef IOC
	ioc_flush(0);
#endif IOC
	mmu_setctp(load_tmpptes());
	early_startup();
#ifdef	MULTIPROCESSOR
#ifdef	VAC
	vac_flush((caddr_t) cpu0, sizeof cpu0);
#endif				/* VAC */
#endif				/* MULTIPROCESSOR */
#ifndef	PC_scb
	tbr = &scb;
#else				/* PC_scb */
	tbr = &PerCPU[0].scb;
#endif				/* PC_scb */
	tv = tbr->impossible[0];
	tv.instr[3] &= ~255;
	tbr->reset = tv;

	nwindows = 8;
#ifdef	NOPROM
	availmem = 8 << 20;
#endif

	CADDR1[1] = CADDR1[1];
	CMAP1[1] = CMAP1[1];
	CMAP2[1] = CMAP2[1];
	Mbmap[1] = Mbmap[1];
	mmap[1] = mmap[1];
	set_itr(0);
	(void)settrap(0, entry);

	cpudelay = cpudelay;
	nkeytables = nkeytables;
	rcache_enable = rcache_enable;
	rcacheinit = rcacheinit;

	intstack[0] = intstack[0];
	(void) max((u_int) 0, (u_int) 0);

#ifndef	FBMAPOUT
	mapout((struct pte *) 0, 0);
#endif
#ifdef LWP
	lwpschedule();
	(void) lwp_create((thread_t *) 0, VFUNCNULL, 0, 0, STACKNULL, 0);
	__NuggetSP = __NuggetSP;
	(void) lwp_errstr();
	(void) cv_destroy(cvnull);
	(void) cv_wait(cvnull);
	(void) cv_notify(cvnull);
	(void) cv_broadcast(cvnull);
	lwp_perror("");
	(void) lwp_checkstkset(THREADNULL, CHARNULL);
	(void) lwp_stkcswset(THREADNULL, CHARNULL);
	(void) lwp_ctxremove(THREADNULL, 0);
	debug_nugget(0);
	(void) lwp_setstkcache(0, 0);
	(void) lwp_datastk(CHARNULL, 0, (caddr_t *) 0);
	(void) lwp_newstk();
	(void) lwp_ctxmemget(CHARNULL, THREADNULL, 0);
	(void) lwp_setregs(THREADNULL, (machstate_t *) 0);
	(void) lwp_getregs(THREADNULL, (machstate_t *) 0);
	__lwpkill();
	__full_swtch();
	__Mem = __Mem;
#endif	/* LWP */

	/*
	 * SCSI subsystem stuff
	 */

	(void) scsi_ifgetcap((struct scsi_address *) 0, (char *) 0, 0);
	(void) scsi_ifsetcap((struct scsi_address *) 0, (char *) 0, 0, 0);
	scsi_dmafree((struct scsi_pkt *) 0);
	makecom_g5((struct scsi_pkt *) 0, (struct scsi_device *) 0, 0, 0, 0, 0);
	(void) scsi_dname(0);

#ifdef	IPCSHMEM
	shmsys();
#endif	IPCSHMEM
        (void) rewhence((struct flock *)0, (struct file *) 0, 0);
	(void) putc (0, (struct clist *) 0);
	(void) m_cappend((char *)0, 0, (struct mbuf *)0);
	swap_cons((u_int)0);
	(void) remintr(0, (int (*)())0);
	bawrite((struct buf *)0);
	call_debug_from_asm();
	(void) kmem_resize((caddr_t)0, (u_int)0, (u_int)0, (u_int)0);
#ifdef	SYSAUDIT
	{
		struct a { char *p; };
		chdir((struct a *)0);
		chroot((struct a *)0);
	}
#ifdef	IPCMESSAGE
	msgsys();
#endif	IPCMESSAGE
#ifdef	IPCSEMAPHORE
	semsys();
#endif	IPCSEMAPHORE
#ifdef	IPCSHMEM
	shmsys();
#endif	IPCSHMEM
#endif	SYSAUDIT
        zeroperms();

	mon_clock14_vec = tbr->interrupts[14 - 1];
	mon_breakpoint_vec = tbr->user_trap[127];

#ifdef	MULTIPROCESSOR
	pxmain();
#endif
	sys_trap(0);

        (void) copy_from_mbufs((u_char *) 0, (struct mbuf *) 0);
	putchar(0, 0, (struct vnode *)0);
	prtmsgbuf();
	(void) mb_nbmapalloc((struct map *) 0, (caddr_t) 0, 0, 0,
		(func_t) 0, (caddr_t) 0);
	mbpresetup((struct mb_hd *)0, (struct buf *)0, 0);
	(void) minphys((struct buf *)0);
	mbdone((struct mb_ctlr *)0);
	mbudone((struct mb_device *)0);
	nullsys();
        trap_ff_tcode = trap_ff_tcode;

#ifdef	DLYPRF
	dlyprf("testing %d %d %d %d\n", 1, 2, 3, 4);
#endif	DLYPRF

#ifdef	LWP
	__lowinit();
#endif
	pixrect_resolve();

#ifdef	OPENPROMS
/*
 * These are never called from the sun4m kernel.
 * But, they are still hanging around in openprom_util.c
 * and may be used by other kernel subarchetectures.
 * So, we make lint happy by using them here.
 */
	{
		typedef int   (*pfi)();
		dnode_t	dn,	path_to_nodeid();
		char   *sd,    *path_to_subdev();
		int	pt,	get_part_from_path();
		pfi	fn,	path_getdecodefunc();

		dn = path_to_nodeid("");
		sd = path_to_subdev("");
		pt = get_part_from_path("");
		fn = path_getdecodefunc((struct dev_info *)0);

		printf("ugh!\n", dn, sd, pt, fn());
	}
#endif	OPENPROMS
}

int
hardlevel10()
{
}

static void
cfunc()
{
}

int
call_entry()
{
	entry(romp, dvec, cfunc);
}

extern void     syscall();
extern void     trap();

int
sys_trap(type)
	unsigned	type;
{
	unsigned        afsr[4];
	unsigned        sfsr, sfar;
	struct regs     rp[1];
	extern void	fp_runq();

#ifdef	TRAPWINDOW
	extern long     trap_window[];

	trap_window[0] = type;
#endif
	mmu_getasyncflt(afsr);
	afsr[0] = afsr[0];
	mmu_getsyncflt(&sfsr, &sfar);
	ASM_interrupt(type, rp);
	syscall(rp);
	trap(type, rp, sfar, sfsr, S_OTHER);
	fp_runq(rp);
	(void) fp_is_disabled(rp);
	(void)getdevaddr((caddr_t)0);
	flush_user_windows();
	sys_rtt(rp);
}


int             overflowcnt = 0;
int             underflowcnt = 0;

extern char	qrunflag;
extern char	queueflag;

extern          queuerun();

void
sys_rtt(rp)
	struct regs    *rp;
{
#ifdef	LWP
	if (__Nrunnable) {
		flush_user_windows();
		lwpschedule();
		mmu_setctx(0);
	}
#endif				/* LWP */
	if (qrunflag && !queueflag) {
			queueflag = 1;
			queuerun();
			queueflag = 0;
	}
	sys_rtt_ast(rp);
}


void
sys_rtt_ast(rp)
	struct regs    *rp;
{
	trap(1, rp, (addr_t) 0, 0, S_OTHER);	/* T_AST ... */
	sys_rtt_continue(rp);
}


void
fork_rtt(rp)
	struct regs    *rp;
{
	sys_rttchk(masterprocp, masterprocp->p_stack, (caddr_t) 0);
	sys_rtt_ast(rp);
}


void
sys_rtt_continue(rp)
	struct regs    *rp;
{
	unsigned        sfsr, sfar;

	mmu_sys_unf(&sfsr, &sfar);
	trap(1, rp, sfar, sfsr, S_WRITE);
}


typedef void    (*pfv) ();

extern void     spurious();
extern void     ASM_level1();
extern void     ASM_level2();
extern void     ASM_level3();
extern void     ASM_level4();
extern void     ASM_level5();
extern void     ASM_level6();
extern void     ASM_level7();
extern void     ASM_level8();
extern void     ASM_level9();
extern void     ASM_level10();
extern void     ASM_level11();
extern void     ASM_level12();
extern void     ASM_level13();
extern void     ASM_level14();
extern void     ASM_level15();
extern void     ASM_softlvl1();
extern void     ASM_softlvl2();
extern void     ASM_softlvl3();
extern void     ASM_softlvl4();
extern void     ASM_softlvl5();
extern void     ASM_softlvl6();
extern void     ASM_softlvl7();
extern void     ASM_softlvl8();
extern void     ASM_softlvl9();
extern void     ASM_softlvl10();
extern void     ASM_softlvl11();
extern void     ASM_softlvl12();
extern void     ASM_softlvl13();
extern void     ASM_softlvl14();
extern void     ASM_softlvl15();

void            (*(int_vector[])) () = {
	spurious,
	ASM_level1,
	ASM_level2,
	ASM_level3,
	ASM_level4,
	ASM_level5,
	ASM_level6,
	ASM_level7,
	ASM_level8,
	ASM_level9,
	ASM_level10,
	ASM_level11,
	ASM_level12,
	ASM_level13,
	ASM_level14,
	ASM_level15,
	spurious,
	ASM_softlvl1,
	ASM_softlvl2,
	ASM_softlvl3,
	ASM_softlvl4,
	ASM_softlvl5,
	ASM_softlvl6,
	ASM_softlvl7,
	ASM_softlvl8,
	ASM_softlvl9,
	ASM_softlvl10,
	ASM_softlvl11,
	ASM_softlvl12,
	ASM_softlvl13,
	ASM_softlvl14,
	ASM_softlvl15,
};

vmevec          vme_vector[256];

int             last_vmevec;
int             last_L1_vmevec;
int             last_L2_vmevec;
int             last_L3_vmevec;
int             last_L4_vmevec;
int             last_L5_vmevec;
int             last_L6_vmevec;
int             last_L7_vmevec;

void
ASM_interrupt(type, rp)
	unsigned	type;
	struct regs    *rp;
{
	unsigned        bits, level;
	void            (*vect) ();

	set_intmask(IR_ENA_INT, 0);
	level = type & 15;
	bits = 1 << level;
	if (!(intregs.pend & bits)) {
		bits <<= 16;
		level |= 16;
		if (!(intregs.pend & bits)) {
			sys_rtt(rp);
		}
	}
	intregs.clr_pend = bits;

	vect = int_vector[level];
#ifdef	MULTIPROCESSOR
	if (klock_intlock(level)) {
		sys_rtt(rp);
		return;
	}
#endif				/* MULTIPROCESSOR */
	(*vect) (level);
	int_rtt(rp, vect, level);

	{
		extern int olvl1_spurious;
		extern int olvl2_spurious;
		extern int olvl3_spurious;
		extern int olvl5_spurious;
		extern int olvl7_spurious;
		extern int olvl10_spurious;
		extern int olvl14_spurious;
		extern int olvl15_spurious;
		olvl1_spurious = olvl1_spurious;
		olvl2_spurious = olvl2_spurious;
		olvl3_spurious = olvl3_spurious;
		olvl5_spurious = olvl5_spurious;
		olvl7_spurious = olvl7_spurious;
		olvl10_spurious = olvl10_spurious;
		olvl14_spurious = olvl14_spurious;
		olvl15_spurious = olvl15_spurious;
	}
}


void
int_rtt(rp, vect, level)
	struct regs    *rp;
	void            (*vect) ();
	unsigned        level;
{
	cnt.v_intr++;
	sys_rtt(rp);
}


void
spurious(level)
{
	printf("spurious interrupt at processor level %d\n", level);
}


int             sipr;
int             vvec;

#define SYNCWB	flush_writebuffers()

extern char     busname_vec[];
extern char     busname_ovec[];
extern char     busname_svec[];
extern char     busname_vvec[];

#define NSSET(LEVEL)							\
	struct autovec *av;						\
	int (*vect)();							\
	int *ctr = &level/**/LEVEL/**/_spurious;			\
	int lvl = LEVEL;						\
	char *bus = busname_vec

#define IOINTR(LEVEL)							\
    {									\
	extern struct autovec level/**/LEVEL[];				\
	extern int level/**/LEVEL/**/_spurious;				\
	av = level/**/LEVEL;						\
	while (vect = av->av_vector) {					\
	    if ((*vect)()) {						\
		av->av_intcnt ++;					\
		level/**/LEVEL/**/_spurious = 0;			\
		return;							\
	    }								\
	    av++;							\
	}								\
	(void)not_serviced(ctr, lvl, bus);				\
	return;								\
    }

#define OBINTR(LEVEL,MASK)						\
    {									\
	extern struct autovec olvl/**/LEVEL[];				\
	extern int level/**/LEVEL/**/_spurious;				\
	extern int olvl/**/LEVEL/**/_spurious;				\
	if (sipr & MASK) {						\
	    av = olvl/**/LEVEL;						\
	    while (vect = av->av_vector) {				\
		if ((*vect)()) {					\
		    av->av_intcnt ++;					\
		    level/**/LEVEL/**/_spurious = 0;			\
		    return;						\
		}							\
		av++;							\
	    }								\
	    ctr = &olvl/**/LEVEL/**/_spurious;				\
	    lvl = LEVEL;						\
	    bus = busname_ovec;						\
	}								\
    }

#define SBINTR(LEVEL)							\
    {									\
	extern struct autovec slvl/**/LEVEL[];				\
	extern int level/**/LEVEL/**/_spurious;				\
	extern int slvl/**/LEVEL/**/_spurious;				\
	if ((sipr>>7) & (1<<(LEVEL-1))) {				\
	    av = slvl/**/LEVEL;						\
	    while (vect = av->av_vector) {				\
		if ((*vect)()) {					\
		    av->av_intcnt ++;					\
		    level/**/LEVEL/**/_spurious = 0;			\
		    return;						\
		}							\
		av++;							\
	    }								\
	    ctr = &slvl/**/LEVEL/**/_spurious;				\
	    lvl = LEVEL;						\
	    bus = busname_svec;						\
	}								\
    }

#define VBINTR(LEVEL)							\
    {									\
	extern struct autovec vlvl/**/LEVEL[];				\
	extern int level/**/LEVEL/**/_spurious;				\
	extern int vlvl/**/LEVEL/**/_spurious;				\
	int    *varg;							\
	if (sipr & (1<<(LEVEL-1))) {					\
	    last_L/**/LEVEL/**/_vmevec = last_vmevec = lvl = vvec;	\
	    vect = vme_vector[lvl].func;				\
	    varg = vme_vector[lvl].arg;					\
	    if (vect)							\
		if ((*vect)(varg))					\
		    return;						\
	    av = vlvl/**/LEVEL;						\
	    while (vect = av->av_vector) {				\
		if ((*vect)()) {					\
		    av->av_intcnt ++;					\
		    level/**/LEVEL/**/_spurious = 0;			\
		    return;						\
		}							\
		av++;							\
	    }								\
	    ctr = &vlvl/**/LEVEL/**/_spurious;				\
	    lvl = LEVEL;						\
	    bus = busname_vvec;						\
	}								\
    }

#define SOINTR(LEVEL)							\
    {									\
	extern struct autovec xlvl/**/LEVEL[];				\
	extern struct autovec level/**/LEVEL[];				\
	extern int level/**/LEVEL/**/_spurious;				\
	struct autovec *av;						\
	int (*vect)();							\
	av = xlvl/**/LEVEL;						\
	while (vect = av->av_vector) {					\
	    if ((*vect)())						\
		av->av_intcnt ++;					\
	    av++;							\
	}								\
	av = level/**/LEVEL;						\
	while (vect = av->av_vector) {					\
	    if ((*vect)())						\
		av->av_intcnt ++;					\
	    av++;							\
	}								\
	level/**/LEVEL/**/_spurious = 0;				\
	return;								\
    }

void
ASM_softlvl1()
{
	SOINTR(1);
}


void
ASM_softlvl2()
{
	SOINTR(2);
}


void
ASM_softlvl3()
{
	SOINTR(3);
}


void
ASM_softlvl4()
{
	SOINTR(4);
}


void
ASM_softlvl5()
{
	SOINTR(5);
}


void
ASM_softlvl6()
{
	SOINTR(6);
}


void
ASM_softlvl7()
{
	SOINTR(7);
}


void
ASM_softlvl8()
{
	SOINTR(8);
}


void
ASM_softlvl9()
{
	SOINTR(9);
}


void
ASM_softlvl10()
{
	SOINTR(10);
}


void
ASM_softlvl11()
{
	SOINTR(11);
}


void
ASM_softlvl12()
{
	SOINTR(12);
}


void
ASM_softlvl13()
{
	SOINTR(13);
}


void
ASM_softlvl14()
{
	SOINTR(14);
}


void
ASM_softlvl15()
{
#ifdef	MULTIPROCESSOR
#ifdef	PC_prom_mailbox
	if (prom_mailbox) {
		(void)prom_stopcpu(0);
		(void)prom_idlecpu(0);
	}
#endif				/* PC_prom_mailbox */
	xc_serv();
#endif				/* MULTIPROCESSOR */
	SOINTR(15);
}


void
ASM_level1()
{
	SYNCWB;
	{
		NSSET(1);
		IOINTR(1);
	}
}


void
ASM_level2()
{
	SYNCWB;
	{
		NSSET(2);
		SBINTR(1);
		VBINTR(1);
		IOINTR(2);
	}
}


void
ASM_level3()
{
	SYNCWB;
	{
		NSSET(3);
		SBINTR(2);
		VBINTR(2);
		IOINTR(3);
	}
}


void
ASM_level4()
{
	SYNCWB;
	{
		NSSET(4);
		OBINTR(4, SIR_SCSI);
		IOINTR(4);
	}
}


void
ASM_level5()
{
	SYNCWB;
	{
		NSSET(5);
		SBINTR(3);
		VBINTR(3);
		IOINTR(5);
	}
}


void
ASM_level6()
{
	SYNCWB;
	{
		NSSET(6);
		OBINTR(6, SIR_ETHERNET);
		IOINTR(6);
	}
}


void
ASM_level7()
{
	SYNCWB;
	{
		NSSET(7);
		SBINTR(4);
		VBINTR(4);
		IOINTR(7);
	}
}


void
ASM_level8()
{
	SYNCWB;
	{
		NSSET(8);
		OBINTR(8, SIR_VIDEO);
		IOINTR(8);
	}
}


void
ASM_level9()
{
	SYNCWB;
	{
		NSSET(9);
		OBINTR(9, SIR_MODULEINT);
		SBINTR(5);
		VBINTR(5);
		IOINTR(9);
	}
}


#if	0
void
ASM_level10()
{
	NSSET(10);
	OBINTR(10, SIR_REALTIME);
	IOINTR(10);
}


#endif

void
ASM_level11()
{
	SYNCWB;
	{
		NSSET(11);
		OBINTR(11, SIR_FLOPPY);
		SBINTR(6);
		VBINTR(6);
		IOINTR(11);
	}
}


void
ASM_level12()
{
	SYNCWB;
	{
		NSSET(12);
		OBINTR(12, SIR_KBDMS | SIR_SERIAL);
		IOINTR(12);
	}
}


void
ASM_level13()
{
	SYNCWB;
	{
		NSSET(13);
		OBINTR(13, SIR_AUDIO);
		SBINTR(7);
		VBINTR(7);
		IOINTR(13);
	}
}


void
ASM_level14()
{
	SYNCWB;
	{
		NSSET(14);
		IOINTR(14);
	}
}


void
ASM_level15()
{
	l15_async_fault();
}

int             clk_intr = 0;
int             trigger_watchdog = 0;

int             ledpat[] = {
	0x041,				/* .....#.....# */
	0x022,				/* ......#...#. */
	0x014,				/* .......#.#.. */
	0x008,				/* ........#... */
	0x014,				/* .......#.#.. */
	0x022,				/* ......#...#. */
	0x041,				/* .....#.....# */
	0x082,				/* ....#.....#. */
	0x104,				/* ...#.....#.. */
	0x208,				/* ..#.....#... */
	0x410,				/* .#.....#.... */
	0x820,				/* #.....#..... */
	0x440,				/* .#...#...... */
	0x280,				/* ..#.#....... */
	0x100,				/* ...#........ */
	0x280,				/* ..#.#....... */
	0x440,				/* .#...#...... */
	0x820,				/* #.....#..... */
	0x410,				/* .#.....#.... */
	0x208,				/* ..#.....#... */
	0x104,				/* ...#.....#.. */
	0x082,				/* ....#.....#. */
};
int             ledcnt = 0;
int             ledptr = 0;

void
ASM_level10(rp)
	struct regs    *rp;
{
	if (ledcnt > 0)
		ledcnt--;
	else {
		ledcnt = 8;
		if (ledptr > 0)
			ledptr--;
		else
			ledptr = sizeof ledpat / sizeof ledpat[0] - 1;
		set_diagled(ledpat[ledptr]);
	}
	clk_intr++;
#ifndef	KERNEL_MEASURESTACK
	hardclock((caddr_t) 0, 0);
#else	KERNEL_MEASURESTACK
	hardclock((caddr_t) 0, 0, 0);
#endif	KERNEL_MEASURESTACK
}

int             auxio_reg;

set_auxioreg(bits, set)
{
	if (set)
		auxio_reg = auxio_reg | bits;
	else
		auxio_reg = auxio_reg & ~bits;
}

fd_set_dor(v, l)
{
	v = v;
	l = l;
}

int
fdc_hardintr()
{
}

int
flush_windows()
{
	flush_windows();
}


int
flush_user_windows()
{
	while (u.u_pcb.pcb_uwm)
		flush_user_windows();
}


int
trash_user_windows()
{
	while (u.u_pcb.pcb_uwm)
		trash_user_windows();
}

int
montrap(func)
	int (*func) ();
{
	flush_windows();
	return (*func) ();
}


void 
icode1()
{
	struct regs     rp[1];

	icode(rp);
}


int
check_cpuid(exp, act)
	int            *exp, *act;
{
	return *exp = *act = 0;
}


/************************************************************************
 *	lwp/low.s
 */

#ifdef	LWP
extern void     __full_swtch();

void
__lowinit()
{
	__Mem = __Mem;
	__full_swtch();
	__lwpkill();
}


__checkpoint()
{
	flush_windows();
}


__swtch()
{
	flush_windows();
}

int
__stacksafe()
{
	return 0;
}

void
__setstack()
{
}


#endif					/* LWP */

/************************************************************************
 *	overflow.s
 */

/************************************************************************
 *	underflow.s
 */

/************************************************************************
 *	swtch.s
 */

int
setrq(p)
	struct proc    *p;
{
	insque(p, qs);
	whichqs |= 1;
}


int
remrq(p)
	struct proc    *p;
{
	remque(p);
	whichqs &= ~1;
}


#ifdef	MULTIPROCESSOR
int
onrq(p)
	struct proc    *p;
{
	struct proc    *t = p->p_link;
	int             rv = 0;

	if (t->p_link == p)
		rv = 1;
	return rv;
}


#endif					/* MULTIPROCESSOR */

int 
idle()
{
	swtch();
}


int
swtch()
{
	struct proc    *p = 0;

	noproc = 1;
	runrun = 0;
	u.u_pcb.pcb_flags &= ~AST_SCHED;
#ifdef	P_PAM
	p->p_pam |= 1 << cpuid;
#endif
#ifdef	MULTIPROCESSOR
	p->p_cpuid = cpuid;
#endif
#ifdef	A_HAT_CPU
	p->p_as->a_hat.hat_oncpu = cpuid;
#endif				/* A_HAT_CPU */
#ifdef	MULTIPROCESSOR
	cpu_idle = (p == &idleproc);
#endif
	remque(p);
	whichqs &= ~1;
	noproc = 0;
	cnt.v_swtch++;
#ifdef	A_HAT_CPU
	p->p_as->a_hat.hat_oncpu = cpuid | 8;
#endif				/* A_HAT_CPU */
	resume(p);
}


extern void     hat_map_percpu();
extern unsigned	pte_rmsync();
extern unsigned	pte_decache();
extern unsigned	pte_recache();

void
resume(p)
	struct proc    *p;
{
	struct proc    *op;

	kadb_defer = 1;
	kadb_want = 0;
	flush_windows();
#ifdef	MULTIPROCESSOR
	p->p_cpuid = cpuid;
#endif
	op = masterprocp;
	masterprocp = p;
#ifdef	MULTIPROCESSOR
	op->p_cpuid = -1;
#endif
#ifdef	A_HAT_CPU
	op->p_as.a_hat.hat_oncpu = 0;
#endif
	sun4m_l15_async_fault(0);
	clr_dirint(0,0);
	set_itr(0);
	flush_writebuffers();
	flush_writebuffers_to((addr_t)0);
	flush_all_writebuffers();
	flush_poke_writebuffers();
	mmu_setctx(0);
	hat_map_percpu(p->p_uarea);
	(void)pte_rmsync((union ptpe *)0);
	(void)pte_decache((union ptpe *)0);
	(void)pte_recache((union ptpe *)0);
	uunix = p->p_uarea;
	hat_free(p->p_as);
	mmu_setctx(p->p_as->a_hat.hat_ctx->c_num);
	if (kadb_want) { kadb_want = 0; call_debug_from_asm(); }
	kadb_defer = 0;
}


start_child(parentp, parentu, childp, nfp, new_context, seg)
	struct proc    *parentp, *childp;
	struct user    *parentu;
	struct file    *nfp;
	int             new_context;
	struct seguser *seg;
{
	cons_child(parentp, parentu, childp, nfp, new_context, seg);
}


/*ARGSUSED*/
yield_child(parent_ar0, child_stack, childp, parent_pid, childu, seg)
	int            *parent_ar0;
	addr_t          child_stack;
	struct proc    *childp;
	short           parent_pid;
	struct user    *childu;
	struct seguser *seg;
{
	struct proc    *op;

	flush_windows();
#ifdef	MULTIPROCESSOR
	childp->p_cpuid = cpuid;
#endif
#ifdef	A_HAT_CPU
	childp->p_as->a_hat_cpu = cpuid + 8;
#endif
#ifdef	MULTIPROCESSOR
	hat_map_percpu(childu);
#endif
	op = masterprocp;
	masterprocp = childp;
	flush_writebuffers();
	uunix = childu;
	mmu_setctx(op->p_as->a_hat.hat_ctx->c_num);
#ifdef	A_HAT_CPU
	op->p_as->a_hat.hat_oncpu = 0;
#endif
#ifdef	MULTIPROCESSOR
	op->p_cpuid = -1;
#endif
}


int
finishexit(va)
	addr_t		va;
{
	struct proc    *op;

	flush_windows();
	uunix = 0;
	noproc = 1;
	segu_release(va);
	op = masterprocp;
	masterprocp = 0;
#ifdef	MULTIPROCESSOR
	op->p_cpuid = -1;
#endif
#ifdef	A_HAT_CPU
	op->p_as->as_hat.hat_oncpu = 0;
#endif
	swtch();
}


int
setmmucntxt(p)
	struct proc    *p;
{
	mmu_setctx(p->p_as->a_hat.hat_ctx->c_num);
}


/************************************************************************
 *	subr.s
 */

int
getidprom(dst)
	char           *dst;
{
	*dst = 0;
}


int             sys_intmask;

void
set_intmask(bit, flag)
	int             bit;
	int             flag;
{
	if (flag)
		sys_intmask |= bit;
	else
		sys_intmask &= ~bit;
}


int             sys_itr;

/*ARGSUSED*/
int
set_intreg(level,flag)
	u_int level, flag;
{
}

int
take_interrupt_target()
{
	sys_itr = cpuid;
}


getprocessorid()
{
	return cpuid;
}


#ifdef	MULTIPROCESSOR
void
chk_cpuid()
{
};
#endif	MULTIPROCESSOR

u_int	mem[1];

u_int
ldphys(pa)
	u_int	pa;
{
	return mem[pa];
}

void
stphys(pa, v)
	u_int	pa, v;
{
	mem[pa] = v;
}

u_int
swphys(v, pa)
	u_int	v, pa;
{
	unsigned r;
	r = mem[pa];
	mem[pa] = v;
	return r;
}


#ifdef IOMMU
int             iommu_reg[] = {0,};
int
iommu_set_ctl(val)
	unsigned	val;
{
	iommu_reg[0] = val;
}

int
iommu_set_base(val)
	unsigned	val;
{
	iommu_reg[0] = val;
}

int
iommu_flush_all()
{
	iommu_reg[0] = 0;
}

int
iommu_addr_flush(addr)
{
	iommu_reg[addr] = 0;
}


#endif

#ifdef	IOC
int             ioc_ram[1];
int
do_load_ioc(addr, val)
{
	ioc_ram[addr] = val;
}

int
do_read_ioc(addr)
{
	return ioc_ram[addr];
}

int
do_flush_ioc(addr)
{
	ioc_ram[addr] = 0;
}


#endif

u_long          sysctl;
u_long
get_sysctl()
{
	return sysctl;
}

void
set_sysctl(val)
	u_long          val;
{
	sysctl = val;
}


u_short         ledval;
void
set_diagled(val)
{
	ledval = val;
}


u_long          dmpr = 0x80;
u_long
get_diagmesg()
{
	return dmpr;
}

void
set_diagmesg(v)
	u_long          v;
{
	dmpr = v;
}


void
enable_dvma()
{
}

void
disable_dvma()
{
}

void
l15_async_fault()
{
}

void
flush_writebuffers()
{
}

void
flush_all_writebuffers()
{
}

void
flush_poke_writebuffers()
{
}

void	/*ARGSUSED*/
flush_writebuffers_to(va)
	addr_t		va;
{
}

int
init_all_fsr()
{
}


int
p4m690_sys_setfunc()
{
}

int
p4m50_sys_setfunc()
{
}


/************************************************************************
 *	zs_asm.s
 */

int
zsintr_hi()
{
}

int
setzssoft()
{
	set_intreg(IR_SOFT_INT(ZS_SOFT_INT), 1);
}

int 
clrzssoft()
{
	set_intreg(IR_SOFT_INT(ZS_SOFT_INT), 0);
	return 1;
}

int		/*ARGSUSED*/
zszread(base, reg)
	struct zscc_device *base;
{
	return 0;
}

int		/*ARGSUSED*/
zszwrite(base, reg, val)
	struct zscc_device *base;
{
}


/************************************************************************
 *	audio_79C30_intr.s
 */

/************************************************************************
 *	module_asm.s
 */

u_int
mmu_getcr()
{
	return 0;
}

u_int
mmu_getctp()
{
	return 0;
}

u_int
mmu_getctx()
{
	return 0;
}

/*ARGSUSED*/
u_int
mmu_probe(v)
	addr_t	v;
{
	return 0;
}

/*ARGSUSED*/
void
mmu_setcr(v)
{
	return;
}

/*ARGSUSED*/
void
mmu_setctp(v)
	unsigned        v;
{
	return;
}

/*ARGSUSED*/
void
mmu_setctx(v)
	unsigned        v;
{
	return;
}

void
mmu_flushall()
{
}

/*ARGSUSED*/
void
mmu_flushctx(v)
	unsigned	v;
{
	return;
}

/*ARGSUSED*/
void
mmu_flushrgn(v)
	unsigned	v;
{
	return;
}

/*ARGSUSED*/
void
mmu_flushseg(v)
	unsigned	v;
{
	return;
}

/*ARGSUSED*/
int
mmu_flushpage(v)
	addr_t		v;
{
	return;
}

/*ARGSUSED*/
void
mmu_flushpagectx(v, c)
	caddr_t		v;
	unsigned	c;
{
	return;
}

u_int
mmu_getsyncflt(p, q)
	unsigned       *p, *q;
{
	*p = *q = ~0;
}

void
mmu_getasyncflt(a)
	unsigned       *a;
{
	*a = ~0;
}

int
mmu_chk_wdreset()
{
	return 0;
}

int
mmu_sys_ovf()
{
}

void
mmu_sys_unf(sp, ap)
	unsigned       *sp, *ap;
{
	*sp = *ap = ~0;
}

int
mmu_wo()
{
}

int
mmu_wu()
{
}

/*ARGSUSED*/
void
mmu_log_module_err(afsr, afar0, aerr0, aerr1)
	u_int afsr;
	u_int afar0;
	u_int aerr0;
	u_int aerr1;
{
	return; 
}

/*ARGSUSED*/
mmu_print_sfsr(sfsr)
	u_int	sfsr;
{
	return;
}

u_int
pte_offon(ptpe, aval, oval)
	union ptpe *ptpe;
	u_int	aval;
	u_int	oval;
{
	register u_int rval;
	rval = ptpe->ptpe_int;
	rval = swapl((rval &~ aval) | oval, &(ptpe->ptpe_int));
	return rval;
}

/*ARGSUSED*/
void
module_wkaround(addr, rp, rw, mmu_fsr)
addr_t *addr;
struct regs *rp;
register enum seg_rw rw;
register u_int mmu_fsr;
{
}

void
vac_noop()
{

}

int
vac_inoop()
{
	return 0;
}

void
vac_init()
{
}

void
vac_flushall()
{
}

void
vac_usrflush()
{
}

/*ARGSUSED*/
void
vac_ctxflush(v)
	unsigned        v;
{
	return;
}

/*ARGSUSED*/
void
vac_rgnflush(v)
	addr_t          v;
{
	return;
}

/*ARGSUSED*/
void
vac_segflush(v)
	addr_t          v;
{
	return;
}

/*ARGSUSED*/
void
vac_pageflush(v)
	addr_t          v;
{
	return;
}

/*ARGSUSED*/
void
vac_pagectxflush(v, c)
	addr_t          v;
	unsigned        c;
{
	return;	
}

/*ARGSUSED*/
void
vac_flush(v, l)
	caddr_t		v;
{
	return;
}

void
cache_on()
{
}

/*ARGSUSED*/
int
vac_parity_chk_dis(v, m)
	unsigned        v, m;
{
	return 0;
}

/************************************************************************
 */

/*ARGSUSED*/
void
bpt_reg(v, l)
{
	return;
}

/*ARGSUSED*/
vik_mxcc_init_asm(v, l)
	u_int	v, l;
{
	return;
}

/*ARGSUSED*/
vik_vac_init_asm(v, l)
	u_int	v, l;
{
	return;
}

/************************************************************************
 *	../sparc/addupc.s
 */

/*ARGSUSED*/
addupc(pc, pr, incr)
	int             pc;
	struct uprof   *pr;
	int             incr;
{
	return;
}


/************************************************************************
 *	../sparc/copy.s
 */

int
kcopy(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
	return 0;
}

int
bcopy(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
}

void
pgcopy(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
}

int
ovbcopy(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
}

int
kzero(p, s)
	caddr_t		p;
	u_int		s;
{
	while (s-- > 0)
		*p++ = 0;
}

int
bzero(p, s)
	caddr_t         p;
	unsigned        s;
{
	while (s-- > 0)
		*p++ = 0;
}

int
blkclr(p, s)
	caddr_t         p;
	unsigned        s;
{
	while (s-- > 0)
		*p++ = 0;
}

int
copystr(p, q, l, n)
	caddr_t         p, q;
	u_int           l;
	u_int          *n;
{
	*n = l;
	while (l-- > 0)
		if (!(*q++ = *p++))
			break;
	*n -= l;
	return 0;
}

int
copyout(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
	return 0;
}

int
copyin(p, q, l)
	caddr_t         p, q;
	u_int           l;
{
	while (l-- > 0)
		*q++ = *p++;
	return 0;
}

int
copyinstr(p, q, l, n)
	caddr_t         p, q;
	u_int           l;
	u_int	       *n;
{
	*n = l;
	while (l-- > 0)
		if (!(*q++ = *p++))
			break;
	*n -= l;
	return 0;
}

int
copyoutstr(p, q, l, n)
	caddr_t         p, q;
	u_int           l;
	u_int          *n;
{
	*n = l;
	while (l-- > 0)
		if (!(*q++ = *p++))
			break;
	*n -= l;
	return 0;
}

int
fuword(a)
	caddr_t         a;
{
	return *a;
}

int
fuiword(a)
	caddr_t         a;
{
	return *a;
}

int
fubyte(a)
	caddr_t         a;
{
	return *a;
}

int
fuibyte(a)
	caddr_t         a;
{
	return *a;
}

int
suword(a, v)
	caddr_t         a;
{
	*a = v;
	return 0;
}

int
suiword(a, v)
	caddr_t         a;
{
	*a = v;
	return 0;
}

int
subyte(a, v)
	caddr_t         a;
{
	*a = v;
	return 0;
}

int
suibyte(a, v)
	caddr_t         a;
{
	*a = v;
	return 0;
}

int
fusword(a)
	caddr_t         a;
{
	return *a;
}

int
susword(a)
	caddr_t         a;
{
	return *a;
}


/************************************************************************
 *	../sparc/crt.s
 */

/*ARGSUSED*/
int 
_ip_umul(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_mul(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_div(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_udiv(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_umulcc(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_mulcc(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_divcc(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/*ARGSUSED*/
int 
_ip_udivcc(m, n, r, y, p)
	u_int           m, n, *r;
	int            *y, *p;
{
	return (0);
}


/************************************************************************
 *	../sparc/float.s
 */

int	/*ARGSUSED*/
syncfpu(rp)
	struct regs *rp;
{
}

int
fpu_probe()
{
}


/*ARGSUSED*/
void
_fp_read_pfreg(pf, n)
	FPU_REGS_TYPE  *pf;
	u_int       n;
{

}


/*ARGSUSED*/
void
_fp_write_pfreg(pf, n)
	FPU_REGS_TYPE  *pf;
	u_int       n;
{
}


/*ARGSUSED*/
_fp_write_pfsr(pfsr)
	FPU_FSR_TYPE   *pfsr;
{
}

/*ARGSUSED*/
fp_enable(fp) 
        struct fpu *fp;
{
}

/*ARGSUSED*/
fp_dumpregs(fp) 
        caddr_t fp;
{
}


/************************************************************************
 *	../sparc/ocsum.s
 */

int	/*ARGSUSED*/
ocsum(p, len) u_short *p; int len; { return (0); }

/************************************************************************
 *	../sparc/sparc_subr.s
 */

/*ARGSUSED*/
int
_insque(e, p)
	caddr_t         e, p;
{
}


/*ARGSUSED*/
int
_remque(e)
	caddr_t         e;
{
}

int
callee()
{
	return 0;
}

func_t
caller()
{
	return 0;
}

int
framepointer()
{
	return 0;
}

int
getpsr()
{
	return 0;
}

int
getsp()
{
	return 0;
}

int
gettbr()
{
	return 0;
}

int
getvbr()
{
	return 0;
}

int
ldstub(a)
	int            *a;
{
	return *a;
}


/*ARGSUSED*/
int 
longjmp(lp)
	label_t        *lp;
{
	flush_windows();
}


/*ARGSUSED*/
int
movtuc(l, f, t, x)
	u_char         *f, *t, *x;
{
	return 0;
}

int
no_fault()
{
}

int
on_fault()
{
	return 0;
}


/*ARGSUSED*/
int
scanc(l, s, t, m)
	unsigned        l;
	caddr_t		s, t;
{
	return 0;
}

int
servicing_interrupt()
{
	return 0;
}

int					/* ARGSUSED */
setjmp(lp)
	label_t        *lp;
{
	return 0;
}

void
setvideoenable()
{
}

int
siron()
{
}

int
spl0()
{
	return 0;
}

int
spl1()
{
	return 0;
}

int
spl2()
{
	return 0;
}

int
spl3()
{
	return 0;
}

int
spl4()
{
	return 0;
}

int
spl5()
{
	return 0;
}

int
spl6()
{
	return 0;
}

int
spl7()
{
	return 0;
}

int
spl8()
{
	return 0;
}

int
splaudio()
{
	return 0;
}

int
splbio()
{
	return 0;
}

int
splclock()
{
	return 0;
}

int
splhigh()
{
	return 0;
}

int
splimp()
{
	return 0;
}

int
splnet()
{
	return 0;
}

int
splr(s)
{
	return s;
}

int
splsoftclock()
{
	return 0;
}

int
spltty()
{
	return 0;
}

int
splvm()
{
	return 0;
}

int
splx(s)
{
	return s;
}

int
splzs()
{
	return 0;
}

int
stackpointer()
{
	return 0;
}

u_int
swapl(v, a)
	u_int v, *a;
{
	u_int r;
	r = *a;
	*a = v;
	return r;
}

int
usec_delay(n)
	unsigned	n;
{
	while (n-->0);
}


/************************************************************************
 *	../sun4m/klock_asm.s
 */
int	klock = 0;

klock_exit()
{ register lid = (0xFF000008 | cpuid);
  if (klock == lid) klock = 0; }

klock_enter()
{ register lid = (0xFF000008 | cpuid);
  while (klock != lid)
    if (klock == 0)
      klock = lid; }

klock_knock()
{ register lid = (0xFF000008 | cpuid);
  if (!klock) klock = lid;
  return klock == lid; }

klock_intlock(level) unsigned level;
{ unsigned lbit = 1<<level;
  if (0xC001C001 & lbit) return 0;
  if (klock_knock()) return 0;
  if (0xFFFF0000 & lbit) send_dirint(klock&3, (int)(level&0xF));
  return 1; }

klock_require(fn,ln,wh) char *fn; int ln; char *wh;
{ register mid = cpuid|8;
  register lid = 0xFF000000 | mid;
  register lv = klock;
  if (lv != lid) klock_reqfail(fn,ln,wh,lv,lid,mid); }

klock_steal(){return 0;}
halt_and_catch_fire(){}

/************************************************************************
 *	pixrect stuff
 */

#include "cgnine.h"
#if	NCGNINE == 0
struct pixrectops cg9_ops;
int 
cg9_putcolormap()
{
	return (0);
}

int 
cg9_putattributes()
{
	return (0);
}

int 
cg9_rop()
{
	return (0);
}

int 
cg9_ioctl()
{
	return (0);
}


#endif					/* NCGNINE == 0 */

#include "cgtwo.h"
#if	NCGTWO == 0
struct pixrectops cg2_ops;
int 
cg2_putcolormap()
{
	return (0);
}

int 
cg2_putattributes()
{
	return (0);
}

int 
cg2_rop()
{
	return (0);
}

int 
cg2_ioctl()
{
	return (0);
}


#endif					/* NCGTWO == 0 */

static void
pixrect_resolve()
{
	extern int      _loop;

	_loop = 23;		/* ...skidoo */
#if	NCGNINE == 0
	cg9_ops.pro_putcolormap = cg9_putcolormap;
	cg9_ops.pro_putattributes = cg9_putattributes;
	cg9_ops.pro_rop = cg9_rop;
	(void) cg9_ioctl();
#endif				/* NCGNINE == 0 */
#if	NCGTWO  == 0
	cg2_ops.pro_putcolormap = cg2_putcolormap;
	cg2_ops.pro_putattributes = cg2_putattributes;
	cg2_ops.pro_rop = cg2_rop;
	(void) cg2_ioctl();
#endif				/* NCGTWO == 0 */
}


/*
 * Variables declared for savecore, or
 * implicitly, such as by config or the loader.
 */
char            version[] = "SunOS Release LINT kernel";
char            etext[1];
char            end[1];
char            edata[1];

#ifdef IOMMU
struct map *	/*ARGSUSED*/
iom_choose_map(flags) u_int flags; { return((struct map *) 0);}
#endif

