#ident	"@(#)Locore.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

#include <sun4/fpu/fpu_simulator.h>
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
#include <vm/seg.h>
#include <vm/seg_u.h>
#include <vm/page.h>

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
extern void __lwpkill();
extern void __full_swtch();
#endif LWP

#ifdef NFS
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif NFS

/*
 * Pseudo file for lint to show what is used/defined
 * in locore.s and other such files.
 */

int	intstack[1];		/* the interrupt stack */

struct	user *uunix;		/* pointer to active u structure */
struct	seguser ubasic;		/* proc 0's kernel stack + u structure */
struct	scb scb;
vmevec vme_vector[1];
struct	msgbuf msgbuf;

typedef	int (*func)();

struct	proc *masterprocp;	/* proc pointer for current process mapped */
int	fpu_exists;		/* flag for fpu existence */
u_char	enablereg = 0;		/* software copy of system enable register */
int	nwindows = 0;		/* number of register windows */
int	kadb_defer, kadb_want;
int	fpneedfitoX;		/* flag for fitoX hack */

extern int level2_spurious, level3_spurious, level4_spurious, level6_spurious;

char	DVMA[0x1000000];

struct	pte Heapptes[HEAPPAGES];
char	Heapbase[1];
char	Heaplimit[1];
struct	pte Bufptes[BUFPAGES];
char	Bufbase[1];
char	Buflimit[1];
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
static struct cv_t cvnull = {CHARNULL, 0};
#endif LWP

#ifdef	BCOPY_BUF
int bzero_cnt, bcopy_cnt ;
#endif	BCOPY_BUF

