/*      @(#)nsrec.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * nsrec.c contains name server recovery functions and
 * other related functions.
 */
#ident	"@(#)nserve:nsrec.c	1.21.1.1"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/stropts.h>
#include <errno.h>
#include <fcntl.h>
#include "nsdb.h"
#include "nslog.h"
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include "stdns.h"
#include <rfs/nserve.h>
#include <sys/mount.h>
#include <rfs/cirmgr.h>
#include <rfs/pn.h>
#include "nsports.h"
#include "nsrec.h"

#define NLS_Q		"nlsadmin -q"
#define NLS_ADDR 	"nlsadmin -l -"
#define UNADV_CMD 	"/usr/bin/unadv"
#define ADV_CMD		"/usr/bin/adv"
#define AUTH_INFO	NSDIRQ/auth.info"
#define MNT_CMD		"/etc/mount -d"
#define UMNT_CMD	"/etc/umount -d"
#define R_TIMEOUT	25

extern int	Done;
extern char	*Netmaster;
extern char	*Net_spec;
extern void	(*Alarmsig)();
int	Polltime=POLLTIME;
char	*Mydomains[MAXDOMAINS];
int	Mylastdom=0;


struct ns_info {
	int		 n_state;
	int		 n_pd;
	char		*n_dname;
	struct address	*n_addr;
};

static struct ns_info	Nsinfo[MAXNS];
static int		getprime();
static int		Rdone=FALSE;
char	*prec();
char	*dompart();
char	*aatos();
struct address	*getmyaddr();
struct address	*astoa();
int	r_poll();
int	r_checkup();
struct ns_info	*pdtonp();
struct ns_info	*nmtonp();

/*
 * setmaster takes the information returned from
 * the NS_INIT call to the primary and writes
 * a new netmaster file.
 */
int
setmaster(req)
struct request	*req;
{
	struct res_rec	*rns;
	struct res_rec	*rar;
	long	nscount;
	int	i;
	char	dname[BUFSIZ];
	char	*ptr;
	FILE	*fp;
	FILE	*fopen();

	LOG2(L_COMM,"(%5d) setmaster: enter\n",Logstamp);

	/*
	 * don't rewrite rfmaster if this machine is listed as "p"
	 * in rfmaster.
	 */

	strcpy(dname,dompart(Dname));
	if (domauth(dname) == SOA) {
		LOG2(L_COMM,"(%5d) setmaster: don't rewrite rfmaster\n",Logstamp);
		return(R_NSFAIL);
	}

	if (req->rq_head->h_rcode != R_NOERR)
		return(R_NSFAIL);

	LOG2(L_COMM,"(%5d) setmaster: rcode ok\n",Logstamp);

	if ((nscount = req->rq_head->h_nscnt) == 0 ||
	    req->rq_head->h_arcnt == 0)
		return(R_NSFAIL);

	LOG3(L_COMM,"(%5d) setmaster: record count = %d\n",Logstamp,
		req->rq_head->h_nscnt);

	if ((fp = fopen(Netmaster,"w")) == NULL) {
		PLOG2("rfs: ns_init can't write master file %s\n",Netmaster);
		return(R_NSFAIL);
	}
	for (i=0, rns = *req->rq_ns, rar = *req->rq_ar;
		i < nscount; i++, rns++, rar++) {
		fprintf(fp,"%s\n",prec(rns));
		fprintf(fp,"%s%c%s\n",dompart(rns->rr_ns),SEPARATOR,prec(rar));
	}
	fclose(fp);
	return(R_NOERR);
}
/*
 * mydom returns TRUE if name is in the list of domains
 * that this machine has authority over, otherwise it
 * returns FALSE.
 */
int
mydom(name)
char	*name;
{
	register int	i;

	for (i=0; i < Mylastdom; i++)
		if (strcmp(name,Mydomains[i]) == NULL)
			return(TRUE);
	return(FALSE);
}
/*
 * r_init initializes recovery.  If this machine is just
 * a normal machine, its state is set, and the function
 * returns, otherwise, the recovery mechanism is set in
 * motion.
 */
int
r_init()
{
	if (getprime(dompart(Dname)) == RFS_FAILURE)
		return(RFS_FAILURE);

	if (!Myaddress && (Myaddress = getmyaddr()) == NULL)
		return(RFS_FAILURE);

	if (findmydoms() == 0) { /* this is a "regular" machine	*/
		alarm(0);
		sigset(SIGALRM,SIG_IGN);
		getnsinfo();
		Primary = FALSE;
		return(RFS_SUCCESS);
	}

	/*
	 * This machine "owns" one or more domains.
	 * Now set up recovery circuits.
	 */

	if (getnsinfo() == RFS_FAILURE)
		return(RFS_FAILURE);

	if (setrstate() == RFS_FAILURE)
		return(RFS_FAILURE);

	return(RFS_SUCCESS);
}
/*
 * find primary machine address(es) from
 * database.
 */
