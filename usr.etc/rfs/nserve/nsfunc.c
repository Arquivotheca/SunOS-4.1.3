/*      @(#)nsfunc.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nserve:nsfunc.c	1.14.1.2"
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "nslog.h"
#include "nsdb.h"
#include "stdns.h"
#include <rfs/nserve.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>

char	*prec();
char	*aatos();
char	*namepart();
char	*dompart();
struct res_rec	**duplist();
struct res_rec	*duprec();
struct question	**dupqd();
extern int	Verify;
extern int	Done;

/*
 *	nsfunc takes a request structure as a transaction,
 *	and returns a request structure as a result.  It
 *	provides the externally known name server functions:
 *	NS_ADV, NS_QUERY, etc., using the ns database.  Normal
 *	failures, e.g., can't find resource, are handled
 *	by including error codes in the returned structure.
 *	These error codes are listed in stdns.h.  Abnormal
 *	conditions, e.g., can't allocate memory, cause nsfunc
 *	to return NULL.
 */
struct request *
nsfunc(request,pd)
struct request *request;
int		pd;
{
	int	type;	/* request type	*/
	int	ret;	/* return code	*/
	struct header *hp;
	struct res_rec *rec;
	struct res_rec *arec;
	struct question *qp=NULL;
	struct res_rec **rlist;
	struct header	*nhp;
	struct request	*nreq;
	char	dom[MAXDNAME];

	struct request	*newreq();
	char *dompart();

	/* set up return request structure	*/
	if ((nreq = newreq(NS_SNDBACK,RESPONSE,Dname)) == NULL)
		return(NULL);

	nhp = nreq->rq_head;
	hp = request->rq_head;
	type = hp->h_opcode;

	LOG5(L_TRANS|L_OVER,"(%5d) nsfunc: type=%s, qname=%s, rec=%s\n",
		Logstamp,fntype(type),
		(hp->h_qdcnt)?request->rq_qd[0]->q_name:"NULL",
		(hp->h_ancnt)?prec(request->rq_an[0]):"NULL");

	/* first screen out the exceptional types	*/
	switch (type) {
	case NS_IM_P:
	case NS_IM_NP:
		if (!r_sane(request,pd)) {
			Done = TRUE;
			return(NULL);
		}
		nhp->h_opcode = (Primary)?NS_IM_P:NS_IM_NP;
		nhp->h_rcode = R_NOERR;
		return(nreq);
	case NS_SNDBACK:
		if ((nhp->h_rcode = nsdata(request)) != R_NOERR) {
			freerp(nreq);
			return(NULL);
		}
		return(nreq);
	case NS_REL:
		nhp->h_rcode = ns_rel(pd);
		return(nreq);
	}

	/*
	 * at this point, we know that this is a normal
	 * query, that should be in the standard format.
	 */

	if (request->rq_qd == NULL || (qp = request->rq_qd[0]) == NULL) {
		nhp->h_rcode = R_FORMAT;
		LOG2(L_TRANS,"(%5d) nsfunc: question section missing\n",Logstamp);
		return(nreq);
	}

	/* return the question section	*/
	nhp->h_qdcnt = hp->h_qdcnt;
	nreq->rq_qd = dupqd(request->rq_qd,hp->h_qdcnt);

	strcpy(dom,dompart(qp->q_name));

	/* Is this name server primary, and is it authority over this domain */
	if (Primary && domauth(dom))
		nhp->h_flags |= AUTHORITY;

	switch (type) {
	case NS_UNADV:
		nhp->h_rcode = ns_unadv(qp->q_name, qp->q_type, hp->h_dname);
		addind(dom,nreq);
		return(nreq);
	case NS_MODADV:
		if ((rlist = findrr(qp->q_name,qp->q_type)) != NULL) {
		    if (strcmp(rlist[0]->rr_owner,hp->h_dname)) {
			nhp->h_rcode = R_PERM;
			LOG4(L_TRANS,"(%5d) nsfunc: %s has no perm. to mod_adv %s\n",
			Logstamp, hp->h_dname, qp->q_name);
		    }
		    else if (hp->h_ancnt == 0 || (rec = request->rq_an[0]) == NULL) {
			nhp->h_rcode = R_FORMAT;
			LOG3(L_TRANS,"(%5d) nsfunc: %s no answer section\n",
				Logstamp,fntype(type));
		    }
		    else {
			remrr(qp->q_name,rlist[0]);
			rec->rr_flag = rlist[0]->rr_flag;
			if (!rec->rr_desc)
				rec->rr_desc = rlist[0]->rr_desc;
			nhp->h_rcode = addrr(qp->q_name,rec);
		    }
		    freelist(rlist);
		    addind(dom,nreq);
		    return(nreq);
		}
		/* modadv acts as normal adv if the resource is not found */
		/* so fall through to case NS_ADV			  */
	case NS_ADV:
		if (hp->h_ancnt == 0 || (rec = request->rq_an[0]) == NULL) {
			nhp->h_rcode = R_FORMAT;
			LOG3(L_TRANS,"(%5d) nsfunc: %s no answer section\n",
				Logstamp,fntype(type));
			return(nreq);
		}
		if ((nhp->h_rcode = ns_adv(qp->q_name,rec,Caddress)) == R_NSFAIL)
			return(NULL);
		addind(dom,nreq);
		return(nreq);
	case NS_BYMACHINE:
	case NS_QUERY:
		if (type == NS_QUERY) {
			if (Primary && domauth(dom))
				rlist = findrr(qp->q_name,qp->q_type);
			else
				rlist=NULL; /* addind() below will get addrs */
		}
		else
			rlist = iquery(dom,qp->q_type,qp->q_name);

		if (rlist) {
			/* get addresses for records found	*/
			nhp->h_ancnt = copyrlist(&(nreq->rq_an),rlist);
			nhp->h_rcode = R_NOERR;
			nhp->h_arcnt = getars(nreq->rq_an,&(nreq->rq_ar),
				nhp->h_ancnt);
			free((char *) rlist);
		}
		else if ((Primary && domauth(dom)) || addind(dom,nreq) == RFS_FAILURE) {
			nhp->h_rcode = R_NONAME;
			LOG4(L_TRANS,"(%5d) nsfunc: %s, no such name %s\n",
				Logstamp,fntype(type),qp->q_name);
		}
		else
			nhp->h_rcode = R_NOERR;

		return(nreq);
	case NS_FINDP:
		{
		struct res_rec	*recp;
		struct res_rec	*recl[2];

		if (!Primary || !domauth(qp->q_name)) {
			if (addind(dom,nreq) == RFS_FAILURE) {
			    nhp->h_rcode = R_NONAME;
			    LOG4(L_TRANS,"(%5d) nsfunc: %s, no such name %s\n",
				Logstamp,fntype(type),qp->q_name);
			}
			else
			    nhp->h_rcode = R_NOERR;
		}
		else { /* make a record to return	*/
			if ((recp = (struct res_rec *)
			    calloc(1,sizeof(struct res_rec))) == NULL ||
			    (recp->rr_rn = (struct rn *)
			    calloc(1,sizeof(struct rn))) == NULL) {
				PLOG2("(%5d) nsfunc: FINDP calloc failed\n",
					Logstamp);
				nhp->h_rcode = R_NSFAIL;
				return(nreq);
			}
			strncpy(recp->rr_name,namepart(qp->q_name),NAMSIZ);
			recp->rr_type = RN;
			recp->rr_owner = copystr(Dname);
			recp->rr_desc = NULL;
			recp->rr_path = NULL;
			recp->rr_flag = 0;
			recl[0] = recp;
			recl[1] = NULL;
			nhp->h_ancnt = copyrlist(&(nreq->rq_an),recl);
			nhp->h_rcode = R_NOERR;
			nhp->h_arcnt = getars(nreq->rq_an,&(nreq->rq_ar),
				nhp->h_ancnt);
		}
		return(nreq);
		}
	case NS_INIT:
		{
		/*
		 *   NS_INIT will unadvertise all resources for the
		 *   local machine.
		 */

		char *key, *p_key, dkey[MAXDNAME];
		struct res_rec *save_rec[MAXREC];
		int i;

		LOG2(L_TRANS, "(%5d) nsfunc: got a request for NS_INIT\n", Logstamp);

		if (!Primary || !domauth(dom)) {
			if (addind(dom,nreq) == RFS_FAILURE) {
			    nhp->h_rcode = R_NONAME;
			    LOG4(L_TRANS,"(%5d) nsfunc: %s, no such name %s\n",
				Logstamp,fntype(type),dom);
			}
			else
			    nhp->h_rcode = R_NOERR;

			return(nreq);
		}

		/*
		 * if this is a potential primary, give it everything,
		 * otherwise, give it just this domain
		 */
		if (nmtonp(hp->h_dname)) {
			dkey[0] = WILDCARD;
			dkey[1] = '\0';
		}
		else
			strcpy(dkey,dom);

		/* there has to be something to send back */
		if (addind(dkey,nreq) == RFS_FAILURE) {
			nhp->h_rcode = R_NSFAIL;
			return(nreq);
		}

		/*
		 *   Create a wildcard key that matches all
		 *   records for this domain.
		 */

		p_key = key = malloc(strlen(dom) + NAMSIZ + 1);
#ifdef RIGHT_TO_LEFT
		*p_key++ = WILDCARD;
		*p_key++ = SEPARATOR;
		strcpy(p_key, dom);
#else
		strcpy(p_key, dom);
		p_key += strlen(key);
		*p_key++ = SEPARATOR;
		*p_key++ = WILDCARD;
		*p_key = '\0';
#endif

		if ((rlist = findrr(key,qp->q_type)) == NULL) {
			nhp->h_rcode = R_NOERR;
			free(key);
			return(nreq);
		}

		/*
		 *   Save the list, since calls to remrr overide
		 *   it...
		 */

		i = 0;
		while ((save_rec[i] = rlist[i]) != NULL) i++;

		/*
		 *   Go through each record, and remove all
		 *   records that this machine has advertised
		 */

		for (i = 0; save_rec[i] != NULL; i++) {
			if (strcmp(save_rec[i]->rr_owner, qp->q_name) == NULL) {
				LOG3(L_TRANS, "(%5d) nsfunc: unadvertising <%s>\n",
					Logstamp, save_rec[i]->rr_name);
				strcpy(key, dom);
				p_key = key + strlen(key);
				*p_key++ = SEPARATOR;
				strcpy(p_key, save_rec[i]->rr_name);
				ret = remrr(key, save_rec[i]);
				nhp->h_rcode |= (unsigned) (ret & 0xF);
			}
		}
		free(key);
		freelist(rlist);
		nhp->h_rcode = R_NOERR;
		return(nreq);
		}
	case NS_VERIFY:
		{
		/*
		 *   NS_VERIFY verifies that the password given in the
		 *   description field matches the password in the
		 *   domain password file for the machine.
		 *   The description field of the returned record
		 *   will contain a string stating if the password is
		 *   correct or not.
		 */
		static struct res_rec *templist[2];
		static struct res_rec temprec;
		static struct rn      temprn;
		int	i;

		templist[0] = &temprec;
		templist[0]->rr_rn   = &temprn;
		templist[0]->rr_type = RN;
		templist[1] = NULL;

		LOG2(L_TRANS, "(%5d) nsfunc: got a request for NS_VERIFY\n", Logstamp);
		if (Primary && domauth(dom)) {
			if ((rec = request->rq_an[0]) == NULL) {
				nhp->h_rcode = R_FORMAT;
				LOG2(L_TRANS,"(%5d) nsfunc: no answer section\n",Logstamp);
				return(nreq);
			}

			ret = vpasswd(rec->rr_desc, namepart(qp->q_name),dom);

			if (ret >= 0) {
				templist[0]->rr_desc = NULL;
				nhp->h_rcode = ret;
			}
			else {
				nhp->h_rcode = R_NOERR;
				if (ret == -1) /* password was correct */
					templist[0]->rr_desc = CORRECT;
				else
					templist[0]->rr_desc = INCORRECT;
			}

			nreq->rq_an = duplist(templist);
			for (i=0; nreq->rq_an[i]; i++)
				;
			nhp->h_ancnt = i;
		}
		else {
			if (addind(dom,nreq) == RFS_SUCCESS)
				nhp->h_rcode = R_NOERR;
			else
				nhp->h_rcode = R_NSFAIL;
		}
		return(nreq);
		}
	case NS_SENDPASS:
		{
		/*
		 *   NS_SENDPASS changes the password for a machine.
		 *   The description field of the incomming record
		 *   is in the form "oldpassword:newpassword".
		 *   The old password is not encrypted and the
		 *   new password is.
		 *   If the oldpassword is correct, it is replaced with
		 *   the new password.
		 */

		char *oldpass, *newpass;

		LOG2(L_TRANS, "(%5d) nsfunc: got a request for NS_SENDPASS\n", Logstamp);
		if (Primary && domauth(dom)) {
			if ((rec = request->rq_an[0]) == NULL) {
				nhp->h_rcode = R_FORMAT;
				LOG2(L_TRANS,"(%5d) nsfunc: no answer section\n",Logstamp);
				return(nreq);
			}
			oldpass = newpass = rec->rr_desc;
			while (*newpass != ':')
				newpass ++;
			*newpass = '\0';
			newpass ++;
			while (*newpass == ':' && *newpass != '\0')
				newpass ++;
			ret = vpasswd(oldpass, namepart(qp->q_name),dom);
			if (ret >= 0) {
				nhp->h_rcode = ret;
			} else {
				if (ret == -1) /* password was correct */
					nhp->h_rcode = changepw(newpass,
							namepart(qp->q_name),dom);
				else
					nhp->h_rcode = R_INVPW;
			}
		}
		else {
			if (addind(dom,nreq) == RFS_SUCCESS)
				nhp->h_rcode = R_NOERR;
			else
				nhp->h_rcode = R_NSFAIL;
		}
		return(nreq);
		}
	default:
		LOG3(L_TRANS,"(%5d) nsfunc: unknown request type = %d\n",
			Logstamp,type);
		nhp->h_rcode = R_IMP;
		return(nreq);
	}
}

