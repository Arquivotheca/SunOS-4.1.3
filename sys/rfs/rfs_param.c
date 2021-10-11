/*	@(#)rfs_param.c 1.1 92/07/30 SMI 	*/

/*LINTLIBRARY*/
	
/* RFS private data structures and sizes - allocated at startup time */
/* From AT&T 5.3.11 usr/src/uts/3b2/master.d/du */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/mount.h>
#include <rfs/rfs_misc.h>
#include <rfs/adv.h>
#include <rfs/nserve.h>
#include <rfs/sema.h>
#include <rfs/cirmgr.h>
#include <rfs/comm.h>
#include <rfs/message.h>
#include <rfs/rfs_mnt.h>
#include <rfs/rfs_serve.h>


#define NADVERTISE	25
#define MAXGDP		24
#define NRCVD		650
#define NRDUSER		1000
#define NSNDD		700
#define MINSERVE	3
#define MAXSERVE	6
#define HIST 		6
#define RFHEAP		4000
#define RFS_VHIGH	1
#define RFS_VLOW	1
#define NRFSMOUNT  	25
#define NSRMOUNT	100

struct advertise *advertise;
int nadvertise = NADVERTISE;
struct rcvd *rcvd;
int nrcvd = NRCVD;
struct sndd *sndd;
int nsndd = NSNDD;
struct gdp *gdp;
int maxgdp = MAXGDP;
int minserve = MINSERVE;
int maxserve = MAXSERVE;
struct rd_user *rd_user;
int nrduser = NRDUSER;
char *rfheap;
int rfsize = RFHEAP;
int rfs_vhigh = RFS_VHIGH;
int rfs_vlow = RFS_VLOW;
struct rfsmnt *rfsmount;
int nrfsmount = NRFSMOUNT;
struct srmnt *srmount;
int nsrmount = NSRMOUNT;
struct serv_proc *serv_proc;
short *rfdev;
int nservers = 0;
int nzombies = 0;
int msglistcnt = 0;
int bootstate = DU_DOWN;
int rcache_enable = 0;
int rcacheinit = 0;
long dudebug = 0x0000000;

/* A server must have had at least this many seconds elapsed since it last 
   serviced a request to be eligible for retirement */
long rfs_serve_time = 10;

/* Magic user id which gives a Sun RFS client super-user permissions on
   an RFS server, which the lookup op DULINK requires to work properly */
int rfs_magic_cred = -1;


/* Translation tables for various S5R3 codes which are shipped over the
   wire by RFS */


/* S5R3 inode formats <--> vnode types */
enum vtype rfstovt_tab[] = {
        VNON, VFIFO, VCHR, VBAD, VDIR, VBAD, VBLK, VBAD, VREG, VBAD, VBAD, VBAD
};

int vttorfs_tab[] = {
        0, RFS_IFREG, RFS_IFDIR, RFS_IFBLK, RFS_IFCHR, RFS_IFREG, 
	RFS_IFMT, RFS_IFMT, RFS_IFIFO
};

/* Error # tables */