static int
getprime(dom)
char	*dom;
{
	int	count;
	int	i,j;
	struct res_rec	**rlist, **arlist=NULL;
	struct res_rec	*srlist[MAXREC];
	struct res_rec	**srp = srlist;
	char	abuf[BUFSIZ];

	strcpy(abuf,dom);

	if ((rlist = findrr(abuf,NSTYPE)) == NULL ||
	    (count = copyrlist(&srp,rlist)) == 0 ||
	    getars(srlist,&arlist,count) == 0) {
		PLOG1("RFS name server can't find primary name server address\n");
		return(RFS_FAILURE);
	}

	for (i=0; i < MAXNS; i++)
		Paddress[i] = NULL;

	for (i=0; i < count; i++) {
		LOG3(L_OVER,"getprime: add primary %d, name = %s\n",
			(srlist[i]->rr_type & ~MASKNS)-1, srlist[i]->rr_ns);
		Paddress[(srlist[i]->rr_type & ~MASKNS)-1] = 
			astoa(arlist[i]->rr_a,NULL);
	}

	/* compress addresses in list to eliminate gaps	*/
	for (i=0, j=0; j < count && i < MAXNS; i++) {
		if (Paddress[i] == NULL)
			continue;
		if (i != j)
			Paddress[j++] = Paddress[i];
		else
			j++;
	}
	if (j == 0) {
		PLOG1("RFS name server can't find good primary ns address\n");
		return(RFS_FAILURE);
	}

	Paddress[count] = NULL;

	for (i=0; Paddress[i]; i++) {
		LOG3(L_OVER,"getprime: using addr=%s as primary %d\n",
			aatos(abuf,Paddress[i],KEEP | HEX),i);
	}

	freelist(rlist);
	freelist(arlist);
	return(RFS_SUCCESS);
}
/*
 * look for this machine's listen address using nlsadmin.
 */
struct address *
getmyaddr()
{
	struct address	*retaddr=NULL;
	FILE	*fp;
	FILE	*popen();
	char	buf[BUFSIZ];

	sprintf(buf,"%s -z %d %s >/dev/null 2>&1",NLS_Q,RFSD,Net_spec);

	switch (system(buf)) {
	case 0:
		LOG2(L_ALL,"getmyaddr: listener correctly set up for net_spec %s\n",
			Net_spec);
		break;
	case -1:
		PLOG4("RFS name server: %s -z %d %s : request failed\n",NLS_Q,RFSD,Net_spec);
		return(NULL);
	default:
		PLOG2("RFS name server: listener not properly set up for RFS on network %s\n",
			Net_spec);
		return(NULL);
	}

	sprintf(buf,"%s %s >/dev/null 2>&1",NLS_Q,Net_spec);

	switch (system(buf)) {
	case 0:
		LOG2(L_ALL,"getmyaddr: listener is running for net_spec %s\n",
			Net_spec);
		break;
	case -1:
		PLOG3("RFS name server: %s %s : request failed\n",NLS_Q,Net_spec);
		return(NULL);
	default:
		PLOG2("RFS name server: listener not currently active for network %s\n",
			Net_spec);
		return(NULL);
	}

	sprintf(buf,"%s %s",NLS_ADDR,Net_spec);
	if ((fp = popen(buf,"r")) == NULL) {
		PLOG2("RFS name server: %s failed\n",buf);
		return(NULL);
	}
	if (fgets(buf,BUFSIZ,fp) == NULL) {
		PLOG1("RFS name server: can't get listen address\n");
		pclose(fp);
	}

	if ((retaddr = astoa(buf,NULL)) == NULL) {
		PLOG2("RFS name server: got bad address from listener '%s'\n",buf);
	}
	else {
		LOG2(L_OVER,"getmyaddr: using addr=%s as my address\n",buf);
	}

	pclose(fp);
	return(retaddr);
}
/*
 * getnsinfo searches the database for the names of potential
 * primary name servers for the domains this machine serves.
 */
