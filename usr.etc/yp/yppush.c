#ifndef lint
static	char sccsid[] = "@(#)yppush.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif


#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include "ypdefs.h"

#define INTER_TRY 12			/* Seconds between tries */
#define PORTMAP_TIME 30			/* Seconds before decide its down */
#define TIMEOUT INTER_TRY*4		/* Total time for timeout */
#define CUR_PAR 4		        /* Total  parallal yppushs*/
#define MIN_GRACE    25                 /*select timeout and minimum grace*/
#define GRACE_PERIOD 60*6		/* Total seconds we'll wait for
					 *  responses from ypxfrs, yes 
					 * virginia yp map transfers can
				         * take a long time, we only worry
				         * if the slave crashes ... */
					 /*was 800*/

USE_YPDBPATH
char *pusage;
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char *map = NULL;
char *host = NULL;
bool verbose = FALSE;
bool noclear = FALSE;
bool hierarchical  = TRUE;
bool onlyhierarchical  = FALSE;
bool debug = FALSE;
bool callback_timeout = FALSE;		/* set when a callback times out */
int  grace_period = GRACE_PERIOD;
int             curpar = CUR_PAR;	/* should be set by otherstuff */
char ypmapname[1024];		/* Used to check for the map's existence */

struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
};
SVCXPRT *transport;
struct server {
	struct server *pnext;
	char name[YPMAXPEER];
	struct dom_binding domb;
	unsigned long xactid;
	unsigned short state;
	unsigned long status;
	bool oldvers;
	int  start_time;     /* time from which grace is measured*/
};
int	users_list = FALSE;
struct server *server_list = (struct server *) NULL;
struct server *active_list = (struct server *) NULL; 

/*  State values for server.state field */

#define SSTAT_INIT 0
#define SSTAT_CALLED 1
#define SSTAT_RESPONDED 2
#define SSTAT_PROGNOTREG 3
#define SSTAT_RPC 4
#define SSTAT_RSCRC 5
#define SSTAT_SYSTEM 6

char err_usage[] =
"Usage:\n\
	yppush [-d <domainname>] [-t seconds] [-p #paralleljobs] [-h host]... [-v] map\n";
char err_bad_args[] =
	"The %s argument is bad.\n";
char err_cant_get_kname[] =
	"Can't get %s from system call.\n";
char err_null_kname[] =
	"The %s hasn't been set on this machine.\n";
char err_bad_hostname[] = "hostname";
char err_bad_mapname[] = "mapname";
char err_bad_domainname[] = "domainname";
char err_cant_bind[] =
	"Can't find a NIS server for domain %s.  Reason:  %s.\n";
char err_cant_build_serverlist[] =
	"Can't build server list from map \"ypservers\".  Reason:  %s.\n";
/*
 * State_duple table.  All messages should take 1 arg - the node name.
 */
struct state_duple {
	int state;
	char *state_msg;
};
struct state_duple state_duples[] = {
	{SSTAT_INIT, "Internal error trying to talk to %s."},
	{SSTAT_CALLED, "%s has been called."},
	{SSTAT_RESPONDED, "%s (v1 ypserv) sent an old-style request."},
	{SSTAT_PROGNOTREG, "NIS server not registered at %s."},
	{SSTAT_RPC, "RPC error to %s:  "},
	{SSTAT_RSCRC, "Local resource allocation failure - can't talk to %s."},
	{SSTAT_SYSTEM, "System error talking to %s:  "},
	{0, (char *) NULL}
};
/*
 * Status_duple table.  No messages should require any args.
 */