int rfstoerr_tab[] = { 
	RFS_OK,	/*	0 	 No error */
	EPERM,  /*	1        Not super-user                       */
	ENOENT, /*	2        No such file or directory            */
	ESRCH,	/*	3        No such process                      */
	EINTR,	/*	4        interrupted system call              */
	EIO,	/*	5        I/O error                            */
	ENXIO,	/*	6        No such device or address            */
	E2BIG,	/*	7        Arg list too long                    */
	ENOEXEC,/*	8        Exec format error                    */
	EBADF,	/*	9        Bad file number                      */
	ECHILD,	/*	10       No children                          */
	EAGAIN,	/*	11       No more processes                    */
	ENOMEM,	/*	12       Not enough core                      */
	EACCES,	/*	13       Permission denied                    */
	EFAULT,	/*	14       Bad address                          */
	ENOTBLK,/*	15       Block device required                */
	EBUSY,	/*	16       Mount device busy                    */
	EEXIST,	/*	17       File exists                          */
	EXDEV,	/*	18       Cross-device link                    */
	ENODEV,	/*	19       No such device                       */
	ENOTDIR,/*	20       Not a directory                      */
	EISDIR,	/*	21       Is a directory                       */
	EINVAL,	/*	22       Invalid argument                     */
	ENFILE,	/*	23       File table overflow                  */
	EMFILE,	/*	24       Too many open files                  */
	ENOTTY,	/*	25       Not a typewriter                     */
	ETXTBSY,/*	26       Text file busy                       */
	EFBIG,	/*	27       File too large                       */
	ENOSPC,	/*	28       No space left on device              */
	ESPIPE,	/*	29       Illegal seek                         */
	EROFS,	/*	30       Read only file system                */
	EMLINK,	/*	31       Too many links                       */
	EPIPE,	/*	32       Broken pipe                          */
	EDOM,	/*	33       Math arg out of domain of func       */
	ERANGE,	/*	34       Math result not representable        */
	BADERR,	/*	35       No message of desired type           */
	BADERR,	/*	36	 Identifier removed			*/
	BADERR,	/*	37	 Channel number out of range		*/
	BADERR,/*       38	 Level 2 not synchronized		*/
	BADERR,	/*	39	 Level 3 halted			*/
	BADERR,	/*	40	 Level 3 reset			*/
	BADERR,	/*	41	 Link number out of range		*/
	BADERR,/* 	42	 Protocol driver not attached		*/
	BADERR,	/*	43	 No CSI structure available		*/
	BADERR,	/*	44	 Level 2 halted			*/
	EDEADLK,/*	45	 Deadlock condition.			*/
	ENOLCK,	/*	46	 No record locks available.		*/

	BADERR,	/*	47	 Not defined in S5R3.		*/
	BADERR,	/*	48	 Not defined in S5R3.		*/
	BADERR,	/*	49	 Not defined in S5R3.		*/
	
	 /* Convergent Error Returns */
	BADERR,	/*	50	 invalid exchange			*/
	BADERR,	/*	51	 invalid request descriptor		*/
	BADERR,	/*	52	 exchange full			*/
	BADERR,	/*	53	 no anode				*/
	BADERR,/*	54	 invalid request code			*/
	BADERR,/*	55	 invalid slot				*/
	BADERR,/* 	56	 file locking deadlock error		*/
	BADERR,	/*	57	 bad font file fmt			*/

	BADERR,	/*	58	 Not defined in S5R3.		*/
	BADERR,	/*	59	 Not defined in S5R3.		*/

	 /* stream problems */
	ENOSTR,	/*	60	 Device not a stream			*/
	BADERR, /*	61	 no data (for no delay io)		*/
	ETIME,	/*	62	 timer expired			*/
	ENOSR,	/*	63	 out of streams resources		*/

	ENONET,	/*	64	 Machine is not on the network	*/
	BADERR,	/*	65	 Package not installed                */
	ERREMOTE,/*	66	 The object is remote			*/
	ENOLINK,/*	67	 the link has been severed */
	EADV,	/*	68	 advertise error */
	ESRMNT,	/*	69	 srmount error */
	ECOMM,	/*	70	 Communication error on send		*/
	EPROTO,	/*	71	 Protocol error			*/

	BADERR,	/*	72	 Not defined in S5R3.		*/
	BADERR,	/*	73	 Not defined in S5R3.		*/

	EMULTIHOP,/* 	74	 multihop attempted */
	BADERR,/*	75	 Inode is remote (not really error)*/
	EDOTDOT,/* 	76	 Cross mount point (not really error)*/
	EBADMSG,/* 	77	 trying to read unreadable message	*/

	BADERR,	/*	78	 Not defined in S5R3.		*/
	BADERR,	/*	79	 Not defined in S5R3.		*/

	BADERR, /* 	80	 given log. name not unique */
	BADERR,	/*	81	 f.d. invalid for this operation */
	EREMCHG,/*	82	 Remote address changed */

	/* shared library problems */
	BADERR,/*	83	 Can't access a needed shared lib. 	*/
	BADERR,/*	84	 Accessing a corrupted shared lib.	*/
	BADERR,/*	85	 .lib section in a.out corrupted.	*/
	BADERR,/*	86	 Attempting to link in too many libs.	*/
	BADERR /*	87	 Attempting to exec a shared library.	*/
};
/* Number of RFS errors */
int rfserr_size = sizeof(rfstoerr_tab)/sizeof(rfstoerr_tab[0]);	