/*
 * addind adds indirect name server references to a request.
 */
int
addind(dom,req)
char	*dom;
struct request	*req;
{
	struct header	*hp = req->rq_head;
	int	i;

	if ((req->rq_ns = findind(dom)) == NULL)
		return(RFS_FAILURE);

	for (i=0; req->rq_ns[i] != NULL; i++)
		;

	hp->h_nscnt = i;

	if ((hp->h_arcnt = getars(req->rq_ns, &(req->rq_ar), hp->h_nscnt)) == 0) {
		freelist(req->rq_ns);
		req->rq_ns = NULL;
		hp->h_nscnt = 0;
		return(RFS_FAILURE);
	}
	return(RFS_SUCCESS);
}
/* copy resource record list and return # of entries	*/
int
copyrlist(target,source)
struct res_rec ***target;
struct res_rec **source;
{
	int	i;

	if (!source) {
		return(0);
	}
	for (i=0; source[i] != NULL; i++)
		;
	if (*target == NULL && (*target =
		(struct res_rec **) calloc(i+1,sizeof(struct res_rec *))) == NULL) {
		LOG4(L_TRANS,"(%5d) copyrlist: calloc(%d,%d) fails\n",
			Logstamp,i+1,sizeof(struct res_rec *));
		return(0);
	}