struct status_duple {
	long status;
	char *status_msg;
};
struct status_duple status_duples[] = {
	{YPPUSH_SUCC, "Map successfully transferred."},
	{YPPUSH_AGE,
	    "Transfer not done:  master's version isn't newer."},
	{YPPUSH_NOMAP, "Failed - ypxfr there can't find a server for map."},
	{YPPUSH_NODOM, "Failed - domain isn't supported."},
	{YPPUSH_RSRC, "Failed - local resource allocation failure."},
	{YPPUSH_RPC, "Failed - ypxfr had an RPC failure"},
	{YPPUSH_MADDR, "Failed - ypxfr couldn't get the map master's address."},
	{YPPUSH_YPERR, "Failed - NIS server or map format error."},
	{YPPUSH_BADARGS, "Failed - args to ypxfr were bad."},
	{YPPUSH_DBM, "Failed - dbm operation on map failed."},
	{YPPUSH_FILE, "Failed - file I/O operation on map failed"},
	{YPPUSH_SKEW, "Failed - map version skew during transfer."},
	{YPPUSH_CLEAR,
"Map successfully transferred, but ypxfr couldn't send \"Clear map\" to ypserv "},
	{YPPUSH_FORCE,
	    "Failed - no local order number in map - use -f flag to ypxfr."},
	{YPPUSH_XFRERR, "Failed - ypxfr internal error."},
	{YPPUSH_REFUSED, "Failed - Transfer request refused."},
	{0, (char *) NULL}
};
/*
 * rpcerr_duple table
 */
struct rpcerr_duple {
	enum clnt_stat rpc_stat;
	char *rpc_msg;
};
struct rpcerr_duple rpcerr_duples[] = {
	{RPC_SUCCESS, "RPC success"},
	{RPC_CANTENCODEARGS, "RPC Can't encode args"},
	{RPC_CANTDECODERES, "RPC Can't decode results"},
	{RPC_CANTSEND, "RPC Can't send"},
	{RPC_CANTRECV, "RPC Can't recv"},
	{RPC_TIMEDOUT, "NIS server registered, but does not respond"},
	{RPC_VERSMISMATCH, "RPC version mismatch"},
	{RPC_AUTHERROR, "RPC auth error"},
	{RPC_PROGUNAVAIL, "RPC remote program unavailable"},
	{RPC_PROGVERSMISMATCH, "RPC program mismatch"},
	{RPC_PROCUNAVAIL, "RPC unknown procedure"},
	{RPC_CANTDECODEARGS, "RPC Can't decode args"},
	{RPC_UNKNOWNHOST, "unknown host"},
	{RPC_PMAPFAILURE, "portmap failure (host is down?)"},
	{RPC_PROGNOTREGISTERED, "RPC prog not registered"},
	{RPC_SYSTEMERROR, "RPC system error"},
	{RPC_SUCCESS, (char *) NULL}		/* Duplicate rpc_stat unused
					         *  in list-end entry */
};

void get_default_domain_name();
void get_command_line_args();
unsigned short send_message();
void make_server_list();
void add_server();
void generate_callback();
void xactid_seed();
void main_loop();
void listener_exit();
void listener_dispatch();
bool read_server_state();
void print_state_msg();
void print_callback_msg();
void rpcerr_msg();
void get_xfr_response();
void set_time_up();

extern unsigned long inet_addr();
extern long errno;
extern struct rpc_createerr rpc_createerr;
extern unsigned sys_nerr;
extern char *sys_errlist[];

void
main (argc, argv)
	int argc;
	char **argv;
	
{
	unsigned long program;
	unsigned short port;
	struct	stat	sbuf;
	
	get_command_line_args(argc, argv);

	if (!domain) {
		get_default_domain_name();
	}
 
	
	if (debug==FALSE){
	/* check to see if the map exists in this domain */
	sprintf(ypmapname,"%s/%s/%s.dir",ypdbpath,domain,map);
	if (stat(ypmapname,&sbuf) < 0) {
		(void) fprintf(stderr,"yppush: Map does not exist.\n");
		exit(1);
	}
	}


	if (noclear == FALSE){
		yp_clear_master (domain, map); 
		}


	if (users_list==FALSE)
		make_server_list();
	
	/*
	 * All process exits after the call to generate_callback should be
	 * through listener_exit(program, status), not exit(status), so the
	 * transient server can get unregistered with the portmapper.
	 */

	generate_callback(&program, &port, &transport);
	
	main_loop(program, port);
	
	listener_exit(program, 0);
	/* NOTREACHED */
}

