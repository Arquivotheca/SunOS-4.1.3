#ident	"@(#)Locore.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
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
#include <vm/page.h>

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
extern void __lwpkill();
extern void __full_swtch();
#endif LWP

#ifdef NFS
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif NFS

#include <pixrect/pixrect.h>
static void pixrect_resolve();

/*
 * Pseudo file for lint to show what is used/defined
 * in locore.s and other such files.
 */

int	intstack[1];		/* the interrupt stack */

struct	user *uunix;		/* pointer to active u structure */
struct	seguser ubasic;		/* proc 0's kernel stack + u structure */
struct	scb scb;
struct	msgbuf msgbuf;

typedef	int (*func)();

struct	proc *masterprocp;	/* proc pointer for current process mapped */
int	fpu_exists;		/* flag for fpu existence */
u_char	enablereg = 0;		/* software copy of system enable register */
int	nwindows = 0;		/* number of register windows */
trapvec	kadb_tcode, trap_ff_tcode, trap_fe_tcode;	/* kadb trap vectors */
int	kadb_defer, kadb_want;

extern int level1_spurious, level2_spurious, level3_spurious, level5_spurious;
extern int level6_spurious, level7_spurious, level8_spurious, level9_spurious;
extern int level11_spurious, level13_spurious;

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

lowinit()
{
	struct regs regs;
	extern char start[];
	extern char *strcpy();
	extern func_t caller();
	extern func_t callee();
	extern void trap();
	extern void syscall();
	extern struct autovec level1[];
	extern struct autovec level2[];
	extern struct autovec level3[];
	extern struct autovec level4[];
	extern struct autovec level5[];
	extern struct autovec level6[];
	extern struct autovec level7[];
	extern struct autovec level8[];
	extern struct autovec level9[];
	extern struct autovec level11[];
	extern struct autovec level13[];
	extern int dk_ndrive;
	extern int nkeytables;
	extern int cpudelay;
#ifdef LWP
	extern stkalign_t *lwp_newstk();
	extern void lwp_perror();
	extern stkalign_t *lwp_datastk();
#endif LWP
	extern void rm_outofhat();

	start[0] = start[0];
	dk_ndrive = dk_ndrive;		/* used by vmstat, iostat, etc. */

	/*
	 * Pseudo-uses of globals.
	 */
	lowinit();
	intstack[0] = intstack[0];
	scb = scb;
	level1[0] = level1[0];
	level2[0] = level2[0];
	level3[0] = level3[0];
	level4[0] = level4[0];
	level5[0] = level5[0];
	level6[0] = level6[0];
	level7[0] = level7[0];
	level8[0] = level8[0];
	level9[0] = level9[0];
	level11[0] = level11[0];
	level13[0] = level13[0];
	enablereg = enablereg;
	uunix = KUSER(&ubasic);
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
	zslevel12();
	memerr(0, 0, (addr_t)0, 0, (struct regs *) 0);
	asyncerr(0, (addr_t)0, 0, (addr_t)0);
	boothowto = 0;

	(void) spl0();
	(void) spl1();
	(void) spl2();
	(void) spl3();
	(void) spl4();
	(void) spl5();
	(void) spl6();
	(void) spl7();
        (void) spl8();
	(void) splzs();
	(void) splnet();
	(void) splimp();
	(void) splsoftclock();
	(void) splbio();
	(void) spltty();
	(void) splhigh();
	(void) splvm();
	(void) splaudio();

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
	(void) remintr(0, (int (*)())0);
	(void) mb_nbmapalloc((struct map *) 0, (caddr_t) 0, 0, 0,
		(func_t) 0, (caddr_t) 0);
	(void) m_cpytoc((struct mbuf *)0, 0, 0, (caddr_t)0);
	(void) m_cappend((char *)0, 0, (struct mbuf *)0);
#ifdef NFS
	(void) xdr_netobj((XDR *)0, (struct netobj *)0);
#endif NFS
#ifdef VAC
	flush_cnt.f_ctx = flush_rate.f_ctx = flush_sum.f_ctx = 0;
	vac_flush((addr_t)0, 0);
	vac_segflush((addr_t)0);
	memerr_70(0, 0, (addr_t)0, 0, 0);
#ifdef MMU_3LEVEL
	vac_usrflush();
	vac_rgnflush((addr_t)0, 0);
	vac_stolen_ctxflush();
#endif MMU_3LEVEL
#endif VAC

	/*
	 * Apparently nobody calls mapout these days
	 * (roach motel kernel- things check in, but....)
	 */

#ifndef	FBMAPOUT
	mapout((struct pte *) 0, 0);
#endif

	/*
	 * and nobody calls the subr_xxx.c max either
	 */

	(void) max((u_int) 0, (u_int) 0);

	/*
	 * apparently nobody here calls putc either
	 */
	(void) putc (0, (struct clist *) 0);

	(void) rewhence((struct flock *)0, (struct file *) 0, 0);

	bawrite((struct buf *)0);
	(void)fp_runq(&regs);
	(void) fp_is_disabled(&regs);
	(void) fusword((caddr_t)0);
	susword((caddr_t)0, 0);
	on_enablereg(0);
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

	/*
	 * SCSI subsystem stuff
	 */

	(void) scsi_ifgetcap((struct scsi_address *) 0, (char *) 0, 0);
	(void) scsi_ifsetcap((struct scsi_address *) 0, (char *) 0, 0, 0);
	makecom_g5((struct scsi_pkt *) 0, (struct scsi_device *) 0, 0, 0, 0, 0);
	scsi_dmafree((struct scsi_pkt *) 0);
	{
		extern char *scsi_dname();
		char *bz = scsi_dname((int) 0);
		bz = bz;
	}
	/*
	 * pixrect nonsense
	 */

	pixrect_resolve();
	/*
	 * vm stuff
	 */
	rm_outofhat();
}

