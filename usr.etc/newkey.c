#ifndef lint
static char sccsid[] = "@(#)newkey.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * Administrative tool to add a new user to the publickey database
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <rpcsvc/ypclnt.h>
#include <mp.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pwd.h>
#include <netdb.h>
#include <vfork.h>

#include "yp/ypdefs.h"
USE_YPDBPATH

#define MAXMAPNAMELEN 256

extern char *getpass();
extern char *malloc();
extern char *strrchr();
extern char *sprintf();
extern long random();

static char *basename();
static char SHELL[] = "/bin/sh";

static char PKMAP[] = "publickey.byname";
static char UPDATEFILE[] = "updaters";

main(argc, argv)
	int argc;
	char *argv[];
{
	char name[MAXNETNAMELEN + 1];
	char public[HEXKEYBYTES + 1];
	char secret[HEXKEYBYTES + 1];
	char crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char crypt2[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	int status;	
	char *pass;
	struct passwd *pw;
	struct hostent *h;

	if (argc != 3 || !(strcmp(argv[1], "-u") == 0 ||
		strcmp(argv[1], "-h") == 0))
	{
		(void)fprintf(stderr, "usage: %s [-u username] [-h hostname]\n", 
			      argv[0]);
		exit(1);
	}
	if (geteuid() != 0) {
		(void)fprintf(stderr, "must be root to run %s\n", argv[0]);
		exit(1);
	}

	if (chdir(ypdbpath) < 0) {
		(void)fprintf(stderr, "cannot chdir to ");
		perror(ypdbpath);
	}
	if (strcmp(argv[1], "-u") == 0) {
		pw = getpwnam(argv[2]);
		if (pw == NULL) {
			(void)fprintf(stderr, "unknown user: %s\n", argv[2]);
			exit(1);
		}
		(void)user2netname(name, (int)pw->pw_uid, (char *)NULL);
	} else {
		h = gethostbyname(argv[2]);
		if (h == NULL) {
			(void)fprintf(stderr, "unknown host: %s\n", argv[2]);
			exit(1);
		}
		(void)host2netname(name, h->h_name, (char *)NULL);
	}

	(void)printf("Adding new key for %s.\n", name);
	pass = getpass("New password:");
	if (strlen(pass) == 0) {
		(void)fprintf(stderr,"Password unchanged.\n");
		exit(1);
	}
	genkeys(public, secret, pass);

	bcopy(secret, crypt1, HEXKEYBYTES);
	bcopy(secret, crypt1 + HEXKEYBYTES, KEYCHECKSUMSIZE);
	crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0;
	xencrypt(crypt1, pass);

	bcopy(crypt1, crypt2, HEXKEYBYTES + KEYCHECKSUMSIZE + 1);	
	xdecrypt(crypt2, getpass("Retype password:"));
	if (bcmp(crypt2, crypt2 + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0 ||
		bcmp(crypt2, secret, HEXKEYBYTES) != 0) 
	{	
		(void)fprintf(stderr, "Password incorrect.\n");
		exit(1);
	}

	(void)printf("Please wait for the database to get updated...\n");	
	if (status = setpublicmap(name, public, crypt1)) {
		(void)fprintf(stderr, 
			      "%s: unable to update yp database (%u): %s\n", 
			      argv[0], status, yperr_string(status));
		exit(1);
	}
	(void)printf("Your new key has been successfully stored away.\n");

	exit(0);
	/* NOTREACHED */
}

/*
 * Generate a seed
 */
getseed(seed, seedsize, pass)
	char *seed;
	int seedsize;
	unsigned char *pass;
{
	int i;
	int rseed;
	struct timeval tv;

	(void)gettimeofday(&tv, (struct timezone *)NULL);
	rseed = tv.tv_sec + tv.tv_usec;
	for (i = 0; i < 8; i++) {
		rseed ^= (rseed << 8) | pass[i];
	}
	srandom(rseed);

	for (i = 0; i < seedsize; i++) {
		seed[i] = (random() & 0xff) ^ pass[i % 8];
	}
}


/*
 * Generate a random public/secret key pair
 */
genkeys(public, secret, pass)
	char *public;
	char *secret;
	char *pass;
{
	int i;
 
#   define BASEBITS (8*sizeof(short) - 1)
#	define BASE		(1 << BASEBITS)
 
	MINT *pk = itom(0);
	MINT *sk = itom(0);
	MINT *tmp;
	MINT *base = itom(BASE);
	MINT *root = itom(PROOT);
	MINT *modulus = xtom(HEXMODULUS);
	short r;
	unsigned short seed[KEYSIZE/BASEBITS + 1];
	char *xkey;

	getseed((char *)seed, sizeof(seed), (u_char *)pass);	
	for (i = 0; i < KEYSIZE/BASEBITS + 1; i++) {
		r = seed[i] % BASE;
		tmp = itom(r);
		mult(sk, base, sk);
		madd(sk, tmp, sk);
		mfree(tmp);  
	}
	tmp = itom(0);
	mdiv(sk, modulus, tmp, sk);
	mfree(tmp);
	pow(root, sk, modulus, pk); 
	xkey = mtox(sk);   
	adjust(secret, xkey);
	xkey = mtox(pk);
	adjust(public, xkey);
	mfree(sk);
	mfree(base);
	mfree(pk);
	mfree(root);
	mfree(modulus);
} 

/*
 * Adjust the input key so that it is 0-filled on the left
 */
adjust(keyout, keyin)
	char keyout[HEXKEYBYTES+1];
	char *keyin;
{
	char *p;
	char *s;

	for (p = keyin; *p; p++) 
		;
	for (s = keyout + HEXKEYBYTES; p >= keyin; p--, s--) {
		*s = *p;
	}
	while (s >= keyout) {
		*s-- = '0';
	}
}

/*
 * Set the entry in the public key map
 */
setpublicmap(name, public, secret)
	char *name;
	char *public;
	char *secret;
{
	char *domain;
	char pkent[1024];
	u_int rslt;
	

	(void)yp_get_default_domain(&domain);
	(void)sprintf(pkent, "%s:%s", public, secret);
	rslt = yp_update(domain, PKMAP, YPOP_STORE, 
		name, strlen(name), pkent, strlen(pkent));
	return (rslt);
}

yp_update(dom, map, op, key, keylen, datum, datumlen)
	char *dom;
	char *map;	
	int op;
	char *key;
	int keylen;
	char *datum;
	int datumlen;
{
	return (update(key, map, (u_int)op, (u_int)keylen, key, 
		       (u_int)datumlen, datum));
}
	 
/*
 * Determine if requester is allowed to update the given map, 
 * and update it if so. Returns the yp status, which is zero
 * if there is no access violation.
 */
update(requester, mapname, op, keylen, key, datalen, data)
	char *requester;
	char *mapname;
	u_int op;
	u_int keylen;
	char *key;
	u_int datalen;
	char *data;
{
	char updater[MAXMAPNAMELEN + 40];
	FILE *childargs;
	FILE *childrslt;
	union wait status;
	int yperrno;
	int pid;

	(void)sprintf(updater, "make -s -f %s %s", UPDATEFILE, mapname);
	pid = _openchild(updater, &childargs, &childrslt);
	if (pid < 0) {
		return (YPERR_YPERR);
	}
 
	/*
	 * Write to child
	 */
	(void)fprintf(childargs, "%s\n", requester);
	(void)fprintf(childargs, "%u\n", op);
	(void)fprintf(childargs, "%u\n", keylen);
	(void)fwrite(key, (int)keylen, 1, childargs);
	(void)fprintf(childargs, "\n");
	(void)fprintf(childargs, "%u\n", datalen);
	(void)fwrite(data, (int)datalen, 1, childargs);
	(void)fprintf(childargs, "\n");
	(void)fclose(childargs);
 
	/*
	 * Read from child
	 */
	(void)fscanf(childrslt, "%d", &yperrno);
	(void)fclose(childrslt);
 
	(void)wait(&status);
	if (status.w_retcode != 0) {
		return (YPERR_YPERR);
	}
	return (yperrno);
}


/*
 * returns pid, or -1 for failure
 */
_openchild(command, fto, ffrom)
	char *command;
	FILE **fto;
	FILE **ffrom;
{
	int i;
	int pid;
	int pdto[2];
	int pdfrom[2];
	char *com;

		
	if (pipe(pdto) < 0) {
		goto error1;
	}
	if (pipe(pdfrom) < 0) {
		goto error2;
	}
	switch (pid = vfork()) {
	case -1:
		goto error3;

	case 0:
		/* 
		 * child: read from pdto[0], write into pdfrom[1]
		 */
		(void)close(0); 
		(void)dup(pdto[0]);
		(void)close(1); 
		(void)dup(pdfrom[1]);
		for (i = getdtablesize() - 1; i >= 3; i--) {
			(void) close(i);
		}
		com = malloc((unsigned) strlen(command) + 6);
		if (com == NULL) {
			_exit(~0);
		}	
		(void)sprintf(com, "exec %s", command);
		execl(SHELL, basename(SHELL), "-c", com, NULL);
		_exit(~0);
 
	default:
		/*
		 * parent: write into pdto[1], read from pdfrom[0]
		 */
		*fto = fdopen(pdto[1], "w");
		(void)close(pdto[0]);
		*ffrom = fdopen(pdfrom[0], "r");
		(void)close(pdfrom[1]);
		break;
	}
	return (pid);

	/*
	 * error cleanup and return
	 */
error3:
	(void)close(pdfrom[0]);
	(void)close(pdfrom[1]);
error2:
	(void)close(pdto[0]);
	(void)close(pdto[1]);
error1:
	return (-1);
}

static char *
basename(path)
	char *path;
{
	char *p;

	p = strrchr(path, '/');
	if (p == NULL) {
		return (path);
	} else {
		return (p + 1);
	}
}