/*
 * This does the command line parsing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
	pusage = err_usage;
	argv++;

	if (argc < 2) {
		(void) fprintf(stderr, pusage);
		exit(1);
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'v':
				if (verbose) debug=TRUE;
				verbose = TRUE;
				argv++;
				break;
				
			case 'c':
				noclear = TRUE;
				argv++;
				break;
				
			case 'M':
				hierarchical = FALSE;
				argv++;
				break;

			case 'T':
				onlyhierarchical = TRUE;
				argv++;
				break;

			case 'd': {

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
					(void) fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}
				
			case 't': {

				if (argc > 1) {
					argv++;
					argc--;
					if (sscanf(*argv,"%d",&grace_period)==1)
						{}
					else {
					(void) fprintf(stderr, pusage);
					exit(1);
					}
					argv++;

					
				} else {
					(void) fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}

			case 'p': {

				if (argc > 1) {
					argv++;
					argc--;
					if (sscanf(*argv,"%d",&curpar)==1)
						{}
					else {
					(void) fprintf(stderr, pusage);
					exit(1);
					}
					argv++;
					if (curpar<1) curpar=1;

					
				} else {
					(void) fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}

			case 'h': {

				if (argc > 1) {
					argv++;
					argc--;
					add_server(*argv, strlen(*argv), server_list);
					users_list=TRUE;
					argv++;

					
				} else {
					(void) fprintf(stderr, pusage);
					exit(1);
				}
				
				break;
			}
			default: {
				(void) fprintf(stderr, pusage);
				exit(1);
			}
			
			}
			
		} else {

			if (!map) {
				map = *argv;
			} else {
				(void) fprintf(stderr, pusage);
				exit(1);
			}
			
			argv++;
			
		}
	}

	if (!map) {
		(void) fprintf(stderr, pusage);
		exit(1);
	}
}

/*
 *  This gets the local kernel domainname, and sets the global domain to it.
 */
void
get_default_domain_name()
{
	if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
		domain = default_domain_name;
	} else {
		(void) fprintf(stderr, err_cant_get_kname,
		    err_bad_domainname);
		exit(1);
	}

	if (strlen(domain) == 0) {
		(void) fprintf(stderr, err_null_kname, err_bad_domainname);
		exit(1);
	}
}

/*
 * This uses NIS operations to retrieve each server name in the map
 *  "ypservers".  add_server is called for each one to add it to the list of
 *  servers.
 */