int
getnsinfo()
{
	struct res_rec	**rlist;
	struct res_rec	**arlist;
	struct res_rec	*srlist[MAXREC];
	struct res_rec	**srp = srlist;
	struct res_rec	*rec;
	char		*dom;
	struct ns_info	*nptr;
	int		i, j;
	long		indx;
	int		count;
	static int	first=TRUE;


	LOG2(L_COMM,"(%5d) getnsinfo: enter\n",Logstamp);
	/* first clear out Nsinfo	*/
	for (i=0, nptr = Nsinfo; i < MAXNS; i++, nptr++) {
		if (!first && nptr->n_pd != -1)
			nsrclose(nptr->n_pd);
		nptr->n_state = R_UNUSED;
		nptr->n_pd = -1;
		if (!first && nptr->n_dname)
			free(nptr->n_dname);	
		nptr->n_dname = NULL;
		if (!first && nptr->n_addr)
			free(nptr->n_addr);
		nptr->n_addr = NULL;
	}
	first = FALSE;

	for (i=0; i < Mylastdom; i++) {
		dom = Mydomains[i];
		if ((rlist = findrr(dom,NSTYPE)) == NULL)
			continue;

		count = copyrlist(&srp,rlist);

		if (getars(srlist, &arlist, count) == 0) {
			freelist(rlist);
			continue;
		}

		for (j=0, rec=srlist[0]; j < count; rec = srlist[++j]) {
			if ((indx = rec->rr_type & ~MASKNS) < 1 || indx > MAXNS)
				continue;
			indx--;
			nptr = &Nsinfo[indx];
			if (nptr->n_state == R_UNUSED) {
				nptr->n_state = R_NOTPRIME;
				nptr->n_dname = copystr(rec->rr_data);
				if ((nptr->n_addr = astoa(arlist[j]->rr_a,NULL))
					== NULL) {
					PLOG2("RFS name server: bad NS addr for %s\n",
						nptr->n_dname);
					freelist(rlist);
					return(RFS_FAILURE);
				}
				LOG4(L_COMM,"(%5d) getnsinfo: add %s to Nsinfo[%d]\n",
					Logstamp, nptr->n_dname,indx);
				continue;
			}
			if (strcmp(rec->rr_data,nptr->n_dname)) {
				PLOG3("RFS name server: NS setup conflict, %s & %s\n",
					rec->rr_data,nptr->n_dname);
				freelist(rlist);
				return(RFS_FAILURE);
			}
		}
		freelist(rlist);
	}
	return(RFS_SUCCESS);
}
/*
 * setrstate is called to find out who is primary, and to
 * set the state of this machine.
 */
