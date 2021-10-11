#ifndef lint
static	char sccsid[] = "@(#)Locore.c 1.1 92/07/30 SMI";
#endif
/* syncd to sun3/Locore.c 1.79 */
/* syncd to sun3/Locore.c 1.86 (4.1) */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

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

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg_u.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/scb.h>
#include <machine/buserr.h>

#include <sundev/mbvar.h>
#include <sys/stream.h>
#include <sys/ttold.h>
#include <sys/tty.h>
#include <sundev/zsreg.h>

#include <machine/seg_kmem.h>

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
#endif LWP

#ifdef NFS
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif NFS

/*
 * Pseudo file for lint to show what is used/defined
 * in locore.s and other such files.
 */

int	intstack[NBPG/sizeof (int)];

struct	scb scb;
struct	scb protoscb;
struct	msgbuf msgbuf;
struct	user *uunix;		/* pointer to active u structure */
struct	seguser ubasic;		/* proc 0's kernel stack + u structure */
u_char  ledpat;

extern func_t *vector[7];

short fppstate = 1;		/* 0 = no 81, 1 = 81 enable, -1 = 81 disabled */
u_short enablereg = 0;		/* software copy of system enable register */
struct proc *fpprocp;		/* last proc to have external state for fpp */

struct proc *masterprocp;	/* proc pointer for current process mapped */

char	DVMA[0x100000];

int	kptstart;
struct	pte Sysmap[SYSPTSIZE];
char	Sysbase[SYSPTSIZE * MMU_PAGESIZE];
struct	pte CMAP1[1];
char	CADDR1[NBPG];
struct	pte CMAP2[1];
char	CADDR2[NBPG];
struct	pte mmap[1];
char	vmmap[NBPG];
struct	pte Mbmap[mmu_btop(MBPOOLBYTES)];
struct	mbuf mbutl[MBPOOLBYTES/sizeof (struct mbuf)];
struct	pte ESysmap[1];
char	Syslimit[1];

#ifdef LWP
static struct mon_t monnull = {CHARNULL, 0};
static struct cv_t cvnull = {CHARNULL, 0};

__swtch() { ; }
__checkpoint() { ; }
__setstack() { ; }

#endif LWP

