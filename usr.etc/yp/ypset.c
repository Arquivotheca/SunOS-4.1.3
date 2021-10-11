#ifndef lint
static	char sccsid[] = "@(#)ypset.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This is a user command which issues a "Set domain binding" command to a 
 * NIS binder (ypbind) process
 *
 *	ypset [-V1] [-h <host>] [-d <domainname>] server_to_use
 *
 * where host and server_to_use may be either names or internet addresses.
 */
#include <stdio.h>
#include <sys/time.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */

bool oldvers = FALSE;
char *pusage;
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char default_host_name[256];
char *host = NULL;
struct in_addr host_addr;
char *server_to_use;
struct in_addr server_to_use_addr;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

char err_usage_set[] =
"Usage:\n\
	ypset [-V1] [-h <host>] [-d <domainname>] server_to_use\n\n\
where host and server_to_use may be either names or internet \n\
addresses of form ww.xx.yy.zz\n";
char err_cant_find_server_addr[] =
	"ypset: Sorry, I can't get an address for host %s from the NIS.\n";
char err_cant_decipher_server_addr[] =
     "ypset: Sorry, I got a garbage address for host %s back from the NIS.\n";
char err_bad_args[] =
	"ypset: Sorry, the %s argument is bad.\n";
char err_cant_get_kname[] =
	"ypset: Sorry, can't get %s back from system call.\n";
char err_null_kname[] =
	"ypset: Sorry, the %s hasn't been set on this machine.\n";
char err_bad_hostname[] = "hostname";
char err_bad_domainname[] = "domainname";
char err_bad_server[] = "server_to_use";
char err_cant_bind[] =
	"ypset: Sorry, I can't make use of the NIS.  I give  up.\n";
char err_udp_failure[] =
	"ypset: Sorry, I can't set up a udp connection to ypbind on host %s.\n";
char err_rpc_failure[] =
	"ypset: Sorry, I couldn't send my rpc message to ypbind on host %s.\n";
char err_access_failure[] =
	"ypset: Sorry, ypbind on host %s has rejected your request.\n";

void get_command_line_args();
void get_host_addr();
void send_message();

/*
 * Funny external reference to inet_addr to make the reference agree with the
 * code, not the documentation.  Should be:
 * extern struct in_addr inet_addr();
 * according to the documentation, but that's not what the code does.
 */
extern u_long inet_addr();

/*
 * This is the mainline for the ypset process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

void
main (argc, argv)
	int argc;
	char **argv;
	
{
	struct sockaddr_in myaddr;

	get_command_line_args(argc, argv);

	if (!domain) {
		
		if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
			domain = default_domain_name;
		} else {
			fprintf(stderr, err_cant_get_kname, err_bad_domainname);
			exit(1);
		}

		if (strlen(domain) == 0) {
			fprintf(stderr, err_null_kname, err_bad_domainname);
			exit(1);
		}
	}

	if (!host) {
		
		if (! gethostname(default_host_name, 256)) {
			host = default_host_name;
		} else {
			fprintf(stderr, err_cant_get_kname, err_bad_hostname);
			exit(1);
		}

		get_myaddress(&myaddr);
		host_addr = myaddr.sin_addr;
	} else  {
		get_host_addr(host, &host_addr);
	}

	get_host_addr(server_to_use, &server_to_use_addr);
	send_message();
	exit(0);
}

/*
 * This does the command line argument processing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
	pusage = err_usage_set;
	argv++;
	
	while (--argc > 1) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'V':

				if ((*argv)[2] == '1') {
					oldvers = TRUE;
					argv++;
					break;
				} else {
					fprintf(stderr, pusage);
					exit(1);
				}

			case 'h': {

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if (strlen(host) > 256) {
						fprintf(stderr, err_bad_args,
						    err_bad_hostname);
						exit(1);
					}
					
				} else {
					fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}
				
			case 'd': {

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						fprintf(stderr, err_bad_args,
						    err_bad_domainname);
						exit(1);
					}
					
				} else {
					fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}
				
			default: {
				fprintf(stderr, pusage);
				exit(1);
			}
			
			}
			
		} else {
			fprintf(stderr, pusage);
			exit(1);
		}
	}

	if (argc == 1) {
		
		if ( (*argv)[0] == '-') {
			fprintf(stderr, pusage);
			exit(1);
		}
		
		server_to_use = *argv;

		if (strlen(server_to_use) > 256) {
			fprintf(stderr, err_bad_args,
			    err_bad_server);
			exit(1);
		}

	} else {
		fprintf(stderr, pusage);
		exit(1);
	}
}

/*
 * This gets an address for the host to which the request will be directed,
 * or the host to be used as the NIS server.
 *
 * If the first character of the host string is a digit, it calls inet_addr(3)
 * to do the translation.  If that fails, it then tries to contact the NIS
 * (any server) and tries to get a match on the host name.  It then calls
 * inet_addr to turn the returned string (equivalent to that which is in
 * /etc/hosts) to an internet address.
 */
 
