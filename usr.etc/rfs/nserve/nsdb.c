/*      @(#)nsdb.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nserve:nsdb.c	1.10"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "nslog.h"
#include "nsdb.h"
#include "stdns.h"
#include <rfs/nserve.h>
#include "nsrec.h"
#include <tiuser.h>
#include <rfs/nsaddr.h>
/*
 *
 *	This file contains the core of the Name Server
 *	database.  From the outside, the database is a
 *	group of resource records that may be accessed
 *	through a set of access functions defined here.
 *
 *	The header file nsdb.h should be included by any
 *	program that uses these access functions.  It
 *	defines the functions and structures that are
 *	used by the database.
 *
 *	
 */

#define ISIZE 4		/* distance to indent when dumping database	*/

/* static variables */
static struct domain  Dstruct;		/* root of database	*/
static struct domain  *Domain = &Dstruct;

static struct {
	char	*name;
	int	type;
} *Tptr, Table[] = {
	"P",	SOA,
	"SOA",	SOA,
	"S",	NS,
	"NS",	NS,
	"DOM",	DOM,
	"RN",	RN,
	"A",	A,
	"NSTYPE",  NSTYPE,
	"ANYTYPE", ANYTYPE,
	"ANY",	ANYTYPE,
	"*",	ANYTYPE,
	NULL,0
};

static int	Indent;	/* for dumping domains	*/

/* static functions */
static struct res_rec	**findrec();
static int		addrec();
static struct domain	*finddom();
static struct domain	*newdom();
static struct res_rec	**findns();
static int		freedom();
struct res_rec	**duplist();
struct question	**dupqd();
struct res_rec	*duprec();

int	writedb();
int	readfile();
char	*prec();
char	*getctype();
char	*dompart();
char	*namepart();

/*
 * Find resource records that match name and type.
 * Return pointer to list or NULL.
 */
struct res_rec **
findrr(name, type)
char	*name;
int	type;
{
	struct res_rec	**list;
	char	*r;
	struct domain	*d;
	struct domain	*d1 = NULL;

	LOG4(L_DB,"(%5d) findrr: name=%s, type=%s\n",
		Logstamp,name,getctype(type));

	d = Domain;
	r = dompart(name);

	if ((d1=finddom(r,d)) == NULL) {
		LOG3(L_DB,"(%5d) findrr: can't find domain '%s'\n",Logstamp,r);
		return(NULL);
	}

	if ((list = findrec(namepart(name),type,d1)) ==  NULL)
		LOG4(L_DB,"(%5d) findrr: search failed, name = '%s', type =%d\n",
			Logstamp,name,type);

	return(duplist(list));
}
/*
 * Add a record in the database using name as key.
 * Return R_NOERR if the operation is successful,
 * otherwise return an appropriate rcode value (see stdns.h)
 * Addrr copies the record before adding it to the database.
 */
int
addrr(name,res)
char	*name;
struct res_rec	*res;
{
	int	ret;
	if ((ret = addraw(name,duprec(res))) == R_NOERR)
		return(R_NOERR);

	return(ret);
}
/*
 * addraw adds a record to the database without first copying it
 */
static int
addraw(name,res)
char	*name;
struct res_rec	*res;
{
	struct domain	*d;
	struct domain	*d1;
	char	*r;

	LOG4(L_DB,"(%5d) addrr: name=%s, res=%s\n",
		Logstamp,name,prec(res));
	d = Domain;
	r = dompart(name);

	if (*r == WILDCARD) {
		LOG2(L_DB,"(%5d) addrr: can't add wildcard\n",Logstamp);
		return(R_FORMAT);
	}

	if ((d1=finddom(r,d)) == NULL) {
		LOG3(L_DB,"(%5d) addrr: can't find domain '%s'\n",
			Logstamp,r);
		return(R_NONAME);
	}

	return(addrec(namepart(name),res,d1));
}
/*
 * put record res in domain dom under name name.
 */
static int
addrec(name,res,dom)
char	*name;
struct res_rec	*res;
struct domain	*dom;
{
	int	type;
	int	i;

	type = res->rr_type;
	LOG4(L_DB,"(%5d) addrec: name=%s, res=%s\n",
		Logstamp,name,prec(res));