int errtorfs_tab[] = {
	RFS_OK,		/*	0  		 No errror  */
	RFS_EPERM,	/*	1		 Not owner */
	RFS_ENOENT,	/*	2		 No such file or directory */ 
	RFS_ESRCH,	/*	3		 No such process */
	RFS_EINTR,	/*	4		 Interrupted system call */
	RFS_EIO,	/*	5		 I/O error */
	RFS_ENXIO,	/*	6		 No such device or address */
	RFS_E2BIG,	/*	7		 Arg list too long */
	RFS_ENOEXEC,	/*	8		 Exec format error */
	RFS_EBADF,	/*	9		 Bad file number */
	RFS_ECHILD,	/*	10		 No children */
	RFS_EAGAIN,	/*	11		 No more processes */
	RFS_ENOMEM,	/*	12		 Not enough core */
	RFS_EACCES,	/*	13		 Permission denied */
	RFS_EFAULT,	/*	14		 Bad address */
	RFS_ENOTBLK,/*		15		 Block device required */
	RFS_EBUSY,	/*	16		 Mount device busy */
	RFS_EEXIST,	/*	17		 File exists */
	RFS_EXDEV,	/*	18		 Cross-device link */
	RFS_ENODEV,	/*	19		 No such device */
	RFS_ENOTDIR,/*		20		 Not a directory*/
	RFS_EISDIR,	/*	21		 Is a directory */
	RFS_EINVAL,	/*	22		 Invalid argument */
	RFS_ENFILE,	/*	23		 File table overflow */
	RFS_EMFILE,	/*	24		 Too many open files */
	RFS_ENOTTY,	/*	25		 Not a typewriter */
	RFS_ETXTBSY,/*		26		 Text file busy */
	RFS_EFBIG,	/*	27		 File too large */
	RFS_ENOSPC,	/*	28		 No space left on device */
	RFS_ESPIPE,	/*	29		 Illegal seek */
	RFS_EROFS,	/*	30		 Read-only file system */
	RFS_EMLINK,	/*	31		 Too many links */
	RFS_EPIPE,	/*	32		 Broken pipe */

	/* math software */
	RFS_EDOM,	/*	33		 Argument too large */
	RFS_ERANGE,	/*	34		 Result too large */

	/* non-blocking and interrupt i/o */
	BADERR,		/*	35		 Operation would block */
	BADERR,		/*	36		 Operation now in progress */
	BADERR,		/*	37	 Operation already in progress */

	/* argument errors */
	BADERR,		/*	38	 Socket operation on non-socket */
	BADERR,		/*	39	 Destination address required */
	BADERR,		/*	40	 Message too long */
	RFS_EPROTO,	/*	41	 Protocol wrong type for socket */
	RFS_EPROTO,	/*	42	 Protocol not available */
	RFS_EPROTO,	/*	43	 Protocol not supported */
	BADERR,		/*	44	 Socket type not supported */
	BADERR,		/*	45	 Operation not supported on socket */
	RFS_EPROTO,	/*	46	 Protocol family not supported */
	RFS_EPROTO,	/*	47	 Address family not supported by protocol family */
	BADERR,		/*	48	 Address already in use */
	BADERR,	/*	49	 Can't assign requested address */

	/* operational errors */
	RFS_ENOLINK,	/*	50	 Network is down */
	RFS_ENONET,	/*	51	 Network is unreachable */
	RFS_ENOLINK,	/*	52	 Network dropped connection on reset */
	BADERR,		/*	53	 Software caused connection abort */
	RFS_ENOLINK,	/*	54	 Connection reset by peer */
	RFS_ENOSPC,	/*	55	 No buffer space available */
	BADERR,		/*	56	 Socket is already connected */
	RFS_ENOLINK,	/*	57	 Socket is not connected */
	RFS_ENOLINK,	/*	58	 Can't send after socket shutdown */
	BADERR,		/*	59	 Too many references: can't splice */
	RFS_ENOLINK,	/*	60	 Connection timed out */
	BADERR,		/*	61	 Connection refused */

	/* */
	BADERR,		/*	62	 Too many levels of symbolic links */
	BADERR,		/*	63	 File name too long */

	/* should be rearranged */
	BADERR,		/*	64	 Host is down */
	BADERR,		/*	65	 No route to host */
	RFS_EEXIST,	/*	66	 Directory not empty */

 	/* mush */
	BADERR,		/*	67	 Too many processes */
	BADERR,		/*	68	 Too many users */
	BADERR,		/*	69	 Disc quota exceeded */

	/* Network File System */
	BADERR,		/*	70	 Stale NFS file handle */
	BADERR,		/*	71	 Too many levels of remote in path */

	/* streams */
	RFS_ENOSTR,	/*	72	 Device is not a stream */
	RFS_ETIME,	/*	73	 Timer expired */
	RFS_ENOSR,	/*	74	 Out of streams resources */
	RFS_ENOMSG,	/*	75	 No message of desired type */
	RFS_EBADMSG,	/*	76	 Trying to read unreadable message */

 	BADERR,		/* 	77       Identifier removed -- System V IPC */

 	RFS_EDEADLK,	/*  	78       Deadlock conditiion */
 	RFS_ENOLCK,	/*  	79       No record locks available  */

	RFS_ENONET,	/*	80	 Machine is not on the network */
 	RFS_EREMOTE,	/* 	81       Object is remote */
 	RFS_ENOLINK,	/* 	82       the link has been severed */
 	RFS_EADV,	/*    	83       advertise error */
 	RFS_ESRMNT,	/*  	84       srmount error */
	RFS_ECOMM,	/*   	85       Communication error on send    */
 	RFS_EPROTO,	/*  	86       Protocol error                 */
 	RFS_EMULTIHOP,	/* 	87    	 multihop attempted */
 	RFS_EDOTDOT,	/*   	88       Cross mount point (not really error) */
 	RFS_EREMCHG,	/* 	89       Remote address changed */
};

/* fcntl request codes */

int fcntltorfs_tab[] = {
	RFS_F_DUPFD, RFS_F_GETFD, RFS_F_SETFD, RFS_F_GETFL, RFS_F_SETFL,
	RFS_F_BADOP, RFS_F_BADOP, RFS_F_GETLK, RFS_F_SETLK, RFS_F_SETLKW, 
	RFS_F_CHKFL 
};

int rfstofcntl_tab[] = {
	F_DUPFD, F_GETFD, F_SETFD, F_GETFL, F_SETFL, F_GETLK, F_SETLK, 
	F_SETLKW, F_CHKFL, F_BADOP, F_ALLOCSP, F_FREESP
};