	for (i=0; source[i] != NULL; i++)
		(*target)[i] = source[i];

	(*target)[i] = NULL;

	return(i);
}
/*
 *
 *	getars gets additional records that match the data part of
 *	records passed in rlist.  The only kind of additional
 *	records retrieved are of type A, which are retrieved
 *	for NSTYPE, and RN type records.  If the A record is
 *	not found or the record in rlist is not of the right
 *	type, a record of type NULLREC is put in the corresponding
 *	place in arlist.  The result is a list with each record
 *	corresponding to the record in the same position in rlist.
 *	If all records that would be put into arlist are of type
 *	NULLREC, no records are returned.
 *	getars returns the number of records in arlist, which
 *	should be 0 or count.
 *
 */
int
getars(rlist,arlist,count)
struct res_rec	**rlist;
struct res_rec	***arlist;
int	count;
{
	static struct res_rec nullrec;
	int	i;
	int	tcount=0;
	struct res_rec	**tlist;

	LOG2(L_TRANS,"(%5d) getars: enter\n",Logstamp);

	if (count == 0) return(0);

	/* set up nullrec	*/
	strcpy(nullrec.rr_name,"NULL");
	nullrec.rr_type = NULLREC;
	nullrec.rr_data = NULL;

	if (((*arlist) = (struct res_rec **) calloc(count+1,
			sizeof(struct res_rec *))) == NULL) {
		LOG4(L_TRANS,"(%5d) getars: can't calloc(%d,%d)\n",
			Logstamp,count,sizeof(struct res_rec *));
		return(0);
	}

	for (i=0; i < count; i++) {
		switch (rlist[i]->rr_type&MASKNS) {
		case NSTYPE:
			if ((tlist = findrr(rlist[i]->rr_ns,A)) != NULL &&
			    (*tlist)->rr_type == A) {
				tcount++;
				(*arlist)[i] = *tlist;
				free(tlist);
			}
			else {
				(*arlist)[i] = duprec(&nullrec);
			}
			break;
		case RN:
			if ((tlist = findrr(rlist[i]->rr_owner,A)) != NULL &&
			    (*tlist)->rr_type == A) {
				tcount++;
				(*arlist)[i] = *tlist;
				free(tlist);
			}
			else {
				(*arlist)[i] = duprec(&nullrec);
			}
			break;
		default:
			(*arlist)[i] = duprec(&nullrec);
		}
	}
	if (tcount == 0) {
		free(*arlist);
		*arlist = NULL;
	}
	else
		(*arlist)[count] = NULL;

	LOG4(L_TRANS,"(%5d) getars: returns %d records, %d found\n",
		Logstamp,(tcount)?count:0, tcount);
	return((tcount)?count:0);
}

