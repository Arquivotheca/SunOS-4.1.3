/*	@(#)grp.h 1.1 92/07/30 SMI	*/

#ifndef	__grp_h
#define	__grp_h

#include <sys/types.h>

/*
 * We have to make this POSIX.1 compatible header compatible with SunOS
 * Release 4.0.x and the BSD interface provided by /usr/include/grp.h
 * so we have a filler to make the gid_t gr_gid field here match the
 * int gr_gid field there.
 * This will all go away in a later release when gid_t is enlarged.
 * Until then watch out for big- vs. little-endian problems in the filler.
 */
struct	group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
#if defined(mc68000) || defined(sparc)
	short	gr_gid_filler;
#endif
	gid_t	gr_gid;
#if defined(i386)
	short	gr_gid_filler;
#endif
	char	**gr_mem;
};

#ifndef	_POSIX_SOURCE
struct group *getgrent();
#endif

struct group *getgrgid(/* gid_t gid */);
struct group *getgrnam(/* char *name */);

#endif	/* !__grp_h */
