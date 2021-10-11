/*	@(#)pwd.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

#ifndef	__pwd_h
#define	__pwd_h

#include <sys/types.h>

/*
 * We have to make this POSIX.1 compatible header compatible with SunOS
 * Release 4.0.x and the BSD interface provided by /usr/include/pwd.h
 * so we have fillers to make the gid_t pw_gid field here match the
 * int pw_gid field there and the uid_t pw_uid field here match the
 * int pw_uid field there.
 * This will all go away in a later release when gid_t is enlarged.
 * Until then watch out for big- vs. little-endian problems in the filler.
 */
struct passwd {
	char	*pw_name;
	char	*pw_passwd;
#if defined(mc68000) || defined(sparc)
	short	pw_uid_filler;
#endif
	uid_t	pw_uid;
#if defined(i386)
	short	pw_uid_filler;
#endif
#if defined(mc68000) || defined(sparc)
	short	pw_gid_filler;
#endif
	gid_t	pw_gid;
#if defined(i386)
	short	pw_gid_filler;
#endif
	char	*pw_age;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};


#ifndef	_POSIX_SOURCE
extern struct passwd *getpwent();

struct comment {
        char    *c_dept;
        char    *c_name;
        char    *c_acct;
        char    *c_bin;
};

#endif

struct passwd *getpwuid(/* uid_t uid */);
struct passwd *getpwnam(/* char *name */);

#endif	/* !__pwd_h */
