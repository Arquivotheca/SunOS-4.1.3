/*	@(#)netboot.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/netboot.c	10.20" */
/*
 *  System call to start file sharing.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/file.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <rfs/nserve.h>
#include <rfs/rfs_mnt.h>
#include <rfs/message.h>
#include <rfs/adv.h>
#include <rfs/recover.h>
#include <rfs/rdebug.h>
#include <rfs/rfs_serve.h>
#include <rfs/idtab.h>
#include <rfs/cirmgr.h>

extern	struct	proc *rfsdp;
extern	struct	vnode *rfs_cdir;
extern	int	msgflag;
extern	rcvd_t	rd_recover;

extern struct vnode *rootdir;   /* pointer to root vnode--set during system init */

extern	int	bootstate;		/*  DU_UP, DU_DOWN, or	*/
					/*  DU_INTER		*/

extern comminit(), gdp_init(), auth_init();
extern void commdinit();

extern void recover_init();
extern  struct  proc *rec_proc;	/* sleep address for recovery */
extern  int	rec_flag;	/* set KILL bit to kill recovery */

rfstart ()
{
	struct serv_proc *sp;
	extern int gdp_init ();
	register int	s;
	extern int maxserve;
	extern void recovery();
	extern void rfdaemon();

	DUPRINT1(DB_RFSTART, "rfstart system call\n");

	if (!suser())
		return;

	while (bootstate & DU_INTER) {
		(void) sleep((caddr_t) &bootstate, PREMOTE);
	}

	/*
	 * This is a critical section. Only one process at a time
	 * can execute this code.
	 */

	if (bootstate == DU_UP)  {
		DUPRINT1 (DB_RFSTART, "rfstart: system already booted\n");
		u.u_error = EEXIST;
		wakeup((caddr_t) &bootstate);
		return;
	}

	bootstate = DU_INTER;	/* RFS in an intermediate state */

	/* Allocate memory for RFS data structures */
	rfs_memalloc();

	if (comminit() == RFS_FAILURE) {
		u.u_error = EAGAIN;
		bootstate = DU_DOWN;
		wakeup((caddr_t) &bootstate);
		return;
	}
	DUPRINT1(DB_RFSTART, "comm initialized\n");
	auth_init();
	gdp_init();
	if (u.u_error) {
		DUPRINT2 (DB_RFSTART, "rfstart u.u_error %d\n", u.u_error);
		commdinit();
		bootstate=DU_DOWN;
		wakeup((caddr_t) &bootstate);
		return;
	}
	DUPRINT1(DB_RFSTART, "gdp initialized\n");

	recover_init();
	DUPRINT1(DB_RFSTART, "rfstart: recovery initialized\n");

	/* start recovery process */
	kern_proc(recovery, 0);

	/* start daemon process */
	kern_proc(rfdaemon, 0);

	/*  now allow advertise calls, set all sysid's in proc table  */
	s = splrf();
	for (sp = &serv_proc[0]; sp < &serv_proc[maxserve]; sp++)
		sp->s_sysid = 0;
	(void) splx(s);

	/*
	 * bootstate set when rfdaemon is running -- see rfadmin.c
	 */
	return;
}

/*
 * Release various unneeded resources for the kernel procs.
 */
netmemfree()
{
	int i;

	VN_RELE(u.u_cdir);
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	if (u.u_rdir) {
		VN_RELE(u.u_rdir);
	}
	u.u_rdir = NULL;
	for (i = 0; i < u.u_lastfile; i++)
		if (u.u_ofile[i] != NULL) {
			closef(u.u_ofile[i]);
			u.u_ofile[i] = NULL;
			u.u_pofile[i] = 0;
		}
}


/*
 *	System call to stop file sharing - if everything is quiet.
 */
rfstop()
{
	register struct rfsmnt *mp;
	register struct srmnt	*sp;
	register struct advertise *ap;


	DUPRINT1(DB_RFSTART, "rfstop system call\n");

	if (!suser())
		return;

	/*
	 * Begin critical section.  As in rfstart, only one process at a time
	 * through this section of code.
	 */
	while (bootstate & DU_INTER) {
		(void) sleep((caddr_t) &bootstate, PREMOTE);
	}

	if (bootstate == DU_DOWN) {
		DUPRINT1(DB_RFSTART, "rfstop: system already stopped\n");
		u.u_error = ENONET;
		wakeup((caddr_t) &bootstate);
		return;
	}

	bootstate &= ~DU_MASK;
	bootstate |= DU_INTER;

	/*
	 * can't stop if anything is remotely mounted
	 * (starting at rfsmount[1] means we're not looking at root)
	 *
	 * If any local proc has an SD (cdir, rdir, open file) that points
	 * into a remotely mounted filesystem, it will not be possible to
	 * unmount that filesystem.  (See rumount.)
	 */
	for (mp = rfsmount; mp < (struct rfsmnt *)&rfsmount[nrfsmount]; mp++) {
		if (mp->rm_flags & RINUSE) {
			DUPRINT1 (DB_RFSTART,
				"rfstop: can't stop with remote mounts.\n");
			u.u_error = EBUSY;
			bootstate=DU_UP;
			wakeup((caddr_t) &bootstate);
			return;
		}
	}

	/* can't stop if this machine has clients */
	for (sp = srmount; sp < &srmount[nsrmount]; sp++) {
		if (sp->sr_flags != MFREE) {
			DUPRINT1 (DB_RFSTART,
				"rfstop: can't stop with clients.\n");
			u.u_error = ESRMNT;
			bootstate=DU_UP;
			wakeup((caddr_t) &bootstate);
			return;
		}
	}

	/* can't stop if anything is advertised  */
	/* adv table locked above */
	for (ap = advertise; ap < &advertise[nadvertise]; ap++) {
		if (ap->a_flags != A_FREE) {
			DUPRINT1 (DB_RFSTART,
			    "rfstop: can't stop with advertised resources.\n");
			u.u_error = EADV;
			bootstate = DU_UP;
			wakeup((caddr_t) &bootstate);
			return;
		}
	}

	DUPRINT1(DB_RFSTART, "rfstop: taking down links \n");
	kill_gdp();		/* cut all connections */
	commdinit();

	/* kill daemons - bootstate goes to DOWN after both die */
	DUPRINT1(DB_RFSTART, "rfstop: killing daemons \n");
	msgflag |= RFSKILL;
	wakeup ((caddr_t) &rd_recover->rd_qslp);
	rec_flag |= RFSKILL;
	wakeup ((caddr_t) &rec_proc);

	/* Wait til all rfs processes are down, free mem and set state DOWN */
	while ((bootstate & DU_ALL_DOWN) != DU_ALL_DOWN) {
		DUPRINT2(DB_RFSTART, "rfstop coming down : bootstate %x\n",
				bootstate);
		(void) sleep((caddr_t) &bootstate, PREMOTE);
	}
	rfs_memfree();
	bootstate = DU_DOWN;
	wakeup((caddr_t) &bootstate);

	DUPRINT1(DB_RFSTART, "rfstop: done\n");
}
