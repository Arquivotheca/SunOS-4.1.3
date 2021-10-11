/*	@(#)init_sysent.c 1.1 92/07/30 SMI; from UCB 6.1 83/08/17	*/

/*
 * System call switch table.
 */

#include <sys/param.h>
#include <sys/systm.h>
#ifdef SYSAUDIT
/*
 * This header file includes many defines which change function names
 * that are used in this file. For example, "open" becomes "au_open",
 * thus initializing the table value to "au_open", while what appears
 * here is "open". This will not happen if the SYSAUDIT option is not
 * used.
 */
#include <sys/init_audit.h>
#endif SYSAUDIT

int	nosys();
int	nullsys();
int	errsys();

/* 1.1 processes and protection */
int	gethostid(),sethostname(),gethostname(),getpid();
int	setdomainname(),getdomainname();
int	fork(),rexit(),execv(),execve(),wait4();
int	getuid(),setreuid(),getgid(),getgroups(),setregid(),setgroups();
int	getpgrp(),setpgrp();
int	sys_setsid(), setpgid();
int	uname();

/* 1.2 memory management */
int	brk(),sbrk(),sstk();
int	getpagesize(),smmap(),mctl(),munmap(),mprotect(),mincore();
int	omsync(),omadvise();

/* 1.3 signals */
int	sigvec(),sigblock(),sigsetmask(),sigpause(),sigstack(),sigcleanup();
int	kill(), killpg(), sigpending();

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	adjtime();
int	getitimer(),setitimer();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	select(),getdopt(),setdopt(),fcntl(),flock();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
#ifdef QUOTA
int	oldquota(), quotactl();
#else
#define	oldquota nullsys	/* for backward compatability with old login */
#endif QUOTA
#ifdef RT_SCHEDULE
int	rtschedule();
#endif RT_SCHEDULE

/* 1.7 system operation support */
int	mount(),unmount(),swapon();
int	sync(),reboot();
#ifdef SYSACCT
int	sysacct();
#endif SYSACCT
#ifdef SYSAUDIT
int	auditsys();
#endif SYSAUDIT

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.1.1 asynch operations */
#ifdef ASYNCHIO
int	aioread(), aiowrite(), aiowait(), aiocancel();
#endif ASYNCHIO

/* 2.2 file system */
int	chdir(),chroot();
int	fchdir(),fchroot();
int	mkdir(),rmdir(),getdirentries(), getdents();
int	creat(),open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),access(),fsync();
int	statfs(),fstatfs();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();		/* XXX */

/* 2.3.1 SystemV-compatible IPC */
#ifdef IPCSEMAPHORE
int	semsys();
#endif
#ifdef IPCMESSAGE
int	msgsys();
#endif
#ifdef IPCSHMEM
int	shmsys();
#endif

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

#ifdef COMPAT
/* emulations for backwards compatibility */
#define	compat(n, name)	n, o/**/name

int	otime();		/* now use gettimeofday */
int	ostime();		/* now use settimeofday */
int	oalarm();		/* now use setitimer */
int	outime();		/* now use utimes */
int	opause();		/* now use sigpause */
int	onice();		/* now use setpriority,getpriority */
int	oftime();		/* now use gettimeofday */
int	osetpgrp();		/* ??? */
int	otimes();		/* now use getrusage */
int	ossig();		/* now use sigvec, etc */
int	ovlimit();		/* now use setrlimit,getrlimit */
int	ovtimes();		/* now use getrusage */
int	osetuid();		/* now use setreuid */
int	osetgid();		/* now use setregid */
int	ostat();		/* now use stat */
int	ofstat();		/* now use fstat */
#else
#define	compat(n, name)	0, nosys
#endif

/* BEGIN JUNK */
#ifdef vax
int	resuba();
#endif vax
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* XXX - was awaiting fork w/ copy on write */
int	ovadvise();		/* awaiting new madvise */
int	indir();		/* indirect system call */
int	ustat();		/* System V compatibility */
int	owait();		/* should use wait4 interface */
#ifdef sparc
int	owait3();		/* should use wait4 interface */
#endif sparc
#ifdef	UFS
int	umount();		/* still more Sys V (and 4.2?) compatibility */
#endif
int	pathconf();		/* posix */
int	fpathconf();		/* posix */
int	sysconf();		/* posix */

#ifdef DEBUG
int debug();
#endif
/* END JUNK */

#ifdef	TRACE
int	vtrace();		/* kernel event tracing */
#endif	TRACE