/*
 *	Verify the password for machine "mach" in domain "dom" matches
 *	the password in the password file.
 */
vpasswd(passwd, mach, dom)
char  *passwd;
char  *mach;
char  *dom;
{
	char filename[BUFSIZ];
	char buf[BUFSIZ];
	char *pw;
	char *m_name, *m_pass, *ptr;
	FILE *fp;
	int  no_pw = 0;

	char *crypt();

	LOG5(L_TRANS,"(%5d) vpasswd: passwd=%s, mach=%s, dom=%s\n",
		Logstamp,passwd,mach,dom);

	sprintf(filename, DOMPASSWD, dom);

	if ((fp = fopen(filename, "r")) != NULL) {
		while(fgets(buf, BUFSIZ, fp) != NULL) {
			if (buf[strlen(buf)-1] == '\n')
				buf[strlen(buf)-1] = '\0';
			m_name = ptr = buf;
			while (*ptr != ':' && *ptr != '\0')
				ptr ++;
			if (*ptr == '\0')
				no_pw = 1;
			else {
				no_pw = 0;
				*ptr = '\0';
			}
			if (strcmp(mach, m_name) == 0) {
				m_pass = no_pw? ptr: ++ptr;
				while (*ptr != ':' && *ptr != '\0')
					ptr ++;
				*ptr = '\0';
				pw = crypt(passwd, m_pass);
				fclose(fp);
				if (strcmp(pw, m_pass) == 0)
					return(-1);  /* correct */
				else
					return(-2);  /* incorrect */
			}
		}
		fclose(fp);
	} else
		return(R_EPASS);

	/*
	 *   Get here is no entry in password file.
	 */
	return(R_NOPW);
}

