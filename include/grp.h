/*	@(#)grp.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03	*/

#ifndef	__grp_h
#define	__grp_h

#include <sys/types.h>

struct	group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	int	gr_gid;
	char	**gr_mem;
};

#ifndef	_POSIX_SOURCE
struct group *getgrent();
#endif

struct group *getgrgid(/* gid_t gid */);
struct group *getgrnam(/* char *name */);

#endif	/* !__grp_h */
