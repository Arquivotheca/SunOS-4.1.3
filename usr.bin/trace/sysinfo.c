#ifndef lint
static char sccsid[] = "@(#)sysinfo.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 *  Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include "trace.h"

struct sysinfo sysinfo [] = {
D, {V}		, "Unknown",	/*   0 = Unknown */
V, {D}		, "exit",	/*   1 = exit */
D, {V}		, "fork",	/*   2 = fork */
D, {D,R,D}	, "read",	/*   3 = read */
D, {D,W,D}	, "write",	/*   4 = write */
D, {S,O,O}	, "open",	/*   5 = open */
D, {D}		, "close",	/*   6 = close */
D, {D,X,X,X}	, "wait4",	/*   7 = wait4 */
D, {S,O}	, "creat",	/*   8 = creat */
D, {S,S}	, "link",	/*   9 = link */
D, {S}		, "unlink",	/*  10 = unlink */
V, {S,X}	, "execv",	/*  11 = execv */
D, {S}		, "chdir",	/*  12 = chdir */
D, {X}		, "time",	/*  13 = old time */
D, {S,O,D}	, "mknod",	/*  14 = mknod */
D, {S,O}	, "chmod",	/*  15 = chmod */
D, {S,D,D}	, "chown",	/*  16 = chown; now 3 args */
X, {X}		, "brk",	/*  17 = old break */
D, {S,X}	, "stat",	/*  18 = old stat */
L, {D,L,D}	, "lseek",	/*  19 = lseek */
D, {V}		, "getpid",	/*  20 = getpid */
D, {S,S,X}	, "mount",	/*  21 = old mount */
D, {S}		, "umount",	/*  22 = old umount */
D, {D}		, "setuid",	/*  23 = old setuid */
D, {V}		, "getuid",	/*  24 = getuid */
D, {X}		, "stime",	/*  25 = old stime */
D, {D,D,X,X,X}	, "ptrace",	/*  26 = ptrace */
U, {U}		, "alarm",	/*  27 = old alarm */
D, {D,X}	, "fstat",	/*  28 = old fstat */
V, {V}		, "pause",	/*  29 = opause */
D, {S,X}	, "utime",	/*  30 = old utime */
V, {V}		, "stty",	/*  31 = old stty */
V, {V}		, "gtty",	/*  32 = old gtty */
D, {S,O}	, "access",	/*  33 = access */
D, {D}		, "nice",	/*  34 = old nice */
V, {V}		, "ftime",	/*  35 = old ftime */
V, {V}		, "sync",	/*  36 = sync */
D, {D,D}	, "kill",	/*  37 = kill */
D, {S,X}	, "stat",	/*  38 = stat */
D, {V}		, "setpgrp",	/*  39 = old setpgrp */
D, {S,X}	, "lstat",	/*  40 = lstat */
D, {D}		, "dup",	/*  41 = dup */
D, {X}		, "pipe",	/*  42 = pipe */
L, {X}		, "times",	/*  43 = old times */
V, {X,D,D,D}	, "profil",	/*  44 = profil */
V, {V}		, "#45",	/*  45 = nosys */
D, {D}		, "setgid",	/*  46 = old setgid */
D, {V}		, "getgid",	/*  47 = getgid */
D, {D,X}	, "signal",	/*  48 = old sig */
V, {V}		, "#49",	/*  49 = reserved for USG */
V, {V}		, "#50",	/*  50 = reserved for USG */
D, {S}		, "acct",	/*  51 = turn acct off/on */
V, {V}		, "phys",	/*  52 = old set phys addr */
D, {X,D,D,X}	, "mctl",	/*  53 = memory control */
D, {D,X,X}	, "ioctl",	/*  54 = ioctl */
V, {D}		, "reboot",	/*  55 = reboot */
V, {X,X,X}	, "wait3",	/*  56 = owait3 */
D, {S,S}	, "symlink",	/*  57 = symlink */
D, {S,R,D}	, "readlink",	/*  58 = readlink */
D, {S,X,X}	, "execve",	/*  59 = execve */
O, {O}		, "umask",	/*  60 = umask */
D, {S}		, "chroot",	/*  61 = chroot */
D, {D,X}	, "fstat",	/*  62 = fstat */
V, {V}		, "nosys",	/*  63 = used internally */
D, {V}		, "getpagesize",/*  64 = getpagesize */
D, {X,D,X}	, "msync",	/*  65 = msync */
D, {V}		, "vfork",	/*  66 = vfork */
D, {D,R,D}	, "old vread - read",/*  67 = old vread */
D, {D,W,D}	, "old vwrite - write",/*  68 = old vwrite */
X, {X}		, "brk",	/*  69 = sbrk */
D, {X}		, "sstk",	/*  70 = sstk */
X, {X,D,X,X,D,D}, "mmap",	/*  71 = mmap */
D, {D}		, "old vadvise",/*  72 = old vadvise */
D, {X,D}	, "munmap",	/*  73 = munmap */
D, {X,D,X}	, "mprotect",	/*  74 = mprotect */
D, {X,D,X}	, "madvise",	/*  75 = madvise */
V, {V}		, "vhangup",	/*  76 = vhangup */
D, {V}		, "old vlimit",	/*  77 = old vlimit */
D, {X,D,X}	, "mincore",	/*  78 = mincore */
D, {D,X}	, "getgroups",	/*  79 = getgroups */
D, {D,X}	, "setgroups",	/*  80 = setgroups */
D, {D}		, "getpgrp",	/*  81 = getpgrp */
D, {D,D}	, "setpgrp",	/*  82 = setpgrp */
D, {D,X,X}	, "setitimer",	/*  83 = setitimer */
D, {V}		, "wait",	/*  84 = owait */
D, {S}		, "swapon",	/*  85 = swapon */
D, {D,X}	, "getitimer",	/*  86 = getitimer */
D, {S,D}	, "gethostname",/*  87 = gethostname */
D, {W,D}	, "sethostname",/*  88 = sethostname */
D, {V}		, "getdtablesize",/*  89 = getdtablesize */
D, {D,D}	, "dup2",	/*  90 = dup2 */
D, {X,X}	, "getdopt",	/*  91 = getdopt */
D, {D,O,X}	, "fcntl",	/*  92 = fcntl */
D, {D,X,X,X,X}	, "select",	/*  93 = select */
D, {X,X}	, "setdopt",	/*  94 = setdopt */
D, {D}		, "fsync",	/*  95 = fsync */
D, {O,D,D}	, "setpriority",/*  96 = setpriority */
D, {D,D,D}	, "socket",	/*  97 = socket */
D, {D,W,D}	, "connect",	/*  98 = connect */
D, {D,X,X}	, "accept",	/*  99 = accept */
D, {O,D}	, "getpriority",/* 100 = getpriority */
D, {D,W,D,O}	, "send",	/* 101 = send */
D, {D,R,D,O}	, "recv",	/* 102 = recv */
D, {V}		, "old socketaddr",/* 103 = old socketaddr */
D, {D,W,D}	, "bind",	/* 104 = bind */
D, {D,D,D,X,D}	, "setsockopt",	/* 105 = setsockopt */
D, {D,D}	, "listen",	/* 106 = listen */
D, {V}		, "old vtimes",	/* 107 = old vtimes */
D, {D,X,X}	, "sigvec",	/* 108 = sigvec */
X, {X}		, "sigblock",	/* 109 = sigblock */
X, {X}		, "sigsetmask",	/* 110 = sigsetmask */
V, {X}		, "sigpause",	/* 111 = sigpause */
D, {X,X}	, "sigstack",	/* 112 = sigstack */
D, {D,X,O}	, "recvmsg",	/* 113 = recvmsg */
D, {D,X,O}	, "sendmsg",	/* 114 = sendmsg */
D, {X,X,X}	, "vtrace",	/* 115 = vtrace */
D, {X,X}	, "gettimeofday",/* 116 = gettimeofday */
D, {D,X}	, "getrusage",	/* 117 = getrusage */
D, {D,D,D,X,D}	, "getsockopt",	/* 118 = getsockopt */
#ifdef vax
D, {X}		, "resuba",	/* 119 = resuba */
#else
D, {V}		, "#119",	/* 119 = nosys */
#endif
D, {D,X,D}	, "readv",	/* 120 = readv */
D, {D,X,D}	, "writev",	/* 121 = writev */
D, {X,X}	, "settimeofday",/* 122 = settimeofday */
D, {D,D,D}	, "fchown",	/* 123 = fchown */
D, {D,O}	, "fchmod",	/* 124 = fchmod */
D, {D,R,D,D,X,X}, "recvfrom",	/* 125 = recvfrom */
D, {D,D}	, "setreuid",	/* 126 = setreuid */
D, {D,D}	, "setregid",	/* 127 = setregid */
D, {S,S}	, "rename",	/* 128 = rename */
D, {S,L}	, "truncate",	/* 129 = truncate */
D, {D,L}	, "ftruncate",	/* 130 = ftruncate */
D, {D,O}	, "flock",	/* 131 = flock */
D, {V}		, "#132",	/* 132 = nosys */
D, {D,W,D,D,X,D}, "sendto",	/* 133 = sendto */
D, {D,D}	, "shutdown",	/* 134 = shutdown */
D, {D,D,D,X,V}	, "socketpair",	/* 135 = socketpair */
D, {S,O}	, "mkdir",	/* 136 = mkdir */
D, {S}		, "rmdir",	/* 137 = rmdir */
D, {S,X}	, "utimes",	/* 138 = utimes */
D, {V}		, "sigcleanup",	/* 139 = used internally */
D, {X,X}	, "adjtime",	/* 140 = adjtime */
D, {D,X,X}	, "getpeername",/* 141 = getpeername */
L, {V,V}	, "gethostid",	/* 142 = gethostid */
D, {V}		, "old sethostid - nosys",/* 143 = old sethostid */
D, {D,X}	, "getrlimit",	/* 144 = getrlimit */
D, {D,X}	, "setrlimit",	/* 145 = setrlimit */
D, {D,D}	, "killpg",	/* 146 = killpg */
D, {V}		, "#147",	/* 147 = nosys */
D, {V}		, "#148",	/* 148 = old setquota */
D, {V}		, "#149",	/* 149 = old qquota */
D, {D,X,X}	, "getsockname",/* 150 = getsockname */
D, {D,X,X,X}	, "getmsg",	/* 151 = getmsg */
D, {D,X,X,X}	, "putmsg",	/* 152 = putmsg */
D, {X,D,D}	, "poll",	/* 153 = poll */
D, {V}		, "nfs_mount",	/* 154 = old nfs_mount */
D, {D}		, "nfs_svc",	/* 155 = nfs_svc */
D, {D,X,D,X}	, "getdirentries",/* 156 = getdirentries */
D, {S,X}	, "statfs",	/* 157 = statfs */
D, {D,X}	, "fstatfs",	/* 158 = fstatfs */
D, {S}		, "unmount",	/* 159 = unmount */
V, {V}		, "async_daemon",/* 160 = async_daemon */
D, {X,X}	, "getfh", 	/* 161 = nfs_getfh */
D, {S,D}	, "getdomainname",/* 162 = getdomainname */
D, {W,D}	, "setdomainname",/* 163 = setdomainname */
D, {V}		, "pcfs_mount",	/* 164 = old pcfs_mount */
D, {D,S,D,X}	, "quotactl",	/* 165 = quotactl */
D, {X,X}	, "exportfs",	/* 166 = exportfs */
D, {S,S,O,X}	, "mount",	/* 167 = mount */
D, {X,X}	, "ustat",	/* 168 = ustat */
D, {X,X,X,X,X}	, "semsys",	/* 169 = semsys */
D, {X,X,X,X,X,X}, "msgsys",	/* 170 = msgsys */
D, {X,X,X,X}	, "shmsys",	/* 171 = shmsys */
D, {X,X,X,X}	, "auditsys",	/* 172 = auditsys */
D, {D,X,X,X,X}	, "rfssys",	/* 173 = rfssys */
D, {D,X,D}	, "getdents",	/* 174 = getdents */
D, {V}		, "setsid",	/* 175 = setsid */
D, {D}		, "fchdir",	/* 176 = fchdir */
D, {D}		, "fchroot",	/* 177 = fchroot */
D, {X,X}	, "vpixsys",	/* 178 = vpixsys */
D, {D,R,D,D,D,X}, "aread",	/* 179 = aread */
D, {D,W,D,D,D,X}, "awrite",	/* 180 = awrite */
D, {X}		, "await",	/* 181 = await */
D, {X}		, "acancel",	/* 182 = acancel */
D, {X}		, "sigpending",	/* 183 = sigpending */
D, {V}		, "#184",	/* 184 = AVAILABLE */
D, {D,D}	, "setpgid",	/* 185 = setpgid */
D, {S,D}	, "pathconf",	/* 186 = pathconf */
D, {D,D}	, "fpathconf",	/* 187 = fpathconf */
D, {D}		, "sysconf",	/* 188 = sysconf */
D, {X}		, "uname",	/* 189 = uname */
D, {V}		, ""
};

