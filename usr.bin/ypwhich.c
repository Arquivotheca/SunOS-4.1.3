#ifndef lint
static  char sccsid[] = "@(#)ypwhich.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This is a user command which tells which NIS server is being used by a
 * given machine, or which NIS server is the master for a named map.
 * 
 * Usage is:
 * ypwhich [-d domain] [-m [mname] [-t] | [-V1 | -V2] host]
 * or
 * ypwhich -x
 * 
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  -m tells the master of that map.  mname may be either a
 * mapname, or a nickname which will be translated into a mapname according
 * to the translation table at transtable.  The  -t switch inhibits this
 * translation.  If the -m option is used, ypwhich will act like a vanilla
 * NIS client, and will not attempt to choose a particular NIS server.  On the
 * other hand, if no -m switch is used, ypwhich will talk directly to the NIS
 * bind process on the named host, or to the local ypbind process if no host
 * name is specified.  The -x switch can be used to dump the nickname
 * translation table.
 *  
 */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */

bool translate = TRUE;
bool dodump = FALSE;
bool newvers = FALSE;
bool oldvers = FALSE;
bool ask_specific = FALSE;
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char *host = NULL;
char default_host_name[256];
struct in_addr host_addr;
bool get_master = FALSE;
bool get_server = FALSE;
char *map = NULL;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byaddr",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases","mail.aliases",
	"ethers", "ethers.byname",
	NULL
};
char err_usage[] =
"Usage:\n\
	ypwhich [-d domain] [[-t] -m [mname] | [-V1 | -V2] host]\n\
	ypwhich -x\n";
char err_bad_args[] =
	"ypwhich:  %s argument is bad.\n";
char err_cant_get_kname[] =
	"ypwhich:  can't get %s back from system call.\n";
char err_null_kname[] =
	"ypwhich:  the %s hasn't been set on this machine.\n";
char err_bad_mapname[] = "mapname";
char err_bad_domainname[] = "domainname";
char err_bad_hostname[] = "hostname";
char err_first_failed[] =
	"ypwhich:  can't get first record from NIS.  Reason:  %s.\n";
char err_next_failed[] =
	"ypwhich:  can't get next record from NIS.  Reason:  %s.\n";

void get_command_line_args();
void getdomain();
void getlochost();
void getrmthost();
void get_server_name();
bool call_binder();
void print_server();
void get_map_master();
void dumptable();
void dump_ypmaps();
void v1dumpmaps();
void v2dumpmaps();


/*
 * This is the main line for the ypwhich process.
 */
main(argc, argv)
	char **argv;
{
	int addr;
	int err;
	struct ypbind_resp response;
	int i;

	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {
		getdomain();
	}
	
	if (get_server) {
		
		if (!host) {
			getlochost();
		} else {
			getrmthost();
		}

		get_server_name();
	} else {
		
		if (map) {
	
			if (translate) {
						
				for (i = 0; transtable[i]; i+=2) {
					    
					if (strcmp(map, transtable[i]) == 0) {
						map = transtable[i+1];
					}
				}
			}
			
			get_map_master();
		} else {
			dump_ypmaps();
		}
	}

	exit(0);
	/* NOTREACHED */
}

/*
 * This does the command line argument processing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
	argv++;

	if (argc == 1) {
		get_server = TRUE;
		return;
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 't': 
				translate = FALSE;
				argv++;
				break;

			case 'x': 
				dodump = TRUE;
				argv++;
				break;

			case 'V':

				if ((*argv)[2] == '1') {
					oldvers = TRUE;
					argv++;
					break;
				} else if ((*argv)[2] == '2') {
					newvers = TRUE;
					argv++;
					break;
				} else {
					fprintf(stderr, err_usage);
					exit(1);
				}

			case 'm': 
				get_master = TRUE;
				argv++;
				
				if (argc > 1) {
					
					if ( (*(argv))[0] == '-') {
						break;
					}
					
					argc--;
					map = *argv;
					argv++;

					if (strlen(map) > YPMAXMAP) {
						fprintf(stderr, err_bad_args,
						    err_bad_mapname);
						exit(1);
					}
					
				}
				
				break;
				
			case 'd': 

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
					fprintf(stderr, err_usage);
					exit(1);
				}
				
				break;
				
			default: 
				fprintf(stderr, err_usage);
				exit(1);
			}
			
		} else {
			
			if (get_server) {
				fprintf(stderr, err_usage);
				exit(1);
			}
		
			get_server = TRUE;
			host = *argv;
			argv++;
			
			if (strlen(host) > 256) {
				fprintf(stderr, err_bad_args, err_bad_hostname);
				exit(1);
			}
		}
	}

	if (newvers && oldvers) {
		fprintf(stderr, err_usage);
		exit(1);
	}
	
	if (newvers || oldvers) {
		ask_specific = TRUE;
	}
	
	if (get_master && get_server) {
		fprintf(stderr, err_usage);
		exit(1);
	}
	
	if (!get_master && !get_server) {
		get_server = TRUE;
	} 
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
void
getdomain()		
{
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

/*
 * This gets the local hostname back from the kernel, and comes up with an
 * address for the local node without using the NIS.  host_addr is set here.
 */
