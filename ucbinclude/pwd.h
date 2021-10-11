/*	@(#)pwd.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

#ifndef	__pwd_h
#define	__pwd_h

#include <sys/types.h>

struct passwd {
	char	*pw_name;
	char	*pw_passwd;
	int	pw_uid;
	int	pw_gid;
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
