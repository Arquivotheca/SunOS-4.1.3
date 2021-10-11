#ident	"@(#)stubs.c 1.1 92/07/30 SMI"

/*
 * Stubs for routines that can't be configured
 * out with binary-only distribution.
 */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <net/if.h>
#include <ufs/quota.h>
#include <ufs/mount.h>

#ifndef GENERIC
setconf() { }
#endif !GENERIC

#ifndef SYSACCT
struct	vnode *acctp;
struct	vnode *savacctp;
sysacct() { return (ENODEV); }
acct() { }
#endif !SYSACCT

#ifndef SYSAUDIT
auditsys() { return (0); }
au_sysaudit() { return (0); }
void au_pathbuild() { }
void cwfree() { }
void cwincr() { }
#endif !SYSAUDIT

#ifndef QUOTA
void qtinit() { }
struct dquot *getinoquota() { return ((struct dquot *)0); }
int chkdq() { return (0); }
int chkiq() { return (0); }
void dqrele() { }
int closedq() { return (0); }
int qsync() { return (0); }
#endif !QUOTA

#ifndef LWP
struct proc *oldproc = (struct proc*)0;
#ifndef ASYNCHIO
int killable() {}
int mark_unwanted() {}
void lwpschedule() {}
void return_stk() {}
int cmsleep() {}
int cmwakeup() {}
int unixset() {}
int unixclr() {}
int sleepset() {}
int sleepget() {}
int lwpinit() {}
int lwp_resume() {}
int lwp_self() {}
int lwp_create() {}
int lwp_datastk() {}
int lwp_destroy() {}
int __Nrunnable = 0;
int runthreads = 0;
long __Curproc = 0;
int sleepqsave = 0;
#endif ASYNCHIO
#endif LWP

#ifndef NIT
struct ifnet *nit_ifwithaddr() { return ((struct ifnet *)0); }
nit_tap() {}
struct domain nitdomain =
	{ AF_NIT, "nit", 0, 0, 0, (struct protosw *)0, (struct protosw *)0 };
#endif !NIT

#include "snit.h"
#if NSNIT == 0
snit_intr() {}
#endif NSNIT == 0

#include "ether.h"
#if NETHER == 0
int arpioctl() { return (ENOPROTOOPT); }
int localetheraddr() { return (0); }
#endif NETHER == 0

#include "kb.h"
#if NKB == 0
kbdreset() {}
kbdsettrans() {}
#endif NKB == 0

#include "ms.h"
#if NMS == 0
msintr() { }
#endif NMS == 0

#include "zs.h"
#if NZS == 0
zslevel6intr() { panic("level 6 interrupt and no ZS device configured"); }
#endif NZS == 0

#ifndef IPCSEMAPHORE
seminit() {}
semexit() {}
#endif !IPCSEMAPHORE

#ifndef IPCMESSAGE
msginit() {}
#endif !IPCMESSAGE

#ifndef IPCSHMEM
shmexec() {}
shmfork() {}
shmexit() {}
#endif !IPCSHMEM

#include "fpa.h"
#if NFPA == 0
int fpa_fork_context() { return (0); }
fpa_shutdown() {}
fpa_dorestore() {}
fpa_save() {}
fpa_restore() {}
#endif NFPA == 0

#ifndef UFS
int ilock() {}
int iunlock() {}
struct vnodeops *ufs_vnodeops;
int old_ufs_readdir() {}
int ufs_vnodeconf() {}
struct mount *mounttab = 0;
int mounttab_flags = 0;
struct inode *ifreeh = 0;
#endif

#include "win.h"
#if NWIN > 0

/* stubs needed when some frame buffer drivers are configured out */

#include "cgtwo.h"
#if NCGTWO == 0
cg2_putcolormap() {}	/* for pixrect/pr_dblbuf.c */
cgtwo_wait() {}		/* for pixrect/pr_dblbuf.c */
#endif NCGTWO == 0

#include "gpone.h"
#if NGPONE == 0
gp1_sync() {}		/* for pixrect/cg2_{colormap, rop}.c */
gp1_sync_from_fd() {}	/* for pixrect/pr_dblbuf.c */
#endif NGPONE == 0

#include "cgfour.h"
#if NCGFOUR == 0
cg4_putattributes() {}	/* for pixrect/pr_plngrp.c */
#endif NCGFOUR == 0

#include "cgeight.h"
#if NCGEIGHT == 0
cg8_putattributes() {}  /* for pixrect/pr_plngrp.c */
#endif NCGEIGHT == 0

#include "cgsix.h"
#if NCGSIX == 0
cg6_putattributes() {}  /* for pixrect/pr_plngrp.c */
#endif NCGSIX == 0

#include "cgnine.h"
#if NCGNINE == 0
cg9_putattributes() {return 0; }
cg9_rop() {return 0; }
cg9_putcolormap() {return 0; }
cgnineioctl() {return 0; }
#endif NCGNINE == 0

#else NWIN

/* stubs needed when windows are configured out */

#include "bwtwo.h"
#if NBWTWO > 0
mem_rop() {}
mem_putcolormap() {}
mem_putattributes() {}
#endif NBWTWO > 0

#include "cgtwo.h"
#if NCGTWO > 0
cg2_rop() {}
cg2_putcolormap() {}
cg2_putattributes() {}
#endif NCGTWO > 0

#include "cgfour.h"
#if NCGFOUR > 0
/* mem_rop(), mem_putcolormap() taken care of above */
cg4_putattributes() {}
#endif NCGFOUR > 0

#include "cgeight.h"
#if NCGEIGHT > 0
cg8_putattributes() {}
#endif NCGEIGHT > 0

#endif NWIN

#ifndef CRYPT
int _des_crypt() { return (0); }
#endif

#include "hrc.h"
#if NHRC == 0
/*
 * HRC device cannot be acquired if Not
 * configured -- these references in kern_trace.c
 */
int    hrc_acquire() { return (0); }
u_long hrc_time() { return (0); }
#endif NHRC == 0

#ifndef WINSVJ
int svjwrite() { return (0); }
void svjsendevent() { }
int svjioctl() { return (0); }
void ws_dealloc_rec_q() { }
int svj_consume_input_event() { return (0); }
void svj_consume_event() { }
#endif !WINSVJ

/*
 * Stuff for interactions between OPENBOOT and SCSA for diskless case
 */
#ifndef OLDSCSI
#include "scsibus.h"
#if	NSCSIBUS == 0
struct scsi_device *sd_root;
#endif	/* NSCSIBUS == 0 */
#endif	/* !OLDSCSI */

#ifndef VDDRV
int vd_unuseddev() { return(0); }
#endif