void
make_server_list()
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	struct dom_binding *pdomb;
	struct dom_binding *mpdomb;
	int vallen;
	int err;
	char *ypservers;
	char *ypslave = "ypslaves.";
	char ypslaves[256+1+10];
	int count;
	int hlen;
	ypservers= "ypservers";
	strcpy(ypslaves,ypslave);
	hlen=256+1;
	gethostname(ypslaves+strlen(ypslave),hlen);

	if (verbose) {
	    printf("Finding NIS servers:", outkey);
	    fflush(stdout);
	    count = 4;
	}
	if (err = _yp_dobind(domain, &pdomb) ) {
		(void) fprintf(stderr, err_cant_bind, domain,
		    yperr_string(err) );
		exit(1);
	}
	if (err = _yp_dobind_master(domain,"hosts.byname", &mpdomb)){
		(void) fprintf(stderr, "cant bind to master for %s %s %s will use slave copy!\n", 
		domain,"hosts.byname" , yperr_string(err) );
	}else{
		/*XXX -- obscenity follows */
		/*rebind library to master for hosts.byname*/
		/*this is illegal and immoral and fattening*/
		/*but if NIS is used for hosts this make sure we use
		the latest copy in gethostbyname*/

		/*destroy libraries handle*/
		clnt_destroy(pdomb->dom_client);
		close(pdomb->dom_socket);

		pdomb->dom_client= mpdomb->dom_client;
		pdomb->dom_socket= mpdomb->dom_socket;
		pdomb->dom_server_addr = mpdomb->dom_server_addr;
		pdomb->dom_server_port = mpdomb->dom_server_port;/*XXX*/
		pdomb->dom_vers = mpdomb->dom_vers;	

		/* end XXX */
	}
	
	
	if (!hierarchical ||(err = yp_first_master(domain, ypslaves , &outkey, &outkeylen,
	    &val, &vallen) )) {
		if (verbose) (void) printf("Map %s %s -- using %s\n",ypslaves,yperr_string(err),ypservers);
		if (onlyhierarchical) {
			(void) fprintf(stderr, "Treepush %s failed %s\n", ypslaves, yperr_string(err) );
			exit(1);	
		}
		if (err = yp_first_master(domain, ypservers, &outkey, &outkeylen,
		    &val, &vallen) ) {
			(void) fprintf(stderr, err_cant_build_serverlist,
			     yperr_string(err) );
			exit(1);
		}
	}
	else {
		ypservers=ypslaves;
		if (verbose) printf("(hierachical push) ");
	}

	while (TRUE) {
		add_server(outkey, outkeylen, server_list);
		if (verbose) {
		    printf(" %s", outkey);
		    fflush(stdout);
		    if (count++ == 8) {
		    	printf("\n");
		        count = 0;
		    }
		}
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next_master(domain, ypservers, key, keylen,
		    &outkey, &outkeylen, &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				(void) fprintf(stderr,
				    err_cant_build_serverlist,
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
	if (count != 0) {
	    printf("\n");
	}
}

/*
 *  This adds a single server to the server list.  The servers name is
 *  translated to an IP address by calling gethostbyname(3n), which will
 *  probably make use of NIS services.
 */
void
add_server(name, namelen)
	char *name;
	int namelen;
{
	struct server *ps;
	struct hostent *h;
	static unsigned long seq;
	static unsigned long xactid = 0;

	if (xactid == 0) {
		xactid_seed(&xactid);
	}
	
	ps = (struct server *) malloc( (unsigned) sizeof (struct server));

	if (ps == (struct server *) NULL) {
		perror("yppush malloc failure");
		exit(1);
	}

	name[namelen] = '\0';
	(void) strcpy(ps->name, name);
	ps->state = SSTAT_INIT;
	ps->status = 0;
	ps->oldvers = FALSE;
	
	if (h = (struct hostent *) gethostbyname(name) ) {
		ps->domb.dom_server_addr.sin_addr =
		    *((struct in_addr *) h->h_addr);
		ps->domb.dom_server_addr.sin_family = AF_INET;
		ps->domb.dom_server_addr.sin_port = 0;
		ps->domb.dom_server_port = 0;
		ps->domb.dom_socket = RPC_ANYSOCK;
		ps->xactid = xactid + seq++;
		ps->pnext = server_list;
		server_list = ps;
	} else {
		(void) fprintf(stderr, "Can't get an address for server %s.\n",
		    name);
		free(ps);
	}
}

/*
 * This sets the base range for the transaction ids used in speaking the the
 *  server ypxfr processes.
 */
void
xactid_seed(xactid)
unsigned long *xactid;
{
	struct timeval t;

	if (gettimeofday(&t, (struct timezone *) NULL) == -1) {
		perror("yppush gettimeofday failure");
		*xactid = 1234567;
	} else {
		*xactid = t.tv_sec;
	}
}

/*
 *  This generates the udp channel which will be used as the listener process'
 *  service rendezvous point, and comes up with a transient program number
 *  for the use of the RPC messages from the ypxfr processes.
 */
void
generate_callback(program, port, transport)
	unsigned long *program;
	unsigned short *port;
	SVCXPRT **transport;
{
	struct sockaddr_in a;
	long unsigned prognum;
	SVCXPRT *xport;

	if ((xport = svcudp_create(RPC_ANYSOCK) ) == (SVCXPRT *) NULL) {
		(void) fprintf(stderr, "Can't set up as a udp server.\n");
		exit(1);
	}
	
	*port = xport->xp_port;
	*transport = xport;
	prognum = 0x40000000;

	while (!pmap_set(prognum++, YPPUSHVERS, IPPROTO_UDP, xport->xp_port) ) {
		;
	}

	*program = --prognum;
}


/*
 * This is the main loop. Send messages to each server,
 * and then wait for a response.
 */


add_to_active()
{
	struct server  *ps;
	ps = server_list;
	if (ps == NULL)
		return (0);
	server_list = server_list->pnext;	/* delete from server_list */
	ps->pnext = active_list;
	active_list = ps;
	return (1);
}

delete_active(in)
	struct server  *in;
{
	struct server  *p;
	struct server  *n;
	if (in == active_list) {
		active_list = active_list->pnext;
		return (1);
	}
	p = active_list;
	for (n = active_list; n; n = n->pnext) {
		if (in == n) {
			p->pnext = n->pnext;
			return (0);

		}
		p = n;
	}
	return (-1);
}

void
main_loop(program, port)
	unsigned long   program;
	unsigned short  port;
{
	int             readfds;
	register struct server *ps;
	long            error;
	int             hpar;	/* this times par count */
	int             i;
	int             j;
	int             time_now;
	int             docb;
	int             actives = 0;
	int             dead = 0;
	static struct timeval timeout = {MIN_GRACE, 0};


	if (grace_period<MIN_GRACE)  grace_period=MIN_GRACE;
	if (!svc_register(transport, program, YPPUSHVERS,
			  listener_dispatch, 0)) {
		(void) fprintf(stderr,
			       "Can't set up transient callback server.\n");
	}
	for (;;) {
		time_now = time(0);
		if (server_list == NULL) {
			actives = 0;
			dead = 0;
			for (ps = active_list; ps; ps = ps->pnext)
				if (ps->state == SSTAT_CALLED) {
					if ((time_now - ps->start_time) < grace_period)
						actives++;
					else
						dead++;

				}
			if (actives == 0) {
				if (debug) (void)printf("terminating %d dead\n", dead);

				for (ps = active_list; ps; ps = ps->pnext)
					if (ps->state == SSTAT_CALLED) {
						if ((time_now - ps->start_time) < grace_period) {
						} else {
						if (debug) (void) printf("no response from %s -- grace of %d seconds expired.\n", ps->name,grace_period);
						 (void) fprintf( stderr,"No response from ypxfr on %s\n", ps->name);
						 }

					}
				break;
			}
		}
		actives=0;
		for (ps = active_list; ps; ps = ps->pnext)
			if (ps->state == SSTAT_CALLED) {
				if ((time_now - ps->start_time) < grace_period) {
					actives++;

				if (debug)
					(void) fprintf(stderr, "No response yet from ypxfr on %s\n", ps->name);
				}
			} else {
				if (debug)
					(void) fprintf(stderr, "Deactivating  %s\n", ps->name);
				delete_active(ps);
			}

		/* add someone to the active list keep up with curpar */
		for (i = 0; i < (curpar - actives); i++) {
			if (add_to_active()) {
				ps = active_list;
				ps->state = send_message(ps, program, port, &error);
				print_state_msg(ps, error);
				if (ps->state != SSTAT_CALLED)
					delete_active(ps);	/* zorch it */
				else
					ps->start_time = time(0);	/* set start time */

			}
		}
		docb = 0;
		for (ps = active_list; ps; ps = ps->pnext)
			if (ps->state == SSTAT_CALLED) {
				docb = 1;
				break;
			}
		if (docb == 0) {
			if (debug)
			(void) fprintf(stderr, "No one to wait for this pass.\n");
			continue;	/* try curpar more */
		}
		readfds = svc_fds;
		errno = 0;
		switch ((int) select(32, &readfds, NULL, NULL, &timeout)) {

		case -1:
			if (errno != EINTR) {
				(void) perror("main loop select");
			}
			break;

		case 0:
			if (debug)
			(void) fprintf(stderr,
				       "timeout in main loop select.\n");
			break;

		default:
			svc_getreq(readfds);
			break;
		}		/* switch */
	}			/* for */

}
/*
 * This does the listener process cleanup and process exit.
 */
void
listener_exit(program, stat)
	unsigned long program;
	int stat;
{
	(void) pmap_unset(program, YPPUSHVERS);
	exit(stat);
}

/*
 * This is the listener process' RPC service dispatcher.
 */
void
listener_dispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {

	case YPPUSHPROC_NULL:
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			(void) fprintf(stderr,
			    "Can't reply to rpc call.\n");
		}

		break;

	case YPPUSHPROC_XFRRESP:
		get_xfr_response(rqstp, transp);
		break;
		
	default:
		svcerr_noproc(transp);
		break;
	}
}