lowinit()
{
	struct regs regs;
	struct stkfmt stkfmt;
	extern char *strcpy();
	extern func_t caller();
	extern func_t callee();
	extern void syscall();
	extern int level1_spurious;
	extern int level1_intcnt[];
	extern int level2_intcnt[];
	extern int level3_intcnt[];
	extern int level4_intcnt[];
	extern char **av_end;
	extern int dk_ndrive;
	extern int nkeytables;
#ifdef LWP
	extern stkalign_t *lwp_newstk();
	extern void __lwpkill();
	extern void lwp_perror();
	extern stkalign_t *lwp_datastk();
	extern void __asmdebug();
	extern void __dumpsp();
#endif LWP
	extern u_int load_tmpptes();
	extern void early_startup();

	dk_ndrive = dk_ndrive;		/* used by vmstat, iostat, etc. */

	/*
	 * Pseudo-uses of globals.
	 */
	lowinit();
	intstack[0] = intstack[0];
	scb = scb;
	protoscb = protoscb;
	vector[0] = vector[0];
	level1_spurious = level1_spurious;
	level1_intcnt[0] = level1_intcnt[0];
	level2_intcnt[0] = level2_intcnt[0];
	level3_intcnt[0] = level3_intcnt[0];
	level4_intcnt[0] = level4_intcnt[0];
	av_end = av_end;
	enablereg = enablereg;
	uunix = KUSER(&ubasic);
	(void) main(regs);
	tracedump();
	dumpsys();
	fpa_restore(uunix);

	/*
	 * Routines called from interrupt vectors.
	 */
	panic("Machine check");
	printf("Write timeout");
	hardclock((caddr_t)0, 0);
	(void) softint();
	(void) trap(0, regs, stkfmt);
	syscall(0, regs);
	ipintr();
	rawintr();
	memerr();
	boothowto = 0;

	(void) spl0();
	(void) spl1();
	(void) spl2();
	(void) spl3();
	(void) spl4();
	(void) spl5();
	(void) spl6();
	(void) spl7();
	(void) splzs();
	(void) splnet();
	(void) splimp();
	(void) splsoftclock();
	(void) splbio();
	(void) spltty();
	(void) splhigh();
	(void) splvm();

	/*
	 * Misc uses for some things which
	 * aren't neccesarily used.
	 */
	harderr((struct buf *)0, "bogus");
	tprintf((struct vnode *)0, "bogus");
	putchar(0, 0, (struct vnode *)0);
	(void) caller();
	(void) callee();
	nullsys();
	nkeytables = nkeytables;
	prtmsgbuf();
	errsys();
	(void) mballoc(&mb_hd, (caddr_t)0, 0, 0);
	(void) mb_nbmapalloc((struct map *)0, (caddr_t)0, 0, 0,
			(func_t)0, (caddr_t)0);
	(void) m_cpytoc((struct mbuf *)0, 0, 0, (caddr_t)0);
	(void) m_cappend((char *)0, 0, (struct mbuf *)0);
#ifdef NFS
	(void) xdr_netobj((XDR *)0, (struct netobj *)0);
#endif NFS
#ifdef VAC
	flush_cnt.f_ctx = flush_rate.f_ctx = flush_sum.f_ctx = 0;
	vac_flush(0, 0);
#endif VAC

	(void) rewhence((struct flock *)0, (struct file *) 0, 0);
	add_default_intr((func_t)0);
	bawrite((struct buf *)0);
	(void) klustdone((struct buf *)0);
	(void) fbmmap((dev_t)0, (off_t)0, 0, 0, (struct mb_device **)0, 0);

	(void) vfs_getmajor((struct vfs *)0);
	vfs_putmajor((struct vfs *)0, 0);
	(void) pn_append((struct pathname *) 0, (char *) 0);
	(void) pn_getlast((struct pathname *) 0, (char *) 0);
	swap_cons((u_int)0);
	(void) kmem_alloc((u_int)0);
	(void) kmem_zalloc((u_int)0);
	(void) kmem_resize((caddr_t)0, (u_int)0, (u_int)0, (u_int)0);
	(void) kmem_fast_alloc((caddr_t *)0, 0, 0);
        (void) kmem_fast_zalloc((caddr_t *)0, 0, 0);
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

	mmap[0] = mmap[0];
	Mbmap[0] = Mbmap[0];
	ESysmap[0] = ESysmap[0];
	CMAP1[0] = CMAP1[0];
	CADDR1[0] = CADDR1[0];
	CMAP2[0] = CMAP2[0];
	CADDR2[0] = CADDR2[0];
	saferss = saferss;
	(void) load_tmpptes();
	early_startup();
#ifdef LWP
	lwpschedule();
	(void)lwp_create((thread_t *)0, VFUNCNULL, 0, 0, STACKNULL, 0);
	__NuggetSP = __NuggetSP;
	(void)lwp_errstr();
	__asmdebug(0, 1, 2, 3, 4, 5);
	__dumpsp();
	(void)lwp_resume(THREADNULL);
	(void)cv_destroy(cvnull);
	(void)cv_wait(cvnull);
	(void)cv_notify(cvnull);
	(void)cv_broadcast(cvnull);
	(void)mon_create(&monnull);
	lwp_perror("");
	(void)lwp_checkstkset(THREADNULL, CHARNULL);
	(void)lwp_stkcswset(THREADNULL, CHARNULL);
	(void)lwp_ctxremove(THREADNULL, 0);
	debug_nugget(0);
	(void)lwp_setstkcache(0, 0);
	(void)lwp_datastk(CHARNULL, 0, (caddr_t *)0);
	(void)lwp_newstk();
	(void)lwp_ctxmemget(CHARNULL, THREADNULL, 0);
	__lwpkill();
	__Mem = __Mem;
#endif LWP
}

level7() { ; }

/*ARGSUSED*/
copyin(udaddr, kaddr, n) caddr_t udaddr, kaddr; u_int n; { return (0); }

/*ARGSUSED*/
copyout(kaddr, udaddr, n) caddr_t kaddr, udaddr; u_int n; { return (0); }

/*ARGSUSED*/
fubyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuibyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
subyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suibyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
fuword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuiword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
suword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suiword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
copyinstr(udaddr, kaddr, maxlength, lencopied)
caddr_t udaddr, kaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copyoutstr(kaddr, udaddr, maxlength, lencopied)
caddr_t kaddr, udaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copystr(kfaddr, kdaddr, maxlength, lencopied)
caddr_t kfaddr, kdaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
montrap(f) void (*f)(); { ; }

siron() { ; }

servicing_interrupt() { return (0); }

/*ARGSUSED*/
setvideoenable(s) u_int s; { ; }

/*ARGSUSED*/
setintrenable(s) u_int s; { ; }

/*ARGSUSED*/
getidprom(s) char *s; { ; }

/*ARGSUSED*/
getideeprom(s) char *s; { ; }

enable_dvma() { ; }

disable_dvma() { ; }

/*ARGSUSED*/
fulwds(uadd, sadd, nlwds) caddr_t uadd, sadd; int nlwds; { return (0); }

struct scb *getvbr() { return ((struct scb *)0); }

/*ARGSUSED*/
setvbr(scbp) struct scb *scbp; { ; }

_halt() { ; }

setfppregs() { ; }

/*ARGSUSED*/
fsave(fp_istate) struct fp_istate *fp_istate; { ; }

char *fpiar() { return((char *)0); }

/*ARGSUSED*/
frestore(fp_istate) struct fp_istate *fp_istate; { ; }

get_cir() { return(0); }