/*
 * Routines from locore.s.
 */

siron() { return; }

servicing_interrupt() { return (0); }

flush_windows() { return; }

flush_user_windows() { return; }

trash_user_windows() { return; }

void fork_rtt() { return; }

sys_trap() { return; }

softlevel1() { return; }

/*ARGSUSED*/
set_auxioreg(bit, flag) int bit, flag; { return; }

/*
 * Routines from crt.s
 */

/*ARGSUSED*/
_ip_umul(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_mul(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }
       
/*ARGSUSED*/
_ip_umulcc(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_mulcc(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_udiv(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_div(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_udivcc(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*ARGSUSED*/
_ip_divcc(rs1,rs2,getregaddr,r_y,pcb_psrp)  
        u_int rs1;
        u_int rs2;
        u_int *getregaddr;
        int *r_y;
        int *pcb_psrp;
        { return (0); }

/*
 * Routines from copy.s.
 */

/*ARGSUSED*/
int kcopy(from, to, count) caddr_t from, to; u_int count; { return (0); }

/*ARGSUSED*/
bcopy(from, to, count) caddr_t from, to; u_int count; { return; }

/*ARGSUSED*/
ovbcopy(from, to, count) caddr_t from, to; u_int count; { return; }

/*ARGSUSED*/
int kzero(base, count) caddr_t base; u_int count; { return (0); }

/*ARGSUSED*/
bzero(base, count) caddr_t base; u_int count; { return; }

/*ARGSUSED*/
blkclr(base, count) caddr_t base; u_int count; { return; }

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
int suword(base, i) caddr_t base; int i; { return (0); }

/*ARGSUSED*/
suiword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
subyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suibyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
fusword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
susword(base, i) caddr_t base; { return; }

/*
 * Routines from float.s
 */
fpu_probe() { return; }

/*ARGSUSED*/
syncfpu(rp) struct regs *rp; { return; }
/*
 * Routines from addupc.s
 */

/*ARGSUSED*/
addupc(pc, pr, incr) int pc; struct uprof *pr; int incr; { return; }

/*
 * Routines from ocsum.s
 */

/*ARGSUSED*/
ocsum(p, len) u_short *p; int len; { return (0); }

/*
 * Routines from sparc_subr.s
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
spl8() { return (0); }
splzs() { return (0); }
splnet() { return (0); }
splimp() { return (0); }
splclock() { return (0); }
splsoftclock() { return (0); }
splbio() { return (0); }
spltty() { return (0); }
splhigh() { return (0); }
splaudio() { return(0); }

no_fault() { return; }
on_fault() { return(0); }
func_t caller() { return((func_t) 0); }
func_t callee() { return((func_t) 0); }

extern int splvm_val;
splvm() { return (splvm_val); }

/*ARGSUSED*/
setjmp(lp) label_t *lp; { return (0); }

/*ARGSUSED*/
longjmp(lp) label_t *lp; { /*NOTREACHED*/ }

enable_dvma() { return; }

/*ARGSUSED*/
on_enablereg(reg_bit) u_char reg_bit; { return; }

/*ARGSUSED*/
off_enablereg(reg_bit) u_char reg_bit; { return; }


/*ARGSUSED*/
set_intreg(bit, flag) int bit, flag; { return; }

/*ARGSUSED*/
_remque(p) caddr_t p; { return; }

/*ARGSUSED*/
_insque(out, in) caddr_t out, in;  { return; }

/*ARGSUSED*/
scanc(size, cp, table, mask)
u_int size; caddr_t cp, table; int mask; { return (0); }

/*ARGSUSED*/
movtuc(size, from, origto, table)
int size; u_char *from;	u_char *origto; u_char *table; { return (0); }

/*ARGSUSED*/
usec_delay(n) int n; { return; }

/*
 * Routines from subr.s
 */
/*ARGSUSED*/
getidprom(s) char *s; { return; }

/*ARGSUSED*/
void
flush_writebuffers_to(v) addr_t v; { return; }

void
flush_all_writebuffers() { flush_all_writebuffers(); }

void
flush_poke_writebuffers() { flush_poke_writebuffers(); }

/*
 * Routines from sparc/swtch.s
 */
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
setmmucntxt(procp) struct proc *procp; { return; }

void
icode1() { struct regs regs; icode(&regs); }

/*ARGSUSED*/
setrq(p) struct proc *p; { return; }

/*ARGSUSED*/
remrq(p) struct proc *p; { return; }

swtch() { if (whichqs) whichqs = 0; resume(masterprocp); }

/*ARGSUSED*/
resume(p) struct proc *p; {
	extern int kadb_defer, kadb_want;
	kadb_defer = 1;
	if (masterprocp) masterprocp = p;
	if (kadb_want) { kadb_want = 0; call_debug_from_asm(); }
	kadb_defer = 0;
}


/*
 * Routines from map.s
 */

/*ARGSUSED*/
u_int map_getpgmap(v) addr_t v; { return ((u_int)0); }

/*ARGSUSED*/
void map_setpgmap(v, pte) addr_t v; u_int pte; { return; }

/*ARGSUSED*/
u_int map_getsgmap(v) addr_t v; { return ((u_int)0); }

/*ARGSUSED*/
map_setsgmap(v, pm) addr_t v; u_int pm; { return; }

u_int map_getctx() { return (0); }

/*ARGSUSED*/
map_setctx(c) u_int c; { return; }

#ifdef VAC

/*ARGSUSED*/
void vac_ctxflush() { return; }

/*ARGSUSED*/
void vac_segflush(v) addr_t v; { return; }

/*ARGSUSED*/
void vac_pageflush(v) addr_t v; { return; }

/*ARGSUSED*/
void vac_flush(v, l) addr_t v; { return; }

/*ARGSUSED*/
void vac_control(n) int n; { return; }

void vac_flushall() { return; }

vac_usrflush() { return; }

/*ARGSUSED*/
vac_dontcache(p) addr_t p; { return; }

#endif VAC

/*
 * From vm_hatasm.s
 */

/*ARGSUSED*/
void hat_pmgloadptes(a, ppte) addr_t a; struct pte *ppte; { return; }

/*ARGSUSED*/
void hat_pmgunloadptes(a, ppte) addr_t a; struct pte *ppte; { return; }

/*ARGSUSED*/
void hat_pmgswapptes(a, pnew, ppte)
addr_t a; struct pte *pnew, *ppte; { return; }


/*
 * From zs_asm.s
 */

#include "zs.h"

setzssoft() { return; }

clrzssoft() { return (0); }

/*ARGSUSED*/
zszread(zsaddr, n) struct zscc_device *zsaddr; int n; { return (0); }

/*ARGSUSED*/
zszwrite(zsaddr, n, v) struct zscc_device *zsaddr; int n, v; { return; }

zslevel12() { return; }

/*
 * from fd_asm.s
 */

fdc_hardintr() { return; }

/*ARGSUSED*/
fd_set_dor(bit, flag) { return; }

/*
 * from audio_79C30_intr.s
 */

/*
 * from sparc/float.s
 */

/*ARGSUSED*/
void _fp_read_pfreg(pf, n)  FPU_REGS_TYPE *pf; u_int n; { return; }

/*ARGSUSED*/
void _fp_write_pfreg(pf, n)  FPU_REGS_TYPE *pf; u_int n; { return; }

/*ARGSUSED*/
void _fp_write_pfsr(pf) FPU_FSR_TYPE *pf; { return; }
/*ARGSUSED*/
void fp_enable(fp) struct fpu *fp; {return; }

#ifdef	LWP
/*
 * from lwp/low.s
 */
void __lowinit() { return; }
__checkpoint() { return; }
__swtch() { return; }
void __setstack() { return; }

#endif	/* LWP */


/*
 * *sigh*- pixrect stuff that I don't want to bug mr. d. about.
 */

#include "cgnine.h"
#if	NCGNINE == 0
struct pixrectops cg9_ops;
int cg9_putcolormap() { return (0); }
int cg9_putattributes() { return (0); }
int cg9_rop() { return (0); }
int cg9_ioctl() { return (0); }
#endif	NCGNINE == 0

#include "cgtwo.h"
#if	NCGTWO == 0
struct pixrectops cg2_ops;
int cg2_putcolormap() { return (0); }
int cg2_putattributes() { return (0); }
int cg2_rop() { return (0); }
int cg2_ioctl() { return (0); }
#endif	NCGTWO == 0

static void
pixrect_resolve()
{
	extern int _loop;
	_loop = 23;	/* ...skidoo */
#if	NCGNINE == 0
	cg9_ops.pro_putcolormap = cg9_putcolormap;
	cg9_ops.pro_putattributes = cg9_putattributes;
	cg9_ops.pro_rop = cg9_rop;
	(void) cg9_ioctl();
#endif	NCGNINE == 0
#if	NCGTWO  == 0
	cg2_ops.pro_putcolormap = cg2_putcolormap;
	cg2_ops.pro_putattributes = cg2_putattributes;
	cg2_ops.pro_rop = cg2_rop;
	(void) cg2_ioctl();
#endif	NCGTWO == 0
}

/*
 * Variables declared for savecore, or
 * implicitly, such as by config or the loader.
 */
char	version[] = "SunOS Release ...";
char	start[1];
char	etext[1];
char	end[1];
