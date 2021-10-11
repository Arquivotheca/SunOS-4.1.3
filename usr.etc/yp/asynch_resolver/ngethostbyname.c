static  char sccsid[] = "@(#)ngethostbyname.c 1.1 92/07/30 1989";
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <syslog.h>		/* for LOG_CRIT */
#include "nres.h"

#ifndef NO_DATA
#define NO_DATA NO_ADDRESS
#endif


int	h_errno;	/*defined here*/

struct hostent *nres_getanswer();
static void     nres_abort_xmit();
static struct nres *nres_setup( /* name, done, userinfo */ );

nres_enabledebug()
{
	_res.options |= RES_DEBUG;

}

nres_disabledebug()
{
	_res.options &= (~RES_DEBUG);

}

/*these two routines return immediate errors in h_errno and null
if they fail or the return a struct nres*/

struct nres    *
nres_gethostbyname(name, handler, info)
	char           *name;
	char           *info;
	void            (*handler) ();
{
	struct nres    *temp, *nres_setup();
	char           *cp;


	/*
	 * disallow names consisting only of digits/dots, unless they end in
	 * a dot.
	 */
	if (isdigit(name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				h_errno = HOST_NOT_FOUND;
				return ((struct nres *) 0);
			}
			if (!isdigit(*cp) && *cp != '.')
				break;
		}


	temp = nres_setup(name, handler, info);

	if (temp != NULL) {
		temp->h_errno = TRY_AGAIN;
		if (nres_dosrch(temp) >= 0)
			return (temp);
		else {
			if (temp->udp_socket >= 0)
				(void) close(temp->udp_socket);
			if (temp->tcp_socket >= 0)
				(void) close(temp->tcp_socket);
			h_errno = temp->h_errno;
			free((char *) temp);
			return ((struct nres *) 0);

		}
	} else {
		IFRESDEBUG(void) printf("nres-gethostbyname:setup failed\n");
		return ((struct nres *) - 1);
	}

}


struct nres    *
nres_gethostbyaddr(addr, len, type, handler, info)
	char           *addr;
	int             len;
	int             type;
	char           *info;
	void            (*handler) ();
{
	struct nres    *temp, *nres_setup();
	char            qbuf[MAXDNAME];
	char           *sprintf() /* bsd */ ;
	if (type != AF_INET)
		return ((struct nres *) 0);
	(void) sprintf(qbuf, "%d.%d.%d.%d.in-addr.arpa.",
		       ((unsigned) addr[3] & 0xff),
		       ((unsigned) addr[2] & 0xff),
		       ((unsigned) addr[1] & 0xff),
		       ((unsigned) addr[0] & 0xff));
	temp = nres_setup(qbuf, handler, info);
	temp->reverse = REVERSE_PTR;
	bcopy((char *) addr, (char *) &(temp->theaddr.s_addr), 4);
	if (temp != NULL) {
		temp->h_errno = TRY_AGAIN;
		if (nres_dosrch(temp) >= 0)
			return (temp);
		else {
			if (temp->udp_socket >= 0)
				(void) close(temp->udp_socket);
			if (temp->tcp_socket >= 0)
				(void) close(temp->tcp_socket);
			h_errno = temp->h_errno;
			free((char *) temp);
			return ((struct nres *) 0);

		}
	} else {
		IFRESDEBUG(void) printf("nres-gethostbyaddr:setup failed\n");
		return ((struct nres *) - 1);
	}

}

/*
 * A timeout has occured -- try to retransmit, if it fails call abort_xmit to
 * decide to pursue the search or give up
 */

static void     nres_abort_xmits();
static void
nres_dotimeout(as)
	rpc_as         *as;
{

	struct nres    *temp;
	temp = (struct nres *) as->as_userptr;

	/*
	 * timeout
	 */
#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("timeout\n");
#endif
	temp->current_ns = temp->current_ns + 1;

	if (temp->using_tcp) {
		(void) close(temp->tcp_socket);
		temp->tcp_socket = -1;
		rpc_as_unregister(as); /*if you close it*/
	}

	if (nres_xmit(temp) < 0) {
		temp->h_errno	= TRY_AGAIN;
		rpc_as_unregister(as);
		nres_abort_xmit(temp);
	}
	else {

		if (temp->using_tcp) {
			if (nres_register(temp, temp->tcp_socket) < 0) {
				temp->h_errno	= TRY_AGAIN;
				nres_abort_xmit(temp);
			}
		} else {
			if (nres_register(temp, temp->udp_socket) < 0) {
				temp->h_errno	= TRY_AGAIN;
				nres_abort_xmit(temp);
			}
		}
	}
}

