/*	@(#)utsname.h 1.1 92/07/30 SMI; from S5R2 6.1	*/

#ifndef	__sys_utsname_h
#define	__sys_utsname_h

struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	nodeext[65-9];  /* extends nodename to MAXHOSTNAMELEN+1 chars */
	char	release[9];
	char	version[9];
	char	machine[9];
};

#ifdef	KERNEL
extern struct utsname utsname;
#else
int	uname(/* struct utsname *name */);
#endif

#endif	/* !__sys_utsname_h */
