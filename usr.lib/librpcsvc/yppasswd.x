/* @(#)yppasswd.x 1.1 92/07/30 Copyr 1990 Sun Micro */

/*
 * NIS password update protocol
 * Requires unix authentication
 */
program YPPASSWDPROG {
	version YPPASSWDVERS {
		/*
		 * Update my passwd entry 
		 */
		int
		YPPASSWDPROC_UPDATE(yppasswd) = 1;
	} = 1;
} = 100009;


struct passwd {
	string pw_name<>;	/* username */
	string pw_passwd<>;	/* encrypted password */
	int pw_uid;		/* user id */
	int pw_gid;		/* group id */
	string pw_gecos<>;	/* in real life name */
	string pw_dir<>;	/* home directory */
	string pw_shell<>;	/* default shell */
};

struct yppasswd {
	string oldpass<>;	/* unencrypted old password */
	passwd newpw;		/* new passwd entry */
};


