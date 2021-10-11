#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)grpauth.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>
#include <signal.h>
#include <grp.h>
#include <rpc/rpc.h>
#include <rpcsvc/pwdnm.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#define PWA_VALID 0
#define PWA_INVALID -1
#define PWA_UNKNOWN -2
 
grpauth(name, password)
	char *name;
	char *password;
{
	/* this routine authenticates a password for the named group.
	 * Check the adjunct file.  There are three results to expect:
	 * 1) group validates in the adjunct file, 2) the adjunct file
	 * exists but the group does not validate, 3) the adjunct file
	 * does not exist.  For case 1 we should return 0; for case
	 * 2 we should return -1; and for case 3 we should check
	 * /etc/group.
	 * NOTE - checking the adjunct file is a privilged operation,
	 * so we will need to rpc to another program to do it
	 */
    
	char hostname[256];
	int	sock = RPC_ANYSOCK;
	CLIENT	*clnt;
	struct timeval	pertry_timeout;
	struct timeval	total_timeout;
	struct sockaddr_in server_addr;
	struct hostent	*hp;
	enum clnt_stat	clnt_stat;
	struct group	gr;
	struct group	*grp;
	struct group	*getgrnam();
	int	answer;
	pwdnm	pwdname;


	/* get name and password into pwdauth structure */
	pwdname.name = malloc(strlen(name) + 1);
	strcpy(pwdname.name, name);
	pwdname.password = malloc(strlen(password) + 1);
	strcpy(pwdname.password, password);

	/*
	 * set up link to server with authentication and make sure that
	 * the server is good
	 */
	gethostname(hostname, sizeof(hostname));
	if ((hp = gethostbyname(hostname)) == NULL) {
		fprintf(stderr, "hostname is bad for this system.\n");
		return -1;
	}
	bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 0;
	pertry_timeout.tv_sec = 5;
	pertry_timeout.tv_usec = 0;
	if ((clnt = clntudp_create(&server_addr, PWDAUTH_PROG, PWDAUTH_VERS,
	    pertry_timeout, &sock)) == NULL) {
		clnt_pcreateerror("clntudp_create");
		return -1;
	}
	if (server_addr.sin_family != AF_INET || 
	    ntohs(server_addr.sin_port) >= IPPORT_RESERVED) {
		fprintf(stderr, 
		    "pwdauth daemon is not running on a privileged port\n");
		return -1;
	}
	clnt->cl_auth = authunix_create_default();

	/* Call the server */
	total_timeout.tv_sec = 25;
	total_timeout.tv_usec = 0;
	clnt_stat = clnt_call(clnt, GRPAUTHSRV, xdr_pwdnm, &pwdname,
	    xdr_int, &answer, total_timeout);
	if (clnt_stat != RPC_SUCCESS) {
		clnt_perror(clnt, "rpc");
		return -1;
	}

	free(pwdname.name);
	free(pwdname.password);

	/* process the results sent back, and if the adjunct file is not
	 * being used, check the group file
	 */
	if (answer == PWA_VALID)
		return 0;
	else if (answer == PWA_INVALID)
		return -1;
	else if (answer == PWA_UNKNOWN) {
		/* we need to check /etc/group */
		if ((grp = getgrnam(name)) == NULL)
			/* group is not in main password system */
			return -1;
		gr = *grp;
		if (gr.gr_passwd[0] == '#' && gr.gr_passwd[1] == '$') {
			/* this means that /etc/group has problems */
			fprintf(stderr, "grpauth: bad group entry for %s\n",
			    gr.gr_name);
			return -1;
		}
		if (strcmp(crypt(password, gr.gr_passwd), gr.gr_passwd) == 0)
			return 0;
		else
			return -1;
	}
	else
		fprintf(stderr, "grpauth: unexpected response from pwdauthd\n");
	return -1;
}