int nsysent = sizeof(sysinfo) / sizeof(struct sysinfo) ;

char *sigids[] = {
	"Unknown",
	"SIGHUP",	/*  1 - Hangup */
	"SIGINT",	/*  2 - Interrupt */
	"SIGQUIT",	/*  3 - Quit */
	"SIGILL",	/*  4 - Illegal instruction */
	"SIGTRAP",	/*  5 - Trace trap */
	"SIGIOT",	/*  6 - IOT instruction */
	"SIGEMT",	/*  7 - EMT instruction */
	"SIGFPE",	/*  8 - Floating point exception */
	"SIGKILL",	/*  9 - Kill (cannot be caught or ignored) */
	"SIGBUS",	/* 10 - Bus error */
	"SIGSEGV",	/* 11 - Segmentation violation */
	"SIGSYS",	/* 12 - Bad argument to system call */
	"SIGPIPE",	/* 13 - Write on a pipe with no reader */
	"SIGALRM",	/* 14 - Alarm clock */
	"SIGTERM",	/* 15 - Software termination signal from kill */
	"SIGURG",	/* 16 - Urgent condition on IO channel */
	"SIGSTOP",	/* 17 - Sendable stop signal not from tty */
	"SIGTSTP",	/* 18 - Stop signal from tty */
	"SIGCONT",	/* 19 - Continue a stopped process */
	"SIGCHLD",	/* 20 - To parent on child stop or exit */
	"SIGTTIN",	/* 21 - To readers pgrp on background tty read */
	"SIGTTOU",	/* 22 - Like TTIN for output if (tp->t_local&LTOSTOP) */
	"SIGIO",	/* 23 - Input/output possible signal */
	"SIGXCPU",	/* 24 - Exceeded cpu time limit */
	"SIGXFSZ",	/* 25 - Exceeded file size limit */
	"SIGVTALRM",	/* 26 - Virtual time alarm */
	"SIGPROF",	/* 27 - Profiling time alarm */
	"SIGWINCH",	/* 28 - Window changed */
	"SIGLOST",	/* 29 - Resource lost (eg, record-lock lost) */
	"SIGUSR1",	/* 30 - User defined signal 1 */
	"SIGUSR2",	/* 31 - User defined signal 2 */
	} ;

int nsig = sizeof(sigids) / sizeof(char *) ;
