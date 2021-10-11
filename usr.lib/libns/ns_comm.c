/*	@(#)ns_comm.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_comm.c	1.10" */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <memory.h>
#include <sys/types.h>
#include <string.h>
#include <rfs/nserve.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include "nslog.h"
#include "stdns.h"
#include "nsdb.h"
#include "nsports.h"
#include <rfs/rfsys.h>
/* extern declarations for debug */

extern  int errno;
int	ns_errno;	/* set by ns_comm functions	*/
char	*nstob();
char	*namepart();
char	*dompart();
char	*getdomain();
struct nssend	*btons();
static int blockmask;
static	int	sigcatch();
static	int	setsigs();
static	int	unsetsigs();
static	int	Pd = -1;
static	struct address	Paddr= {
	0, { 0, 0,/* will change later	*/
	NS_PIPE }
};

int
ns_setup()
{
	ns_errno = R_NOERR;
	setsigs();
	if (Pd != -1) {
		LOG3(L_ALL,"(%5d) ns_setup: port already assigned, Pd=%d\n",
			Logstamp,Pd);
		ns_errno = R_SETUP;
		unsetsigs();
		return(RFS_FAILURE);
	}
	Paddr.addbuf.len = strlen(Paddr.addbuf.buf)+1;
	if ((Pd = nsconnect(&Paddr)) == -1) {
		LOG3(L_ALL,"(%5d) ns_setup: connect failed, Pd=%d\n",
			Logstamp,Pd);
		ns_errno = R_SETUP;
		unsetsigs();
		return(RFS_FAILURE);
	}
	LOG3(L_COMM,"(%5d) ns_setup: Pd=%d\n",Logstamp,Pd);
        return(RFS_SUCCESS);
}
int
ns_send(msg_out)
struct nssend *msg_out;
{
	char	*block;
	int	size;

	ns_errno = R_NOERR;
        if (Pd == -1) {  /* NS never opened      */
                LOG2(L_ALL,"(%5d) ns_send: FAILED, NS never set up\n",Logstamp);
		ns_errno = R_SETUP;	/* was R_SEND */
                return(RFS_FAILURE);
        }
        if ((block = nstob(msg_out,&size)) == NULL) {
		ns_errno = R_SETUP;	/* was R_SEND */
                return(RFS_FAILURE);
	}

        if (nswrite(Pd,block,size) == -1) {
                LOG2(L_ALL,"(%5d) ns_send: nswrite failed\n",Logstamp);
		free(block);
		ns_errno = R_SETUP;	/* was R_SEND */
                return(RFS_FAILURE);
        }
        LOG2(L_COMM,"(%5d) ns_send: nswrite succeeded\n",Logstamp);
	free(block);
        return(RFS_SUCCESS);
}
struct nssend *
ns_rcv()
{
	int	size;
	static char	*block=NULL;
	static struct nssend *reply;
	static struct nssend *ret;
	static int	rindex=0;	/* current index in reply */
	static int	last=FALSE;

	ns_errno = R_NOERR;
        if (Pd == -1) {  /* NS never opened      */
                LOG2(L_ALL,"(%5d) ns_rcv: FAILED, NS never set up\n",Logstamp);
		ns_errno = R_SETUP;	/* was R_RCV */
                return(NULL);
        }
	if (last) {	/* last time was end of block, reset for next time */
		free(block);
		block = NULL;
		free(reply);
		reply = NULL;
		rindex = 0;
		last=FALSE;
	} 
	if (block == NULL) { /* first call, read for response	*/
        	if ((size=nsread(Pd,&block,0)) == -1) {
                	LOG2(L_ALL,"(%5d) ns_rcv: nsread failed\n",Logstamp);
			ns_errno = R_SETUP;	/* was R_RCV */
                	return(NULL);
        	}
		if ((reply = btons(block,size)) == NULL) {
                	LOG2(L_ALL,"(%5d) ns_rcv: btons failed\n",Logstamp);
			ns_errno = R_SETUP;	/* was R_RCV */
                	return(NULL);
        	}
	}
	ret = &reply[rindex];
	if (!(reply[rindex].ns_code & MORE_DATA)) /*  this is the last one, reset */
		last=TRUE;
	else
		rindex++;

	return(ret);
}
int
ns_close()
{
	LOG2(L_COMM,"(%5d) ns_close\n",Logstamp);
        nsclose(Pd);
	Pd = -1;
	unsetsigs();
	return(RFS_SUCCESS);
}


