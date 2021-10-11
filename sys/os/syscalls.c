/*	@(#)syscalls.c 1.1 92/07/30 SMI; from UCB 4.2 83/05/21	*/

/*
 * System call names.
 */
char *syscallnames[] = {
	"indir",		/*   0 = indir */
	"exit",			/*   1 = exit */
	"fork",			/*   2 = fork */
	"read",			/*   3 = read */
	"write",		/*   4 = write */
	"open",			/*   5 = open */
	"close",		/*   6 = close */
	"old wait",		/*   7 = old wait */
	"creat",		/*   8 = creat */
	"old link",		/*   9 = link */
	"unlink",		/*  10 = unlink */
	"execv",		/*  11 = execv */
	"chdir",		/*  12 = chdir */
	"old time",		/*  13 = old time */
	"mknod",		/*  14 = mknod */
	"chmod",		/*  15 = chmod */
	"chown",		/*  16 = chown; now 3 args */
	"old break",		/*  17 = old break */
	"old stat",		/*  18 = old stat */
	"lseek",		/*  19 = lseek */
	"getpid",		/*  20 = getpid */
	"old mount - nosys",	/*  21 = old mount */
	"old umount - nosys",	/*  22 = old umount */
	"old setuid",		/*  23 = old setuid */
	"getuid",		/*  24 = getuid */
	"old stime",		/*  25 = old stime */
	"ptrace",		/*  26 = ptrace */
	"old alarm",		/*  27 = old alarm */
	"old fstat",		/*  28 = old fstat */
	"old pause",		/*  29 = opause */
	"old utime",		/*  30 = old utime */
	"old stty",		/*  31 = old stty */
	"old gtty",		/*  32 = old gtty */
	"access",		/*  33 = access */
	"old nice",		/*  34 = old nice */
	"old ftime",		/*  35 = old ftime */
	"sync",			/*  36 = sync */
	"kill",			/*  37 = kill */
	"stat",			/*  38 = stat */
	"old setpgrp",		/*  39 = old setpgrp */
	"lstat",		/*  40 = lstat */
	"dup",			/*  41 = dup */
	"pipe",			/*  42 = pipe */
	"old times",		/*  43 = old times */
	"profil",		/*  44 = profil */
	"#45",			/*  45 = nosys */
	"old setgid",		/*  46 = old setgid */
	"getgid",		/*  47 = getgid */
	"old signal",		/*  48 = old sig */
	"#49",			/*  49 = reserved for USG */
	"#50",			/*  50 = reserved for USG */
	"acct",			/*  51 = turn acct off/on */
	"old phys - nosys",	/*  52 = old set phys addr */
	"old lock - nosys",	/*  53 = old lock in core */
	"ioctl",		/*  54 = ioctl */
	"reboot",		/*  55 = reboot */
	"wait3",		/*  56 = new wait3 (old mpxchan) */
	"symlink",		/*  57 = symlink */
	"readlink",		/*  58 = readlink */
	"execve",		/*  59 = execve */
	"umask",		/*  60 = umask */
	"chroot",		/*  61 = chroot */
	"fstat",		/*  62 = fstat */
	"#63",			/*  63 = used internally */
	"getpagesize",		/*  64 = getpagesize */
	"msync",		/*  65 = msync */
	"vfork",		/*  66 = vfork */
	"old vread - read",	/*  67 = old vread */
	"old vwrite - write",	/*  68 = old vwrite */
	"sbrk",			/*  69 = sbrk */
	"sstk",			/*  70 = sstk */
	"mmap",			/*  71 = mmap */
	"old vadvise",		/*  72 = old vadvise */
	"munmap",		/*  73 = munmap */
	"mprotect",		/*  74 = mprotect */
	"madvise",		/*  75 = madvise */
	"vhangup",		/*  76 = vhangup */
	"old vlimit",		/*  77 = old vlimit */
	"mincore",		/*  78 = mincore */
	"getgroups",		/*  79 = getgroups */
	"setgroups",		/*  80 = setgroups */
	"getpgrp",		/*  81 = getpgrp */
	"setpgrp",		/*  82 = setpgrp */
	"setitimer",		/*  83 = setitimer */
	"wait",			/*  84 = wait */
	"old swapon",		/*  85 = old swapon */
	"getitimer",		/*  86 = getitimer */
	"gethostname",		/*  87 = gethostname */
	"sethostname",		/*  88 = sethostname */
	"getdtablesize",	/*  89 = getdtablesize */
	"dup2",			/*  90 = dup2 */
	"getdopt",		/*  91 = getdopt */
	"fcntl",		/*  92 = fcntl */
	"select",		/*  93 = select */
	"setdopt",		/*  94 = setdopt */
	"fsync",		/*  95 = fsync */
	"setpriority",		/*  96 = setpriority */
	"socket",		/*  97 = socket */
	"connect",		/*  98 = connect */
	"accept",		/*  99 = accept */
	"getpriority",		/* 100 = getpriority */
	"send",			/* 101 = send */
	"recv",			/* 102 = recv */
	"old socketaddr",	/* 103 = old socketaddr */
	"bind",			/* 104 = bind */
	"setsockopt",		/* 105 = setsockopt */
	"listen",		/* 106 = listen */
	"old vtimes",		/* 107 = old vtimes */
	"sigvec",		/* 108 = sigvec */
	"sigblock",		/* 109 = sigblock */
	"sigsetmask",		/* 110 = sigsetmask */
	"sigpause",		/* 111 = sigpause */
	"sigstack",		/* 112 = sigstack */
	"recvmsg",		/* 113 = recvmsg */
	"sendmsg",		/* 114 = sendmsg */
#ifdef TRACE
	"vtrace",		/* 115 = vtrace */
#else
#ifdef TESTECC
	"testecc",		/* 115 = testecc */
#else
	"#115",			/* 115 = nosys */
#endif TESTECC
#endif
	"gettimeofday",		/* 116 = gettimeofday */
	"getrusage",		/* 117 = getrusage */
	"getsockopt",		/* 118 = getsockopt */
#ifdef vax
	"resuba",		/* 119 = resuba */
#else
	"#119",			/* 119 = nosys */
#endif
	"readv",		/* 120 = readv */
	"writev",		/* 121 = writev */
	"settimeofday",		/* 122 = settimeofday */
	"fchown",		/* 123 = fchown */
	"fchmod",		/* 124 = fchmod */
	"recvfrom",		/* 125 = recvfrom */
	"setreuid",		/* 126 = setreuid */
	"setregid",		/* 127 = setregid */
	"rename",		/* 128 = rename */
	"truncate",		/* 129 = truncate */
	"ftruncate",		/* 130 = ftruncate */
	"flock",		/* 131 = flock */
	"#132",			/* 132 = nosys */
	"sendto",		/* 133 = sendto */
	"shutdown",		/* 134 = shutdown */
	"socketpair",		/* 135 = socketpair */
	"mkdir",		/* 136 = mkdir */
	"rmdir",		/* 137 = rmdir */
	"utimes",		/* 138 = utimes */
	"#139",			/* 139 = used internally */
	"adjtime",		/* 140 = adjtime */
	"getpeername",		/* 141 = getpeername */
	"gethostid",		/* 142 = gethostid */
	"old sethostid - nosys",/* 143 = old sethostid */
	"getrlimit",		/* 144 = getrlimit */
	"setrlimit",		/* 145 = setrlimit */
	"killpg",		/* 146 = killpg */
	"#147",			/* 147 = nosys */
	"#148",			/* 148 = old setquota */
	"#149",			/* 149 = old qquota */
	"getsockname",		/* 150 = getsockname */
	"getmsg",		/* 151 = getmsg */
	"putmsg",		/* 152 = putmsg */
	"poll",			/* 153 = poll */
#ifdef	NFS
	"old nfs_mount - nosys",/* 154 = nfs_mount */
	"nfs_svc",		/* 155 = nfs_svc */
#else
	"#154",			/* 154 = nosys */
	"#155",			/* 155 = nosys */
#endif
	"getdirentries",	/* 156 = getdirentries */
	"statfs",		/* 157 = statfs */
	"fstatfs",		/* 158 = fstatfs */
	"unmount",		/* 159 = unmount */
#ifdef NFS
	"async_daemon",		/* 160 = async_daemon */
	"getfh", 		/* 161 = nfs_getfh */
#else
	"#160",			/* 160 = nosys */
	"#161",			/* 161 = nosys */
#endif
	"getdomainname",	/* 162 = getdomainname */
	"setdomainname",	/* 163 = setdomainname */
#ifdef RT_SCHEDULE
	"rtschedule",		/* 164 = rtschedule */
#else
	"#164",			/* 164 = nosys */
#endif
#ifdef QUOTA
	"quotactl",		/* 165 = quotactl */
#else
	"#165",			/* 165 = nullsys */
#endif
#ifdef NFS
	"exportfs",		/* 166 = exportfs */
#else
	"#166",			/* 166 = nosys */
#endif
	"mount",		/* 167 = mount */
	"ustat",		/* 168 = ustat */
#ifdef IPCSEMAPHORE
	"semsys",		/* 169 = semsys */
#else
	"#169",			/* 169 = nosys */
#endif
#ifdef IPCMESSAGE
	"msgsys",		/* 170 = msgsys */
#else
	"#170",			/* 170 = nosys */
#endif
#ifdef IPCSHMEM
	"shmsys",		/* 171 = shmsys */
#else
	"#171",			/* 171 = nosys */
#endif
#ifdef SYSAUDIT
	"auditsys",		/* 172 = auditsys */
#else
	"#172",			/* 172 = nullsys */
#endif
#ifdef RFS
	"rfssys",		/* 173 = RFS calls */
#else
	"#173",			/* 173 = errsys */
#endif
	"getdents",		/* 174 = getdents */
	"setsid",		/* 175 = setsid */
	"fchdir",		/* 176 = fchdir */
	"fchroot",		/* 177 = fchroot */
#ifdef VPIX
	"vpixsys",		/* 178 = v86 system call */
#else
	"#178",			/* 178 = errsys */
#endif
#ifdef ASYNCHIO
	"aioread",		/* 179 = aioread */
	"aiowrite",		/* 180 = aiowrite */
	"aiowait",		/* 181 = aiowait */
	"aiocancel",		/* 182 = aioancel */
#else
	"#179",			/* 179 = errsys */
	"#180",			/* 180 = errsys */
	"#181",			/* 181 = errsys */
	"#182",			/* 182 = errsys */
#endif ASYNCHIO
	"sigpending",		/* 183 = sigpending */
	"#184",			/* 184 = AVAILABLE */
	"setpgid",		/* 185 = setpgid */
	"pathconf",		/* 186 = pathconf */
	"fpathconf",		/* 187 = fpathconf */
	"sysconf",		/* 188 = sysconf */
	"uname",		/* 189 = uname */
#ifdef VDDRV
	"#190",			/* 190 = reserved for loadable module */
	"#191",			/* 191 = reserved for loadable module */
	"#192",			/* 192 = reserved for loadable module */
	"#193",			/* 193 = reserved for loadable module */
	"#194",			/* 194 = reserved for loadable module */
	"#195",			/* 195 = reserved for loadable module */
	"#196",			/* 196 = reserved for loadable module */
	"#197",			/* 197 = reserved for loadable module */
#endif
};
