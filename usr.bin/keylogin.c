#ifndef lint
static char sccsid[] = "@(#)keylogin.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * Set secret key on local machine
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>

main(argc,argv)
	int argc;
	char *argv[];
{
	char fullname[MAXNETNAMELEN + 1];
	char secret[HEXKEYBYTES + 1];
	char *getpass();

	getnetname(fullname);
	if (! getsecretkey(fullname, secret, getpass("Password:"))) {
		fprintf(stderr, "Can't find %s's secret key\n", fullname);
		exit(1);
	}
	if (secret[0] == 0) {
		fprintf(stderr, "Password incorrect for %s\n", fullname);
		exit(1);
	}
	if (key_setsecret(secret) < 0) {
		fprintf(stderr, "Could not set %s's secret key\n", fullname);
		fprintf(stderr, "Maybe the keyserver is down?\n");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}
