/*	@(#)rfs_misc.h 1.1 92/07/30 SMI 	*/

/* 
 *	Miscellaneous include stuff for RFS
 *
 */

#ifndef _rfs_rfs_misc_h
#define _rfs_rfs_misc_h

#define PREMOTE	(PZERO + 14) 	/* Priority at which RFS sleeps for messages */
#define RFS_BSIZE 1024		/* Max S5R3 filesystem block size */
#define RFS_DIRSIZ 14		/* Size of a S5 directory entry name */
#define RFS_DIRENT 24 		/* Max size of a S5R3 directory entry */
#define RFS_PSCOMSIZ		/* Size of executable name for ps */
#define NMSZ	15		/* Size of an advertised name (s5 DIRSIZ+1) */
#define RFDEVSIZE 256		/* Hash table size for server dev# generation */
#define RFDEVEMPTY -1		/* Empty server dev# hash table slot */
#define IOVSIZE	16		/* Size of saved iovec array - 
				    XXX should be dynamic (but see readv()) */
#define RFS_RETRY 10		/* Number of client retries on server NACK */

#define lobyte(X)       (((unsigned char *)&X)[1])
#define hibyte(X)       (((unsigned char *)&X)[0])
#define loword(X)       (((ushort *)&X)[1])
#define hiword(X)       (((ushort *)&X)[0])

typedef short           index_t;
typedef short           sysid_t;

/* Client-side caching */
#ifndef lint
extern int rcache_enable;
extern int rcacheinit;
#endif lint

#define DEF_CMASK	0	/* cmask passed with RFS calls */

/* 
 * Miscellaneous data structures from S5R3 /usr/include files, used by 
 * RFS. Someday these may go in /usr/5include. In the meantime they reside here 
 */

/* flock structure: From S5R3 sys/fnctl.h */

struct rfs_flock {
        short l_type;
        short l_whence;
        long l_start;
        long l_len;
        short l_sysid;
        short l_pid;
};
typedef struct rfs_flock rfs_flock;


/* stat structure: From S5R3 sys/stat.h */

struct rfs_stat {
	short st_dev;
	u_short st_ino;
	u_short st_mode;
	short st_nlink;
	u_short st_uid;
	u_short st_gid;
	short st_rdev;
	long st_size;
	long st_atime;
	long st_mtime;
	long st_ctime;
};
typedef struct rfs_stat rfs_stat;


/* statfs structure: From S5R3 sys/statfs.h */

struct rfs_statfs {
	short f_fstyp;
	long f_bsize;
	long f_frsize;
	long f_blocks;
	long f_bfree;
	long f_files;
	long f_ffree;
	char f_fname[6];
	char f_fpack[6]; 
}; 
typedef struct rfs_statfs rfs_statfs;


/* dirent structure: From S5R3 sys/dirent.h */
#define RFS_MAXNAMLEN	512

struct rfs_dirent {
	long d_ino;
	long d_off;
	short d_reclen;
	char d_name[1];
};
typedef struct rfs_dirent rfs_dirent;

/* timebuf structure for RFS utimes system call: defined in S5R3 man page */
struct rfs_utimbuf {
	time_t	ut_actime;
	time_t	ut_modtime;
};
typedef struct rfs_utimbuf rfs_utimbuf;

struct rfs_ustat {
	long f_tfree;
	u_short f_tinode;
	char f_fname[6];
	char f_fpack[6];
};
typedef struct rfs_ustat rfs_ustat;


/*
 * Signal handling macros for RFS
 */

/* Save and reset the current process's signals. */
#define SAVE_SIG(P, SIG, CURSIG) \
{ \
	(SIG) = (P)->p_sig; \
	(CURSIG) = (P)->p_cursig; \
	(P)->p_sig = 0; \
	(P)->p_cursig = 0; \
}

/* Accumulate signals for the current process */
#define ACCUM_SIG(P, SIG, CURSIG) \
{ \
	(SIG) |= (P)->p_sig; \
	if ((P)->p_cursig) { \
		if (CURSIG) \
			(SIG) |= sigmask((P)->p_cursig); \
		else \
			(CURSIG) = (P)->p_cursig; \
	} \
	(P)->p_sig = 0; \
	(P)->p_cursig = 0; \
}

/* Restore saved signals for the current process. The signals are "or'd"
   in with any currently accumulated. */
