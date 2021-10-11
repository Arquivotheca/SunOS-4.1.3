/*      @(#)pwdadj.h 1.1 92/07/30 SMI      */ /* c2 secure */

#ifndef _pwdadj_h
#define _pwdadj_h

struct	passwd_adjunct { /* see getpwaent(3) */
	char		*pwa_name;
	char		*pwa_passwd;
	blabel_t	pwa_minimum;
	blabel_t	pwa_maximum;
	blabel_t	pwa_def;
	audit_state_t	pwa_au_always;
	audit_state_t	pwa_au_never;
	int		pwa_version;
	char		*pwa_age;
};

struct passwd_adjunct *getpwaent(), *getpwauid(), *getpwanam();

#define PWA_VALID 0
#define PWA_INVALID -1
#define PWA_UNKNOWN -2

#endif /*!_pwdadj_h*/