void
getlochost()
{
	struct sockaddr_in myaddr;

	if (! gethostname(default_host_name, 256)) {
		host = default_host_name;
	} else {
		fprintf(stderr, err_cant_get_kname, err_bad_hostname);
		exit(1);
	}

	get_myaddress(&myaddr);
	host_addr = myaddr.sin_addr;
}

/*
 * This gets an address for some named node by calling the standard library
 * routine gethostbyname.  host_addr is set here.
 */
void
getrmthost()
{		
	struct in_addr tempaddr;
	struct hostent *hp;
	
	if (isdigit(*host) ) {
		tempaddr.s_addr = inet_addr(host);

		if ((int) tempaddr.s_addr != -1) {
			host_addr = tempaddr;
			return;
		}
	}
	
	hp = gethostbyname(host);
		
	if (hp == NULL) {
	    	fprintf(stderr, "ypwhich: can't find %s\n", host);
		exit(1);
	}
	
	host_addr.s_addr = *(u_long *)hp->h_addr;
}

/*
 * This tries to find the name of the server to which the binder in question
 * is bound.  If one of the -Vx flags was specified, it will try only for
 * that protocol version, otherwise, it will start with the current version,
 * then drop back to the previous version.
 */
void
get_server_name()
{
	int vers;
	struct in_addr server;
	char *notbound = "Domain %s not bound.\n";
	
	if (ask_specific) {
		vers = oldvers ? YPBINDOLDVERS : YPBINDVERS;
		
		if (call_binder(vers, &server)) {
			print_server(&server);
		} else {
			fprintf(stderr, notbound, domain);
		}
		
	} else {
		vers = YPBINDVERS;
		
		if (call_binder(vers, &server)) {
			print_server(&server);
		} else {

			vers = YPBINDOLDVERS;
		
			if (call_binder(vers, &server)) {
				print_server(&server);
			} else {
				fprintf(stderr, notbound, domain);
			}
		}
	}
}

/*
 * This sends a message to the ypbind process on the node with address held
 * in host_addr.
 */
bool
call_binder(vers, server)
	int vers;
	struct in_addr *server;
{
	struct sockaddr_in query;
	CLIENT *client;
	int sock = RPC_ANYSOCK;
	enum clnt_stat rpc_stat;
	struct ypbind_resp response;
	char errstring[256];
	extern struct rpc_createerr rpc_createerr;

	query.sin_family = AF_INET;
	query.sin_port = 0;
	query.sin_addr = host_addr;
	bzero(query.sin_zero, 8);
	
	if ((client = clntudp_create(&query, YPBINDPROG, vers, udp_intertry,
	    &sock)) == NULL) {
		if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED) {
			(void) printf("ypwhich: %s is not running ypbind\n", 
				host);
			exit(1);
		}
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
		    (void) printf("ypwhich: %s is not running port mapper\n", 
				host);
			exit(1);
		}
		(void) clnt_pcreateerror("ypwhich:  clntudp_create error");
		exit(1);
	}

	rpc_stat = clnt_call(client, YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp, &response,
	    udp_timeout);
	    
	if ((rpc_stat != RPC_SUCCESS) &&
	    (rpc_stat != RPC_PROGVERSMISMATCH) ) {
		(void) sprintf(errstring,
		    "ypwhich: can't call ypbind on %s", host);
		(void) clnt_perror(client, errstring);
		exit(1);
	}

	*server = response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
	clnt_destroy(client);
	close(sock);
	
	if ((rpc_stat != RPC_SUCCESS) ||
	    (response.ypbind_status != YPBIND_SUCC_VAL) ) {
		return (FALSE);
	}
	
	return (TRUE);
}