static char	*
nstob(nsp,size)
struct nssend	*nsp;
int		*size;	/* return value for size	*/
{
	char	*block;
	char	domain[20];	/* domain name, will point to utsname	*/
				/* entry when there is one		*/
	char	*dptr;
	char	fullname[BUFSIZ];	/* full domain name of resource	*/
	place_p	pp;
	char nodename[SZ_MACH];

	LOG4(L_COMM,"(%5d) nstob(nsp=%x,size(ptr)=%x)\n",Logstamp,nsp,size);
	if ((block = malloc(DBLKSIZ)) == NULL) {
		LOG3(L_ALL,"(%5d) nstob: malloc(%d) FAILED\n",
			Logstamp,DBLKSIZ);
		return(NULL);
	}
	/* later nodename will have domain name too		*/
	/* for now use a function that will just read a file	*/

	(void) gethostname(nodename, SZ_MACH - 1);
	nodename[SZ_MACH - 1] = '\0';
#ifdef RIGHT_TO_LEFT
	sprintf(domain,"%s.%s",nodename,getdomain());
#else
	sprintf(domain,"%s.%s",getdomain(),nodename);
#endif

	pp = setplace(block,DBLKSIZ);

	/* do header first	*/
	putlong(pp,NSVERSION);		/* h_version	*/
	putlong(pp,QUERY | AUTHORITY);	/* h_flags	*/
	putlong(pp,nsp->ns_code);	/* h_opcode	*/
	putlong(pp,0);			/* h_rcode	*/

	putlong(pp,1);			/* h_qdcnt	*/
	if (nsp->ns_code == NS_ADV     ||
	    nsp->ns_code == NS_MODADV  ||
	    nsp->ns_code == NS_UNADV   ||
	    nsp->ns_code == NS_VERIFY  ||
	    nsp->ns_code == NS_SENDPASS )
		putlong(pp,1);		/* h_ancnt	*/
	else
		putlong(pp,0);

	putlong(pp,0);			/* h_nscnt	*/
	putlong(pp,0);			/* h_arcnt	*/
	putstr(pp,domain);		/* h_dname	*/

	/* end of header	*/
	/* now do query		*/

	/* first see if domain name needs to be added	*/
	/* if a domain is part of the name, use it	*/
	/* otherwise, append the name of this domain	*/

	dptr = dompart(nsp->ns_name);
	if (dptr == NULL || *dptr == '\0')
#ifdef RIGHT_TO_LEFT
		sprintf(fullname,"%s.%s",nsp->ns_name,dompart(domain));
#else
		sprintf(fullname,"%s.%s",dompart(domain),nsp->ns_name);
#endif
	else {
#ifdef RIGHT_TO_LEFT
		if (*nsp->ns_name == SEPARATOR)
			sprintf(fullname,"%c%s",WILDCARD,nsp->ns_name);
		else
			strcpy(fullname,nsp->ns_name);
#else
		if (nsp->ns_name[strlen(dptr)-1] == SEPARATOR)
			sprintf(fullname,"%s%c",nsp->ns_name,WILDCARD);
		else
			strcpy(fullname,nsp->ns_name);
#endif
	}
	putstr(pp,fullname);		/* q_name	*/
	putlong(pp,RN);			/* q_type	*/

	/* add resource records, if needed			*/

	if (nsp->ns_code == NS_ADV     ||
	    nsp->ns_code == NS_MODADV  ||
	    nsp->ns_code == NS_UNADV   ||
	    nsp->ns_code == NS_VERIFY  ||
	    nsp->ns_code == NS_SENDPASS ) {
		if (overbyte(pp,c_sizeof(namepart(fullname)) + L_SIZE +
		    c_sizeof(domain) + c_sizeof(nsp->ns_desc) +
		    c_sizeof(nsp->ns_path) + L_SIZE)
		    || explace(pp,0) == RFS_FAILURE) {
			LOG2(L_ALL,"(%5d) nstob: can't lengthen block\n",
				Logstamp);
			free(block);
			free(pp);
			return(NULL);
		}
		putstr(pp,namepart(fullname));	/* rr_name	*/
		putlong(pp,RN);			/* rr_type	*/
		putstr(pp,domain);		/* rr_owner	*/
		putstr(pp,nsp->ns_desc);	/* rr_desc	*/
		putstr(pp,nsp->ns_path);	/* rr_path	*/
		putlong(pp,nsp->ns_flag);	/* rr_flag	*/
	}
	*size = pp->p_ptr - pp->p_start;
	free(pp);
	return(pp->p_start);
}
static struct nssend	*
btons(block,size)
char	*block;
int	size;
{
	struct request	*btoreq();
	struct request	*rp;
	struct header	*hp;
	struct nssend	*reply;
	struct nssend	*np;
	struct res_rec	*rr;
	struct address	*astoa();
	int	n;	/* number of replies	*/
	int	i;

	if ((rp = btoreq(block,size)) == NULL) {
		LOG2(L_COMM,"(%5d) btons: btoreq failed\n",Logstamp);
		return(NULL);
	}

	hp = rp->rq_head;
	n = (hp->h_ancnt)?hp->h_ancnt:1;

	if ((reply = (struct nssend *) calloc(n,sizeof(struct nssend))) == NULL) {
		LOG4(L_ALL,"(%5d) btons: calloc(%d,%d) failed\n",
			Logstamp,n,sizeof(struct nssend));
		return(NULL);
	}

	for (i=0, np=reply; i < n; i++,np++) {
		/* set errno, note that information can be lost */
		/* if more than one different errno is returned */
		/* in one set of replies.			*/
		if (hp->h_rcode != R_NOERR)
			ns_errno = hp->h_rcode;
		/* set up header */
		np->ns_code = (hp->h_rcode == R_NOERR)?RFS_SUCCESS:RFS_FAILURE;
		np->ns_type = 0;	/* ? what should this be	*/
		if (hp->h_ancnt <= i)
			break;
		rr = rp->rq_an[i];
		np->ns_flag = rr->rr_flag;
		np->ns_name = rr->rr_name;
		np->ns_desc = rr->rr_desc;
		np->ns_path = rr->rr_path;
		np->ns_mach = &(rr->rr_owner);
		if (hp->h_arcnt <= i)
			np->ns_addr = NULL;
		else
			np->ns_addr = (rp->rq_ar[i]->rr_a)?
					astoa(rp->rq_ar[i]->rr_a,NULL):NULL;
		if (n>i+1)
			np->ns_code |= MORE_DATA;
	}
	return(reply);
}
char	*
getdomain()
{
	static char dname[MAXDNAME];

	/* if after first call, just return the answer	*/
	if (*dname)
		return(dname);

	if (rfsys(RF_GETDNAME,dname,MAXDNAME)) {
		perror("RFS name server: can't get domain name");
		return(NULL);
	}
	/* get rid of trailing newline, if there	*/
	if (dname[strlen(dname)-1] == '\n')
		dname[strlen(dname)-1] = '\0';
	return(dname);
}
static int
setsigs()
{
	blockmask = sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT));
}
static int
unsetsigs()
{
	sigsetmask(blockmask);
}
static int
sigcatch(sig)
int	sig;
{
void	(*Oldsig)();

	Oldsig = signal(sig,SIG_IGN);
	ns_close();
	unsetsigs();
	signal(sig,Oldsig);
	/* post original signal to self	*/
	kill(getpid(),sig);
}