	if (findrec(namepart(name),type,dom) != NULL) {
		LOG4(L_DB,"(%5d) addrec: DUPLICATE NAME='%s', TYPE='%s'\n",
			Logstamp, name, getctype(type));
		return(R_DUP);
	}
	for(i=0;1;i++) {
		if (i+1 >= dom->d_size) {
			dom->d_size += NREC;
			if (dom->d_rec) {
			    free(dom->d_rec);
			    if ((dom->d_rec=(struct res_rec **)realloc(dom->d_rec,
				 sizeof(struct res_rec *) * dom->d_size)) == NULL) {
					PLOG4(
					  "(%5d) addrec: can't realloc(%s,%d)\n",
					  Logstamp,"dom->rec",
					  sizeof(struct res_rec *)*dom->d_size);
					return(R_NSFAIL);
			    }
			}
			else {
			    if ((dom->d_rec=(struct res_rec **)
				 calloc(dom->d_size,sizeof(struct res_rec *)))
				    == NULL) {
					PLOG4(
					  "(%5d) addrec: can't calloc(%d,%d)\n",
					  Logstamp,dom->d_size,
					  sizeof(struct res_rec *));
					return(R_NSFAIL);
			    }
			}
		}
		if (dom->d_rec[i] == NULL)
			break;	/* found */
	}
	dom->d_rec[i] = res;
	dom->d_rec[i+1] = NULL;
	LOG2(L_DB,"(%5d) addrec: returns R_NOERR\n",Logstamp);
	return(R_NOERR);
}
/*
 * findrec takes a domain ptr, a name, and a type and
 * returns all records in domain that match name and type.
 * Name is an unqualified (i.e., no dots) domain or resource name.
 *
 * if name is "*", all names are matched.
 * if type is ANYTYPE, all types are matched.
 * if type is NSTYPE, all name server types are matched.
 *
 * findrec returns a NULL terminated list of pointers
 * to struct res_rec if any records are found.  If none
 * are found, it returns NULL.
 *
 * findrec puts the list that points to its result in static storage,
 * therefore, subsequent calls will destroy previous results.
 */

static struct res_rec	**
findrec(name,type,d)
char	*name;
int	type;
struct domain	*d;
{
	static	int rrsize = 0;
	static	struct res_rec	**rrlist = NULL;
	int	trrec = 0;
	int	i;
	int	nstype;
	struct res_rec *rp;

	LOG5(L_DB,"(%5d) findrec: name=%s, type=%s, domain=%d\n",
		Logstamp,name,getctype(type),d);

	nstype = (type == NSTYPE)?NSTYPE:0;

	if (d->d_rec == NULL)
		return(NULL);

	if (rrsize < d->d_size)
		/* expand rrlist */
		if (!(rrlist = (rrlist == NULL) ?
			(struct res_rec **)malloc((rrsize = d->d_size) * sizeof(struct res_rec *)) :
			(struct res_rec **)realloc(rrlist, (rrsize = d->d_size) * sizeof(struct res_rec *))))
			return NULL;

	for (i=0, rp=d->d_rec[0]; i < d->d_size && rp != NULL; rp=d->d_rec[++i])
		if ((type == ANYTYPE || type == rp->rr_type || nstype & rp->rr_type) &&
		    (*name == WILDCARD || strcmp(name,rp->rr_name) == NULL))
			rrlist[trrec++] = rp;

	LOG3(L_DB,"(%5d) findrec: returns %d records\n",
		Logstamp,trrec);

	if (trrec > 0) {
		rrlist[trrec] = NULL;
		return(rrlist);
	}

	return(NULL);
}
/*
 *
 * finddom searches the database from the given
 * root and returns a pointer to the domain structure
 * that represents the argument "name."
 *
 * finddom returns NULL if there is an error.
 * errors include:
 * 	given name is not a domain.
 * 	can't find name.
 *
 */
static struct domain *
finddom(name,domain)
char	*name;
struct domain *domain;
{
	char	*r;
	char	sname[BUFSIZ];
	struct domain	*d;
	struct res_rec	**rec;
	char	*rtoken();

	LOG4(L_DB,"(%5d) finddom: name=%s, domain=%d\n",
		Logstamp, (name)?name:"NULL",domain);

	if (name == NULL)
		return(NULL);

	/* null name means this is the correct domain already */
	if (*name == '\0')
		return(domain);

	strcpy(sname,name);

	for (r=rtoken(sname), d=domain; r != NULL; r=rtoken(NULL))
		if ((rec=findrec(r,DOM,d)) != NULL)
			d = rec[0]->rr_dom;
		else
			return(NULL);

	return(d);
}
/*
 * find list of name servers that should know about this
 * name.  Uses findns, starting from the root of the database.
 */