lowinit()
{
	struct regs regs;
	extern char start[];
	extern char *strcpy();
	extern func_t caller();
	extern func_t callee();
	extern void trap();
	extern void syscall();
	extern int level2_intcnt[];
	extern int level3_intcnt[];
	extern int level4_intcnt[];
	extern int level6_intcnt[];
	extern char **av_end;
	extern int dk_ndrive;
	extern int nkeytables;
	extern int cpudelay;
#ifdef LWP
	extern stkalign_t *lwp_newstk();
	extern void lwp_perror();
	extern stkalign_t *lwp_datastk();
#endif LWP

	start[0] = start[0];
	dk_ndrive = dk_ndrive;		/* used by vmstat, iostat, etc. */

	/*
	 * Pseudo-uses of globals.
	 */
	lowinit();
	intstack[0] = intstack[0];
	scb = scb;
	level2_intcnt[0] = level2_intcnt[0];
	level3_intcnt[0] = level3_intcnt[0];
	level4_intcnt[0] = level4_intcnt[0];
	level6_intcnt[0] = level6_intcnt[0];
	av_end = av_end;
	enablereg = enablereg;
	uunix = KUSER(&ubasic);

#ifdef	BCOPY_BUF
	bzero_cnt = bzero_cnt;
	bcopy_cnt = bcopy_cnt;
#endif	BCOPY_BUF

	(void) main(&regs);
	tracedump();
	dumpsys();
	sys_rttchk((struct proc *)0, (caddr_t)0, (caddr_t)0);

	/*
	 * Routines called from interrupt vectors.
	 */
	panic("Machine check");
	printf("Write timeout");
	hardclock((caddr_t)0, 0);
	(void) softint();
	trap(0, &regs, (caddr_t) 0, 0, S_READ);
	syscall(&regs);
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
	(void) gettbr();
	(void) getvbr();
	(void) getsp();
	(void) fp_dumpregs();
	(void) __stacksafe();	  
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
	(void) mb_nbmapalloc((struct map *)0, (caddr_t)0, 0, 0, (func_t)0,
	    (caddr_t)0);
	(void) m_cpytoc((struct mbuf *)0, 0, 0, (caddr_t)0);
	(void) m_cappend((char *)0, 0, (struct mbuf *)0);
#ifdef NFS
	(void) xdr_netobj((XDR *)0, (struct netobj *)0);
#endif NFS
#ifdef VAC
	flush_cnt.f_ctx = flush_rate.f_ctx = flush_sum.f_ctx = 0;
	vac_flush((addr_t)0, 0);
#endif VAC

	(void) rewhence((struct flock *)0, (struct file *) 0, 0);

	add_default_intr((func)0);
	bawrite((struct buf *)0);
	(void) klustdone((struct buf *)0);
	(void) fbmmap((dev_t)0, (off_t)0, 0, 0, (struct mb_device **)0, 0);
	(void)fp_runq(&regs);
	(void) fp_is_disabled(&regs);
	(void) fusword((caddr_t)0);
	susword((caddr_t)0, 0);
	off_enablereg(0);
	call_debug_from_asm();

	(void) kmem_alloc((u_int)0);
	(void) kmem_zalloc((u_int)0);
	(void) kmem_resize((caddr_t)0, (u_int)0, (u_int)0, (u_int)0);
	(void) kmem_fast_alloc((caddr_t *)0, 0, 0);
	(void) kmem_fast_zalloc((caddr_t *)0, 0, 0);
	(void) vfs_getmajor((struct vfs *)0);
	vfs_putmajor((struct vfs *)0, 0);
	(void) pn_append((struct pathname *)0, (char *)0);
	(void) pn_getlast((struct pathname *)0, (char *)0);
	swap_cons((u_int)0);
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
	cpudelay = cpudelay;		/* for backwards compatability */
	Buflimit[0] = Buflimit[0];
#ifdef LWP
	lwpschedule();
	(void)lwp_create((thread_t *)0, VFUNCNULL, 0, 0, STACKNULL, 0);
	__NuggetSP = __NuggetSP;
	(void)lwp_errstr();
	(void)cv_destroy(cvnull);
	(void)cv_wait(cvnull);
	(void)cv_notify(cvnull);
	(void)cv_broadcast(cvnull);
	lwp_perror("");
	(void)lwp_checkstkset(THREADNULL, CHARNULL);
	(void)lwp_stkcswset(THREADNULL, CHARNULL);
	(void)lwp_ctxremove(THREADNULL, 0);
	debug_nugget(0);
	(void)lwp_setstkcache(0, 0);
	(void)lwp_datastk(CHARNULL, 0, (caddr_t *)0);
	(void)lwp_newstk();
	(void)lwp_ctxmemget(CHARNULL, THREADNULL, 0);
	(void)lwp_setregs(THREADNULL, (machstate_t *)0);
	(void)lwp_getregs(THREADNULL, (machstate_t *)0);
	__lwpkill();
	__full_swtch();
	__Mem = __Mem;
#endif LWP
}

/*
 * Routines from locore.s.
 */

void
fork_rtt() { ; }

vme_read_vector() { ; }

spurious() { ; }

flush_windows() { ; }

flush_user_windows() { ; }

trash_user_windows() { ; }

/*ARGSUSED*/
montrap(f) void (*f)(); { ; }

void
icode1() { struct regs regs; icode(&regs); }

/*
 * Routines from copy.s.
 */

/*ARGSUSED*/
int kcopy(from, to, count) caddr_t from, to; u_int count; { return (0); }

/*ARGSUSED*/
bcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
ovbcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
int kzero(base, count) caddr_t base; u_int count; { return (0); }

/*ARGSUSED*/
bzero(base, count) caddr_t base; u_int count; { ; }

/*ARGSUSED*/
blkclr(base, count) caddr_t base; u_int count; { ; }

/*ARGSUSED*/
copystr(kfaddr, kdaddr, maxlength, lencopied)
caddr_t kfaddr, kdaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copyout(kaddr, udaddr, n) caddr_t kaddr, udaddr; u_int n; { return (0); }

/*ARGSUSED*/
copyin(udaddr, kaddr, n) caddr_t udaddr, kaddr; u_int n; { return (0); }

