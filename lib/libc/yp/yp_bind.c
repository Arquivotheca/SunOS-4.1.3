#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)yp_bind.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
#ifdef	sun
#define	MAP_STUFF 1
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/syslog.h>

#include "yp_prot.h"
#include "ypv1_prot.h"
#include "ypclnt.h"

#define	BFSIZE (YPMAXDOMAIN + 32) /* size of binding file */

/* This should match the one in ypbind.c */

#define	CACHE_DIR "/var/yp/binding"
extern int errno;
extern int sleep();
extern malloc_t malloc();
extern char *strcpy();
static void _yp_unbind();

enum	bind_status {
	BS_BAGIT,
	BS_RETRY,
	BS_OK
};

bool check_pmap_up();
bool check_binder_up();
enum	bind_status talk2_pmap();
enum	bind_status talk2_binder();
void talk2_server();
enum	bind_status get_binder_port();
bool check_binding();
void newborn();
struct	dom_binding *load_dom_binding();

/*
 * Time parameters when talking to the ypbind and pmap processes
 */

#define	YPSLEEPTIME 5			/* Time to sleep between tries */
unsigned int _ypsleeptime = YPSLEEPTIME;

#define	YPBIND_TIMEOUT 30		/* Total seconds for timeout */
#define	YPBIND_INTER_TRY 30		/* Seconds between tries */

static struct timeval bind_intertry = {
	YPBIND_INTER_TRY,		/* Seconds */
	0				/* Microseconds */
	};