struct res_rec **
findind(name)
char	*name;
{
	return(duplist(findns(name,Domain)));
}
/*
 *
 * findns searches the database for a reference to a
 * remote name servers anywhere in the given name.
 * It returns a list of pointers to the MOST qualified
 * name servers found. (i.e., if SOA or NS records are found
 * for b.c.e and b.c, the reference to b.c.e will be returned).
 *
 * findns returns NULL if there is an error or if
 * no name server records are found.
 *
 */
static struct res_rec **
findns(name,domain)
char	*name;
struct domain *domain;
{
	static	int reclen = 0;	
	int	i, tot;
	char	*r;
	char	sname[BUFSIZ];
	struct domain	*d;
	
	struct res_rec	**trec=NULL;
	struct res_rec	**recptr=NULL;
	static struct res_rec	**rec=NULL;
	char	*rtoken();

	LOG4(L_DB,"(%5d) findns: name=%s, domain=%d\n",
		Logstamp,(name)?name:"NULL",domain);

	if (name == NULL || domain == NULL)
		return(NULL);

	strcpy(sname,name);

	for (r=rtoken(sname), d=domain; r != NULL; r=rtoken(NULL)) {
		if ((trec=findrec(r,NSTYPE,d)) != NULL) {
			for (i=0; trec[i] != NULL; i++);
			if (i >= reclen) {
				reclen = i + 1;
				rec = (rec == NULL) ?
				(struct res_rec **)malloc((i + 1) * sizeof(struct res_rec *)) :
				(struct res_rec **)realloc(rec, (i + 1) * sizeof(struct res_rec *));
			}
			tot = i;
			for (;i >= 0; --i)
				rec[i] = trec[i];
			recptr = rec;
		}
		if ((trec=findrec(r,DOM,d)) != NULL)
			d = trec[0]->rr_dom;
		else
			break;
	}

	LOG3(L_DB,"(%5d) findns: returns %d records\n",
		Logstamp,(recptr)?tot:0);

	return(recptr);
}
/*
 * dump entire database to stdout.
 */
int
dumpdb()
{
	struct domain	*d;

	Indent = 0;
	d = Domain;
	dumpdom(d,"");
	return;
}
/*
 * dump the contents of a domain and all of its
 * subdomains to stdout.
 */
int
dumpdom(d,name)
struct domain	*d;
char	*name;	/* name so far	*/
{
	struct res_rec	**list;
	struct res_rec	*rec;
	char	dname[MAXDNAME];

	if (d == NULL || d->d_rec == NULL)
		return;

	for (list=d->d_rec; *list != NULL; list++) {
		rec = *list;
#ifdef RIGHT_TO_LEFT
		sprintf(dname,"%s.%s",rec->rr_name,name);
#else
		sprintf(dname,"%s.%s",name,rec->rr_name);
#endif
		if (*name == '\0')
			dname[strlen(dname)-1] = '\0';
		printf("%*s%s\t%s",Indent,"",dname,getctype(rec->rr_type));
		switch (rec->rr_type) {
		case RN:
			printf("\t%s\n",rec->rr_owner);
			break;
		case DOM:
			printf(", auth=%d, size=%d\n",rec->rr_dauth,rec->rr_dsize);
			Indent += ISIZE;
			dumpdom(rec->rr_dom,dname);
			Indent -= ISIZE;
			break;
		default:
			printf("\t%s\n",rec->rr_data);
			break;
		}
	}
	return;
}
/*
 *  remrr removes the resource record that matches
 *  the name and type passed in.  WILDCARDS are illegal.
 *
 *  It returns R_NOERR if the operation is successful,
 *  otherwise, it returns an rcode (see stdns.h).
 */
int
remrr(name,res)
char	*name;
struct res_rec	*res;
{
	char	*r;
	struct domain	*d;
	struct domain	*d1;
	int	i,j;
	struct res_rec	*rptr;

	LOG4(L_DB,"(%5d) remrr: name=%s, res=%s\n",
		Logstamp,name,prec(res));

	d1 = Domain;
	r = dompart(name);

	if ((d=finddom(r,d1)) == NULL) {
		LOG3(L_DB,"(%5d) remrr: can't find domain '%s'\n",
			Logstamp,r);
		return(R_NONAME);
	}

	r = namepart(name);
	for (i=0, rptr=d->d_rec[0]; i < d->d_size && rptr != NULL;
	     rptr = d->d_rec[++i])
		if ((strcmp(r,rptr->rr_name) == NULL) &&
		    (res->rr_type == rptr->rr_type)) {
			for (j=i; j+1 < d->d_size && d->d_rec[j] != NULL; j++)
				d->d_rec[j] = d->d_rec[j+1];
			d->d_rec[j] = NULL;
			freerec(rptr);
			return(R_NOERR);
		}
	return(R_NONAME);
}
/*
 * Inverse query.  Search for resource(s) given domain, rdata, and type.
 * The argument name is matched as a character string with the data portion
 * of records of all types except RN, where name is matched with rr_owner.
 * Since DOM records are internal, an inverse query for DOM records will
 * return NULL.
 */