#ifdef TESTECC
int testecc();			/* Test ecc error handling */
#endif

#ifdef NFSCLIENT
/* nfs */
int	async_daemon();		/* client async daemon */
#endif
#ifdef NFSSERVER
int	nfs_svc();		/* run nfs server */
int	nfs_getfh();		/* get file handle */
int	exportfs();		/* export file systems */
#endif

#ifdef RFS
int  	rfssys();		/* RFS-related calls */
#endif

int	getmsg();
int	putmsg();
int	poll();

#ifdef VPIX
int	vpixsys();		/* VP/ix system calls */
#endif

struct sysent sysent[] = {
	1, indir,			/*   0 = indir */
	1, rexit,			/*   1 = exit */
	0, fork,			/*   2 = fork */
	3, read,			/*   3 = read */
	3, write,			/*   4 = write */
	3, open,			/*   5 = open */
	1, close,			/*   6 = close */
	4, wait4,			/*   7 = wait4 */
	2, creat,			/*   8 = creat */
	2, link,			/*   9 = link */
	1, unlink,			/*  10 = unlink */
	2, execv,			/*  11 = execv */
	1, chdir,			/*  12 = chdir */
	compat(0,time),			/*  13 = old time */
	3, mknod,			/*  14 = mknod */
	2, chmod,			/*  15 = chmod */
	3, chown,			/*  16 = chown; now 3 args */
	1, brk,				/*  17 = brk */
	compat(2,stat),			/*  18 = old stat */
	3, lseek,			/*  19 = lseek */
	0, getpid,			/*  20 = getpid */
	0, nosys,			/*  21 = old mount */
#ifdef	UFS
	1, umount,			/*  22 = old umount */
#else
	0, nosys,			/*  22 = old umount */
#endif
	compat(1,setuid),		/*  23 = old setuid */
	0, getuid,			/*  24 = getuid */
	compat(1,stime),		/*  25 = old stime */
	5, ptrace,			/*  26 = ptrace */
	compat(1,alarm),		/*  27 = old alarm */
	compat(2,fstat),		/*  28 = old fstat */
	compat(0,pause),		/*  29 = opause */
	compat(2,utime),		/*  30 = old utime */
	0, nosys,			/*  31 = was stty */
	0, nosys,			/*  32 = was gtty */
	2, access,			/*  33 = access */
	compat(1,nice),			/*  34 = old nice */
	compat(1,ftime),		/*  35 = old ftime */
	0, sync,			/*  36 = sync */
	2, kill,			/*  37 = kill */
	2, stat,			/*  38 = stat */
	compat(2,setpgrp),		/*  39 = old setpgrp */
	2, lstat,			/*  40 = lstat */
	2, dup,				/*  41 = dup */
	0, pipe,			/*  42 = pipe */
	compat(1,times),		/*  43 = old times */
	4, profil,			/*  44 = profil */
	0, nosys,			/*  45 = nosys */
	compat(1,setgid),		/*  46 = old setgid */
	0, getgid,			/*  47 = getgid */
	compat(2,ssig),			/*  48 = old sig */
	0, nosys,			/*  49 = reserved for USG */
	0, nosys,			/*  50 = reserved for USG */
#ifdef SYSACCT
	1, sysacct,			/*  51 = turn acct off/on */
#else
	0, errsys,			/*  51 = not configured */
#endif SYSACCT
	0, nosys,			/*  52 = old set phys addr */
	4, mctl,			/*  53 = memory control */
	3, ioctl,			/*  54 = ioctl */
	2, reboot,			/*  55 = reboot */
#ifdef sparc
	3, owait3,			/*  56 = wait3 */
#else
	0, nosys,			/*  56 = old mpxchan */
#endif sparc
	2, symlink,			/*  57 = symlink */
	3, readlink,			/*  58 = readlink */
	3, execve,			/*  59 = execve */
	1, umask,			/*  60 = umask */
	1, chroot,			/*  61 = chroot */
	2, fstat,			/*  62 = fstat */
	0, nosys,			/*  63 = used internally */
	1, getpagesize,			/*  64 = getpagesize */
	3, omsync,			/*  65 = old msync */
	0, vfork,			/*  66 = vfork */
	0, read,			/*  67 = old vread */
	0, write,			/*  68 = old vwrite */
	1, sbrk,			/*  69 = sbrk */
	1, sstk,			/*  70 = sstk */
	6, smmap,			/*  71 = mmap */
	1, ovadvise,			/*  72 = old vadvise */
	2, munmap,			/*  73 = munmap */
	3, mprotect,			/*  74 = mprotect */
	3, omadvise,			/*  75 = old madvise */
	1, vhangup,			/*  76 = vhangup */
	compat(2,vlimit),		/*  77 = old vlimit */
	3, mincore,			/*  78 = mincore */
	2, getgroups,			/*  79 = getgroups */
	2, setgroups,			/*  80 = setgroups */
	1, getpgrp,			/*  81 = getpgrp */
	2, setpgrp,			/*  82 = setpgrp */
	3, setitimer,			/*  83 = setitimer */
	0, owait,			/*  84 = old wait & wait3 */
	1, swapon,			/*  85 = swapon */
	2, getitimer,			/*  86 = getitimer */
	2, gethostname,			/*  87 = gethostname */
	2, sethostname,			/*  88 = sethostname */
	0, getdtablesize,		/*  89 = getdtablesize */
	2, dup2,			/*  90 = dup2 */
	2, getdopt,			/*  91 = getdopt */
	3, fcntl,			/*  92 = fcntl */
	5, select,			/*  93 = select */
	2, setdopt,			/*  94 = setdopt */
	1, fsync,			/*  95 = fsync */
	3, setpriority,			/*  96 = setpriority */
	3, socket,			/*  97 = socket */
	3, connect,			/*  98 = connect */
	3, accept,			/*  99 = accept */
	2, getpriority,			/* 100 = getpriority */
	4, send,			/* 101 = send */
	4, recv,			/* 102 = recv */
	0, nosys,			/* 103 = old socketaddr */
	3, bind,			/* 104 = bind */
	5, setsockopt,			/* 105 = setsockopt */
	2, listen,			/* 106 = listen */
	compat(2,vtimes),		/* 107 = old vtimes */
	3, sigvec,			/* 108 = sigvec */
	1, sigblock,			/* 109 = sigblock */
	1, sigsetmask,			/* 110 = sigsetmask */
	1, sigpause,			/* 111 = sigpause */
	2, sigstack,			/* 112 = sigstack */
	3, recvmsg,			/* 113 = recvmsg */
	3, sendmsg,			/* 114 = sendmsg */
#ifdef	TRACE
	3, vtrace,			/* 115 = vtrace */
#else	TRACE
#ifdef TESTECC
	0, testecc,			/* 115 = testecc */
#else
	0, nosys,			/* 115 = nosys */
#endif TESTECC
#endif	TRACE
	2, gettimeofday,		/* 116 = gettimeofday */
	2, getrusage,			/* 117 = getrusage */
	5, getsockopt,			/* 118 = getsockopt */
#ifdef vax
	1, resuba,			/* 119 = resuba */
#else
	0, nosys,			/* 119 = nosys */
#endif
	3, readv,			/* 120 = readv */
	3, writev,			/* 121 = writev */
	2, settimeofday,		/* 122 = settimeofday */
	3, fchown,			/* 123 = fchown */
	2, fchmod,			/* 124 = fchmod */
	6, recvfrom,			/* 125 = recvfrom */
	2, setreuid,			/* 126 = setreuid */
	2, setregid,			/* 127 = setregid */
	2, rename,			/* 128 = rename */
	2, truncate,			/* 129 = truncate */
	2, ftruncate,			/* 130 = ftruncate */
	2, flock,			/* 131 = flock */
	0, nosys,			/* 132 = nosys */
	6, sendto,			/* 133 = sendto */
	2, shutdown,			/* 134 = shutdown */
	5, socketpair,			/* 135 = socketpair */
	2, mkdir,			/* 136 = mkdir */
	1, rmdir,			/* 137 = rmdir */
	2, utimes,			/* 138 = utimes */
	0, sigcleanup,			/* 139 = signalcleanup */
	2, adjtime,			/* 140 = adjtime */
	3, getpeername,			/* 141 = getpeername */
	2, gethostid,			/* 142 = gethostid */
	0, nosys,			/* 143 = old sethostid */
	2, getrlimit,			/* 144 = getrlimit */
	2, setrlimit,			/* 145 = setrlimit */
	2, killpg,			/* 146 = killpg */
	0, nosys,			/* 147 = nosys */
	0, oldquota,	/* XXX */	/* 148 = old quota */
	0, oldquota,	/* XXX */	/* 149 = old qquota */
	3, getsockname,			/* 150 = getsockname */
	4, getmsg,			/* 151 = getmsg */
	4, putmsg,			/* 152 = putmsg */
	3, poll,			/* 153 = poll */
#ifdef NFSSERVER
	0, nosys,			/* 154 = old nfs_mount */
	1, nfs_svc,			/* 155 = nfs_svc */
#else
	0, nosys,			/* 154 = nosys */
	0, errsys,			/* 155 = errsys */
#endif
	4, getdirentries,		/* 156 = getdirentries */
	2, statfs,			/* 157 = statfs */
	2, fstatfs,			/* 158 = fstatfs */
	1, unmount,			/* 159 = unmount */
#ifdef NFSCLIENT
	0, async_daemon,		/* 160 = async_daemon */
#else
	0, errsys,			/* 160 = errsys */
#endif
#ifdef NFSSERVER
	2, nfs_getfh,			/* 161 = get file handle */
#else
	0, nosys,			/* 161 = nosys */
#endif
	2, getdomainname,		/* 162 = getdomainname */
	2, setdomainname,		/* 163 = setdomainname */
#ifdef RT_SCHEDULE
	5, rtschedule,			/* 164 = rtschedule */
#else
	0, errsys,			/* 164 = not configured */
#endif RT_SCHEDULE
#ifdef QUOTA
	4, quotactl,			/* 165 = quotactl */
#else
	0, errsys,			/* 165 = not configured */
#endif QUOTA
#ifdef NFSSERVER
	2, exportfs,			/* 166 = exportfs */
#else
	0, errsys,			/* 166 = not configured */
#endif
	4, mount,			/* 167 = mount */
	2, ustat,			/* 168 = ustat */
#ifdef IPCSEMAPHORE
	5, semsys,			/* 169 = semsys */
#else
	0, errsys,			/* 169 = not configured */
#endif
#ifdef IPCMESSAGE
	6, msgsys,			/* 170 = msgsys */
#else
	0, errsys,			/* 170 = not configured */
#endif
#ifdef IPCSHMEM
	4, shmsys,			/* 171 = shmsys */
#else
	0, errsys,			/* 171 = not configured */
#endif
#ifdef SYSAUDIT
	4, auditsys,			/* 172 = auditsys (audit control) */
#else
	0, nullsys,			/* 172 = not configured */
#endif SYSAUDIT
#ifdef RFS
	5, rfssys,			/* 173 = RFS calls */
#else
	0, errsys,			/* 173 = not configured */
#endif
	3, getdents,			/* 174 = getdents */
	1, sys_setsid,			/* 175 = setsid & s5 setpgrp() */
	1, fchdir,			/* 176 = fchdir */
	1, fchroot,			/* 177 = fchroot */
#ifdef VPIX
	2, vpixsys,			/* 178 = VP/ix system calls */
#else
	0, errsys,			/* 178 = not configured */
#endif
#ifdef ASYNCHIO
	6, aioread,			/* 179 = aioread */
	6, aiowrite,			/* 180 = aiowrite */
	1, aiowait,			/* 181 = aiowait */
	1, aiocancel,			/* 182 = aiocancel */
#else
	0, errsys,			/* 179 not configured */
	0, errsys,			/* 180 not configured */
	0, errsys,			/* 181 not configured */
	0, errsys,			/* 182 not configured */
#endif ASYNCHIO
	1, sigpending,			/* 183 = sigpending */
	0, errsys,			/* 184 = AVAILABLE */
	2, setpgid,			/* 185 = setpgid */
	2, pathconf,			/* 186 = pathconf */
	2, fpathconf,			/* 187 = fpathconf */
	1, sysconf,			/* 188 = sysconf */
	1, uname,			/* 189 = uname */
#ifdef VDDRV
	0, nosys,			/* 190 = reserved - Loadable syscalls */
	0, nosys,			/* 191 = reserved - Loadable syscalls */
	0, nosys,			/* 192 = reserved - Loadable syscalls */
	0, nosys,			/* 193 = reserved - Loadable syscalls */
	0, nosys,			/* 194 = reserved - Loadable syscalls */
	0, nosys,			/* 195 = reserved - Loadable syscalls */
	0, nosys,			/* 196 = reserved - Loadable syscalls */
	0, nosys,			/* 197 = reserved - Loadable syscalls */
#endif
};
int	nsysent = sizeof (sysent) / sizeof (sysent[0]);

#ifdef VDDRV
int	firstvdsys = 190;		/* first loadable syscall number */
int	lastvdsys  = 197;		/* last loadable syscall number */
#endif