#define RESTORE_SIG(P, SIG, CURSIG) \
{ \
	(P)->p_sig |= (SIG); \
	if (CURSIG) { \
		if ((P)->p_cursig) \
			(P)->p_sig |= sigmask((P)->p_cursig); \
		(P)->p_cursig = (CURSIG); \
	} \
}

/* Return all the signals currently associated with process P, except those
   reserved for internal use by RFS server processes */
#define GET_SIGS(P)  \
		(((P)->p_sig | ((P)->p_cursig ? sigmask((P)->p_cursig) : 0)) \
		& ~ (sigmask(SIGTERM) | sigmask(SIGUSR1) | sigmask(SIGKILL)))
 
/* Tell a file is mandatorily lockable based on permission bits */
#define	MND_LCK(MODE)	(((MODE) & VSGID) && !((MODE) & (VEXEC >> 3)))

/* Mapping table macros for mapping RFS (S5) 
   data items to sun and vice versa */

/* No mapping is provided for S5 file modes <--> Sun file modes. These are
   currently identical (see S5/sys/stat.h SUN/ufs/inode.h) . If they change, 
   mapping will have to be provided */

/* Mapping for file open modes. Defines from S5/sys/fcntl.h and SUN/h/file.h */

#define RFS_O_RDONLY 0
#define RFS_O_WRONLY 1
#define RFS_O_RDWR   2
#define RFS_O_NDELAY 04     /* Non-blocking I/O */
#define RFS_O_APPEND 010    /* append (writes guaranteed at the end) */
#define RFS_O_SYNC   020    /* synchronous write option */
#define RFS_O_CREAT 00400   /* open with file create (uses third open arg)*/
#define RFS_O_TRUNC 01000   /* open with truncation */
#define RFS_O_EXCL  02000   /* exclusive open */

#define RFSTOOMODE(M) ( \
	(((M) & 1) == RFS_O_RDONLY ? FREAD : 0) | \
	((M) & RFS_O_WRONLY ? FWRITE : 0) | \
	((M) & RFS_O_RDWR ? (FREAD | FWRITE) : 0) | \
	((M) & RFS_O_NDELAY ? FNBIO : 0) | \
	((M) & RFS_O_APPEND ? FAPPEND : 0) | \
	((M) & RFS_O_CREAT ? FCREAT : 0) | \
	((M) & RFS_O_TRUNC ? FTRUNC : 0) | \
	((M) & RFS_O_EXCL ? FEXCL : 0))

#define OMODETORFS(M) ( \
	(((M) & (FREAD | FWRITE)) == FREAD ? RFS_O_RDONLY : 0) | \
	(((M) & (FREAD | FWRITE)) == FWRITE ? RFS_O_WRONLY : 0) | \
	(((M) & (FREAD | FWRITE)) == (FREAD | FWRITE) ? RFS_O_RDWR : 0) | \
	((M) & FNBIO ? RFS_O_NDELAY : 0) | \
	((M) & FAPPEND ? RFS_O_APPEND : 0) | \
	((M) & FCREAT ? RFS_O_CREAT : 0) | \
	((M) & FTRUNC ? RFS_O_TRUNC : 0) | \
	((M) & FEXCL ? RFS_O_EXCL : 0))

/* Mapping for file flags. Used by close() and fcntl(). Defines from 
   S5/sys/file.h and SUN/h/file.h */

#define RFS_FOPEN   0xffffffff
#define RFS_FREAD   0x01
#define RFS_FWRITE  0x02
#define RFS_FNDELAY 0x04
#define RFS_FAPPEND 0x08
#define RFS_FSYNC   0x10
#define RFS_FRCACH  0x20	/* Used for file and record locking cache*/
#define RFS_FMASK   0x7f        /* FMASK should be disjoint from FNET */
#define RFS_FNET    0x80        /* needed by 3bnet */

#define FFLAGTORFS(M) ( \
	((M) & FREAD ? RFS_FREAD : 0) | \
	((M) & FWRITE ? RFS_FWRITE : 0) | \
	((M) & FNDELAY ? RFS_FNDELAY : 0) | \
	((M) & FNBIO ? RFS_FNDELAY : 0) | \
	((M) & FAPPEND ? RFS_FAPPEND : 0))

#define RFSTOFFLAG(M) ( \
	((M) & RFS_FREAD ? FREAD : 0) | \
	((M) & RFS_FWRITE ? FWRITE : 0) | \
	((M) & RFS_FNDELAY ? FNBIO : 0) | \
	((M) & RFS_FAPPEND ? FAPPEND : 0))