struct res_rec **
iquery(domain, type, name)	/* find rrs where rdata matches name		*/
char	*domain;
int	type;
char	*name;
{
	static int reclen = 0;
	struct res_rec	**list;
	int	i,j;
	char	*str;
	static struct res_rec	**rlist;
	char	*dompart();
	char	*namepart();
	char	qname[2];
	struct domain	*d;
	struct domain	*d1 = NULL;

	LOG5(L_DB,"(%5d) iquery: domain=%s, type=%s, name=%s\n",
		Logstamp,domain,getctype(type),name);

	d = Domain;

	if ((d1=finddom(domain,d)) == NULL) {
		if ((list=findns(domain,d)) == NULL)
			LOG3(L_DB,"(%5d) iquery: can't find domain '%s'\n",
				Logstamp,domain);
		return(duplist(list));
	}

	/* get all records with the right type regardless of name.	*/
	qname[0] = WILDCARD;
	qname[1] = '\0';

	if ((list = findrec(qname,type,d1)) ==  NULL) {
		LOG4(L_DB,"(%5d) iquery: search failed, name = '%s', type =%d\n",
			Logstamp,qname,type);
		return(NULL);
	}

	for (i=0, j=0; list[i] != NULL; i++) {
		LOG3(L_DB,"(%5d) iquery: checks record %s\n",
			Logstamp,prec(list[i]));
		switch (type) {
		case DOM:
			return(NULL);
		case RN:
			str = list[i]->rr_owner;
			break;
		default:
			str = list[i]->rr_data;
			break;
		}
		if ((str == NULL) || (strcmp(str,name) != NULL))
			continue;

		LOG2(L_DB,"(%5d) iquery: record matches\n",Logstamp);
		if (j >= reclen)
			rlist = (rlist == NULL) ?
				(struct res_rec **)malloc((1+ ++reclen) * sizeof(struct res_rec *)) :
				(struct res_rec **)realloc(rlist, (1+ ++reclen) * sizeof(struct res_rec *));
		rlist[j++] = list[i];
	}

	LOG3(L_DB,"(%5d) iquery: returns %d records\n",Logstamp,j);

	if (j > 0) {
		rlist[j] = 0;
		return(duplist(rlist));
	}
	return(NULL);
}
char *
prec(rec)
struct res_rec *rec;
{
	static	char	buf[BUFSIZ];
	char	*ptr;

	sprintf(buf,"%s %s",rec->rr_name,getctype(rec->rr_type));
	ptr = &(buf[strlen(buf)]);

	switch (rec->rr_type) {
	default:
		sprintf(ptr," %s",rec->rr_data);
		break;
	case DOM:
		sprintf(ptr," auth=%d size=%d",rec->rr_dauth,rec->rr_dsize);
		break;
	case RN:
		sprintf(ptr," %s",rec->rr_owner);
		break;
	}
	return(buf);
}
/*
 * convert string representation of a record type into
 * its internal numeric value.
 */
int
gettype(nm)
char	*nm;
{
	char	type[10];
	int	i;

	for (i=0; i < 10 && *nm != NULL; i++, nm++)
		type[i] = toupper(*nm);

	type[i] = '\0';

	for (Tptr = Table; Tptr->name != NULL; Tptr++)
		if (strcmp(Tptr->name,type) == NULL) {
			LOG4(L_DB,"(%5d) gettype: nm='%s', returns %d\n",
				Logstamp,type,Tptr->type);
			return(Tptr->type);
		}

	LOG4(L_DB,"(%5d) gettype: nm='%s', NOT FOUND returns %d\n",
		Logstamp,type,NULL);
	return(NULL);
}
/*
 * convert numeric record type into character string
 * NOTE: all NSTYPES are forced to fold into either SOA or NS.
 */
char *
getctype(num)
int num;
{
	if (((num & MASKNS) == NSTYPE) && num != SOA)
		num = NS;

	for (Tptr = Table; Tptr->name != NULL; Tptr++)
		if (Tptr->type == num)
			return((Tptr->name)?Tptr->name:"NULL");
	return(" NULL");
}
/*
 * writedb writes the entire database to a file, returning
 * the number of lines written.
 */
