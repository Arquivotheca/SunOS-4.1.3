
/*      @(#)pwdnm.h 1.1 92/07/30 SMI      */ /* c2 secure */

#ifndef _rpcsvc_pwdnm_h
#define _rpcsvc_pwdnm_h

struct pwdnm {
	char *name;
	char *password;
};
typedef struct pwdnm pwdnm;


#define PWDAUTH_PROG 100036
#define PWDAUTH_VERS 1
#define PWDAUTHSRV 1
#define GRPAUTHSRV 2

bool_t xdr_pwdnm();

#endif /*!_rpcsvc_pwdnm_h*/
