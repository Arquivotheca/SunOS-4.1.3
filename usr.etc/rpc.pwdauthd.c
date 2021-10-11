#ifndef lint
static  char sccsid[] = "@(#)rpc.pwdauthd.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/pwdnm.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <pwd.h>
#include <pwdadj.h>
#include <grpadj.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <netdb.h>
#define AUDIT_USER "AUpwdauthd"
#define PPWA_VALID 3
#define PPWA_INVALID -3

#include <sys/param.h>

void pwdauth_prog_1();

struct passwd_adjunct *pwadj;
struct group_adjunct *gradj;
char *audit_argv[] = { "rpc.pwdauthd", 0, 0, 0, 0, 0 };

main()
{
	int s;
	SVCXPRT *transp;

	if ((s = rresvport()) < 0) {
		fprintf(stderr,
			"pwdauthd: can't bind to a privileged socket\n");
		exit(1);
	}
	transp = svcudp_create(s);

	if (transp == NULL) {
		fprintf(stderr, "pwdauthd: couldn't create an RPC server\n");
		exit(1);
	}

	pmap_unset(PWDAUTH_PROG, PWDAUTH_VERS);
	if (! svc_register(transp, PWDAUTH_PROG, PWDAUTH_VERS,
			pwdauth_prog_1, IPPROTO_UDP)) {
		fprintf(stderr, "Unable to register pwdauthd\n");
		exit(1);
	}
#ifndef DEBUG
	if (fork())
		exit(0);
	{ int t;
	for (t = getdtablesize()-1; t >= 0; t--)
		if (t != s)
			(void) close(t);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}
#endif DEBUG
	svc_run();
	fprintf(stderr, "pwdauthd: svc_run shouldn't have returned\n");
	exit(1);
	/* NOTREACHED */
}



static void
pwdauth_prog_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int notlochost;
	char raddr[256];

	if (setauditid(transp) == PPWA_INVALID){
		int reply = PPWA_INVALID;
		fprintf(stderr,"pwdauthd: can't find pseudo-user in passwd or passwd.adjunct\n");
		if (! svc_sendreply(transp, xdr_int, &reply))
                        fprintf(stderr,
                                "pwdauthd: couldn't reply to RPC call\n");
		return;
	}

	notlochost = notlocalhost(transp);
	if (notlochost > 0) {
		/* this is a foreign host request */
		int reply = PWA_INVALID;
                audit_argv[1] = "foreign host is attempting to use daemon";
		sprintf(raddr, "IP address = 0x%x", *(u_long *)&transp->xp_raddr.sin_addr);
		audit_argv[2] = raddr;
                audit_text(AU_ADMIN, 0, 1, 3, audit_argv);

		fprintf(stderr,"pwdauthd: can't service a remote request\n");
	/* reply to system cracker */
		if (! svc_sendreply(transp, xdr_int, &reply))
                        fprintf(stderr,
                                "pwdauthd: couldn't reply to RPC call\n");
		return;
	} 
	if (notlochost < 0) {
		/* error occurred in notlocalhost() */
		fprintf(stderr,"pwdauthd: can't identify origin of RPC request\n");
		if (! svc_sendreply(transp, xdr_void, NULL))
                        fprintf(stderr,
                                "pwdauthd: couldn't reply to RPC call\n");
                return;
        } 

	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (! svc_sendreply(transp, xdr_void, NULL))
			fprintf(stderr,
				"pwdauthd: couldn't reply to RPC call\n");
		return;

	case PWDAUTHSRV:
		pwdauthsrv_1(rqstp, transp);
		break;

	case GRPAUTHSRV:
		grpauthsrv_1(rqstp, transp);
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
}



pwdauthsrv_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int ans = PWA_INVALID;
	char *name, *password;
	pwdnm pwdname;
	struct authunix_parms	*unix_cred;
	int uid;
	char struid[10];

	bzero(&pwdname, sizeof(pwdname));
	if (! svc_getargs(transp, xdr_pwdnm, &pwdname)) {
		svcerr_decode(transp);
		printf("svc_getargs failed!\n");
		return;
	}

	/* get name and passwd */
	name = malloc(strlen(pwdname.name)+1);
	strcpy(name, pwdname.name);
	password = malloc(strlen(pwdname.password)+1);
	strcpy(password, pwdname.password);

	if (access("/etc/security/passwd.adjunct", R_OK) < 0) {
		ans = PWA_UNKNOWN;
		audit_argv[5] = "not using passwd.adjunct";
	}
	else {
		if ((pwadj = getpwanam(name)) != NULL) {
			/* we found the person in the adjunct files */
			if ((pwadj->pwa_passwd[0] != '#') &&
			   (strcmp(crypt(password, pwadj->pwa_passwd),
					 pwadj->pwa_passwd) == 0)) {
				ans = PWA_VALID;
				audit_argv[5] = "valid";
			}
			else
				/* slow down password crackers */
				sleep(1);

		}
	}
	/* audit who is doing an authentication and what the answer is */
	switch(rqstp->rq_cred.oa_flavor) {
	case AUTH_UNIX:
		unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
		uid = unix_cred->aup_uid;
		break;
	case AUTH_NULL:
	default:
		svcerr_weakauth(transp);
		return;
	}
	audit_argv[1] = "user";
	sprintf(struid, "%d", uid);
	audit_argv[2] = struid;
	audit_argv[3] = name;
	audit_argv[4] = password;
	if (ans == PWA_INVALID) {
		audit_argv[5] = "invalid";
		audit_text(AU_ADMIN, 0, 1, 6, audit_argv);
	}
	else {
		audit_text(AU_ADMIN, 0, 0, 6, audit_argv);
	}

	if (! svc_sendreply(transp, xdr_int, &ans)) {
		fprintf(stderr, "pwdauthd: couldnt reply to RPC call\n");
	}

	if (! svc_freeargs(transp, xdr_pwdnm, &pwdname)) {
	 	fprintf(stderr, "pwdauthd: unable to free arguments\n");
		exit(1);
	}
}