/* Mapping for file types. These defines are from S5/sys/inode.h for the file 
   type, mode and access. These are also defined in S5/sys/stat.h */

#define	RFS_IFMT	0xf000		/* type of file */
#define	RFS_IFDIR	0x4000	/* directory */
#define	RFS_IFCHR	0x2000	/* character special */
#define	RFS_IFBLK	0x6000	/* block special */
#define	RFS_IFREG	0x8000	/* regular */
#define	RFS_IFMPC	0x3000	/* multiplexed char special */
#define	RFS_IFMPB	0x7000	/* multiplexed block special */
#define	RFS_IFIFO	0x1000	/* fifo special */

#define	RFS_ISUID	0x800		/* set user id on execution */
#define	RFS_ISGID	0x400		/* set group id on execution */
#define RFS_ISVTX	0x200		/* save swapped text even after use */

#define	RFS_IREAD		0x100	/* read permission */
#define	RFS_IWRITE		0x080	/* write permission */
#define	RFS_IEXEC		0x040	/* execute permission */
#define	RFS_ICDEXEC		0x020	/* cd permission */
#define	RFS_IOBJEXEC		0x010	/* execute as an object file */
					/* i.e., 410, 411, 413 */
#define RFS_IMNDLCK		0x001	/* mandatory locking set */

extern enum vtype rfstovt_tab[];
extern int vttorfs_tab[];
#define RFSTOVT(M)       (rfstovt_tab[((M) & RFS_IFMT) >> 12])
#define VTTORFS(T)       (vttorfs_tab[(int)(T)])

/* Mapping for errno's. These defines from S5/uts/3b2/sys/errno.h */

#define	RFS_EPERM	1	/* Not super-user			*/
#define	RFS_ENOENT	2	/* No such file or directory		*/
#define	RFS_ESRCH	3	/* No such process			*/
#define	RFS_EINTR	4	/* interrupted system call		*/
#define	RFS_EIO		5	/* I/O error				*/
#define	RFS_ENXIO	6	/* No such device or address		*/
#define	RFS_E2BIG	7	/* Arg list too long			*/
#define	RFS_ENOEXEC	8	/* Exec format error			*/
#define	RFS_EBADF	9	/* Bad file number			*/
#define	RFS_ECHILD	10	/* No children				*/
#define	RFS_EAGAIN	11	/* No more processes			*/
#define	RFS_ENOMEM	12	/* Not enough core			*/
#define	RFS_EACCES	13	/* Permission denied			*/
#define	RFS_EFAULT	14	/* Bad address				*/
#define	RFS_ENOTBLK	15	/* Block device required		*/
#define	RFS_EBUSY	16	/* Mount device busy			*/
#define	RFS_EEXIST	17	/* File exists				*/
#define	RFS_EXDEV	18	/* Cross-device link			*/
#define	RFS_ENODEV	19	/* No such device			*/
#define	RFS_ENOTDIR	20	/* Not a directory			*/
#define	RFS_EISDIR	21	/* Is a directory			*/
#define	RFS_EINVAL	22	/* Invalid argument			*/
#define	RFS_ENFILE	23	/* File table overflow			*/
#define	RFS_EMFILE	24	/* Too many open files			*/
#define	RFS_ENOTTY	25	/* Not a typewriter			*/
#define	RFS_ETXTBSY	26	/* Text file busy			*/
#define	RFS_EFBIG	27	/* File too large			*/
#define	RFS_ENOSPC	28	/* No space left on device		*/
#define	RFS_ESPIPE	29	/* Illegal seek				*/
#define	RFS_EROFS	30	/* Read only file system		*/
#define	RFS_EMLINK	31	/* Too many links			*/
#define	RFS_EPIPE	32	/* Broken pipe				*/
#define	RFS_EDOM	33	/* Math arg out of domain of func	*/
#define	RFS_ERANGE	34	/* Math result not representable	*/
#define	RFS_ENOMSG	35	/* No message of desired type		*/

/* File and record locking */
#define	RFS_EDEADLK	45	/* Deadlock condition 			*/
#define	RFS_ENOLCK	46	/* No record locks available		*/

/* stream problems */
#define RFS_ENOSTR	60	/* Device not a stream			*/
#define RFS_ENODATA	61	/* no data (for no delay io)		*/
#define RFS_ETIME	62	/* timer expired			*/
#define RFS_ENOSR	63	/* out of streams resources		*/