int
setrstate()
{
	int	i;
	register struct ns_info	*np;
	struct ns_info	*pnp=NULL;
	struct ns_info	*npnp=NULL;
	struct ns_info	*Mynp=NULL;
	void	(*usr2sig)();
	char	*block=NULL;
	int	size=0;
	int	state;
	long	code;
	int	timesave;
	void	rst_alrm();
	int	ret;
	int	pd = -1;
	struct request	*req=NULL;
	int	savetime = R_TIMEOUT;

	LOG4(L_ALL,"(%5d) recovery: enter setrstate, Primary=%s, Recover=%s\n",
		Logstamp, (Primary)?"TRUE":"FALSE", (Recover)?"TRUE":"FALSE");

	if (!Recover)
		return(R_NOTPRIME);

	/* stop polling until after state is set	*/
	usr2sig = sigset(SIGUSR2,SIG_HOLD);
	sigset(SIGALRM,SIG_IGN);
	timesave = alarm(0);

	Primary = FALSE;

	for (i=0, np=Nsinfo; i < MAXNS; i++, np++) {
		if (np->n_state == R_UNUSED)
			continue;
		if (strcmp(Dname,np->n_dname) == NULL) {
			Mynp = np;
			Mynp->n_state = R_NOTPRIME;
			continue;
		}

		LOG4(L_COMM,"(%5d) recover: polling %s, current state = %d\n",
			Logstamp, np->n_dname, np->n_state);

		if (np->n_pd == -1) {
			if ((np->n_pd = rconnect(np->n_addr,RECOVER)) == -1)
				np->n_state = R_DEAD;
			else
				np->n_state = R_PENDING;
		}
		else {
			if (r_sndreq(np->n_pd, np, QUERY, NS_IM_NP) == RFS_FAILURE)
				np->n_state = R_DEAD;
			else
				np->n_state = R_UNK;
		}

		LOG4(L_ALL,"(%5d) recovery: state of %s set to %d\n",
			Logstamp,np->n_dname,np->n_state);
	}

	/* now wait for replies and don't take other requests	*/
	sigset(SIGALRM,rst_alrm);
	Rdone = FALSE;

	while (Rdone == FALSE) {
		/* is anything pending or unknown? if not, we are through */
		for (i=0, np = Nsinfo; i < MAXNS; i++, np++) {
			if (np != Mynp && (np->n_state == R_PENDING ||
			    np->n_state == R_UNK))
				break;
		}
		if (i == MAXNS)
			break;

		alarm(savetime);
		ret = nswait(&pd);
		if ((savetime = alarm(0)) == 0)
			break;

		np = pdtonp(pd);

		if (block) {
			free(block);
			block = NULL;
		}
		if (req) {
			freerp(req, TRUE);
			req = NULL;
		}

		switch (ret) {
		case LOC_REQ:	/* fail normal request with R_INREC */
		case REM_REQ:
			LOG3(L_COMM,"(%5d) recovery: rcvd %s, ignore\n",Logstamp,
				(ret==LOC_REQ)?"local request":"remote request");

			nsread(pd, &block, 0);
			sndback(pd,R_INREC);
			break;
		case REC_IN:
			LOG4(L_COMM,"(%5d) recovery: rcvd REC_IN from %s on pd %d\n",
				Logstamp,(np)?np->n_dname:"unknown machine",pd);

			if ((size = nsread(pd, &block, 0)) == -1 ||
			    (req = btoreq(block,size)) == NULL ||
			     checkver(req) == RFS_FAILURE) {
				LOG3(L_ALL,"(%5d) recovery: REC_IN failed for %s\n",
					Logstamp, (np)?np->n_dname:"UNKNOWN");
				break;
			}
			if (!np && (np = nmtonp(req->rq_head->h_dname)) == NULL) {
				LOG3(L_ALL,"(%5d) recovery: unexpected ns msg from %s\n",
					Logstamp, req->rq_head->h_dname);
				break;
			}
			if (strcmp(np->n_dname,req->rq_head->h_dname) != 0) {
				LOG4(L_ALL,"(%5d) recover: expect ns %s got ns %s\n",
					Logstamp,np->n_dname,req->rq_head->h_dname);
				nsrclose(pd);
				if (pd != np->n_pd)
					nsrclose(np->n_pd);
				if ((np->n_pd = rconnect(np->n_addr,RECOVER)) == -1)
					np->n_state = R_DEAD;
				else
					np->n_state = R_PENDING;

				LOG4(L_COMM,"(%5d) recover: set state of %s to %d\n",
					Logstamp,np->n_dname,np->n_state);
				break;
			}

			/* only deal with NS_IM_NP and NS_IM_P msg types	*/
			code = req->rq_head->h_opcode;

			if (code == NS_IM_NP)
				state = R_NOTPRIME;
			else if (code == NS_IM_P)
				state = R_PRIME;
			else {
				if (req->rq_head->h_flags & QUERY)
					sndback(pd,R_INREC);
				break;
			}

			if (np->n_state == R_DEAD) {
				np->n_pd = pd;
				np->n_state = state;
			}
			else if (np->n_state == R_UNK) {
				if (pd != np->n_pd) {
					/*
					 * shouldn't happen, but if it does
					 * treat like a collision
					 */
					LOG5(L_ALL,"(%5d) recover: %s, pd %d != %d\n",
						Logstamp, np->n_dname, pd, np->n_pd);
					if (np > Mynp) {
						nsrclose(np->n_pd);
						np->n_pd = pd;
					}
					else {
						nsrclose(pd);
						break;
					}
				}
				np->n_state = state;
			}
			else if (np->n_state == R_PENDING) {
				if (pd == np->n_pd)
					LOG2(L_ALL,"recovery: WARNING NS %s out of state\n",
						np->n_dname);
				else { /* collision */
					if (np > Mynp) {
						nsrclose(np->n_pd);
						np->n_pd = pd;
					}
					else {
						nsrclose(pd);
						break;
					}
				}
				np->n_state = state;
			}
			else 
				np->n_state = state;


			if (req->rq_head->h_flags & QUERY)
				r_sndreq(pd, np, RESPONSE, NS_IM_NP);

			LOG4(L_COMM,"(%5d) recover: set state of %s to %d\n",
				Logstamp,np->n_dname,np->n_state);
			break;
		case REC_ACC:
			if (!np) { /* collision */
				LOG3(L_ALL,"(%5d) recovery: connects collided ns=%s\n",
					Logstamp, np->n_dname);
				nsrclose(pd);
				break;
			}
			LOG3(L_COMM,"(%5d) recovery: rcvd REC_ACC from %s\n",
				Logstamp, np->n_dname);

			/* got connection, send poll	*/
			if (r_sndreq(pd, np, QUERY, NS_IM_NP) == RFS_FAILURE) {
				nsrclose(pd);
				np->n_state = R_DEAD;
				np->n_pd = -1;
			}
			else
				np->n_state = R_UNK;

			LOG4(L_COMM,"(%5d) recover: set state of %s to %d\n",
				Logstamp,np->n_dname,np->n_state);
			break;
		case REC_HUP:
			LOG4(L_COMM,"(%5d) recovery: rcvd hangup from %s, pd=%d\n",
				Logstamp, (np)?np->n_dname:"unknown ns",pd);

			nsrclose(pd);
			if (np) {
				if ((np->n_pd = rconnect(np->n_addr,RECOVER)) == -1)
					np->n_state = R_DEAD;
				else
					np->n_state = R_PENDING;

				LOG4(L_COMM,"(%5d) recover: set state of %s to %d\n",
					Logstamp,np->n_dname,np->n_state);
			}
			break;
		case FATAL:
			PLOG3("(%5d) recovery: fatal return from nswait, errno=%d\n",
				Logstamp, errno);
			sigset(SIGUSR2,usr2sig);
			return(RFS_FAILURE);
		case REC_CON:	/* wait for poll from connector	*/
		case NON_FATAL:
		default:
			break;
		}
	}
	alarm(0);
	LOG2(L_COMM,"(%5d) recovery: ready to decide\n",Logstamp);

	for (np=Nsinfo; np < &Nsinfo[MAXNS]; np++) {
		switch (np->n_state) {
		case R_PRIME:
			if (pnp != NULL) {
				PLOG3("RFS name server: WARNING: found 2 name servers, %s and %s\n",
					pnp->n_dname, np->n_dname);
				PLOG2("RFS name server: Using %s as primary\n",
					pnp->n_dname);
				break;
			}
			pnp = np;

			LOG3(L_COMM,"(%5d) recovery: state of %s is R_PRIME\n",
				Logstamp, np->n_dname);
			break;
		case R_NOTPRIME:
			LOG4(L_COMM,"(%5d) recovery: state of %s is %s R_NOTPRIME\n",
				Logstamp, np->n_dname, (npnp)?"":"first");
			if (!npnp)
				npnp = np;
			break;
		case R_UNK:
			LOG3(L_COMM,"(%d) recovery: no resp from connected ns %s\n",
				Logstamp, np->n_dname);
			np->n_state = R_DEAD;
			/* fall through to clean up port	*/
		case R_DEAD:
		case R_PENDING:
			LOG4(L_COMM,"(%5d) recovery state of %s is %s\n",
				Logstamp, np->n_dname, (np->n_state == R_DEAD)?
				"R_DEAD":"R_PENDING, set to R_DEAD");
			np->n_state = R_DEAD;
			if (np->n_pd != -1) {
				nsrclose(np->n_pd);
				np->n_pd = -1;
			}
			break;
		}
	}
	if (!pnp && !(pnp = npnp))  {
		PLOG1("RFS name server: recovery: no primary assignment possible\n");
		sigset(SIGUSR2,usr2sig);
		return(RFS_FAILURE);
	}
	pnp->n_state = R_PRIME;
	if (Mynp == pnp) {
		LOG2(L_ALL,"(%5d) recovery: becoming PRIMARY\n",
			Logstamp);
			Primary = TRUE;
	}
	else {
		LOG3(L_ALL,"(%5d) recovery: using %s as PRIMARY\n",
			Logstamp, pnp->n_dname);
	}
	if (Primary)	/* tell the world	*/
		for (np=Nsinfo; np < &Nsinfo[MAXNS]; np++)
			if (np->n_state == R_NOTPRIME)
				r_sndreq(np->n_pd, np, QUERY, NS_IM_P);

	/* now re-set sigsets for polling and do any initialization.	*/
	if (!Primary) {
		sigset(SIGALRM,r_poll);
		Alarmsig = (void (*)()) r_poll;
	}
	else {
		sigset(SIGALRM,r_checkup);
		Alarmsig = (void (*)()) r_checkup;
	}

	alarm((timesave)?timesave:Polltime);
	sigset(SIGUSR2,usr2sig);
	return(RFS_SUCCESS);
}
int
r_sndreq(pd, np, flags, opcode)
int	pd;
struct ns_info	*np;
int	flags;
int	opcode;
{
	char	*block=NULL;
	static int	size=0;
	static struct header	Head;
	static struct request	Req;

	Req.rq_head = &Head;
	Head.h_version = NSVERSION;
	Head.h_flags = flags;
	Head.h_opcode = opcode;
	Head.h_dname = Dname;

	LOG6(L_COMM,"(%5d) r_sndreq: send %s %s to machine %s on pd %d\n",
		Logstamp, fntype(opcode), (flags&QUERY)?"query":"response",
		np->n_dname, pd);

	if ((block = reqtob(&Req, block, &size)) == NULL) {
		LOG3(L_ALL,"(%5d) recovery reqtob 2 failed, ns=%s\n",
			Logstamp,np->n_dname);
		return(RFS_FAILURE);
	}
	if (nswrite(pd,block,size) == -1) {
	    LOG3(L_ALL,"(%5d) recovery: can't reply IM_NP to %s\n",
		Logstamp,np->n_dname);
		np->n_state = R_DEAD;
		free(block);
		return(RFS_FAILURE);
	}
	free(block);
	return(RFS_SUCCESS);
}
struct ns_info *
pdtonp(pd)
int	pd;
{
	register struct ns_info	*np = &Nsinfo[0];
	register struct ns_info	*ep = &Nsinfo[MAXNS];

	if (pd == -1)
		return(NULL);

	while (np < ep) {
		if (np->n_pd == pd && np->n_state != R_UNUSED)
			return(np);
		np++;
	}
	return(NULL);
}
/*
 * check a port descriptor, pd, before it is closed and clean up
 * any ns_info structure associated with it.
 */