int
writedb(file)
char	*file;
{
	struct domain	*d;
	FILE	*fp;
	int	ret;

	if ((fp = fopen(file,"w")) == NULL) {
		perror(file);
		return(0);
	}
	d = Domain;
	ret = wrdom(d,"",fp);
	fclose(fp);
	return(ret);
}
/*
 * recursively dump the contents of a domain
 * to a file.  The format is the same as used
 * in the rfmaster file and can be read by readfile.
 */
static int
wrdom(d,name,fp)
struct domain	*d;
char	*name;	/* name so far	*/
FILE	*fp;
{
	struct res_rec	**list;
	struct res_rec	*rec;
	char	dname[64];
	int	type;
	char	*ctype;
	int	i;

	if (d == NULL || d->d_rec == NULL)
		return(0);

	for (i=0,list=d->d_rec; *list != NULL; list++,i++) {
		rec = *list;
#ifdef RIGHT_TO_LEFT
		sprintf(dname,"%s.%s",rec->rr_name,name);
		if (*name == '\0')
			dname[strlen(dname)-1] = '\0';
#else
		if (*name == '\0')
			strcpy(dname,rec->rr_name);
		else
			sprintf(dname,"%s.%s",name,rec->rr_name);
#endif
		type = rec->rr_type;

		if (strcmp((ctype=getctype(type)),"NULL") == 0)
			continue;

		switch (type) {
		case RN:
			fprintf(fp,"%s %s %s %s %s %s\n",dname,ctype,
				rec->rr_owner,
				(rec->rr_flag == A_RDONLY)?"RO":"RW",
				rec->rr_path, rec->rr_desc);
			break;
		case DOM:
			i += wrdom(rec->rr_dom,dname,fp);
			break;
		default:
			fprintf(fp,"%s %s %s\n",dname,ctype,
				(rec->rr_data)?(rec->rr_data):"NULL");
		}
	}
	return(i);
}
/*
 * read a file in rfmaster format and update the database accordingly.
 * If clflg is set, the current database will be cleared first, otherwise
 * the contents of the file will be merged on top of the current database.
 * If override is set, a record read in from the file will replace any
 * matching record already in the database (even if that record has just
 * been read in from the same file), otherwise, duplicate records are
 * discarded.  readfile returns the number of valid records read,
 * or -1 if it cannot access the file.
 */