/*
 *  This dumps a server state message to stdout.  It is called in cases where
 *  we have no expectation of receiving a callback from the remote ypxfr.
 */
void
print_state_msg(s, e)
	struct server *s;
	long e;
{
	struct state_duple *sd;

	if (s->state == SSTAT_SYSTEM)
		return;			/* already printed */
	if (!verbose && ( s->state == SSTAT_RESPONDED ||
			  s->state == SSTAT_CALLED) )
		return;
	
	for (sd = state_duples; sd->state_msg; sd++) {
		if (sd->state == s->state) {
			(void) printf(sd->state_msg, s->name);

			if (s->state == SSTAT_RPC) {
				rpcerr_msg((enum clnt_stat) e);
			}
			
			(void) printf("\n");
			fflush(stdout);
			return;
		}
	}

	(void) fprintf(stderr,
	  "yppush: Bad server state value %d.\n", s->state);
}

/*
 *  This dumps a transfer status message to stdout.  It is called in 
 *  response to a received RPC message from the called ypxfr.
 */
void
print_callback_msg(s)
	struct server *s;
{
	register struct status_duple *sd;

	if (!verbose && (s->status==YPPUSH_AGE) || (s->status==YPPUSH_SUCC))
		return;
	for (sd = status_duples; sd->status_msg; sd++) {

		if (sd->status == s->status) {
			(void) printf(
			    "Status received from ypxfr on %s:\n\t%s\n",
			    s->name, sd->status_msg);
			fflush(stdout);
			return;
		}
	}