int
np_clean(pd)
int	pd;
{
	struct ns_info	*np;

	if ((np = pdtonp(pd)) != NULL) {
		np->n_pd = -1;
		np->n_state = R_DEAD;
	}
	return;
}
void
rst_alrm()
{
	alarm(0);
	Rdone = TRUE;
}
struct ns_info *
nmtonp(name)
char	*name;
{
	register struct ns_info	*np = &Nsinfo[0];
	register struct ns_info	*ep = &Nsinfo[MAXNS];

	if (name == NULL)
		return(NULL);

	while (np < ep) {
		if (np->n_state != R_UNUSED &&
		    strcmp(name,np->n_dname) == NULL)
			return(np);
		np++;
	}
	return(NULL);
}
checkver(req)
struct request	*req;
{
	if (req->rq_head->h_version > NSVERSION) {
		LOG4(L_ALL,"(%5d) checkver: version mismatch, reqver %d, nsver %d\n",
			Logstamp, req->rq_head->h_version, NSVERSION);
		return(RFS_FAILURE);
	}
	return(RFS_SUCCESS);
}
int
nsrecover(pd,type)
int	pd;
int	type;
{
	struct ns_info	*np;

	LOG2(L_COMM,"(%5d) nsrecover: enter\n",Logstamp);

	np = pdtonp(pd);

	switch(type) {
	case REC_CON: /* wait for poll message	*/
		break;
	case REC_ACC: /* got reply to checkup connection, send poll.	*/
		if (r_sndreq(pd, np, QUERY, (Primary)?NS_IM_P:NS_IM_NP) == RFS_FAILURE) {
			nsrclose(pd);
			np->n_state = R_DEAD;
			np->n_pd = -1;
		}
		else
			np->n_state = R_UNK;

		break;
	case REC_HUP:
		nsrclose(pd);
		if (np) {
			np->n_pd = -1;
			if (np->n_state == R_PRIME)
				return(setrstate());
			np->n_state = R_DEAD;
		}
		break;
	}
	return(RFS_SUCCESS);
}
/*
 * check an incoming request of type NS_IM_P or NS_IM_NP for
 * basic sanity.  I.e., make sure there are no conflicts with
 * what is known about the machine already.
 */
