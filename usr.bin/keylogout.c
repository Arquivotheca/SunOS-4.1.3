#ifndef lint
static char sccsid[] = "@(#)keylogout.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * unset the secret key on local machine
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>

main(argc, argv)
	int argc;
	char *argv[];
{
	static char secret[HEXKEYBYTES + 1];
	char *getpass();

	if (geteuid() == 0) {
		if ((argc != 2)	|| (strcmp(argv[1], "-f") != 0)) {
			warning();
			exit(-1);
		}
	}
	if (key_setsecret(secret) < 0) {
		fprintf(stderr, "Could not unset your secret key.\n");
		fprintf(stderr, "Maybe the keyserver is down?\n");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}

static
warning()
{
	fprintf(stderr, "keylogout by root would break ");
	fprintf(stderr, "all servers that use secure rpc!\n");
	fprintf(stderr, "root may use keylogout -f to do ");
	fprintf(stderr, "this (at your own risk)!\n");
}
