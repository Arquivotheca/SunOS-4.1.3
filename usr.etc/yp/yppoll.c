#ifndef lint
static	char sccsid[] = "@(#)yppoll.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This is a user command which asks a particular ypserv which version of a
 * map it is using.  Usage is:
 * 
 * yppoll [-h <host>] [-d <domainname>] mapname
 * 
 * where host may be either a name or an internet address of form ww.xx.yy.zz
 * 
 * If the host is ommitted, the local host will be used.  If host is specified
 * as an internet address, no NIS services need to be locally available.
 *  
 */
#include <stdio.h>
#include <sys/time.h>
#include <ctype.h>
#include <netdb.h>
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

int status = 0;				/* exit status */
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char *map = NULL;
char *host = NULL;
char default_host_name[256];
struct in_addr host_addr;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

char err_usage[] =
"Usage:\n\
	yppoll [-h <host>] [-d <domainname>] mapname\n\n\
where host may be either a name or an internet \n\
address of form ww.xx.yy.zz\n";
char err_bad_args[] =
	"Bad %s argument.\n";
char err_cant_get_kname[] =
	"Can't get %s back from system call.\n";
char err_null_kname[] =
	"%s hasn't been set on this machine.\n";
char err_bad_hostname[] = "hostname";
char err_bad_mapname[] = "mapname";
char err_bad_domainname[] = "domainname";
char err_bad_resp[] =
	"Ill-formed response returned from ypserv on host %s.\n";

void get_command_line_args();
void getdomain();
void getlochost();
void getrmthost();
void getmapparms();
void newresults();
void oldresults();

extern u_long inet_addr();

/*
 * This is the mainline for the yppoll process.
 */

void
main (argc, argv)
	int argc;
	char **argv;
	
