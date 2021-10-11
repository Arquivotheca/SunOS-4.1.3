/*	@(#)syscall.h 1.1 92/07/30 SMI; from UCB 4.11 06/09/83	*/

#ifndef	__sys_syscall_h
#define	__sys_syscall_h

#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_read	3
#define	SYS_write	4
#define	SYS_open	5
#define	SYS_close	6
#define	SYS_wait4	7
#define	SYS_creat	8
#define	SYS_link	9
#define	SYS_unlink	10
#define	SYS_execv	11
#define	SYS_chdir	12
				/* 13 is old: time */
#define	SYS_mknod	14
#define	SYS_chmod	15
#define	SYS_chown	16
				/* 17 is old: sbreak */
				/* 18 is old: stat */
#define	SYS_lseek	19
#define	SYS_getpid	20
				/* 21 is old: mount */
				/* 22 is old: umount */
				/* 23 is old: setuid */
#define	SYS_getuid	24
				/* 25 is old: stime */
#define	SYS_ptrace	26
				/* 27 is old: alarm */
				/* 28 is old: fstat */
				/* 29 is old: pause */
				/* 30 is old: utime */
				/* 31 is old: stty */
				/* 32 is old: gtty */
#define	SYS_access	33
				/* 34 is old: nice */
				/* 35 is old: ftime */
#define	SYS_sync	36
#define	SYS_kill	37
#define	SYS_stat	38
				/* 39 is old: setpgrp */
#define	SYS_lstat	40
#define	SYS_dup		41
#define	SYS_pipe	42
				/* 43 is old: times */
#define	SYS_profil	44
				/* 45 is unused */
				/* 46 is old: setgid */
#define	SYS_getgid	47
				/* 48 is old: sigsys */
				/* 49 is unused */
				/* 50 is unused */
#define	SYS_acct	51
				/* 52 is old: phys */
#define	SYS_mctl	53
#define	SYS_ioctl	54
#define	SYS_reboot	55
				/* 56 is old: mpxchan */
#define	SYS_symlink	57
#define	SYS_readlink	58
#define	SYS_execve	59
#define	SYS_umask	60
#define	SYS_chroot	61
#define	SYS_fstat	62
				/* 63 is unused */
#define	SYS_getpagesize 64
#define	SYS_msync	65
				/* 66 is old: vfork */
				/* 67 is old: vread */
				/* 68 is old: vwrite */
#define	SYS_sbrk	69
#define	SYS_sstk	70
#define	SYS_mmap	71
#define	SYS_vadvise	72
#define	SYS_munmap	73
#define	SYS_mprotect	74
#define	SYS_madvise	75
#define	SYS_vhangup	76
				/* 77 is old: vlimit */
#define	SYS_mincore	78
#define	SYS_getgroups	79
#define	SYS_setgroups	80
#define	SYS_getpgrp	81
#define	SYS_setpgrp	82
#define	SYS_setitimer	83
				/* 84 is old: wait & wait3 */
#define	SYS_swapon	85
#define	SYS_getitimer	86
#define	SYS_gethostname	87
#define	SYS_sethostname	88
#define	SYS_getdtablesize 89
#define	SYS_dup2	90
#define	SYS_getdopt	91
#define	SYS_fcntl	92
#define	SYS_select	93
#define	SYS_setdopt	94
#define	SYS_fsync	95
#define	SYS_setpriority	96
#define	SYS_socket	97
#define	SYS_connect	98
#define	SYS_accept	99
#define	SYS_getpriority	100
#define	SYS_send	101
#define	SYS_recv	102
				/* 103 was socketaddr */
#define	SYS_bind	104
#define	SYS_setsockopt	105
#define	SYS_listen	106
				/* 107 was vtimes */
#define	SYS_sigvec	108
#define	SYS_sigblock	109
#define	SYS_sigsetmask	110
#define	SYS_sigpause	111
#define	SYS_sigstack	112
#define	SYS_recvmsg	113
#define	SYS_sendmsg	114
#define	SYS_vtrace	115
#define	SYS_gettimeofday 116
#define	SYS_getrusage	117
#define	SYS_getsockopt	118
				/* 119 is old resuba */
#define	SYS_readv	120
#define	SYS_writev	121
#define	SYS_settimeofday 122
#define	SYS_fchown	123
#define	SYS_fchmod	124
#define	SYS_recvfrom	125
#define	SYS_setreuid	126
#define	SYS_setregid	127
#define	SYS_rename	128
#define	SYS_truncate	129
#define	SYS_ftruncate	130
#define	SYS_flock	131
				/* 132 is unused */
#define	SYS_sendto	133
#define	SYS_shutdown	134
#define	SYS_socketpair	135
#define	SYS_mkdir	136
#define	SYS_rmdir	137
#define	SYS_utimes	138
				/* 139 is unused */
#define	SYS_adjtime	140
#define	SYS_getpeername	141
#define	SYS_gethostid	142
				/* 143 is old: sethostid */
#define	SYS_getrlimit	144
#define	SYS_setrlimit	145
#define	SYS_killpg	146
				/* 147 is unused */
				/* 148 is old: setquota */
				/* 149 is old: quota */
#define	SYS_getsockname	150
#define	SYS_getmsg	151
#define	SYS_putmsg	152
#define	SYS_poll	153
				/* 154 is old: nfs_mount */
#define	SYS_nfssvc	155
#define	SYS_getdirentries 156
#define	SYS_statfs	157
#define	SYS_fstatfs	158
#define	SYS_unmount	159
#define	SYS_async_daemon 160
#define	SYS_getfh	161
#define	SYS_getdomainname 162
#define	SYS_setdomainname 163
				/* 164 is old: pcfs_mount */
#define	SYS_quotactl	165
#define	SYS_exportfs	166
#define	SYS_mount	167
#define	SYS_ustat	168
#define	SYS_semsys	169
#define	SYS_msgsys	170
#define	SYS_shmsys	171
#define	SYS_auditsys	172
#define	SYS_rfssys	173
#define	SYS_getdents	174
#define	SYS_setsid	175
#define	SYS_fchdir	176
#define	SYS_fchroot	177
#define	SYS_vpixsys	178

#define	SYS_aioread	179
#define	SYS_aiowrite	180
#define	SYS_aiowait	181
#define	SYS_aiocancel	182

#define	SYS_sigpending	183
				/* 184 is available */
#define	SYS_setpgid	185
#define	SYS_pathconf	186
#define	SYS_fpathconf	187
#define	SYS_sysconf	188

#define	SYS_uname	189

#endif	/* !__sys_syscall_h */