void
get_host_addr(name, addr)
	char *name;
	struct in_addr *addr;
{
	char *ascii_addr;
	int addr_len;
	struct in_addr tempaddr;

	if (isdigit(*name) ) {
		tempaddr.s_addr = inet_addr(name);

		if ((int) tempaddr.s_addr != -1) {
			*addr = tempaddr;
			return;
		}
	}
	
	if (!yp_bind(domain) ) {
		
		if (!yp_match (domain, "hosts.byname", name, strlen(name),
		    &ascii_addr, &addr_len) ) {
			tempaddr.s_addr = inet_addr(ascii_addr);

			if ((int) tempaddr.s_addr != -1) {
				*addr = tempaddr;
			} else {
				fprintf(stderr, err_cant_decipher_server_addr,
				    name);
				exit(1);
			}

		} else {
			fprintf(stderr, err_cant_find_server_addr, name);
			exit(1);
		}
		
	} else {
		fprintf(stderr, err_cant_bind);
		exit(1);
	}
}


/*
 * This takes the internet address of the NIS host of interest,
 * and fires off the "set domain binding" message to the ypbind process.
 */
 
void
send_message()
{
	struct dom_binding domb;
	struct ypbind_setdom req;
	struct ypbind_oldsetdom oldreq;
	enum clnt_stat clnt_stat;
	int vers;

	vers = oldvers ? YPBINDOLDVERS : YPBINDVERS;
	domb.dom_server_addr.sin_addr = host_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = 0;
	domb.dom_server_port = 0;
	domb.dom_socket = RPC_ANYSOCK;

	/*
	 * Open up a udp path to the server
	 */

	if ((domb.dom_client = clntudp_create(&(domb.dom_server_addr),
	    YPBINDPROG, vers, udp_intertry, &(domb.dom_socket))) == NULL) {
		fprintf(stderr, err_udp_failure, host);
		exit(1);
	}
	(domb.dom_client)->cl_auth = authunix_create_default();

	/*
	 * Load up the message structure and fire it off.
	 */
	if (oldvers) {
		strcpy(oldreq.ypoldsetdom_domain, domain);
		oldreq.ypoldsetdom_addr = server_to_use_addr;
		oldreq.ypoldsetdom_port = 0;
	
		clnt_stat = (enum clnt_stat) clnt_call(domb.dom_client,
		    YPBINDPROC_SETDOM, _xdr_ypbind_oldsetdom, &oldreq,
		    xdr_void, 0, udp_timeout);
		
	} else {
		strcpy(req.ypsetdom_domain, domain);
		req.ypsetdom_addr = server_to_use_addr;
		req.ypsetdom_port = 0;
		req.ypsetdom_vers = YPVERS;
	
		clnt_stat = (enum clnt_stat) clnt_call(domb.dom_client,
		    YPBINDPROC_SETDOM, xdr_ypbind_setdom, &req, xdr_void, 0,
		    udp_timeout);
	}
		auth_destroy((domb.dom_client)->cl_auth);
	/* 
	 * ypbind returns RPC_PROGUNAVAIL to indicate 
	 *	-ypset or -ypsetme is not set
	 */
	if (clnt_stat == RPC_SUCCESS)
		return;
	else if (clnt_stat == RPC_PROGUNAVAIL) 
		fprintf(stderr, err_access_failure, host);
	else
		fprintf(stderr, err_rpc_failure, host);
	exit(1);
}
