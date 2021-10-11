/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)parms.h 1.1 92/07/30"	/* from SVR3.2 uucp:parms.h 2.14 */

/* go through this carefully, configuring for your site */

/* If running SVR3, #define both ATTSVR3 and ATTSV */
/* #define ATTSVR3	/* System V Release 3 */

/* One of the following five lines should not be commented out.
 * The other four should be unless you are running a unique hybrid.
 */

/* #define	ATTSV	/* System III or System V */
/* #define	V7	/* Version 7 systems (32V, Berkeley 4BSD, 4.1BSD) */
#define	BSD4_3	/* Berkeley 4.3BSD */
/* #define	BSD4_2	/* Berkeley 4.2BSD */
/* #define	V8	/* Research Eighth Edition */


/* Owner of setud files running on behalf of uucp.  Needed in case
 * root runs uucp and euid is not honored by kernel.
 * GID is needed for some chown() calls.
 * Also used if guinfo() cannot find the current users ID in the
 * password file.
 */
#define UUCPUID		4	/* */
#define UUCPGID		8	/* */

/* define ATTSVKILL if your system has a kill(2) that accepts kill(0, pid)
 * as a test for killability.  If ATTSV or BSD4_2 is defined this will
 * automatically be defined anyway.
 */
/* #define ATTSVKILL	/* */

/* define MKDIR if your system has mkdir(2).  If ATTSVR3 or BSD4_2 is defined
 * this will automatically be defined anyway.
 */
/* #define MKDIR	/* */

/* define POSIXDIRLIB if your system has a POSIX-style "directory library"
 * (with "readdir" returning a "struct dirent *".  If ATTSVR3 is defined this
 * will automatically be defined anyway.
 */
#define POSIXDIRLIB	/* */

/* define ATTSVTTY if your system has a System V (or System III)-style tty
 * driver ("termio").  If ATTSV is defined this will automatically be defined
 * anyway.
 */
#define ATTSVTTY	/* */

/* define BSDINETD if you are using /etc/inetd with 4.2bsd.  If BSD4_3 is
 * defined this will automatically be defined anyway.
 */
#define BSDINETD	/**/

/*
 * the next two lines control high resolution sleeps, called naps.
 *
 * most UNIX versions have no nap() system call; they want NONAP defined,
 * in which case one is provided in the code.  this includes all standard
 * versions of UNIX.
 *
 * some sites use a fast timer that reads a number of clock ticks and naps
 * for that interval; they want NONAP defined, and FASTTIMER defined as
 * the name of the device, e.g., /dev/ft.
 *
 * repeating, NONAP should be disabled *only* if your standard library has a
 * function called nap.
 */


#define NONAP	/* nominal case -- no nap() in the standard library */
/* #define FASTTIMER "/dev/ft"   /* identify the device used for naps */

/*
 * we use ustat to decide whether there's enough space to receive a
 * file.  if you're not ATTSV:
 *
 *	1) if you have the SunOS "statfs" system call (many systems with NFS
 *	   that have NFS have it), you can use it instead of "ustat";
 *
 *	2) otherwise, you can use a setgid program to read the number of free
 *	   blocks and free inodes directly off the disk.
 *
 * if you choose either course, do not define NOUSTAT; rather:
 *
 *	1) if you choose 1) above, define STATFS;
 *
 *	2) if you choose 2) above, define V7USTAT to be the name of the
 *	   program.  be sure it accepts 2 args, major and minor device numbers,
 *	   and returns two numbers, blocks and inodes, in "%d %d" format, or
 *	   you'll never receive another file.
 */
/* #define V7USTAT  "/usr/local/lib/ustat" /* */
#define STATFS		/* if you have "statfs" system call */
/* #define NOUSTAT  /* define NOUSTAT if you don't have ustat */

/* define GRPCHK if you want to restrict the ability to read */
/* Systems file stuff by way of the DEBUG flags based on a group id range */
/* ex: if (GRPCHK(getgid()) no_secrets(); */
#define GRPMIN	4	/* */
#define GRPMAX	4	/* */
#define GRPCHK(gid)	( gid >= GRPMIN && gid <= GRPMAX ? 1 : 0 )	/* */
/* #define GRPCHK(gid)	1	/* Systems info is not protected from DEBUG */

/* definitions for the types of networks and dialers that are available */
/* #define DATAKIT		/* define DATAKIT if datakit is available.  Datakit
			 * can also be used by cu.  Thus it is defined
			 * outside of STANDALONE. */
#ifndef STANDALONE
/* #define UNET		/* define UNET if you have 3com ethernet software */
#define TCP		/* TCP (bsd systems) */
/* #define SYTEK	/* for sytek network */
#endif /* ! STANDALONE */

#if	defined ( ATTSVR3 ) && ! defined ( DIAL )