/* this advances the search with dosrch or gives up */
/* if it gives up it calls the users 'done' function */
/* this is the timeout way to call the user */
static void
nres_abort_xmit(temp)
	struct nres    *temp;
{
	/* called on timeout */
	int             give_up;
	IFRESDEBUG(void) printf("nres_abort()\n");
	give_up = 0;
	if (temp->search_index == 1) {
		give_up = 1;
		IFRESDEBUG(void) printf("Name server(s) seem to be down\n");
	} else if (nres_dosrch(temp) < 0)
		give_up = 1;
	if (give_up) {
		IFRESDEBUG(void) printf("more searching aborted -- would return try_again to caller.\n");
		temp->h_errno = TRY_AGAIN;
		if (temp->done)
			(temp->done) (temp, NULL, temp->userinfo, temp->h_errno);
		if (temp->udp_socket >= 0)
			(void) close(temp->udp_socket);
		if (temp->tcp_socket >= 0)
			(void) close(temp->tcp_socket);
		free((char *) temp);
	}
}

/*
 * try to pursue the search by calling nres_search and nres_xmit -- if both
 * work then register an asynch reply to come to nres_dorcv or a timeout to
 * nres_dotimeout
 */
/*
 * 0 means that the search is continuing -1 means that the search is not
 * continuing h_errno has the cause.
 */

static
nres_dosrch(temp)
	struct nres    *temp;
{
	int             type;

	if (nres_search(temp) >= 0) {
		if (temp->reverse == REVERSE_PTR)
			type = T_PTR;
		else
			type = T_A;
		IFRESDEBUG(void) printf("search \'%s\'\n", temp->search_name);
		temp->question_len = res_mkquery(QUERY, temp->search_name, C_IN, type, (char *) NULL, 0, NULL,
						 temp->question, MAXPACKET);
		if (temp->question_len < 0) {
			temp->h_errno = NO_RECOVERY;
			IFRESDEBUG(void) printf("res_mkquery --NO RECOVERY\n");
			return (-1);
		}
		if (nres_xmit(temp) < 0) {
			IFRESDEBUG(void) printf("nres_xmit() fails\n");
			temp->h_errno	= TRY_AGAIN;
			return (-1);
		} else {
			if (temp->using_tcp) {
				if (nres_register(temp, temp->tcp_socket) < 0) {
					temp->h_errno	= TRY_AGAIN;
					return (-1);
				}
			} else {
				if (nres_register(temp, temp->udp_socket) < 0) {
					temp->h_errno	= TRY_AGAIN;
					return (-1);
				}
			}
		}
		return (0);
	}
	return (-1);
}


/*
 * this processes an answer received asynchronously a nres_rcv is done to
 * pick up the packet, if it fails we just return, otherwise we unregister
 * the fd, check the reply. If the reply has an answer we call nres_getanswer
 * to get the answer, otherwise there is no answer an we call nres_dosrch to
 * press the search forward, if nres_dosrch works we return.  If the search
 * can not be continued or if we got the answer we call the users done
 * routine
 */

static void
nres_dorecv(as)
	struct rpc_as  *as;
{
	struct nres    *temp;
	struct nres    *again;
	struct hostent *theans;
	int status;
	struct in_addr **a;
	void            (*done) ();
	temp = (struct nres *) as->as_userptr;
	theans = NULL;
	errno = 0;
	status= nres_rcv(temp);
	if (status > 0) {
		IFRESDEBUG(void) printf("Recieved chars=%d\n", temp->answer_len);
		IFRESDEBUG      p_query(temp->answer);
	} else if (status  <0) {
		IFRESDEBUG(void) printf("nres_rcv() hard fails\n");
		rpc_as_unregister(as);	/*socket was closed for us */
		nres_dotimeout(as);	/*this may revive it*/
		return;		/* keep running */
	} else {
		IFRESDEBUG(void) printf("nres_rcv() soft fails\n");
		return;		/* keep running */
	}
	rpc_as_unregister(as);

	/* reply part */
	temp->answer_len = nres_chkreply(temp);