int
readfile(file,clflg,override)
char	*file;
int	clflg;
int	override;
{
	FILE	*fp;
	char	buf[BUFSIZ];
	int	i, j, k;
	int	ret;
	int	itype;
	int	found = FALSE;
	struct res_rec	*res;
	struct res_rec	*tres;
	struct res_rec	**tlist;
	struct domain	*d;
	int	auth;
	char	*name;
	char	*type;
	char	*rdata;

	LOG4(L_OVER,"%s: read file '%s'%s\n",(clflg)?"readdb":"merge",file,
		(override)?", override previous values":"");

	if ((fp = fopen(file,"r")) == NULL)
		return(-1);

	if (clflg)
		freedom(Domain);

	for (i=1, j=0; fgets(buf,BUFSIZ,fp) != NULL; i++, fflush(Logfd)) {
		/* ignore comment lines	*/
		if (*buf == '#') continue;
		/* handle lines ending with backslashes	*/
		while (buf[strlen(buf)-2] == '\\' &&
			fgets(&buf[strlen(buf)-2],BUFSIZ-strlen(buf),fp))
				i++;
		buf[strlen(buf)-1] = '\0';	/* get rid of newline */

		LOG3(L_DB|L_OVER,"readfile: line %d: %s\n",i,buf);

		/* ignore blank lines	*/
		if ((name = strtok(buf,WHITESP)) == NULL) 
			continue;

		if ((type = strtok(NULL,WHITESP)) == NULL) {
			PLOG4("Warning: file %s, line %d incomplete '%s'\n",
				file,i,buf);
			continue;
		}
		if ((itype = gettype(type)) == NULL) {
			PLOG4("Warning: file %s, line %d, unknown type = '%s'\n",
				file,i,type);
			continue;
		}
		rdata = type + 2;
		rdata += strspn(rdata,WHITESP);
		if ((rdata = strtok(rdata, "\n")) == NULL) {
			PLOG4("Warning: file %s, line %d incomplete '%s'\n",
			    file,i,buf);
			continue;
		}

		if (!islegal(name,(itype==A)?SZ_MACH:SZ_RES)) {
			PLOG4("Warning: file %s, line %d, illegal name %s\n",
				file,i,name);
			continue;
		}
		res = (struct res_rec *) calloc(1,sizeof(struct res_rec));
		strncpy(res->rr_name,namepart(name),NAMSIZ);
		res->rr_type = itype;

		switch(res->rr_type & MASKNS) {
		case NSTYPE:
			if (!islegal(rdata,SZ_MACH)) {
				PLOG4("Warning: file %s, line %d, illegal name %s\n",
					file,i,rdata);
				free(res);
				continue;
			}

			if (strcmp(Dname,rdata) == NULL)
				auth = TRUE;
			else
				auth = FALSE;

			if ((d=newdom(name)) == NULL) {
				PLOG4("Warning: file %s, line %d bad domain %s\n",
					file,i,name);
				free(res);
				continue;
			}
			/* set authorization of domain	*/
			if (auth)
				setauth(d,res->rr_type);

			res->rr_ns = copystr(rdata);
			break;
		case A:
			if (newdom(dompart(name)) == NULL) {
				PLOG4("Warning: file %s, line %d bad domain %s\n",
					file,i,name);
				free(res);
				continue;
			}
			res->rr_a = copystr(rdata);
			break;
		case DOM:
			res->rr_dom = (struct domain *)calloc(1,sizeof(struct domain));
			res->rr_dauth = atoi(rdata);
			res->rr_dsize = 0;
			break;
		case RN:
			if (newdom(dompart(name)) == NULL) {
				PLOG4("Warning: file %s, line %d bad domain %s\n",
					file,i,name);
				free(res);
				continue;
			}
			if ((rdata = strtok(rdata,WHITESP)) == NULL) {
				PLOG4("Warning: file %s, line %d incomplete '%s'\n",
					file,i,buf);
				free(res);
				continue;
			}

			res->rr_rn = (struct rn *) calloc(1,sizeof(struct rn));
			res->rr_owner = copystr(rdata);

			/* the rest of these are optional	*/
			if ((rdata = strtok(NULL,WHITESP)) == NULL)
				break;
			if (strcmp(rdata,"RW") == NULL)
				res->rr_flag = 0;
			else
				res->rr_flag = 1;
			if ((rdata = strtok(NULL,WHITESP)) == NULL)
				break;
			res->rr_path = copystr(rdata);
			if ((rdata = strtok(NULL,"\n")) == NULL)
				break;
			res->rr_desc = copystr(rdata);
			break;
		default:
			if (newdom(dompart(name)) == NULL) {
				PLOG4("Warning: file %s, line %d bad domain %s\n",
					file,i,name);
				free(res);
				continue;
			}
			res->rr_data = copystr(rdata);
			break;
		}

		if (override && res->rr_type != DOM)
			remrr(name,res);

		if ((res->rr_type & MASKNS) == NSTYPE) {
			if ((tlist = iquery(dompart(name),NSTYPE,res->rr_data))
			     != NULL) {
			    found = FALSE;
			    for (k=0; tlist[k]; k++) {
				LOG4(L_COMM,"(%5d) readfile: check %s and %s\n",
					Logstamp, namepart(name),tlist[k]->rr_name);
				if (!strcmp(namepart(name),tlist[k]->rr_name)) {
				    found = TRUE;
				    break;
				}
			    }
			    freelist(tlist);
			    if (found) {
				free(res);
				continue;
			    }
			}
			if ((ret=addraw(name,res)) == R_DUP && res->rr_type != SOA) {
				for (k=res->rr_type&~MASKNS; k < MAXNS; k++) {
					res->rr_type++;
					if ((ret=addraw(name,res)) == R_NOERR)
						break;
				}
			}
			if (ret != R_NOERR) {
				free(res);
				continue;
			}
		}
		else if ((ret=addraw(name,res)) != R_NOERR) {
		        free(res);
		        continue;
		}
		j++;
	}
	fclose(fp);
	return(j);
}
/*
 * free contents of a domain.
 */
int
cleardom(dname)
char	*dname;
{
	struct domain	*d;

	if ((d = finddom(dname,Domain)) == NULL) {
		LOG3(L_DB,"(%5d) cleardom: can't find domain %s\n",
			Logstamp, dname);
	}
	else
		freedom(d);
}
/*
 * free everything below this domain.
 */
static int
freedom(d)
struct domain	*d;
{
	int	i;
	struct res_rec	*rec;

	for (i=0; i < d->d_size && d->d_rec[i] != NULL; i++) {
		rec = d->d_rec[i];
		freerec(rec);
		d->d_rec[i] = NULL;
	}
}
/*
 * free a list of resource records, including the list itself.
 */
int
freelist(list)
struct res_rec	**list;
{
	register int	i;

	if (!list)
		return;

