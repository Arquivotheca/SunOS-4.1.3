#ifndef lint
static char sccsid[] = "@(#)publickey.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * Public key lookup routines
 */
#include <stdio.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>


extern char *strchr();
extern char *strcpy();

static char PKMAP[] = "publickey.byname";

/*
 * Get somebody's encrypted secret key from the database, using
 * the given passwd to decrypt it.
 */
getsecretkey(netname, secretkey, passwd)
	char *netname;
	char *secretkey;
	char *passwd;
{
	char *domain;
	int len;
	char *lookup;
	int err;
	char *p;


	err = yp_get_default_domain(&domain);
	if (err) {
		return(0);
	}
	err = yp_match(domain, PKMAP, netname, strlen(netname), &lookup, &len);
	if (err) {
		return(0);
	}
	lookup[len] = 0;
	p = strchr(lookup,':');
	if (p == NULL) {
		free(lookup);
		return(0);
	}
	p++;
	if (!xdecrypt(p, passwd)) {
		free(lookup);
		return(0);
	}
	if (bcmp(p, p + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0) {
		secretkey[0] = 0;
		free(lookup);
		return(1);
	}
	p[HEXKEYBYTES] = 0;
	(void) strcpy(secretkey, p);
	free(lookup);
	return(1);
}



/*
 * Get somebody's public key
 */
getpublickey(netname, publickey)
	char *netname;
	char *publickey;
{
	char *domain;
	int len;
	char *lookup;
	int err;
	char *p;

	err = yp_get_default_domain(&domain);	
	if (err) {
		return(0);
	}
	err = yp_match(domain, PKMAP, netname, strlen(netname), &lookup, &len);
	if (err) {
		return(0);
	}
	p = strchr(lookup, ':');
	if (p == NULL) {
		free(lookup);
		return(0);
	}
	*p = 0;	
	(void) strcpy(publickey, lookup);
	free(lookup);
	return(1);
}