/*
 * This translates a server address to a name and prints it.  If the address
 * is the same as the local address as returned by get_myaddress, the name
 * is that retrieved from the kernel.  If it's any other address (including
 * another ip address for the local machine), we'll get a name by using the
 * standard library routine (which calls the NIS).  
 */
void
print_server(server)
	struct in_addr *server;
{
	struct sockaddr_in myaddr;
	char myname[256];
	struct hostent *hp;
	
	get_myaddress(&myaddr);
	
	if (server->s_addr == myaddr.sin_addr.s_addr) {
		
		if (gethostname(myname, 256)) {
			fprintf(stderr, err_cant_get_kname, err_bad_hostname);
			exit(1);
		}

		printf(myname);
		printf("\n");
	} else {
		extern char *inet_ntoa();

		hp = gethostbyaddr(server, sizeof(server), AF_INET);
	
		printf("%s\n", hp ? hp->h_name : inet_ntoa(*server) );
	}
}

/*
 * This asks any NIS server for the map's master.  
 */
void
get_map_master()
{
	int err;
	char *master;
     
	err = yp_master(domain, map, &master);
	
	if (err) {
		fprintf(stderr,
		    "ypwhich:  Can't find the master of %s.  Reason: %s.\n",
		    map, yperr_string(err) );
	} else {
		printf("%s\n", master);
	}
}

/*
 * This will print out the map nickname translation table.
 */
void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		printf("Use \"%s\" for map \"%s\"\n", transtable[i],
		    transtable[i + 1]);
	}
}

/*
 * This enumerates the entries within map "ypmaps" in the domain at global 
 * "domain", and prints them out key and value per single line.  dump_ypmaps
 * just decides whether we are (probably) able to speak the new NIS protocol,
 * and dispatches to the appropriate function.
 */
void
dump_ypmaps()
{
	int err;
	struct dom_binding *binding;
	
	if (err = _yp_dobind(domain, &binding)) {
		fprintf(stderr,
		    "dump_ypmaps: Can't bind for domain %s.  Reason: %s\n",
		    domain, yperr_string(ypprot_err(err)));
		return;
	}

	if (binding->dom_vers == YPVERS) {
		v2dumpmaps(binding);
	} else {
		v1dumpmaps();
	}
}

void
v1dumpmaps()
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	char *scan;
	
	if (err = yp_first(domain, "ypmaps", &outkey, &outkeylen, &val,
	    &vallen) ) {

		if (err == YPERR_NOMORE) {
			exit(0);
		} else {
			fprintf(stderr, err_first_failed,
			    yperr_string(err) );
		}

		exit(1);
	}

	while (TRUE) {

		for (scan = outkey; *scan != NULL; scan++) {

			if (*scan == '\n') {
				*scan = ' ';  
			}
		}

		if (strlen(outkey) < 3 || strncmp(outkey, "YP_", 3) ) {
			printf(outkey);
			printf(val);
		}
		
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next(domain, "ypmaps", key, keylen, &outkey,
		    &outkeylen, &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				fprintf(stderr, err_next_failed,
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
}

void
v2dumpmaps(binding)
	struct dom_binding *binding;
{
	enum clnt_stat rpc_stat;
	int err;
	char *master;
	struct ypmaplist *pmpl;
	struct ypresp_maplist maplist;

	maplist.list = (struct ypmaplist *) NULL;
	
	rpc_stat = clnt_call(binding->dom_client, YPPROC_MAPLIST,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypresp_maplist, &maplist,
	    udp_timeout);

	if (rpc_stat != RPC_SUCCESS) {
		(void) clnt_perror(binding->dom_client,
		    "ypwhich(v2dumpmaps): can't get maplist");
		exit(1);
	}

	if (maplist.status != YP_TRUE) {
		fprintf(stderr,
		    "ypwhich:  Can't get maplist.  Reason:  %s.\n",
		    yperr_string(ypprot_err(maplist.status)) );
		exit(1);
	}

	for (pmpl = maplist.list; pmpl; pmpl = pmpl->ypml_next) {
		printf("%s ", pmpl->ypml_name);
		
		err = yp_master(domain, pmpl->ypml_name, &master);
	
		if (err) {
			printf("????????\n");
			fprintf(stderr,
		  "      ypwhich:  Can't find the master of %s.  Reason: %s.\n",
		   	    pmpl->ypml_name, yperr_string(err) );
		} else {
			printf("%s\n", master);
		}
	}
}