	for (i=0; list[i]; i++)
		freerec(list[i]);

	free(list);
	return;
}
/*
 * free this record, if record is a domain, recursively free
 * free everything under it.
 */
int
freerec(rec)
struct res_rec *rec;
{
		switch(rec->rr_type) {
		case RN:
			if (rec->rr_rn) {
				if (rec->rr_owner)
					free(rec->rr_owner);
				if (rec->rr_desc)
					free(rec->rr_desc);
				if (rec->rr_path)
					free(rec->rr_path);
				free(rec->rr_rn);
			}
			break;
		case DOM:
			freedom(rec->rr_dom);
			if (rec->rr_drec)
				free(rec->rr_drec);
			if (rec->rr_dom)
				free(rec->rr_dom);
			break;
		default:
			if (rec->rr_data)
				free(rec->rr_data);
			break;
		}
		free(rec);
}
/*
 * domauth returns the value of the d_auth field for the
 * domain "name."  This value is presumed to be SOA or
 * NS if this name server has authority over this domain.
 * Otherwise, it is 0.  If the domain cannot be found,
 * the result is also 0. (Can't be an authority over
 * a domain you don't have.)
 */

int
domauth(name)
char	*name;
{
	struct domain	*d;

	if ((d = finddom(name,Domain)) == NULL) {
		LOG2(L_DB,"(%5d) domauth: returns 0\n",Logstamp);
		return(0);
	}

	LOG3(L_DB,"(%5d) domauth: returns %d\n",Logstamp,d->d_auth);
	return(d->d_auth);
}
/*
 * findmydoms searches the database looking for domains that
 * this machine is a name server for.  It fills in the
 * array Mydomains with the names of these domains.
 * findmydoms returns the number of domains it found.
 */
int
findmydoms()
{
	LOG3(L_COMM,"(%5d) findmydoms: enter, Dname = %s\n",
		Logstamp, Dname);
	Mylastdom = 0;
	rdfind(Domain,"");
	return(Mylastdom);
}
static int
rdfind(d,name)
struct domain	*d;
char		*name;
{
	int	i;
	char	*fmt;
	struct res_rec	*rec;
	char	nbuf[MAXDNAME];

	LOG3(L_COMM,"(%5d) findmydoms: rdfind enter, name = '%s'\n",
		Logstamp, name);

	fmt = (*name)?"%s.%s":"%s%s";

	for (i=0, rec = d->d_rec[0]; i < d->d_size && rec; rec = d->d_rec[++i]) {
#ifdef RIGHT_TO_LEFT
		sprintf(nbuf,fmt,rec->rr_name,name);
#else
		sprintf(nbuf,fmt,name,rec->rr_name);
#endif
		switch (rec->rr_type & MASKNS) {
		case DOM:
			rdfind(rec->rr_dom,nbuf);
			break;
		case NSTYPE:
			LOG4(L_COMM,"(%5d) findmydoms: check dom %s, primary %s\n",
				Logstamp, name, rec->rr_data);

			if (strcmp(Dname,rec->rr_data))
				break;

			Mydomains[Mylastdom++] = copystr(nbuf);

			LOG2(L_OVER,"findmydoms: found domain %s\n",
				Mydomains[Mylastdom-1]);
			break;
		}
	}
	return;
}
/*
 * make a copy of a resource record.
 * do not recurse through DOM type records.
 */
struct res_rec *
duprec(rec)
struct res_rec	*rec;
{
	struct res_rec	*rrec;

	LOG3(L_CONV,"(%5d) duprec: enter with record %s\n",
		Logstamp,(rec)?prec(rec):"NULL RECORD");

	if (!rec)
		return((struct res_rec *) NULL);

	if ((rrec = (struct res_rec *) calloc(1,sizeof(struct res_rec))) == NULL) {
		PLOG2("(%5d) nsdb: calloc 1 failed in duprec\n",Logstamp);
		return((struct res_rec *) NULL);
	}

	strncpy(rrec->rr_name,rec->rr_name,NAMSIZ);
	rrec->rr_type = rec->rr_type;