/*ARGSUSED*/
copyinstr(udaddr, kaddr, maxlength, lencopied)
caddr_t udaddr, kaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copyoutstr(kaddr, udaddr, maxlength, lencopied)
caddr_t kaddr, udaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
fuword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuiword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fubyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuibyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
suword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suiword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
subyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suibyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
fusword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
susword(base, i) caddr_t base; { ; }

/*
 * Routines from addupc.s
 */

/*ARGSUSED*/
addupc(pc, pr, incr) int pc; struct uprof *pr; int incr; { ; }

/*
 * Routines from ocsum.s
 */

/*ARGSUSED*/
ocsum(p, len) u_short *p; int len; { return (0); }

/*
 * Routines from subr.s
 */

/*ARGSUSED*/
getidprom(s) char *s; { ; }

/*
 * Routines from sparc/sparc_subr.s
 */

spl8() { return (0); }
spl7() { return (0); }
splzs() { return (0); }
splhigh() { return (0); }
splclock() { return (0); }
spltty() { return (0); }
splbio() { return (0); }
spl6() { return (0); }
spl5() { return (0); }
spl4() { return (0); }
splimp() { return (0); }
spl3() { return (0); }
spl2() { return (0); }
spl1() { return (0); }
splnet() { return (0); }
splsoftclock() { return (0); }
spl0() { return (0); }

extern int splvm_val;
splvm() { return (splvm_val); }

/*ARGSUSED*/
splr(s) int s; { return (0); }

/*ARGSUSED*/
splx(s) int s; { return (0); }

no_fault() { ; }

on_fault() { return(0); }

/*ARGSUSED*/
setjmp(lp) label_t *lp; { return (0); }

/*ARGSUSED*/
longjmp(lp) label_t *lp; { /*NOTREACHED*/ }

/*ARGSUSED*/
setvideoenable(s) u_int s; { ; }

enable_dvma() { ; }

/*ARGSUSED*/
on_enablereg(reg_bit) u_char reg_bit; { ; }

/*ARGSUSED*/
usec_delay(n) int n; { ; }

/*ARGSUSED*/
movtuc(len, from, to, table)
int len;
u_char *from, *to, *table;
{ return(0); }

disable_dvma() { ; }

/*ARGSUSED*/
off_enablereg(reg_bit) u_char reg_bit; { ; } 

/*ARGSUSED*/
scanc(size, cp, table, mask)
u_int size; caddr_t cp, table; int mask; { return (0); }

func_t caller() { return((func_t)0); }

func_t callee() { return((func_t)0); }

gettbr() { return(0); }

getvbr() { return(0); }

getpsr() { return(0); }

getsp() { return(0); }

/*ARGSUSED*/
_insque(out, in) caddr_t out, in;  { ; }

/*ARGSUSED*/
_remque(p) caddr_t p; { ; }

/*ARGSUSED*/
set_intreg(bit, value) int bit; int value; { ; }

servicing_interrupt() { return(0); }

siron() { ; }

/*ARGSUSED*/
setintrenable(s) u_int s; { ; }


/*
 * Routines from swtch.s
 */
/*ARGSUSED*/

/*ARGSUSED*/
setrq(p) struct proc *p; { ; }

/*ARGSUSED*/
remrq(p) struct proc *p; { ; }

swtch() { if (whichqs) whichqs = 0; resume(masterprocp); }

/*ARGSUSED*/
resume(p) struct proc *p; {
        extern int kadb_defer, kadb_want;
        kadb_defer = 1;
        if (masterprocp) masterprocp = p;
        if (kadb_want) { kadb_want = 0; call_debug_from_asm(); }
        kadb_defer = 0;
}

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

finishexit(addr) addr_t addr; { segu_release(addr); swtch(); /*NOTREACHED*/ }

/*ARGSUSED*/
setmmucntxt(p) struct proc *p; { ; }

/*
 * Routines from map.s
 */

/*ARGSUSED*/
u_int map_getpgmap(v) addr_t v; { return ((u_int)0); }

