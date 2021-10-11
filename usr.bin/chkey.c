
#ifndef lint
static char sccsid[] = "@(#)chkey.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
/*
 * Copyright (c) 1990, Sun Microsystems, Inc.
 */

/*
 * Command to change one's public key in the public key database
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <rpcsvc/ypclnt.h>
#include <sys/time.h>
#include <sys/file.h>
#include <pwd.h>
#include <mp.h>

extern char *getpass();
extern char *index();
extern char *crypt();
extern char *sprintf();
extern long random();

static char PKMAP[] = "publickey.byname";
static char *domain;
struct passwd *ypgetpwuid();

main(argc, argv)
	int argc;
	char **argv;
{
	char name[MAXNETNAMELEN+1];
	char public[HEXKEYBYTES + 1];
	char secret[HEXKEYBYTES + 1];
	char crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char crypt2[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	int status;	
	char *pass;
	struct passwd *pw;
	char *master;
	int euid;
	int fd;
	int force;
	char *self;
	int tries;

	self = argv[0];
	force = 0;
	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (argv[0][2] != 0) {
			usage(self);
		}
		switch (argv[0][1]) {
		case 'f':
			force = 1;
			break;
		default:
			usage(self);
		}
	}
	if (argc != 0) {
		usage(self);
	}

	(void)yp_get_default_domain(&domain);
	if (yp_master(domain, PKMAP, &master) != 0) {
		(void)fprintf(stderr, 
			"can't find master of publickey database\n");
		exit(1);
	}

	getnetname(name);
	(void)printf("Generating new key for %s.\n", name);

	
	if (!force){
		euid = geteuid();
		if (euid != 0) {
			pw = ypgetpwuid(euid);
			if (pw == NULL) {
				(void)fprintf(stderr, 
				"No NIS password found: can't change key.\n");
				exit(1);
			}
		} else {
			pw = getpwuid(0);
			if (pw == NULL) {
				(void)fprintf(stderr, 
				"No password found: can't change key.\n");
				exit(1);
			}
		}
	}
	pass = getpass("Password:");
	if (!force) {
		if (strcmp(crypt(pass, pw->pw_passwd), pw->pw_passwd) != 0) {
			(void)fprintf(stderr, "Invalid password.\n");
			exit(1);
		}
	}

	genkeys(public, secret, pass);	

	bcopy(secret, crypt1, HEXKEYBYTES);
	bcopy(secret, crypt1 + HEXKEYBYTES, KEYCHECKSUMSIZE);
	crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0;
	xencrypt(crypt1, pass);

	if (force) {
		bcopy(crypt1, crypt2, HEXKEYBYTES + KEYCHECKSUMSIZE + 1);	
		xdecrypt(crypt2, getpass("Retype password:"));
		if (bcmp(crypt2, crypt2 + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0 ||
	    	    bcmp(crypt2, secret, HEXKEYBYTES) != 0) {	
			(void)fprintf(stderr, "Password incorrect.\n");
			exit(1);
		}
	}

	for (tries = 0; tries < 2; tries++){
	(void)printf("Sending key change request to %s...\n", master);
	status = setpublicmap(name, public, crypt1);
	if (status != 0) {
		(void)printf("%s: unable to update NIS database (%u): %s\n", 
			     self, status, yperr_string(status));
		if (status == YPERR_RESRC) {
		(void) printf("I couldn't generate a secure RPC authenticator to %s\n",
			master);
		if (tries != 0){
		(void)printf("%s%s%s%s%s",
		"The keyserver /usr/etc/keyserv must be running.\n",
		"You may have to keylogin before doing a chkey.\n",
		"If you do not have a key, you may need to get a system\n",
		"administrator to create an initial key for you with newkey.\n",
		"The system could be loaded, so you might try this again.\n");
		exit(2);
		}
		else{
			(void) printf("I'll try again.\n");
			continue;
			}
		}
		else if (status == YPERR_ACCESS) {
		    if (tries == 0) {
			(void)printf("The server %s%s\n",
			    master,
			    " rejected your change for security reasons.");
			(void)printf("%s%s%s",
			    "You may have to keylogin before doing a chkey.\n",
			    "If you have done a chkey recently your new\n",
			    "key may not have been propogated to your local NIS server yet.\n"
			    );
			    (void)printf("%s%s",
			    "Lets try doing a keylogin first...\n",
			    "Enter Current ");
		    system("/usr/bin/keylogin");
		    }
		    else {
			(void)printf("The server %s%s\n",
			    master,
			    " rejected your change for security reasons again."
			    );
		    }
		}
		else if (status == YPERR_YPSERV) {
		(void)printf("%s is down or not running rpc.ypupdated\n", master);
		}
		else{
		(void)printf("Perhaps %s is down?\n", master);
		exit(1);
		}
	} else {
		(void)printf("Done.\n");
		break;
	}
	}


	if (key_setsecret(secret) < 0) {
		(void)printf("Unable to login with new secret key.\n");
		exit(1);
	}
}

usage(name)
	char *name;
{
	(void)fprintf(stderr, "usage: %s [-f]\n", name);
	exit(1);
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
 
#define BASEBITS	(8*sizeof(short) - 1)
#define BASE		(1 << BASEBITS)
 
	MINT *pk = itom(0);
 	MINT *sk = itom(0);
	MINT *tmp;
	MINT *base = itom(BASE);
	MINT *root = itom(PROOT);
	MINT *modulus = xtom(HEXMODULUS);
	short r;
	unsigned short seed[KEYSIZE/BASEBITS + 1];
	char *xkey;

	getseed((char *)seed, sizeof(seed), (unsigned char *)pass);	
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
	char pkent[1024];
	u_int rslt;
	
	(void)sprintf(pkent,"%s:%s", public, secret);
	rslt = yp_update(domain, PKMAP, YPOP_STORE, 
		name, strlen(name), pkent, strlen(pkent));
	return (rslt);
}

struct passwd *
ypgetpwuid(uid)
	int uid;
{
	char uidstr[10];
	char *val;
	int vallen;
	static struct passwd pw;
	char *p;

	(void)sprintf(uidstr, "%d", uid);
	if (yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr), 
		     &val, &vallen) != 0) {
		return (NULL);
	}
	p = index(val, ':');
	if (p == NULL) {	
		return (NULL);
	}
	pw.pw_passwd = p + 1;
	p = index(pw.pw_passwd, ':');
	if (p == NULL) {
		return (NULL);
	}
	*p = 0;
	return (&pw);
}