static struct timeval bind_timeout = {
	YPBIND_TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

/*
 * Time parameters when talking to the ypserv process
 */

#ifdef	 DEBUG
#define	YPTIMEOUT 120			/* Total seconds for timeout */
#define	YPINTER_TRY 60			/* Seconds between tries */
#else
#define	YPTIMEOUT 90			/* Total seconds for timeout */
#define	YPINTER_TRY 5			/* Seconds between tries */
#endif

#define	MAX_TRIES_FOR_NEW_YP 1		/* Number of times we'll try to get
					 *   a new YP server before we'll
					 *   settle for an old one. */
static struct timeval ypserv_intertry = {
	YPINTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct	timeval _ypserv_timeout = {
	YPTIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

static struct in_addr my_addr;		/* Local internet addr */
static struct dom_binding *bound_domains; /* List of bound domains */
static char *default_domain;
static char	bfinvalid;		/* Binding file invalid	*/
static char     *bfname;
#ifdef	MAP_STUFF
#include <sys/mman.h>
static u_short	*binderfilemap;		/* Mapped version of binder */
#define	MAP_LEN (2+sizeof(struct ypbind_resp))			/* just for current udp only*/
#endif
static int	bf;			/* Binding file fd	*/
/*
 * binder_port holds what we believe to be the local port for ypbind.  It is
 * set only by talk2_pmap.  It is cleared (set to 0) by:
 *	1. talk2_pmap: always upon entry.
 *	2. check_binder_up if:
 *		- It can't create a socket to speak to the binder.
 *		- If it fails to bind to the port.
 *	3. talk2_binder if there are RPC errors when trying to use the port.
 */
static unsigned long binder_port;	/* Initialize to "no port" */

/*
 * Attempts to locate a NIS server that serves a passed domain.  If
 * one is found, an entry is created on the static list of domain-server pairs
 * pointed to by cell bound_domains, a udp path to the server is created and
 * the function returns 0.  Otherwise, the function returns a defined errorcode
 * YPERR_xxxx.
 */
int
_yp_dobind(domain, binding)
	char *domain;
	struct dom_binding **binding;	/* if result == 0, ptr to dom_binding */
{
return (_yp_dobind_soft(domain,binding, -1));
}
int
_yp_dobind_soft(domain, binding,usertries)
	char *domain;
	struct dom_binding **binding;	/* if result == 0, ptr to dom_binding */
	int usertries;
{
	struct dom_binding *pdomb;	/* Ptr to new domain binding */
	struct sockaddr_in ypbinder;	/* To talk with ypbinder */
	char *pdomain;			/* For xdr interface */
	struct ypbind_resp ypbind_resp; /* Response from local ypbinder */
	int vers;			/* ypbind program version number */
	int tries;			/* Number of times we've tried with
					 *  the current protocol */
	int gbtries;			/* Number of times we've tried to
					 *  get the binder */
	int status;
	u_short		b_port;
	enum bind_status loopctl;
	bool bound;
	bool oldport = FALSE;
#ifdef	MAP_STUFF
	char tmpbfname[BFSIZE];
#endif

	if ( (domain == NULL) ||(strlen(domain) == 0) ) {
		return (YPERR_BADARGS);
	}

	newborn();
	if (bfname==0){
	bfname=(char *)calloc(1,BFSIZE);
	}
	if (bfname==0){
	bfinvalid=TRUE;
	}

	if (!bfinvalid) {

#ifdef	MAP_STUFF
		sprintf(tmpbfname,"%s/%s.%d",CACHE_DIR,domain,YPVERS);
		if (strcmp(bfname,tmpbfname)==0){ /*same name*/
			if (binderfilemap){
			binder_port =  binderfilemap[0];
			}
		} else {
			bfname[0]=0;
			if (binderfilemap){
			munmap(binderfilemap,MAP_LEN);
			binderfilemap=0;
			}
			if ((bf = open(tmpbfname,O_RDONLY)) != -1) {
			if (flock(bf,LOCK_EX+LOCK_NB) == 0) {
				close(bf);
				bf = -1; /* Not valid if we would lock it */
			} else {
			binderfilemap=(u_short *)mmap(bfname,MAP_LEN,PROT_READ,MAP_SHARED,bf,0);
			if ((int)binderfilemap == -1) binderfilemap=0;
			if (binderfilemap){
				strcpy(bfname,tmpbfname);
				binder_port =  binderfilemap[0];
				close(bf);
				}
			else   {
				close(bf);
				bf= -1; /*map failed*/
				}
			   }
			}
		}
#else
		sprintf(bfname,"%s/%s.%d",CACHE_DIR,domain,YPVERS);
		if ((bf = open(bfname,O_RDONLY)) != -1) {
			if (flock(bf,LOCK_EX+LOCK_NB) == 0) {
				close(bf);
				bf = -1; /* Not valid if we would lock it */
			} else {
				read(bf,&b_port,sizeof(u_short));
				binder_port = b_port;
				close(bf);
			}
		}
#endif
	}

	if (check_binding(domain, binding) )
		return (0);		/* We are bound */

	/*
	 * Use loopback address.
	 */
	my_addr.s_addr = INADDR_LOOPBACK;

	pdomain = domain;

	/*
	 * Try to get the binder's port, using the current program version.
	 * The version may be changed to the old version, deep in the bowels
	 * of talk2_binder.
	 */
	for (bound = FALSE, vers = YPBINDVERS; !bound; ) {

		if (binder_port) {
			oldport = TRUE;
		} else {
			oldport = FALSE;

			/*
			 * Get the binder's port.  We'll loop as long as
			 * get_binder_port returns BS_RETRY.
			 */
			for (gbtries=0,loopctl = BS_RETRY; loopctl != BS_OK;gbtries++ ) {

				if ((usertries>=0) && gbtries >=usertries) return (status);
	 			switch (loopctl =
				    get_binder_port(vers, &status) ) {
				case BS_BAGIT:
					return (status);
				case BS_OK:
					break;
				}
			}
		}

		/*
		 * See whether ypbind is up.  If no, bag it if it's a
		 * resource error, or if we are using a port we just got
		 * from the port mapper.  Otherwise loop around to try to
		 * get a valid port.
		 */
		if (!check_binder_up(&ypbinder, &status)) {

			if (status == YPERR_RESRC) {
				return (status);
			}

			if (!oldport && status == YPERR_YPBIND) {
				return (status);
			}

			continue;
		}

		/*
		 * At this point, we think we know how to talk to the
		 * binder, and the binder is apparently alive.  Until we
		 * succeed in binding the domain, or we know we can't ever
		 * bind the domain, we will try forever.  This loops when
		 * talk2_binder returns BS_RETRY, and terminates when
		 * talk2_binder returns BS_BAGIT, or BS_OK.  If binder_port
		 * gets cleared, we will not execute this loop again, but
		 * will go to the top of the enclosing loop to try to get
		 * the binder's port again.  It is never the case that both
		 * talk2_binder returns  BS_OK and that it clears the
		 * binder_port.
		 */
		for (loopctl = BS_RETRY, tries = 1;
		    binder_port && (loopctl != BS_OK); tries++) {

			switch (loopctl = talk2_binder(&ypbinder, &vers,
			    tries, &pdomain, &ypbind_resp, &status) ) {
			case BS_BAGIT:
				return (status);
			case BS_OK:
				bound = TRUE;
			}
			if ((usertries>=0) && tries >=usertries) return (status);
		}
	}

	if ( (pdomb = load_dom_binding(&ypbind_resp, vers, domain, &status) ) ==
	    (struct dom_binding *) NULL) {
		return (status);
	}

	if (vers == YPBINDOLDVERS) {
		talk2_server(pdomb);
	}

	*binding = pdomb;			/* Return ptr to the binding
						 *   entry */
	return (0);				/* This is the go path */
}

/*
 * This is a "wrapper" function for _yp_dobind_soft for vanilla user-level
 * functions which neither know nor care about struct dom_bindings.
 */
int
yp_bind(domain)
	char *domain;
{

	struct dom_binding *binding;

	return (_yp_dobind(domain, &binding) );
}


/*
 * This is a "wrapper" function for _yp_dobind_soft for vanilla user-level
 * functions which neither know nor care about struct dom_bindings.
 */
int
yp_softbind(domain,tries)
	char *domain;
	int tries;
{

	struct dom_binding *binding;

	return (_yp_dobind_soft(domain, &binding, tries) );
}

/*
 * Attempts to find a dom_binding in the list at bound_domains having the
 * domain name field equal to the passed domain name, and removes it if found.
 * The domain-server binding will not exist after the call to this function.
 * All resources associated with the binding will be freed.
 */
void
yp_unbind (domain)
	char *domain;
{
_yp_unbind(domain, TRUE);
}

static void
_yp_unbind (domain, invalidate)
	char *domain;
	bool_t invalidate; /*the binding file*/
{
	struct dom_binding *pdomb;
	struct dom_binding *ptrail = 0;
	struct sockaddr_in local_name;
	int local_name_len;


	if ( (domain == NULL) ||(strlen(domain) == 0) ) {
		return;
	}
	if (invalidate) bfinvalid = TRUE; /* Don't use binding file again */

	for (pdomb = bound_domains; pdomb != NULL;
	    ptrail = pdomb, pdomb = pdomb->dom_pnext) {

		if (strcmp(domain, pdomb->dom_domain) == 0) {
			/*
			 * The act of destroying the rpc client closes
			 * the associated socket.  However, the fd
			 * representing what we believe to be our socket may
			 * really belong to someone else due to closes
			 * followed by new opens.   So let us be very careful!
			 */
			local_name_len = sizeof (local_name);
			if ((getsockname(pdomb->dom_socket,
			    (struct sockaddr *) &local_name,
			    &local_name_len) != 0)  ||
			    (local_name.sin_family != AF_INET) ||
			    (local_name.sin_port != pdomb->dom_local_port) ) {
				int tmp, tobesaved;

				tmp = dup(tobesaved = pdomb->dom_socket);
				clnt_destroy(pdomb->dom_client);
				tobesaved = dup2(tmp, tobesaved);
				(void) close(tmp);
			} else {
				clnt_destroy(pdomb->dom_client);
				(void) close(pdomb->dom_socket);
			}

			if (pdomb == bound_domains) {
				bound_domains = pdomb->dom_pnext;
			} else {
				ptrail->dom_pnext = pdomb->dom_pnext;
			}

			free((char *) pdomb);
			break;
		}

	}


}

static char *
_default_domain()
{
	char temp[256];

	if (default_domain)
		return (default_domain);
	if (getdomainname(temp, sizeof(temp)))
		return (0);
	if (strlen(temp) > 0) {
		default_domain = (char *)malloc(strlen(temp)+1);
		if (default_domain == 0)
			return (0);
		strcpy(default_domain, temp);
		return (default_domain);
	}
	return (0);
}

/*
 * This is a wrapper for the system call getdomainname which returns a
 * ypclnt.h error code in the failure case.  It also checks to see that
 * the domain name is non-null, knowing that the null string is going to
 * get rejected elsewhere in the NIS client package.
 */
int
yp_get_default_domain(domain)
	char **domain;
{

	if ((*domain = _default_domain()) != 0)
		return (0);
	return (YPERR_YPERR);
}

/*
 * This checks to see if this is a new process incarnation which has
 * inherited bindings from a parent, and unbinds the world if so.
 */
static void
newborn()
{
	static long int mypid;	/* Cached to detect forks */
	long int testpid;

	if ((testpid = getpid() ) != mypid) {
		mypid = testpid;

		while (bound_domains) {
			_yp_unbind(bound_domains->dom_domain, FALSE);
			/*do not unmap the file*/
		}
	}
}

/*
 * This checks that the socket for a domain which has already been bound
 * hasn't been closed or changed under us.  If it has, unbind the domain
 * without closing the socket, which may be in use by some higher level
 * code.  This returns TRUE and points the binding parameter at the found
 * dom_binding if the binding is found and the socket looks OK, and FALSE
 * otherwise.
 */
static bool
check_binding(domain, binding)
	char *domain;
	struct dom_binding **binding;
{
	struct dom_binding *pdomb;
	struct sockaddr_in local_name;
	int local_name_len;

	for (pdomb = bound_domains; pdomb != NULL; pdomb = pdomb->dom_pnext) {

		if (strcmp(domain, pdomb->dom_domain) == 0) {

			local_name_len = sizeof(local_name);
			if ((getsockname(pdomb->dom_socket,
			    (struct sockaddr *) &local_name,
			    &local_name_len) != 0)  ||
			    (local_name.sin_family != AF_INET) ||
			    (local_name.sin_port != pdomb->dom_local_port) ) {
				yp_unbind(domain);
				break;
			} else {
				*binding = pdomb;
				return (TRUE);
			}
		}
	}

	return (FALSE);
}

/*
 * This checks whether the portmapper is up.  If the connect fails, the
 * portmapper is dead.  As a side effect, the pmapper sockaddr_in is
 * initialized. The connection is not used for anything else, so it is
 * immediately closed.
 *
 * If there is a valid binding file we assume pmap is up.
 */
static bool
check_pmap_up(pmapper, err)
	struct sockaddr_in *pmapper;
	int *err;
{
	int sokt;
	int status;

	pmapper->sin_addr = my_addr;
	pmapper->sin_family = AF_INET;
	pmapper->sin_port = PMAPPORT;
	bzero(pmapper->sin_zero, 8);
	if (bf == -1) { /* No valid binding */
		sokt =  socket(AF_INET, SOCK_STREAM, 0);

		if (sokt == -1) {
			*err = YPERR_RESRC;
			return (FALSE);
		}

		if ((status = connect(sokt, (struct sockaddr *) pmapper,
		    sizeof(struct sockaddr_in))) < 0) {
			(void) close(sokt);
			*err = YPERR_PMAP;
			return (FALSE);
		}

		(void) close(sokt);
	}
	return (TRUE);
}

/*
 * This checks whether ypbind is up.  If the bind succeeds, ypbind is dead,
 * because the binder port returned from portmap should already be in use.
 * There are two side effects.  The ypbind sockaddr_in is initialized. If
 * the function returns FALSE, the global binder_port will be set to 0.
 */
static bool
check_binder_up(ypbinder, err)
	struct sockaddr_in *ypbinder;
	int *err;
{
	int sokt;
	int status;

	if (binder_port == 0) {
		return (FALSE);
	}

	ypbinder->sin_addr = my_addr;
	ypbinder->sin_family = AF_INET;
	ypbinder->sin_port = htons(binder_port);
	bzero(ypbinder->sin_zero, 8);
	sokt =  socket(AF_INET, SOCK_DGRAM, 0); /* Throw-away socket */

	if (sokt == -1) {
		binder_port = 0;
		*err = YPERR_RESRC;
		return (FALSE);
	}

	errno = 0;
	status = bind(sokt, (struct sockaddr *) ypbinder,
	    sizeof(struct sockaddr_in));
	(void) close(sokt);

	if (status == -1) {
		if (errno == EADDRINUSE) {
			/*
			 * since we cannot grab the port, ypbind must be
			 * up and healthy.
			 */
			return (TRUE);
		}
		if ((errno == EACCES) &&
		    (binder_port < IPPORT_RESERVED) &&
		    (geteuid() != 0)) {
			/*
			 * The port is priviledged.  This must be "secure"
			 * unix where have to use the yelow pages.
			 */
			return (TRUE);
		}
	}

	binder_port = 0;
	*err = YPERR_YPBIND;
	return (FALSE);
}

/*
 * This asks the portmapper for addressing info for ypbind speaking a passed
 * program version number.  If it gets that info, the port number is stashed
 * in binder_port, but binder_port will be set to 0 when talk2_pmap returns
 * anything except BS_OK.  If the RPC call to the portmapper failed, the
 * current process will be put to sleep for _ypsleeptime seconds before
 * this function returns.
 */
static enum bind_status
talk2_pmap(pmapper, vers, err)
	struct sockaddr_in *pmapper;
	int vers;
	int *err;
{
	int sokt;
	CLIENT *client;
	struct pmap portmap;
	enum clnt_stat clnt_stat;

	binder_port = 0;
	portmap.pm_prog = YPBINDPROG;
	portmap.pm_vers = vers;
	portmap.pm_prot = IPPROTO_UDP;
	portmap.pm_port = 0;		/* Don't care */

	sokt = RPC_ANYSOCK;

	if ((client = clntudp_bufcreate(pmapper, PMAPPROG, PMAPVERS,
	    bind_intertry, &sokt, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))  == NULL) {
		*err = YPERR_RPC;
		return (BS_BAGIT);
	}

	clnt_stat = (enum clnt_stat) clnt_call(client, PMAPPROC_GETPORT,
	    xdr_pmap, &portmap, xdr_u_long, &binder_port, bind_timeout);
	clnt_destroy(client);
	(void) close(sokt);

	if (clnt_stat  == RPC_SUCCESS) {

		if (binder_port != 0) {
			return (BS_OK);
		} else {
			*err = YPERR_YPBIND;
			return (BS_BAGIT);
		}

	} else {
		(void) sleep(_ypsleeptime);
		*err = YPERR_RPC;
		return (BS_RETRY);
	}
}

/*
 * This talks to the local ypbind process, and asks for a binding for the
 * passed domain.  As a side effect, if a version mismatch is detected, the
 * ypbind program version number may be changed to the old version.  In the
 * success case, the ypbind response will be returned as it was loaded by
 * ypbind - that is, containing a valid binding.  If the RPC call to ypbind
 * failed, the current process will be put to sleep for _ypsleeptime seconds
 * before this function returns.
 */
static enum bind_status
talk2_binder(ypbinder, vers, tries, ppdomain, ypbind_resp, err)
	struct sockaddr_in *ypbinder;
	int *vers;
	int tries;
	char **ppdomain;
	struct ypbind_resp *ypbind_resp;
	int *err;
{
	int sokt;
	CLIENT *client;
	enum clnt_stat clnt_stat;
	if (bfname==0) bfinvalid=TRUE;
	if ((bf != -1) && !bfinvalid) {
		/* this should work but we check anyway */
#ifdef	MAP_STUFF
			if (binderfilemap){
			bcopy( &binderfilemap[1],ypbind_resp,sizeof(struct ypbind_resp));
			clnt_stat = RPC_SUCCESS;
			}
	       else{
#endif
		bf = open(bfname,O_RDONLY);
		if (bf != -1) {
			if (flock(bf,LOCK_EX+LOCK_NB) != 0) {
				lseek(bf,(long)(sizeof(u_short)),L_SET);
	    			read(bf,ypbind_resp,sizeof(struct ypbind_resp));
		    		close(bf);
       		     		clnt_stat = RPC_SUCCESS;
			}
	  	}
#ifdef		MAP_STUFF
		}
#endif
	} else {

		sokt = RPC_ANYSOCK;

		if ((*vers == YPBINDVERS) && (tries > MAX_TRIES_FOR_NEW_YP) )
			*vers = YPBINDOLDVERS;

		if ((client = clntudp_bufcreate(ypbinder, YPBINDPROG, *vers,
		    bind_intertry, &sokt, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))  == NULL) {
			*err = YPERR_RPC;
			return (BS_BAGIT);
		}

		clnt_stat = (enum clnt_stat)clnt_call(client, YPBINDPROC_DOMAIN,
		    xdr_ypdomain_wrap_string, ppdomain, xdr_ypbind_resp,
		    ypbind_resp, bind_timeout);
		clnt_destroy(client);
		(void) close(sokt);
		bfinvalid = !(clnt_stat == RPC_SUCCESS);
	}

	if (clnt_stat == RPC_SUCCESS) {

		if (ypbind_resp->ypbind_status == YPBIND_SUCC_VAL) {
			/* Binding successfully returned from ypbind */
			return (BS_OK);
		} else {
			if ( ((*vers == YPBINDVERS) &&
			    (tries < MAX_TRIES_FOR_NEW_YP)) ||
			    (*vers == YPBINDOLDVERS) ) {
				(void) sleep(_ypsleeptime);
			}
			bfinvalid = TRUE;
			*err = YPERR_DOMAIN;
			return (BS_RETRY);
		}

	} else {

		bfinvalid = TRUE;
		if (clnt_stat == RPC_PROGVERSMISMATCH) {

			if (*vers == YPBINDOLDVERS) {
				*err = YPERR_YPBIND;
				return (BS_BAGIT);
			} else {
				*vers = YPBINDOLDVERS;
			}
		} else {
			(void) sleep(_ypsleeptime);
			binder_port = 0;
		}

		*err = YPERR_RPC;
		return (BS_RETRY);
	}
}

/*
 * This handles all the conversation with the portmapper to find the port
 * ypbind is listening on.  If binder_port is already non-zero, this returns
 * BS_OK immediately without changing anything.
 */
static enum bind_status
get_binder_port(vers, err)
	int vers;			/* !ypbind! program version number */
	int *err;
{
	struct sockaddr_in pmapper;

	if (binder_port) {
		return (BS_OK);
	}

	if (!check_pmap_up(&pmapper, err) ) {
		return (BS_BAGIT);
	}

	return (talk2_pmap(&pmapper, vers, err) );
}

/*
 * This allocates some memory for a domain binding, initialize it, and
 * returns a pointer to it.  Based on the program version we ended up
 * talking to ypbind with, fill out an opvector of appropriate protocol
 * modules.
 */
static struct dom_binding *
load_dom_binding(ypbind_resp, vers, domain, err)
	struct ypbind_resp *ypbind_resp;
	int vers;
	char *domain;
	int *err;
{
	struct dom_binding *pdomb;
	struct sockaddr_in dummy;	/* To get a port bound to socket */
	struct sockaddr_in local_name;
	int local_name_len = sizeof(struct sockaddr_in);

	pdomb = (struct dom_binding *) NULL;

	if ((pdomb = (struct dom_binding *) malloc(sizeof(struct dom_binding)))
		== NULL) {
		(void) syslog(LOG_ERR, "load_dom_binding:  malloc failure.");
		*err = YPERR_RESRC;
		return (struct dom_binding *) (NULL);
	}

	pdomb->dom_server_addr.sin_addr =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
	pdomb->dom_server_addr.sin_family = AF_INET;
	pdomb->dom_server_addr.sin_port =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_port;
	bzero(pdomb->dom_server_addr.sin_zero, 8);
	pdomb->dom_server_port =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_port;
	pdomb->dom_socket = RPC_ANYSOCK;
	pdomb->dom_vers = (vers == YPBINDOLDVERS) ? YPOLDVERS : YPVERS;

	/*
	 * Open up a udp path to the server, which will remain active globally.
	 */
	if ((pdomb->dom_client = clntudp_bufcreate(&(pdomb->dom_server_addr),
	    YPPROG, ((vers == YPBINDVERS) ? YPVERS : YPOLDVERS) ,
	    ypserv_intertry, &(pdomb->dom_socket), RPCSMALLMSGSIZE, YPMSGSZ))
	    == NULL) {
		free((char *) pdomb);
		*err = YPERR_RPC;
		return (struct dom_binding *) (NULL);
	}

	/*
	 * Bind the socket to a bogus address so a port gets allocated for
	 * the socket, but so that sendto will still work.
	 */
	(void) fcntl(pdomb->dom_socket, F_SETFD, 1);
	bzero((char *)&dummy, sizeof (dummy));
	dummy.sin_family = AF_INET;
	(void)bind(pdomb->dom_socket, (struct sockaddr *)&dummy, sizeof(dummy));

	/*
	 * Remember the bound port number
	 */
	if (getsockname(pdomb->dom_socket, (struct sockaddr *) &local_name,
	    &local_name_len) == 0) {
		pdomb->dom_local_port = local_name.sin_port;
	} else {
		free((char *) pdomb);
		*err = YPERR_YPERR;
		return (struct dom_binding *) (NULL);
	}

	(void) strcpy(pdomb->dom_domain, domain);/* Remember the domain name */
	pdomb->dom_pnext = bound_domains;	/* Link this to the list as */
	bound_domains = pdomb;			/* ... the head entry */
	return (pdomb);
}

/*
 * This checks to see if a ypserv which we know speaks v1 NIS program number
 * also speaks v2 version.  This makes the assumption that the NIS service at
 * the node is supplied by a single process, and that RPC will deliver a
 * message for a different program version number than that which the server
 * regestered.
 */
static void
talk2_server(pdomb)
	struct dom_binding *pdomb;
{
	int sokt;
	CLIENT *client;
	enum clnt_stat clnt_stat;

	sokt = RPC_ANYSOCK;

	if ((client = clntudp_bufcreate(&(pdomb->dom_server_addr),
	    YPPROG, YPVERS, ypserv_intertry, &sokt, RPCSMALLMSGSIZE, YPMSGSZ))
	    == NULL) {
		return;
	}
	clnt_stat = (enum clnt_stat) clnt_call(client, YPBINDPROC_NULL,
	    xdr_void, 0, xdr_void, 0, _ypserv_timeout);

	if (clnt_stat == RPC_SUCCESS) {
		clnt_destroy(pdomb->dom_client);
		(void) close(pdomb->dom_socket);
		pdomb->dom_client = client;
		pdomb->dom_socket = sokt;
		pdomb->dom_vers = YPVERS;
	} else {
		clnt_destroy(client);
		(void) close(sokt);
	}
}

/*
 * Awful utility function that other libc routines no longer call to
 * see that are getting data from a NIS map.
 * using it tends to double the NIS traffic on the average
 * changed to not call yp_match any more.
 */

int
usingypmap(ddn, map)
	char **ddn;  /* the default domainname set by this routine */
	char *map;  /* the map we are interested in. */
{
	char *key = NULL, *outval = NULL;
	int keylen, outvallen, stat;

	if (_default_domain() == 0) return (FALSE);
	/* does the map exist ? */
	stat = yp_first(*ddn=default_domain, map, &key, &keylen, &outval, &outvallen);
	if (outval != NULL)
		free(outval);
	if (key != NULL)
		free(key);
	return (yp_ismapthere(stat));
}
/*
 * Handy utility function that other libc routines can call to
 * see that are getting data from a NIS map.
 * e.g. if a yp_match fails and the stat is checked by this
 * routine FALSE is a fatal error and you should stop using NIS
 * TRUE means that the map exists and you should return
 * the error.
 */

int
yp_ismapthere(stat)
	int stat;
{

	switch (stat) {

	case 0:  /* it actually succeeded! */
	case YPERR_KEY:  /* no such key in map */
	case YPERR_NOMORE:
	case YPERR_BUSY:
		return (TRUE);
	}
	return (FALSE);
}