/*ARGSUSED*/
map_setpgmap(v, pte) addr_t v; u_int pte; { ; }

/*ARGSUSED*/
u_int map_getsgmap(v) addr_t v; { return ((u_int)0); }

/*ARGSUSED*/
map_setsgmap(v, pm) addr_t v; u_int pm; { ; }

#ifdef MMU_3LEVEL

/*ARGSUSED*/
map_getrgnmap(v) addr_t v; { return(0); }

/*ARGSUSED*/
map_setrgnmap(v, pm) addr_t v; u_int pm; { ; }

#endif MMU_3LEVEL

u_int map_getctx() { return (0); }

/*ARGSUSED*/
map_setctx(c) u_int c; { ; }

#ifdef VAC
/*ARGSUSED*/
void vac_dontcache(addr) addr_t addr; { ; }

void vac_tagsinit() { ; }

void vac_ctxflush() { ; }

#ifdef MMU_3LEVEL

/*ARGSUSED*/
void vac_usrflush() { ; }

/*ARGSUSED*/
void vac_rgnflush(v) addr_t v; { ; }

#endif MMU_3LEVEL

/*ARGSUSED*/
void vac_segflush(v) addr_t v; { ; }

/*ARGSUSED*/
void vac_pageflush(v) addr_t v; { ; }

/*ARGSUSED*/
void vac_flush(v, nbytes) addr_t v; int nbytes; { ; }

/*ARGSUSED*/
vac_flushone(v) addr_t v; { ; }

#endif VAC

int read_confreg() { return(0); }

/*
 * Routines from vm_hatasm.s
 */
/*ARGSUSED*/
void hat_pmgloadptes(a, ppte)
addr_t a;
struct pte *ppte;
{ ; }

/*ARGSUSED*/
void hat_pmgswapptes(a, ppte1, ppte2)
addr_t a;
struct pte *ppte1, *ppte2;
{ ; }

/*ARGSUSED*/
void hat_pmgunloadptes(a, ppte)
addr_t a;
struct pte *ppte;
{ ; }

/*
 * From zs_asm.s
 */
#include "zs.h"

setzssoft() { ; }

clrzssoft() { return (0); }

/*ARGSUSED*/
zszread(zsaddr, n) struct zscc_device *zsaddr; int n; { return (0); }

/*ARGSUSED*/
zszwrite(zsaddr, n, v) struct zscc_device *zsaddr; int n, v; { ; }

/*
 * from sparc/float.s
 */

int	fpu_version;		/* version of FPU */

/*ARGSUSED*/
syncfpu(rp) struct regs *rp; { ; }

fpu_probe() { ; }

fp_dumpregs() { ; }

/*ARGSUSED*/
void
_fp_read_pfreg(pf, n) FPU_REGS_TYPE *pf, n; { ; }

/*ARGSUSED*/
void
_fp_write_pfreg(pf, n) FPU_REGS_TYPE *pf, n; { ; }

/*ARGSUSED*/
void
_fp_write_pfsr(fsr) FPU_FSR_TYPE *fsr; { ; }

/*ARGSUSED*/
void
fp_enable(fp) struct fpu *fp; {return;}
/*
 * from sparc/crt.s
 */

/*ARGSUSED*/
_ip_umul(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_mul(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_umulcc(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_mulcc(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_udiv(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_div(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_udivcc(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*ARGSUSED*/
_ip_divcc(rs1, rs2, reg_addr, r_yp, psrp)
u_int rs1, rs2;
u_int *reg_addr;
int *r_yp, *psrp;
{ return(0); }

/*
 * from lwp/sparc/low.s
 */

__checkpoint() { ; }
__swtch() { ; }
void __lowinit() { ; }
__stacksafe() { return(0); }
void __setstack() { ; }

/*
 * Variables declared for savecore, or
 * implicitly, such as by config or the loader.
 */
char	version[] = "SunOS Release ...";
char	start[1];
char	etext[1];
char	end[1];