	for (;;) {
		if (temp->answer_len < 0) {
			if (errno == ECONNREFUSED) {
				temp->h_errno = TRY_AGAIN;
				break;
			}
			if (temp->h_errno == NO_DATA)
				temp->got_nodata++;
			if ((temp->h_errno != HOST_NOT_FOUND && temp->h_errno != NO_DATA) ||
			    (_res.options & RES_DNSRCH) == 0)
				break;

		} else {
			IFRESDEBUG(void) printf("nres_getanswer()\n");
			theans = nres_getanswer(temp);
			break;
		}
		IFRESDEBUG(void) printf("continuing search...\n");
		if (nres_dosrch(temp) < 0)
			break;
		return;		/* keep running  with new search */
	}
	IFRESDEBUG(void) printf("done with this case\n");
	/* answer resolution */
	if ((theans == NULL) && temp->got_nodata) {
		temp->h_errno = NO_DATA;
		IFRESDEBUG(void) printf("no_data\n");
	}


	done = temp->done;
	if (theans) {
		if (temp->reverse == REVERSE_PTR) {
			/* raise security */
			theans->h_addrtype = AF_INET;
			theans->h_length = 4;
			theans->h_addr_list[0] = (char *) &(temp->theaddr);
			theans->h_addr_list[1] = (char *) 0;
			if (temp->udp_socket >= 0)
				(void) close(temp->udp_socket);
			if (temp->tcp_socket >= 0)
				(void) close(temp->tcp_socket);
			again = nres_setup(theans->h_name, temp->done, temp->userinfo);
			if (again != NULL) {
				again->reverse = REVERSE_A;
				again->theaddr = temp->theaddr;
				again->h_errno = TRY_AGAIN;
				if (nres_dosrch(again) < 0) {
					if (done)
						(*done) (again, NULL, again->userinfo, again->h_errno);
					if (again->udp_socket >= 0)
						(void) close(again->udp_socket);
					if (again->tcp_socket >= 0)
						(void) close(again->tcp_socket);
					free((char *) again);
				}
			} else {
				/* memory error */
				temp->h_errno = TRY_AGAIN;
				if (done)
					(*done) (temp, NULL, temp->userinfo, temp->h_errno);

			}
			free((char *) temp);
			return;
		} else if (temp->reverse == REVERSE_A) {
			int found_addr = FALSE;
			for (a = (struct in_addr **) theans->h_addr_list; *a; a++)
				if (bcmp((char *) *a, (char *) &(temp->theaddr), theans->h_length) == 0) {
					if (done)
						(*done) (temp, theans, temp->userinfo, temp->h_errno);
					done = NULL;
					found_addr = TRUE;
				} 
			if (!found_addr) {  /* weve been spoofed */
				syslog(LOG_CRIT, "nres_gethostbyaddr: %s != %s", 
				       temp->name, inet_ntoa(temp->theaddr));
				theans = NULL;
				temp->h_errno = HOST_NOT_FOUND;
			}

		}
	}
	if (done)
		(*done) (temp, theans, temp->userinfo, temp->h_errno);
	if (temp->udp_socket >= 0)
		(void) close(temp->udp_socket);
	if (temp->tcp_socket >= 0)
		(void) close(temp->tcp_socket);
	free((char *) temp);
	return;			/* done running */

}

static
nres_register(a, b)
	int             b;
	struct nres    *a;
{
	a->nres_rpc_as.as_fd = b;
	a->nres_rpc_as.as_timeout_flag = TRUE;
	a->nres_rpc_as.as_timeout = nres_dotimeout;
	a->nres_rpc_as.as_recv = nres_dorecv;
	a->nres_rpc_as.as_userptr = (char *) a;
	return (rpc_as_register(&(a->nres_rpc_as)));
}

static struct nres *
nres_setup(name, done, userinfo)
	char           *name;
	char           *userinfo;
	void            (*done) ();
{
	char           *calloc();
	struct nres    *tmp;
	tmp = (struct nres *) calloc(1, sizeof(struct nres));
	if (tmp == NULL)
		return (tmp);
	strncpy(tmp->name, name, MAXDNAME);
	tmp->tcp_socket = -1;
	tmp->udp_socket = -1;
	tmp->done = done;
	tmp->userinfo = userinfo;
	return (tmp);
}