changepw(newpass, mach, dom)
char *newpass, *mach, *dom;
{
	unsigned sleep();
	char filename[BUFSIZ];
	char tempfile[BUFSIZ];
	char buf[BUFSIZ];
	char newbuf[BUFSIZ];
	int i;
	int ret;
	int colon = 0;
	char *m_name, *ptr;
	FILE *fp_t;
	FILE *fp;

	LOG5(L_TRANS,"(%5d) changepw: newpass=%s, mach=%s, dom=%s\n",
		Logstamp,newpass,mach,dom);

	sprintf(filename, DOMPASSWD, dom);

	strcpy(tempfile, filename);
	strcat(tempfile, ".t");

	i = 0;
	while (access(tempfile, 0) >= 0) {
		sleep(20);
		if (i++ > 5)
			return(R_EPASS);
	}

	if ((fp_t = fopen(tempfile, "w")) == NULL) {
		unlink(tempfile);
		return(R_EPASS);
	}

	ret = R_NOPW;
	if ((fp = fopen(filename, "r")) != NULL) {
		while(fgets(buf, 512, fp) != NULL) {
			if (buf[strlen(buf)-1] == '\n')
				buf[strlen(buf)-1] = '\0';
			m_name = ptr = buf;
			while (*ptr != ':' && *ptr != '\0')
				ptr ++;
			if (*ptr == ':') {
				colon = 1;
				*ptr = '\0';
			} else {
				colon = 0;
			}

			if (strcmp(mach, m_name) != 0)
				*ptr = colon? ':' : '\0';
			else {
				ret = R_NOERR;
				strcpy(newbuf, mach);
				strcat(newbuf, ":");
				strcat(newbuf, newpass);
				if (colon) ++ptr;
				while (*ptr != ':' && *ptr != '\0')
					ptr ++;
				strcat(newbuf, ptr);
				strcpy(buf, newbuf);
			}
			fputs(buf, fp_t);
			fputs("\n", fp_t);
		}
		fclose(fp);
		fclose(fp_t);

		if (unlink(filename) || link(tempfile, filename) || unlink(tempfile))
			return(R_EPASS);
	}
	return(ret);
}
int
ns_unadv(name, type, dname)
char	*name;
int	type;
char	*dname;
{
	int	ret;
	struct	res_rec	**rlist;

	if ((rlist = findrr(name,type)) == NULL) {
		ret = R_NONAME;
		LOG4(L_TRANS,"(%5d) nsfunc: %s, no such name %s\n",
			Logstamp,fntype(NS_UNADV),name);
	}
	else if ((Primary && strcmp(Dname,dname) == NULL) ||
	          strcmp(rlist[0]->rr_owner,dname) == NULL) {
		ret = remrr(name,rlist[0]);
	}
	else {
		ret = R_PERM;
		LOG4(L_TRANS,"(%5d) nsfunc: %s, no permission to unadv %s\n",
			Logstamp, dname, name);
	}
	freelist(rlist);

	return(ret);
}
int
ns_adv(name, rec, addr)
char	*name;
struct res_rec	*rec;
struct address	*addr;
{
	int	ret;
	struct res_rec	*arec;
	struct res_rec	**rlist;