#define RFS_ENONET	64	/* Machine is not on the network	*/
#define RFS_ENOPKG	65	/* Package not installed                */
#define RFS_EREMOTE	66	/* The object is remote			*/
#define RFS_ENOLINK	67	/* the link has been severed */
#define RFS_EADV	68	/* advertise error */
#define RFS_ESRMNT	69	/* srmount error */

#define	RFS_ECOMM	70	/* Communication error on send		*/
#define RFS_EPROTO	71	/* Protocol error			*/
#define	RFS_EMULTIHOP 	74	/* multihop attempted */
#define	RFS_ELBIN	75	/* Inode is remote (not really error)*/
#define	RFS_EDOTDOT 	76	/* Cross mount point (not really error)*/
#define RFS_EBADMSG 	77	/* trying to read unreadable message	*/

#define RFS_ENOTUNIQ 	80	/* given log. name not unique */
#define RFS_EBADFD 	81	/* f.d. invalid for this operation */
#define RFS_EREMCHG	82	/* Remote address changed */

#define BADERR		-1	/* Returned value if error could not map */
#define RFS_OK		 0      /* No error, i.e., successful completion */

extern int rfstoerr_tab[];
extern int errtorfs_tab[];
#define RFSTOERR(M)       (rfstoerr_tab[M])
#define ERRTORFS(T)       (errtorfs_tab[T])


/* RFS signal numbers defined in S5/sys/signal.h */
#define sm(m)      (1 << ((m)-1))

#define	RFS_SIGHUP	sm(1)	/* hangup */ 
#define	RFS_SIGINT	sm(2)	/* interrupt (rubout) */
#define	RFS_SIGQUIT	sm(3)	/* quit (ASCII FS) */
#define	RFS_SIGILL	sm(4)	/* illegal instruction (not reset when caught)*/
#define	RFS_SIGTRAP	sm(5)	/* trace trap (not reset when caught) */
#define	RFS_SIGIOT	sm(6)	/* IOT instruction */
#define RFS_SIGABRT 	sm(6)	/* used by abort, replace SIGIOT in the  future */
#define	RFS_SIGEMT	sm(7)	/* EMT instruction */
#define	RFS_SIGFPE	sm(8)	/* floating point exception */
#define	RFS_SIGKILL	sm(9)	/* kill (cannot be caught or ignored) */
#define	RFS_SIGBUS	sm(10)	/* bus error */
#define	RFS_SIGSEGV	sm(11)	/* segmentation violation */
#define	RFS_SIGSYS	sm(12)	/* bad argument to system call */
#define	RFS_SIGPIPE	sm(13)	/* write on a pipe with no one to read it */
#define	RFS_SIGALRM	sm(14)	/* alarm clock */
#define	RFS_SIGTERM	sm(15)	/* software termination signal from kill */
#define	RFS_SIGUSR1	sm(16)	/* user defined signal 1 */
#define	RFS_SIGUSR2	sm(17)	/* user defined signal 2 */
#define	RFS_SIGCLD	sm(18)	/* death of a child */
#define	RFS_SIGPWR	sm(19)	/* power-fail restart */
#define RFS_SIGPOLL 	sm(22)	/* pollable event occured */

#define	RFS_NSIG	23	/* The valid signal number is from 1 to NSIG-1 */

#define RFSTOSIG(M) ( \
((M) & RFS_SIGHUP ? sm(SIGHUP) : 0) | ((M) & RFS_SIGINT ? sm(SIGINT) : 0) | \
((M) & RFS_SIGQUIT ? sm(SIGQUIT) : 0) | ((M) & RFS_SIGILL ? sm(SIGILL) : 0) | \
((M) & RFS_SIGTRAP ? sm(SIGTRAP) : 0) | ((M) & RFS_SIGIOT ? sm(SIGIOT) : 0) | \
((M) & RFS_SIGEMT ? sm(SIGEMT) : 0) | ((M) & RFS_SIGFPE ? sm(SIGFPE) : 0) | \
((M) & RFS_SIGKILL ? sm(SIGKILL) : 0) | ((M) & RFS_SIGBUS ? sm(SIGBUS) : 0) | \
((M) & RFS_SIGSEGV ? sm(SIGSEGV) : 0) | ((M) & RFS_SIGSYS ? sm(SIGSYS) : 0) | \
((M) & RFS_SIGPIPE ? sm(SIGPIPE) : 0) | ((M) & RFS_SIGALRM ? sm(SIGALRM) : 0) | \
((M) & RFS_SIGTERM ? sm(SIGTERM) : 0) | ((M) & RFS_SIGUSR1 ? sm(SIGUSR1) : 0) | \
((M) & RFS_SIGUSR2 ? sm(SIGUSR2) : 0) | ((M) & RFS_SIGCLD ? sm(SIGCHLD) : 0) | \
((M) & RFS_SIGPWR ? 0 : 0) | ((M) & RFS_SIGPOLL ? sm(SIGPOLL) : 0))

