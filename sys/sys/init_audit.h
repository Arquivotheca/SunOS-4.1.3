/*      @(#)init_audit.h 1.1 92/07/30 SMI	*/

/*
 * Definitions which replace system calls with thier wrappers for
 * auditing.
 */

#ifndef _sys_init_audit_h
#define _sys_init_audit_h

#ifdef	SYSAUDIT

/*
 * These wrappers are found in au_wrappers.c
 */
#define open		au_open
#define creat		au_creat
#define link		au_link
#define unlink		au_unlink
#define execv		au_execv
#define chdir		au_chdir
#define chmod		au_chmod
#define chown		au_chown
#define ptrace		au_ptrace
#define access		au_access
#define kill		au_kill
#define stat		au_stat
#define lstat		au_lstat
#define sysacct		au_sysacct
#define reboot		au_reboot
#define symlink		au_symlink
#define readlink	au_readlink
#define execve		au_execve
#define chroot		au_chroot
#define socket		au_socket
#define sethostname	au_sethostname
#define settimeofday	au_settimeofday
#define fchown		au_fchown
#define fchmod		au_fchmod
#define rename		au_rename
#define truncate	au_truncate
#define ftruncate	au_ftruncate
#define mkdir		au_mkdir
#define rmdir		au_rmdir
#define utimes		au_utimes
#define adjtime		au_adjtime
#define killpg		au_killpg
#define statfs		au_statfs
#define unmount		au_unmount
#define setdomainname	au_setdomainname
#define quotactl	au_quotactl
#define mount		au_mount
/*
 * These three wrappers are found in au_ipc_wrappers.c
 * The ipc wrappers are a little more complex than the others
 */
#define semsys		au_semsys
#define msgsys		au_msgsys
#define shmsys		au_shmsys

/*
 * These wrappers are found in au_wrappers.c
 * Unlike the others which just do auditing, these also control the
 * semantic differences in a secure and insecure system.
 */
#define fchdir		au_fchdir
#define fchroot		au_fchroot

#endif	SYSAUDIT

#endif /*!_sys_init_audit_h*/