	(void) fprintf(stderr,
	"yppush listener: Garbage transaction status (value %d) from ypxfr on %s.\n",
	    s->status, s->name);
}

/*
 *  This dumps an RPC error message to stdout.  This is basically a rewrite
 *  of clnt_perrno, but writes to stdout instead of stderr.
 */
void
rpcerr_msg(e)
	enum clnt_stat e;
{
	struct rpcerr_duple *rd;

	for (rd = rpcerr_duples; rd->rpc_msg; rd++) {

		if (rd->rpc_stat == e) {
			(void) printf(rd->rpc_msg);
			return;
		}
	}

	(void) fprintf(stderr,"Bad error code passed to rpcerr_msg: %d.\n",e);
}

/*
 * This picks up the response from the ypxfr process which has been started
 * up on the remote node.  The response status must be non-zero, otherwise
 * the status will be set to "ypxfr error".
 */
void
get_xfr_response(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yppushresp_xfr resp;
	register int i;
	register struct server *s;
	
	if (!svc_getargs(transp, xdr_yppushresp_xfr, &resp) ) {
		svcerr_decode(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		(void) fprintf(stderr, "Can't reply to rpc call.\n");
	}

	for( s =active_list;s;s=s->pnext){
		
		if (s->xactid == resp.transid) {
			s->status  = resp.status ? resp.status: YPPUSH_XFRERR;
			print_callback_msg(s);
			s->state = SSTAT_RESPONDED;
			return;
		}
	}

}

/*
 * This is a UNIX signal handler which is called when the
 * timer expires waiting for a callback.
 */
void
set_time_up()
{
	callback_timeout = TRUE;
}
#include <setjmp.h>
jmp_buf port_died;

send_time_up(){
longjmp(port_died,1);
}

/*
 * This sends a message to a single ypserv process.  The return value is
 * a state value.  If the RPC call fails because of a version
 * mismatch, we'll assume that we're talking to a version 1 ypserv process,
 * and will send him an old "YPPROC_GET" request, as was defined in the
 * earlier version of yp_prot.h
 */
unsigned short
send_message(ps, program, port, err)
	struct server *ps;
	unsigned long program;
	unsigned short port;
	long *err;
{
	struct ypreq_xfr req;
	struct yprequest oldreq;
	enum clnt_stat s;
	char my_name[YPMAXPEER +1];
	struct rpc_err rpcerr;

	signal(SIGALRM, send_time_up);
	alarm(PORTMAP_TIME);
	if (setjmp(port_died,0) != 0) {

			(void) printf("Timeout talking to %s: ",ps->name);
			(void) printf("\n");
			return(SSTAT_SYSTEM);
	}
	if ((ps->domb.dom_client = clntudp_create(&(ps->domb.dom_server_addr),
	    YPPROG, YPVERS, udp_intertry, &(ps->domb.dom_socket)))  == NULL) {
		alarm(0);

		if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED) {
			return(SSTAT_PROGNOTREG);
		} else {
			(void) printf("Error talking to %s: ",ps->name);
			rpcerr_msg(rpc_createerr.cf_stat);
			(void) printf("\n");
			fflush(stdout);
			return(SSTAT_SYSTEM);
		}
	}
	alarm(0); /*wait only so long*/

	if (gethostname(my_name, sizeof (my_name) ) == -1) {
		return(SSTAT_RSCRC);
	}

	req.ypxfr_domain = domain;
	req.ypxfr_map = map;
	req.ypxfr_ordernum = 0;
	req.ypxfr_owner = my_name;
	req.transid = ps->xactid;
	req.proto = program;
	req.port = port;
	s = (enum clnt_stat) clnt_call(ps->domb.dom_client, YPPROC_XFR,
	    xdr_ypreq_xfr, &req, xdr_void, 0, udp_timeout);
	clnt_geterr(ps->domb.dom_client, &rpcerr);
	clnt_destroy(ps->domb.dom_client);
	close(ps->domb.dom_socket);
	
	if (s == RPC_SUCCESS) {
		return (SSTAT_CALLED);
	} else {

		if (s == RPC_PROGVERSMISMATCH) {
			ps->domb.dom_server_addr.sin_family = AF_INET;
			ps->domb.dom_server_addr.sin_port = 0;
			ps->domb.dom_server_port = 0;
			ps->domb.dom_socket = RPC_ANYSOCK;
			
			if ((ps->domb.dom_client =
			    clntudp_create(&(ps->domb.dom_server_addr),
	    		    YPPROG, (YPVERS - 1), udp_intertry,
			    &(ps->domb.dom_socket)))  == NULL) {

				if (rpc_createerr.cf_stat ==
				    RPC_PROGNOTREGISTERED) {
					return(SSTAT_PROGNOTREG);
				} else {
			(void) printf("V1 Error talking to %s: ",
				ps->name);
					rpcerr_msg(rpc_createerr.cf_stat);
					(void) printf("\n");
					fflush(stdout);
					return(SSTAT_SYSTEM);
				}
			}
			
			oldreq.yp_reqtype = YPGET_REQTYPE;
			oldreq.ypget_req_domain = domain;
			oldreq.ypget_req_map = map;
			oldreq.ypget_req_ordernum = 0;
			oldreq.ypget_req_owner = my_name;
		
			s = (enum clnt_stat) clnt_call(
			    ps->domb.dom_client, YPOLDPROC_GET,
			    _xdr_yprequest, &oldreq, xdr_void, 0, udp_timeout);
			clnt_geterr(ps->domb.dom_client, &rpcerr);
			clnt_destroy(ps->domb.dom_client);
			close(ps->domb.dom_socket);
		}

		if (s == RPC_SUCCESS) {
			return (SSTAT_RESPONDED);
		} else {
			*err = (long) rpcerr.re_status;
			return (SSTAT_RPC);
		}
	}
}