	ret = addrr(name,rec);
	/* save address for future use */
	if ((rlist=findrr(rec->rr_owner,A)) == NULL) {
	    if ((arec=(struct res_rec *) calloc(sizeof(struct res_rec),1)) == NULL) {
		    PLOG3("(%5d) nsfunc: calloc(%d,1) FAILED\n",
			Logstamp,sizeof(struct res_rec));
		    return(R_NSFAIL); /* give up */
	    }
	    if (addr && (arec->rr_a=aatos((struct address *)NULL,addr,HEX)) != NULL) {
		    arec->rr_type = A;
		    strncpy(arec->rr_name,
			namepart(rec->rr_owner),NAMSIZ);
		    addrr(rec->rr_owner,arec);
	    }
	    freerec(arec);
	}
	else
	    freelist(rlist);

	return(ret);
}
struct request *
newreq(opcode,flags,dname)
int	opcode;
int	flags;
char	*dname;
{
	struct header	*nhp;
	struct request	*nreq;

	if ((nreq = (struct request *)calloc(1,sizeof(struct request))) == NULL) {
		PLOG3("(%5d) newreq: calloc(1,%d) FAILED\n",
			Logstamp,sizeof(struct request));
		return(NULL);
	}
	if ((nhp = (struct header *)calloc(1,sizeof(struct header))) == NULL) {
		PLOG3("(%5d) newreq: calloc(1,%d) FAILED\n",
			Logstamp,sizeof(struct header));
		return(NULL);
	}
	nreq->rq_head = nhp;
	nhp->h_opcode = opcode;
	nhp->h_flags = flags;
	nhp->h_qdcnt = 0;
	nhp->h_ancnt = 0;
	nhp->h_nscnt = 0;
	nhp->h_arcnt = 0;
	nhp->h_dname = copystr(dname);
	return(nreq);
}
