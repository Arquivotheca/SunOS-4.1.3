#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)uname.c 1.1 92/07/30 SMI";
#endif

/* 	Xpg87 version of uname, with slightly different
 *	utsname structures.
/*
 * 	Copyright (C) 1988 by Sun Microsystems, Inc.
 */

#include <limits.h>

struct _fake_utsname {
	char	sysname[SYS_NMLN];
	char	nodename[SYS_NMLN];
	char	release[SYS_NMLN];
	char	version[SYS_NMLN];
	char	machine[SYS_NMLN];
};

struct _real_utsname {
	char	sysname[9];
	char	nodename[65];	/* MAXHOSTNAMELEN + 1 */
	char	release[9];
	char	version[9];
	char	machine[9];
};

int _xpg87_uname(ptr) 
struct _fake_utsname *ptr;
{
	register int p;

	struct _real_utsname realone;

	/* first test for invalid address */
	
	if ((p = uname(ptr)) < 0)
                return p;
	if ((p = uname(&realone)) < 0)
		return p;
	strcpy(ptr->sysname, realone.sysname);
	strcpy(ptr->nodename, realone.nodename);
	strcpy(ptr->release, realone.release);
	strcpy(ptr->version, realone.version);
	strcpy(ptr->machine, realone.machine);
	return 0;
}	