#define	SIGTORFS(M) ( \
((M) & sm(SIGHUP) ? RFS_SIGHUP : 0) | ((M) & sm(SIGINT) ? RFS_SIGINT : 0) | \
((M) & sm(SIGQUIT) ? RFS_SIGQUIT : 0) | ((M) & sm(SIGILL) ? RFS_SIGILL : 0) | \
((M) & sm(SIGTRAP) ? RFS_SIGTRAP : 0) | ((M) & sm(SIGIOT) ? RFS_SIGIOT : 0) | \
((M) & sm(SIGEMT) ? RFS_SIGEMT : 0) | ((M) & sm(SIGFPE) ? RFS_SIGFPE : 0) | \
((M) & sm(SIGKILL) ? RFS_SIGKILL : 0) | ((M) & sm(SIGBUS) ? RFS_SIGBUS : 0) | \
((M) & sm(SIGSEGV) ? RFS_SIGSEGV : 0) | ((M) & sm(SIGSYS) ? RFS_SIGSYS : 0) | \
((M) & sm(SIGPIPE) ? RFS_SIGPIPE : 0) | ((M) & sm(SIGALRM) ? RFS_SIGALRM : 0) | \
((M) & sm(SIGTERM) ? RFS_SIGTERM : 0) | ((M) & sm(SIGUSR1) ? RFS_SIGUSR1 : 0) | \
((M) & sm(SIGUSR2) ? RFS_SIGUSR2 : 0) | ((M) & sm(SIGCHLD) ? RFS_SIGCLD : 0) | \
((M) & sm(SIGPOLL) ? RFS_SIGPOLL : 0))


/* File access codes for the DUACCESS system call -- defined in 
   S5/head/unistd.h and h/vnode.h */

#define RFS_R_OK    4       /* Test for Read permission */
#define RFS_W_OK    2       /* Test for Write permission */
#define RFS_X_OK    1       /* Test for eXecute permission */
#define RFS_F_OK    0       /* Test for existence of File */

#define VACCTORFS(M) (	\
		  RFS_F_OK | \
		  ((M) & VREAD ? RFS_R_OK : 0) | \
		  ((M) & VWRITE ? RFS_W_OK : 0) | \
		  ((M) & VEXEC ? RFS_X_OK : 0) )

#define RFSTOVACC(M) (	\
		  ((M) & RFS_R_OK ? VREAD : 0) | \
		  ((M) & RFS_W_OK ? VWRITE : 0) | \
		  ((M) & RFS_X_OK ? VEXEC : 0) )

/* fcntl request codes for DUFCNTL -- defined in S5/sys/fcntl.h and 
   SUN/sys/fcntl.h  */


#define RFS_F_BADOP -1	    /* Invalid request */
#define RFS_F_DUPFD 0       /* Duplicate fildes */
#define RFS_F_GETFD 1       /* Get fildes flags */
#define RFS_F_SETFD 2       /* Set fildes flags */
#define RFS_F_GETFL 3       /* Get file flags */
#define RFS_F_SETFL 4       /* Set file flags */
#define RFS_F_GETLK 5       /* Get file lock */
#define RFS_F_SETLK 6       /* Set file lock */
#define RFS_F_SETLKW        7       /* Set file lock and wait */
#define RFS_F_CHKFL         8       /* Check legality of file flag changes */
#define RFS_F_ALLOCSP       10      /* Pre-allocate file space */
#define RFS_F_FREESP        11      /* Free file space */

#define F_BADOP -1	    /* Invalid RFS op */
#define F_CHKFL -2	    /* Check SETFL flags, no such op on	Sun */
#define F_ALLOCSP -3	    /* Pre-allocate file space , no such op on Sun */
#define F_FREESP -4	    /* Release file space, no such op on Sun */

extern int rfstofcntl_tab[];
extern int fcntltorfs_tab[];
#define RFSTOFCNTL(M)	(rfstofcntl_tab[M])
#define FCNTLTORFS(T)	(fcntltorfs_tab[T])

#endif /*!_rfs_rfs_misc_h*/