#define TLI		/* for AT&T Transport Layer Interface networks */
#define TLIS		/* for AT&T Transport Layer Interface networks */
			/* with streams module "tirdwr" */
#endif /* ATTSVR3, not from dial(3) */


/* #define DIAL801	/* 801/212-103 auto dialers */

#define X25		/* define X25 if you want to use the xio protocol */
#define PAD		/* for `f' (7-bit) protocol */

#define MAXCALLTRIES	2	/* maximum call attempts per Systems file line */

/* define DUMB_DN if your dn driver (801 acu) cannot handle '=' */
/* #define DUMB_DN /*  */

/* define DEFAULT_BAUDRATE to be the baud rate you want to use when both */
/* Systems file and Devices file allow Any */
#define DEFAULT_BAUDRATE "9600"	/* */

/*define permission modes for the device */
#define M_DEVICEMODE 0600	/* MASTER device mode */
#define S_DEVICEMODE 0600	/* SLAVE device mode */
#define R_DEVICEMODE 0600	/* default mode to restore */

/* NO_MODEM_CTRL - define this if you have very old hardware
 * that does not know how to correctly handle modem control
 * Some old pdp/11 hardware such as dk, dl
 * If you define this, and have DH devices for direct lines,
 * the ports will often hang and be unusable.
*/
/*#define NO_MODEM_CTRL		/* */


/* UUSTAT_TBL - this is the maximum number of machines that
 * status may be needed at any instant.
 * If you are not concerned with memory for a seldom used program,
 * make it very large.
 * This number is also used in uusched for its machine table -- it has
 * the same properties as the one in uustat.
 */

#define UUSTAT_TBL 1000		/* big machine with lots of traffic */
/* #define UUSTAT_TBL 200

/* define UNAME if uname() should be used to get uucpname
 * This will be defined automatically if ATTSV is defined
 */
/* #define UNAME /*  */

/* initial wait time after failure before retry */
#define RETRYTIME 300		/* 5 minutes */
/* MAXRETRYTIME is for exponential backoff  limit.
 * NOTE - this should not be 24 hours so that
 * retry is not always at the same time each day
 */
#define MAXRETRYTIME 82800	/* 23 hours */
#define ASSERT_RETRYTIME 86400	/* retry time for ASSERT errors */

/*  This is the path that will be used for uuxqt command executions */
#define PATH	"PATH=/bin:/usr/bin " /* */
/* #define PATH	"PATH=/bin:/usr/bin:/usr/lbin " /* */

/*  This is the set of default commands that can be executed */
/*  if non is given for the system name in PERMISSIONS file */
/*  It is a colon separated list as in PERMISSIONS file */
#define DEFAULTCMDS	"rmail"	/* standard default command list */

/* define HZ to be the number of clock ticks per second */
#define HZ 60 /* not needed for ATTSV or above */

/*
 * put in local uucp name of this machine if there is no "/etc/whoami"
 * and no uname() (this is a last resort)
 */
/* #define MYNAME		"kilroy"	/* */

/* define NOSTRANGERS if you want to reject calls from systems that
 * are not in your Systems file.   If defined, NOSTRANGERS should be the name
 * of the program to execute when such a system dials in.  The argument
 * to said program will be the name of said system.  Typically this is a shell
 * procedure that sends mail to the uucp administrator informing them of an
 * attempt to communicate by an unknown system.
 * NOTE - if this is defined, it can be overridden by the administrator
 * by making the command non-executable.  (It can be turned on and off
 * by changing the mode of the command.)
 */
#define NOSTRANGERS	"/etc/uucp/remote.unknown"	/* */

/* define LMTUUXQT to be the name of a file which contains the number
 * (in ascii) of simultaneous uuxqt's which you will permit.  If it is
 * not defined, there may be "many" uuxqt's running. 5 is reasonable number.
 */
#define LMTUUXQT	"/etc/uucp/Maxuuxqts"	/* */

/* define LMTUUSCHED to be the name of a file which contains the number
 * (in ascii) of simultaneous uusched's which you will permit.  If it is
 * not defined, there may be "many"  uusched's running. 2 is reasonable number.
 */
#define LMTUUSCHED	"/etc/uucp/Maxuuscheds"	/* */

/* define USRSPOOLLOCKS if you like your lock files in /usr/spool/locks
 * be sure other programs such as 'cu' and 'ct' know about this
 *
 * WARNING: if you do not define USRSPOOLLOCKS, then $LOCK in 
 * uudemon.cleanup must be changed.
 */
#define USRSPOOLLOCKS  /* define to use /usr/spool/locks for LCK files */

/* define PKSPEEDUP if you want to try the recommended speedup in pkcget.
 * this entails sleeping between reads at low baud rates.
 */
#define PKSPEEDUP	/* */
