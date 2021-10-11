/*	@(#)yppasswd.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _rpcsvc_yppasswd_h
#define _rpcsvc_yppasswd_h

#define YPPASSWDPROG 100009
#define YPPASSWDPROC_UPDATE 1
#define YPPASSWDVERS_ORIG 1
#define YPPASSWDVERS 1

struct yppasswd {
	char *oldpass;		/* old (unencrypted) password */
	struct passwd newpw;	/* new pw structure */
};

int xdr_yppasswd();

#endif /*!_rpcsvc_yppasswd_h*/