int
r_sane(req,pd)
struct request	*req;
int		pd;
{
	struct ns_info	*np;
	struct header	*hp = req->rq_head;

	if ((np = nmtonp(hp->h_dname)) == NULL) {
		LOG3(L_ALL,"RFS name server: rcvd %s msg from unk NS %s\n",
			getctype(hp->h_opcode),hp->h_dname);
		/* return success since we don't want to stop here */
		nsrclose(pd);
		return(RFS_SUCCESS);
	}
	if (np->n_pd == -1)
		np->n_pd = pd;

	switch (hp->h_opcode) {
	case NS_IM_P:
		if (Primary) {
			PLOG2("RFS name server: NS %s, primary collision\n",
				hp->h_dname);
			/* yield if other machine is higher on list */
			if (nmtonp(Dname) > np)
		    		return(setrstate());
			/* otherwise tell it who's boss	*/
			if (r_sndreq(pd, np, QUERY, NS_IM_P) == RFS_FAILURE) {
				nsrclose(pd);
				np->n_state = R_DEAD;
				np->n_pd = -1;
		    		return(setrstate());
			}
		    	return(RFS_SUCCESS);
		}
		if (np->n_state != R_PRIME) {
			LOG3(L_ALL,"(%5d): found possible second primary %s\n",
				Logstamp, hp->h_dname);
			return(setrstate());
		}
		break;
	case NS_IM_NP:
		switch (np->n_state) {
			case R_UNK:
			case R_DEAD:
				np->n_state = R_NOTPRIME;
				break;
			case R_NOTPRIME:
				break;
			default:
				LOG3(L_ALL,"(%5d) %s changing state to NS_IM_NP\n",
					Logstamp,hp->h_dname);
				return(setrstate());
		}
		break;
	}
	return(RFS_SUCCESS);
}
/*
 * periodic polling routine for primary machine.
 */
int
r_checkup()
{
	register struct ns_info	*np = &Nsinfo[0];
	register struct ns_info	*ep = &Nsinfo[MAXNS];
	void	(*usr2sig)();

	usr2sig = sigset(SIGUSR2,SIG_HOLD);
	sigset(SIGALRM,SIG_IGN);
	alarm(0);

	for (; np < ep; np++) {
		if (np->n_state == R_UNUSED)
			continue;
		if (strcmp(Dname,np->n_dname) == 0)
			continue;
		if (np->n_pd == -1) {
			if ((np->n_pd = rconnect(np->n_addr,RECOVER)) == -1)
				np->n_state = R_DEAD;
			else
				np->n_state = R_PENDING;
		}
		else if (r_sndreq(np->n_pd, np, QUERY,(Primary)?NS_IM_P:NS_IM_NP)
				== RFS_FAILURE) {
			np->n_state = R_DEAD;
			nsrclose(np->n_pd);
			np->n_pd = -1;
		}
	}
	sigset(SIGALRM,r_checkup);
	alarm(Polltime);
	sigset(SIGUSR2,usr2sig);
	return;
}
/*
 * periodic polling routine for secondary machine.
 */