/*ARGSUSED*/
on_enablereg(reg_bit) u_char reg_bit; { ; }

/*ARGSUSED*/
off_enablereg(reg_bit) u_char reg_bit; { ; }

func_t caller() { return ((func_t)0); }

func_t callee() { return ((func_t)0); }

start_child(parentp, parentu, childp, nfp, new_context, seg)
	struct proc *parentp;
	struct user *parentu;
	struct proc *childp;
	struct file *nfp;
	int new_context;
	struct seguser *seg;
{ cons_child(parentp, parentu, childp, nfp, new_context, seg); }

/*ARGSUSED*/
yield_child(parent_ar0, child_stack, childp, parent_pid, childu, seg)
	int *parent_ar0;
	addr_t child_stack;
	struct proc *childp;
	short parent_pid;
	struct user *childu;
	struct seguser *seg;
{ }

void
icode1() { struct regs regs; icode(regs); }

finishexit(addr) addr_t addr; { segu_release(addr); swtch(); /*NOTREACHED*/ }

/*ARGSUSED*/
setmmucntxt(parentp) struct proc *parentp; { ; }

/*ARGSUSED*/
void
atc_flush_entry(addr) caddr_t addr; { ; }

void
atc_flush_all() { ; }

mmu_getrootptr() { return(0); }

/*ARGSUSED*/
mmu_setrootptr(l1pt) struct l1pt *l1pt; { ; }

/*ARGSUSED*/
u_short
mmu_geterror(faultaddr, usr, flag)
u_int	faultaddr;
char	usr;
int	flag;
{ return(0); }

void
vac_flush() { ; }

# ifdef PARITY
pperr() { ; }
#endif PARITY

/*
 * Routines from movc.s
 */
/*ARGSUSED*/
int kcopy(from, to, count) caddr_t from, to; u_int count; { return (0); }

/*ARGSUSED*/
bcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
lcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
ovbcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
int kzero(base, count) caddr_t base; u_int count; { return (0); }

/*ARGSUSED*/
bzero(base, count) caddr_t base; u_int count; { ; }

/*ARGSUSED*/
blkclr(base, count) caddr_t base; u_int count; { ; }

/*
 * From addupc.s
 */
/*ARGSUSED*/
addupc(pc, pr, incr) int pc; struct uprof *pr; int incr; { ; }

/*
 * From ocsum.s
 */
/*ARGSUSED*/
ocsum(p, len) u_short *p; int len; { return (0); }

/*
 * Routines from setjmp.s
 */
/*ARGSUSED*/
setjmp(lp) label_t *lp; { return (0); }

/*ARGSUSED*/
longjmp(lp) label_t *lp; { /*NOTREACHED*/ }

/*ARGSUSED*/
syscall_setjmp(lp) label_t *lp; { return (0); }

/*
 * Routines from vax.s
 */
/*ARGSUSED*/
splx(s) int s; { return (0); }

/*ARGSUSED*/
splr(s) int s; { return (0); }

spl0() { return (0); }
spl1() { return (0); }
spl2() { return (0); }
spl3() { return (0); }
spl4() { return (0); }
spl5() { return (0); }
spl6() { return (0); }
spl7() { return (0); }
splzs() { return (0); }
splnet() { return (0); }
splimp() { return (0); }
splclock() { return (0); }
splsoftclock() { return (0); }
splbio() { return (0); }
spltty() { return (0); }
splhigh() { return (0); }

extern short splvm_val;
splvm() { return (splvm_val); }

/*ARGSUSED*/
setrq(p) struct proc *p; { ; }

/*ARGSUSED*/
remrq(p) struct proc *p; { ; }

swtch() { if (whichqs) whichqs = 0; resume(masterprocp); }

/*ARGSUSED*/
resume(p) struct proc *p; { if (masterprocp) masterprocp = p; }

/*ARGSUSED*/
_remque(p) caddr_t p; { ; }

/*ARGSUSED*/
_insque(out, in) caddr_t out, in; { ; }

/*ARGSUSED*/
scanc(size, cp, table, mask)
u_int size; caddr_t cp, table; int mask; { return (0); }

/*ARGSUSED*/
movtuc(size, from, to, table)
int size; unsigned char *from; unsigned char *to; unsigned char *table;
{ return (0); }

/*
 * From zs_asm.s
 */
#include "zs.h"
int (*zsvectab[NZS][8])();

setzssoft() { ; }

clrzssoft() { return (0); }

/*ARGSUSED*/
zszread(zsaddr, n) struct zscc_device *zsaddr; int n; { return (0); }

/*ARGSUSED*/
zszwrite(zsaddr, n, v) struct zscc_device *zsaddr; int n, v; { ; }

/*
 * Variables declared for savecore, or
 * implicitly, such as by config or the loader.
 */
char	version[] = "SunOS Release ...";
char	start[1];
char	etext[1];
char	edata[1];
char	end[1];