{
	struct sockaddr_in myaddr;

	get_command_line_args(argc, argv);

	if (!domain) {
		getdomain();
	}
	
	if (!host) {
		getypserv();
	} else {
		getrmthost();
	}
	
	getmapparms();
	exit(status);
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
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'h': 

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if (strlen(host) > 256) {
						(void) fprintf(stderr,
						    err_bad_args,
						    err_bad_hostname);
						exit(1);
					}
					
				} else {
					(void) fprintf(stderr, err_usage);
					exit(1);
				}
				
				break;
				
			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						(void) fprintf(stderr,
						    err_bad_args,
						    err_bad_domainname);
						exit(1);
					}
					
				} else {
					(void) fprintf(stderr, err_usage);
					exit(1);
				}
				
				break;
				
			default: 
				(void) fprintf(stderr, err_usage);
				exit(1);
			
			}
			
		} else {
			if (!map) {
				map = *argv;

				if (strlen(map) > YPMAXMAP) {
					(void) fprintf(stderr, err_bad_args,
					    err_bad_mapname);
					exit(1);
				}

			} else {
				(void) fprintf(stderr, err_usage);
				exit(1);
			}
		}
	}

	if (!map) {
		(void) fprintf(stderr, err_usage);
		exit(1);
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

void
getmapparms()
{
	struct dom_binding domb;
	struct ypreq_nokey req;
	struct ypresp_master mresp;
	struct ypresp_order oresp;
	struct ypresp_master *mresults = (struct ypresp_master *) NULL;
	struct ypresp_order *oresults = (struct ypresp_order *) NULL;
	struct yprequest oldreq;
	struct ypresponse oldresp;
	enum clnt_stat s;

	domb.dom_server_addr.sin_addr = host_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = 0;
	domb.dom_server_port = 0;
	domb.dom_socket = RPC_ANYSOCK;
	req.domain = domain;
	req.map = map;
	mresp.master = NULL;

	if ((domb.dom_client = clntudp_create(&(domb.dom_server_addr),
	    YPPROG, YPVERS, udp_intertry, &(domb.dom_socket)))  == NULL) {
		(void) fprintf(stderr,
		    "Can't create UDP connection to %s.\n	", host);
		clnt_pcreateerror("Reason");
		exit(1);
	}

	s = (enum clnt_stat) clnt_call(domb.dom_client, YPPROC_MASTER,
	    xdr_ypreq_nokey, &req, xdr_ypresp_master, &mresp, udp_timeout);
	
	if(s == RPC_SUCCESS) {
		mresults = &mresp;
		s = (enum clnt_stat) clnt_call(domb.dom_client, YPPROC_ORDER,
	    	    xdr_ypreq_nokey, &req, xdr_ypresp_order, &oresp,
		    udp_timeout);

		if(s == RPC_SUCCESS) {
			oresults = &oresp;
			newresults(mresults, oresults);
		} else {
			(void) fprintf(stderr,
		"Can't make YPPROC_ORDER call to ypserv at %s.\n	",
			        host);
			clnt_perror(domb.dom_client, "Reason");
			exit(1);
		}
		
	} else {

		if (s == RPC_PROGVERSMISMATCH) {
			clnt_destroy(domb.dom_client);
			close(domb.dom_socket);
			domb.dom_server_addr.sin_port = 0;
			domb.dom_server_port = 0;
			domb.dom_socket = RPC_ANYSOCK;

			if ((domb.dom_client = clntudp_create(
			    &(domb.dom_server_addr), YPPROG, YPOLDVERS,
			    udp_intertry, &(domb.dom_socket)))  == NULL) {
				(void) fprintf(stderr,
		"Can't create V1 UDP connection to %s.\n	", host);
				clnt_pcreateerror("Reason");
				exit(1);
			}
			
			oldreq.yp_reqtype = YPPOLL_REQTYPE;
			oldreq.yppoll_req_domain = domain;
			oldreq.yppoll_req_map = map;
			oldresp.yppoll_resp_domain = NULL;
			oldresp.yppoll_resp_map = NULL;
			oldresp.yppoll_resp_ordernum = 0;
			oldresp.yppoll_resp_owner = NULL;

			s = (enum clnt_stat) clnt_call(domb.dom_client,
			    YPOLDPROC_POLL, _xdr_yprequest, &oldreq,
			    _xdr_ypresponse, &oldresp, udp_timeout);
			
			if(s == RPC_SUCCESS) {
				oldresults(&oldresp);
			} else {
				(void) fprintf(stderr,
			"Can't make YPPROC_POLL call to ypserv at %s.\n	",
			        host);
				clnt_perror(domb.dom_client, "yppoll");
				exit(1);
			}

		} else {
			(void) fprintf(stderr,
		"Can't make YPPROC_MASTER call to ypserv at %s.\n	",
			        host);
			clnt_perror(domb.dom_client, "Reason");
			exit(1);
		}
	}
}

void
newresults(m, o)
	struct ypresp_master *m;
	struct ypresp_order *o;
{
	char *s_domok = "Domain %s is supported.\n";
	char *s_ook = "Map %s has order number %d.\n";
	char *s_mok = "The master server is %s.\n";
	char *s_mbad = "Can't get master for map %s.\n	Reason:  %s\n";
	char *s_obad = "Can't get order number for map %s.\n	Reason:  %s\n";

	if (m->status == YP_TRUE	&& o->status == YP_TRUE) {
		(void) fprintf(stdout, s_domok, domain);
		(void) fprintf(stdout, s_ook, map, o->ordernum);
		(void) fprintf(stdout, s_mok, m->master);
	} else if (o->status == YP_TRUE)  {
		(void) fprintf(stdout, s_domok, domain);
		(void) fprintf(stdout, s_ook, map, o->ordernum);
		(void) fprintf(stderr, s_mbad, map,
		    yperr_string(ypprot_err(m->status)) );
		status = 1;
	} else if (m->status == YP_TRUE)  {
		(void) fprintf(stdout, s_domok, domain);
		(void) fprintf(stderr, s_obad, map,
		    yperr_string(ypprot_err(o->status)) );
		(void) fprintf(stdout, s_mok, m->master);
		status = 1;
	} else {
		(void) fprintf(stderr, "Can't get any map parameter information.\n");
		(void) fprintf(stderr, s_obad, map,
		    yperr_string(ypprot_err(o->status)) );
		(void) fprintf(stderr, s_mbad, map,
		    yperr_string(ypprot_err(m->status)) );
		status = 1;
	}
}

void
oldresults(resp)
	struct ypresponse *resp;
{
	if (resp->yp_resptype != YPPOLL_RESPTYPE) {
		(void) fprintf(stderr, err_bad_resp, host);
		status = 1;
	}

	if (!strcmp(resp->yppoll_resp_domain, domain) ) {
		(void) fprintf(stdout, "Domain %s is supported.\n", domain);

		if (!strcmp(resp->yppoll_resp_map, map) ) {

			if (resp->yppoll_resp_ordernum != 0) {
				(void) fprintf(stdout, "Map %s has order number %d.\n",
				    map, resp->yppoll_resp_ordernum);

				if (strcmp(resp->yppoll_resp_owner, "") ) {
					(void) fprintf(stdout,
					    "The master server is %s.\n",
					    resp->yppoll_resp_owner);
				} else {
					(void) fprintf(stderr,
					    "Unknown master server.\n");
					status = 1;
				}
				
			} else {
				(void) fprintf(stderr,
				    "Map %s is not supported.\n",
				    map);
				status = 1;
			}
			
		} else {
			(void) fprintf(stderr, 
			    "Map %s does not exist at %s.\n",
			    map, host);
			status = 1;
		}
		
	} else {
		(void) fprintf(stderr,
		    "Domain %s is not supported.\n", domain);
		status = 1;
	}
}

getypserv()
{
	int x;
	char host[256];
	struct in_addr addr;
	struct ypbind_resp response;

	if (gethostname(host, sizeof(host)) != 0) {
		fprintf(stderr, "can't get hostname\n");
		exit(1);
	}
	x = callrpc(host, YPBINDPROG, YPBINDVERS, YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp, &response);
	if (x)	{
		x = callrpc(host, YPBINDPROG, YPBINDOLDVERS, YPBINDPROC_DOMAIN,
		    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp,
		    &response);
		if (x) {
			clnt_perrno(x);
			fprintf(stderr, "\n");
			exit(1);
		}
	}
	if (response.ypbind_status != YPBIND_SUCC_VAL) {
		fprintf(stderr, "couldn't get yp server %d\n",
		    response.ypbind_status);
		exit(1);
	}
	host_addr=response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
}