int
r_poll()
{
	int	i;
	int	pd;
	char	qbuf[BUFSIZ];
	char	*block = NULL;
	int	size = 0;
	void	(*usr2sig)();
	static struct header	Head;
	static struct request	Req;
	static struct question	Question;
	struct question		*qp = &Question;

	LOG2(L_COMM,"(%5d) r_poll: enter\n",Logstamp);

	usr2sig = sigset(SIGUSR2,SIG_HOLD);
	sigset(SIGALRM,SIG_IGN);
	alarm(0);

	if ((pd = primepd()) == -1) {
		if (setrstate() == RFS_FAILURE) 
			Done = TRUE;
		/* don't reset SIGALRM, setrstate will do that	*/
		sigset(SIGUSR2,usr2sig);
		return;
	}

	LOG2(L_COMM,"(%5d) r_poll: polling primary\n",Logstamp);

	Req.rq_head = &Head;
	Head.h_version = NSVERSION;
	Head.h_flags = QUERY;
	Head.h_opcode = NS_QUERY;
	Head.h_dname = Dname;
	Head.h_qdcnt = 1;
	Req.rq_qd = &qp;
	Question.q_name = qbuf;
	Question.q_type = ANYTYPE;

	for (i=0; i < Mylastdom; i++) {
#ifdef RIGHT_TO_LEFT
		sprintf(qbuf,"%c%c%s",WILDCARD,SEPARATOR,Mydomains[i]);
#else
		sprintf(qbuf,"%s%c%c",Mydomains[i],SEPARATOR,WILDCARD);
#endif
		LOG3(L_COMM,"(%5d) r_poll: poll primary for domain %s\n",
			Logstamp, Mydomains[i]);

		if ((block = reqtob(&Req,block,&size)) == NULL) {
			LOG3(L_ALL,"(%5d) r_poll: reqtob failed domain = %s\n",
				Logstamp, Mydomains[i]);
			continue;
		}
		if (nswrite(pd,block,size) == -1) {
			LOG2(L_ALL,"(%5d) r_poll: write to primary failed\n",
				Logstamp);
			free(block);
			if (setrstate() == RFS_FAILURE)
				Done = TRUE;
			/* don't reset SIGALRM, setrstate will do that	*/
			sigset(SIGUSR2,usr2sig);
			return;
		}
		free(block);
		block = NULL;
	}
	sigset(SIGALRM,r_poll);
	alarm(Polltime);
	sigset(SIGUSR2,usr2sig);
	return;
}
int
primepd()
{
	register struct ns_info	*np = &Nsinfo[0];
	register struct ns_info *ep = &Nsinfo[MAXNS];

	while (np < ep) {
		if (np->n_state == R_PRIME)
			return(np->n_pd);
		np++;
	}
	return(-1);
}
/*
 * add data collected by poll to database
 */
int
nsdata(req)
struct request	*req;
{
	long	i;
	char	fname[BUFSIZ];
	char	dname[BUFSIZ];
	struct header	*hp;

	LOG2(L_COMM,"(%5d) nsdata: enter\n",Logstamp);

	hp = req->rq_head;

	if (hp->h_rcode != R_NOERR || hp->h_qdcnt <= 0) {
		LOG4(L_ALL,"(%5d) nsdata: rcode %d or qdcnt %d bad\n",
			Logstamp, hp->h_rcode, hp->h_qdcnt);
		return(R_NSFAIL);
	}

	if (!(hp->h_flags & AUTHORITY)) {
		if (setrstate() == RFS_FAILURE) {
			Done = TRUE;
			return(R_NSFAIL);
		}
		return(R_NOERR);
	}
	strcpy(dname,dompart(req->rq_qd[0]->q_name));

	LOG3(L_COMM,"(%5d) nsdata: reply for domain %s\n",
		Logstamp,dname);

	if (!mydom(dname)) {
		LOG3(L_ALL,"(%5d) nsdata: reply rcvd for un-owned domain %s\n",
			Logstamp,dname);
		return(R_NSFAIL);
	}

	cleardom(dname);

	for (i=0; i < hp->h_ancnt; i++) {
#ifdef RIGHT_TO_LEFT
		sprintf(fname,"%s%c%s",req->rq_an[i]->rr_name,SEPARATOR,dname);
#else
		sprintf(fname,"%s%c%s",dname,SEPARATOR,req->rq_an[i]->rr_name);
#endif
		LOG4(L_COMM,"(%5d) nsdata: addrr %s, %s\n",
			Logstamp,fname,prec(req->rq_an[i]));
		addrr(fname,req->rq_an[i]);
	}
	return(R_NOERR);
}
/*
 * ns_rel handles rfadmin -p.  if the request is local, it sends
 * a copy of its current database to the secondaries and goes into
 * recovery, sending NS_RELs to each secondary.  If the request
 * is remote, it should be from the primary, and signals the need
 * to go into recovery to get a new primary.
 */