	switch(rec->rr_type) {
	case RN:
		if ((rrec->rr_rn =
		    (struct rn *) calloc(1,sizeof(struct rn))) == NULL) {
			PLOG2("(%5d) nsdb: calloc 2 failed in duprec\n",Logstamp);
			free(rrec);
			return((struct res_rec *) NULL);
		}
		rrec->rr_owner = copystr(rec->rr_owner);
		rrec->rr_desc = copystr(rec->rr_desc);
		rrec->rr_path = copystr(rec->rr_path);
		rrec->rr_flag = rec->rr_flag;
		LOG3(L_CONV,"(%5d) duprec: copy record %s\n",Logstamp,prec(rrec));
		break;
	case DOM:
		if ((rrec->rr_dom =
		    (struct domain *) calloc(1,sizeof(struct domain))) == NULL) {
			PLOG2("(%5d) nsdb: calloc 3 failed in duprec\n",Logstamp);
			free(rrec);
			return((struct res_rec *) NULL);
		}
		*(rrec->rr_dom) = *(rec->rr_dom);
		rrec->rr_drec = NULL;
		rrec->rr_dsize = 0;
		LOG3(L_CONV,"(%5d) duprec: copy record %s\n",Logstamp,prec(rrec));
		break;
	default:
		rrec->rr_data = copystr(rec->rr_data);
		LOG3(L_CONV,"(%5d) duprec: copy record %s\n",Logstamp,prec(rrec));
		break;
	}
	return(rrec);
}
/*
 * copy the query section of a name server request.
 */
struct question **
dupqd(qd,count)
struct question **qd;
int	count;
{
	register int	i;
	struct question **rqd;

	LOG3(L_CONV,"(%5d) dupqd: copy question section %x\n",Logstamp,qd);

	if (!qd || count <= 0)
		return((struct question **) NULL);

	if ((rqd = (struct question **)
		calloc(count,sizeof(struct question *))) == NULL) {
		PLOG2("(%5d) nsdb: calloc in dupqd fails\n",
			Logstamp);
		return((struct question **) NULL);
	}
	for (i=0; i < count && qd[i]; i++) {
		rqd[i] = (struct question *) calloc(1,sizeof(struct question));
		rqd[i]->q_name = copystr(qd[i]->q_name);
		rqd[i]->q_type = qd[i]->q_type;
	}

	return(rqd);
}
/*
 * copy a list of resource records.
 */
struct res_rec **
duplist(list)
struct res_rec	**list;
{
	register int	i;
	int		count;
	struct res_rec	**rlist;

	LOG3(L_CONV,"(%5d) duplist: copy list %x\n",Logstamp,list);

	if (!list)
		return((struct res_rec **) NULL);

	for (i=0; list[i]; i++)
			;
	count = i;

	if ((rlist = (struct res_rec **)
		calloc(count+1,sizeof(struct res_rec *))) == NULL) {
		PLOG2("(%5d) nsdb: calloc in duplist fails\n",
			Logstamp);
		return((struct res_rec **) NULL);
	}

	for (i=0; i < count; i++)
		rlist[i] = duprec(list[i]);

	rlist[i] = NULL;
	return(rlist);
}
/*
 * check validity of a domain name.
 * return TRUE if name is valid.
 * otherwise return FALSE
 */
int
islegal(dname,size)
char	*dname;
int	size;
{
	int	len;
	int	sflag=FALSE;	/* set when possible error is found	*/
	int	i;
	char	dbuf[MAXDNAME];
	char	*p, *np=NULL;

	if ((len = strlen(dname)) > MAXDNAME || len <= 0 ||
	    *dname == SEPARATOR || dname[len-1] == SEPARATOR)
		return(FALSE);

	strcpy(dbuf,dname);
	p=namepart(dbuf);
	if (((size == SZ_RES)?v_resname(p):v_uname(p)) != 0 || strlen(p) > size)
		return(FALSE);

	for (p=rtoken(dbuf); p != NULL && *p != '\0'; p=rtoken(NULL)) {
		/* return false only when error is found in domain part of name */
		if (sflag)
			return(FALSE);
		if (v_dname(p) != 0)
			sflag = TRUE;
		else if (strlen(p) > SZ_DELEMENT)
			sflag = TRUE;
	}
	return(TRUE);
}
/*
 * look for named domain.  If it exists, return a pointer to
 * the domain structure.  If not, add it to the database.
 */
static struct domain *
newdom(name)
char	*name;
{
	struct res_rec	*tres;
	struct domain	*d;

	if ((d=finddom(name,Domain)) == NULL) {
		tres = (struct res_rec *) calloc(1,sizeof(struct res_rec));
		strncpy(tres->rr_name,namepart(name),NAMSIZ);
		tres->rr_type = DOM;
		d = tres->rr_dom = (struct domain *) calloc(1,sizeof(struct domain));
		setauth(tres->rr_dom,0);
		tres->rr_dsize = 0;
		if (addraw(name,tres) != R_NOERR) {
			free(tres);
			return((struct domain *) NULL);
		}
	}
	return(d);
}