grpauthsrv_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int ans = PWA_INVALID;
	char *name, *password;
	pwdnm pwdname;
	struct authunix_parms	*unix_cred;
	int uid;
	char struid[10];

        bzero(&pwdname, sizeof(pwdname));
        if (! svc_getargs(transp, xdr_pwdnm, &pwdname)) {
                svcerr_decode(transp);
                printf("svc_getargs failed!\n");
                return;
        }
 
        /* get name and passwd */
        name = malloc(strlen(pwdname.name)+1);
        strcpy(name, pwdname.name);
        password = malloc(strlen(pwdname.password)+1);
        strcpy(password, pwdname.password);
 
        if (access("/etc/security/group.adjunct", R_OK) < 0) {
                ans = PWA_UNKNOWN;
		audit_argv[5] = "not using group.adjunct";
        }
        else {
                if ((gradj = getgranam(name)) != NULL) {
                        /* we found the person in the adjunct files */
			if ((gradj->gra_passwd[0] != '#') &&
                           (strcmp(crypt(password, gradj->gra_passwd),
                                        gradj->gra_passwd) == 0)) {
                                ans = PWA_VALID;
				audit_argv[5] = "valid";
			}
			else
				/* slow down password crackers */
				sleep(1);
                }
        }
 
	/* audit who is doing an authentication and what the answer is */
	switch(rqstp->rq_cred.oa_flavor) {
	case AUTH_UNIX:
		unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
		uid = unix_cred->aup_uid;
		break;
	case AUTH_NULL:
	default:
		svcerr_weakauth(transp);
		return;
	}
	audit_argv[1] = "group";
	sprintf(struid, "%d", uid);
	audit_argv[2] = struid;
	audit_argv[3] = name;
	audit_argv[4] = password;
	if (ans == PWA_INVALID) {
		audit_argv[5] = "invalid";
		audit_text(AU_ADMIN, 0, 1, 6, audit_argv);
	}
	else {
		audit_text(AU_ADMIN, 0, 0, 6, audit_argv);
	}

        if (! svc_sendreply(transp, xdr_int, &ans)) {
                fprintf(stderr, "pwdauthd: couldn't reply to RPC call\n");
        }
 
        if (! svc_freeargs(transp, xdr_pwdnm, &pwdname)) {
                fprintf(stderr, "pwdauthd: unable to free arguments\n");
                exit(1);
        }
}


/* Straight from rpc.yppasswdd - get a reserved socket */
rresvport()
{
	struct sockaddr_in sin;
	int s, alport = IPPORT_RESERVED - 1;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_DGRAM, 0, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin), 0) >= 0)
			return (s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
			perror("socket");
			return (-1);
		}
		(alport)--;
		if (alport == IPPORT_RESERVED/2) {
			fprintf(stderr, "socket: All ports in use\n");
			return (-1);
		}
	}
}

setauditid()
{
        int     audit_uid;              /* default audit uid */
        audit_state_t audit_state;      /* audit state */
        struct passwd *pw;              /* password file entry */
        struct passwd_adjunct *pwa;     /* adjunct file entry */

	if ((pw  = getpwnam(AUDIT_USER))  == (struct passwd *) NULL ||
            (pwa = getpwanam(AUDIT_USER)) == (struct passwd_adjunct *) NULL)
                        /* AUDIT_USER not found in adjunct file */
		return PPWA_INVALID;
       else {
                audit_uid = pw->pw_uid;
                if (getfauditflags(&pwa->pwa_au_always,
                   &pwa->pwa_au_never, &audit_state) != 0) {
                 /*
                  * if we can't tell how to audit from the flags, audit
                  * everything that's not never for this user.
                  */
                  audit_state.as_success = pwa->pwa_au_never.as_success
                  ^ (-1);
                  audit_state.as_failure = pwa->pwa_au_never.as_failure
                  ^ (-1);
                }
        }
        (void) setauid(audit_uid);
        (void) setaudit(&audit_state);
        return PPWA_VALID;
}

int notlocalhost(transp)
	SVCXPRT *transp;
{
	u_long *raddr;		  /* remote address */
        u_long *laddr;     	  /* local addr */
        struct hostent *hent;	  /* local host entry from /etc/hosts */
	char name[MAXHOSTNAMELEN];

        /* if remote host, do not provide service */ 
 
	raddr = (u_long *)&transp->xp_raddr.sin_addr;

	if (gethostname(name,sizeof(name)))
		return -1;
	hent = gethostbyname(name);
	if (hent == NULL)
		return -2;
	laddr = (u_long *)*hent->h_addr_list;

	if (*raddr == *laddr)
                /* local host */                       
                return(0);
        else
                /* not local host */
                return(1);
}