int
ns_rel(pd)
int	pd;
{
	struct res_rec	**rlist;
	struct request	*req;
	struct header	*hp;
	struct request	*nreq;
	int		ret;
	int		i;
	char		*key=NULL;
	struct request	*newreq();
	struct request	*nsfunc();
	struct ns_info	*np;
	struct nsport	*pptr = pdtoptr(pd);

	if (!pptr)
		return(R_FAIL);

	switch(pptr->p_mode) {
	case LOCAL:
		/* this is the primary side of rfadmin -p	*/
		if (!Primary)
			return(R_PERM);
		break;
	case RECOVER:
		/* this is the secondary side of rfadmin -p	*/
		if ((np=pdtonp(pd)) == NULL)
			return(R_FAIL);
		if (Primary || np->n_state != R_PRIME)
			return(R_PERM);
		/* now "remove" primary from recovery algorithm	*/
		np->n_state = R_UNUSED;
		remfrsel(pptr);
		if (setrstate() == RFS_FAILURE) {
			PLOG1("RFS name server: FATAL recovery error\n");
			Done = TRUE;
			return(R_NSFAIL);
		}
		addtosel(pptr);
		np->n_state = R_UNK;
		return(R_NOERR);
	default:
		return(R_FORMAT);
	}

	/* at this point we have legitimate local requests only	*/

	if ((req = newreq(NS_QUERY,QUERY,Dname)) == NULL)
		return(R_NSFAIL);

	if ((key = malloc(MAXDNAME)) == NULL ||
	   (req->rq_qd=(struct question **)malloc(sizeof(struct question *))) == NULL||
	   (req->rq_qd[0]=(struct question *)malloc(sizeof(struct question))) == NULL){
		if (key)
			free(key);
		freerp(req);
		return(R_NSFAIL);
	}
	hp = req->rq_head;
	hp->h_qdcnt = 1;
	req->rq_qd[0]->q_name = key;
	req->rq_qd[0]->q_type = ANYTYPE;

	for (i=0; i < Mylastdom; i++) {
		sprintf(key,"%s.%c",Mydomains[i],WILDCARD);
		if ((nreq = nsfunc(req,-1)) == NULL) {
			freerp(req);
			return(R_NSFAIL);
		}
		if (nspass(nreq) == RFS_FAILURE) {
			freerp(req);
			freerp(nreq);
			return(R_NSFAIL);
		}
		freerp(nreq);
	}
	freerp(req);
	ret = nsrel();
	return(ret);
}
int
nspass(nreq)
struct request	*nreq;
{
	char	*block = NULL;
	int	size = 0;
	int	i;
	struct ns_info	*np;
	struct header	*hp = nreq->rq_head;

	LOG2(L_ALL,"(%5d) nsrel: relinquish primary status\n",Logstamp);

	if ((block = reqtob(nreq,block,&size)) == NULL) {
		LOG2(L_ALL,"(%5d) nsrel: reqtob failed\n",Logstamp);
		return(RFS_FAILURE);
	}

	for (i=0, np = Nsinfo; i < MAXNS; i++, np++)
		if (np->n_state == R_NOTPRIME)
			nswrite(np->n_pd,block,size);

	free(block);
	return(RFS_SUCCESS);
}
int
nsrel()
{
	int	i;
	int	polled = 0;
	struct ns_info	*np;
	struct nsport	*pptr;

	for (i=0, np = Nsinfo; i < MAXNS; i++, np++)
		if (np->n_state == R_NOTPRIME && strcmp(Dname,np->n_dname)) {
			if (r_sndreq(np->n_pd, np, QUERY, NS_REL) == RFS_FAILURE) {
				nsrclose(np->n_pd);
				np->n_pd = -1;
				np->n_state = R_DEAD;
			}
			else
				polled++;
		}
	if (polled == 0) {
		LOG2(L_ALL,"(%5d) nsrel: can't pass control, no secondaries\n",
			Logstamp);
		return(R_NONAME);
	}

	alarm(0);
	sigrelse(SIGALRM);
	sleep(R_TIMEOUT+5);

	for (i=0, np = Nsinfo; i < MAXNS; i++, np++)
		if (np->n_state == R_NOTPRIME && strcmp(Dname,np->n_dname)) {
			pptr = pdtoptr(np->n_pd);
			if (ioctl(pptr->p_fd,I_FLUSH,FLUSHR) == -1) {
				nsrclose(np->n_pd);
				np->n_pd = -1;
				np->n_state = R_DEAD;
			}
		}

	if (setrstate() == RFS_FAILURE) {
		PLOG1("RFS name server: FATAL error, nsrel recovery failure\n");
		Done = TRUE;
		return(R_NSFAIL);
	}
	/* make sure we still aren't primary	*/
	if (Primary)
		return(R_NONAME);

	return(R_NOERR);
}
