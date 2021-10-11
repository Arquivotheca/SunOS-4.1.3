#ifndef lint
static	char sccsid[] = "@(#)doname.c 1.1 92/08/05 SMI"; /* from S5R2 1.3 03/28/83 */
#endif

#include "defs"
#include <signal.h>
#include <ctype.h>


char Makecall;			/* flag which says whether to exec $(MAKE) */
extern char archmem[];
extern char archname[];

/*  BASIC PROCEDURE.  RECURSIVE.  */

/*
p->done = 0   don't know what to do yet
p->done = 1   file in process of being updated
p->done = 2   file already exists in current state
p->done = 3   file make failed
*/


doname(p, reclevel, tval)
register NAMEBLOCK p;
int reclevel;
TIMETYPE *tval;
{
	register DEPBLOCK q;
	register LINEBLOCK lp;
	int errstat;
	int okdel1;
	int didwork;
	TIMETYPE td, td1, tdep, ptime, ptime1;
	extern void appendq();
	DEPBLOCK suffp, suffp1;
	NAMEBLOCK p1, p2;
	SHBLOCK implcom, explcom;
	LINEBLOCK lp1, lp2;
	char sourcename[BUFSIZ],prefix[BUFSIZ],temp[BUFSIZ],concsuff[20];
	CHARSTAR pnamep, p1namep;
	CHAIN qchain;
	int found, onetime;
	CHARSTAR savenamep = 0;

	if(p == 0)
	{
		*tval = 0;
		return(0);
	}

	if(IS_ON(DBUG))
	{
		blprt(reclevel);
		(void)printf("doname(%s,%d)\n",p->namep,reclevel);
		(void)fflush(stdout);
	}

	if(p->done > 0)
	{
		*tval = p->modtime;
		return(p->done == 3);
	}

	errstat = 0;
	tdep = 0;
	implcom = 0;
	explcom = 0;
	ptime = exists(p);
	if(reclevel == 0 && IS_ON(DBUG))
	{
		blprt(reclevel);
		(void)printf("TIME(%s)=%ld\n", p->namep, ptime);
	}
	ptime1 = 0;
	didwork = NO;
	p->done = 1;	/* avoid infinite loops */

	qchain = NULL;

/*
 *	Perform runtime dependency translations.
 */
	if(p->rundep == 0)
	{
		setvar("@", p->namep);
		dynamicdep(p);
		setvar("@", Nullstr);
	}

/*
 *	Expand any names that have embedded metacharaters. Must be
 *	done after dynamic dependencies because the dyndep symbols
 *	($(*D)) may contain shell meta characters.
 */
	expand(p);



/*
 *	FIRST SECTION -- GO THROUGH DEPENDENCIES
 */

	if(IS_ON(DBUG))
	{
		blprt(reclevel);
		(void)printf("look for explicit deps. %d \n", reclevel);
	}
	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
	{
		td = 0;
		for(q = lp->depp ; q!=0 ; q=q->nextdep)
		{
			q->depname->backname = p;
			errstat += doname(q->depname, reclevel+1, &td1);
			if(IS_ON(DBUG))
			{
			    blprt(reclevel);
			    (void)printf("TIME(%s)=%ld\n", q->depname->namep, td1);
			}
			td = max(td1,td);
			if(ptime < td1)
				appendq((CHAIN)&qchain, q->depname->namep);
		}
		if(p->septype == SOMEDEPS)
		{
			if(lp->shp!=0)
				if( ptime<td || (ptime==0 && td==0) || lp->depp==0)
				{
					okdel1 = okdel;
					okdel = NO;
					setvar("@", p->namep);
					if(savenamep)
						setvar("%", archmem);
					setvar("?", mkqlist(qchain) );
					qchain = NULL;
					if( IS_OFF(QUEST) )
					{
						ballbat(p);
						errstat += docom(lp->shp);
					}
					setvar("@", Nullstr);
					setvar("%", Nullstr);
					okdel = okdel1;
					if( (ptime1 = exists(p)) == 0)
						ptime1 = prestime();
					didwork = YES;
				}
		}

		else
		{
			if(lp->shp != 0)
			{
				if(explcom)
					(void)fprintf(stderr, "Too many command lines for `%s'\n",
						p->namep);
				else
					explcom = lp->shp;
			}

			tdep = max(tdep, td);
		}
	}

/*
 *	SECOND SECTION -- LOOK FOR IMPLICIT DEPENDENTS
 */

	if(IS_ON(DBUG))
	{
		blprt(reclevel);
		(void)printf("look for implicit rules. %d \n", reclevel);
	}
	found = 0; onetime = 0;
	if(any(p->namep, LPAREN))
	{
		savenamep = p->namep;
		p->namep = copys(archmem);
		if(IS_ON(DBUG))
		{
			blprt(reclevel);
			(void)printf("archmem = %s\n", archmem);
		}
		if(IS_ON(DBUG)) 
		{
			blprt(reclevel);
			(void)printf("archname = %s\n", archname);
		}
	}
	else
		savenamep = 0;


	for(lp=sufflist ; lp!=0 ; lp = lp->nextline)
	for(suffp = lp->depp ; suffp!=0 ; suffp = suffp->nextdep)
	{
		pnamep = suffp->depname->namep;
		if(suffix(p->namep , pnamep , prefix))
		{
			if(IS_ON(DBUG)) 
			{
				blprt(reclevel);
				(void)printf("right match = %s\n",p->namep);
			}
			found = 1;
			if(savenamep)
				pnamep = ".a";
searchdir:

			(void)copstr(temp, prefix);
			addstars(temp);
			(void)srchdir( temp , NO);
			for(lp1 = sufflist ; lp1!=0 ; lp1 = lp1->nextline)
			for(suffp1=lp1->depp ; suffp1!=0 ; suffp1 = suffp1->nextdep)
			{
				p1namep = suffp1->depname->namep;
				(void)concat(p1namep, pnamep, concsuff);
				if( (p1=srchname(concsuff)) == 0)
					continue;
				if(p1->linep == 0)
					continue;
				(void)concat(prefix, p1namep, sourcename);
				if(any(p1namep, WIGGLE))
				{
					sourcename[strlen(sourcename) - 1] = CNULL;
					if(!sdot(sourcename))
						trysccs(sourcename);
				}
				if( (p2=srchname(sourcename)) == 0)
					continue;
				if(equal(sourcename, p->namep))
					continue;
/*
 *	FOUND -- left and right match
 */

				found = 2;
				if(IS_ON(DBUG))
				{
				  blprt(reclevel);
				  (void)printf("%s ---%s--- %s\n",
					sourcename, concsuff, p->namep);
				}
				p2->backname = p;
				errstat += doname(p2, reclevel+1, &td);
				if(ptime < td)
					appendq((CHAIN)&qchain, p2->namep);
				if(IS_ON(DBUG))
				{
					blprt(reclevel);
					(void)printf("TIME(%s)=%ld\n",p2->namep,td);
				}
				tdep = max(tdep, td);
				setvar("*", prefix);
				setvar("<", p2->alias ? p2->alias : p2->namep);
				for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextline)
					if(implcom = lp2->shp) break;
				goto endloop;
			}
/*
 *	quit search for single suffix rule.
 */
			if(onetime == 1)
				goto endloop;
		}
	}

endloop:


/*
 * look for a single suffix type rule.
 * only possible if no explicit dependents and no shell rules
 * are found, and nothing has been done so far. (previously, `make'
 * would exit with 'Don't know how to make ...' message.
 */
	if(found == 0)
	if(onetime == 0)
	if(	  p->linep == 0 ||
		( p->linep->depp == 0 && p->linep->shp == 0))
	{
		onetime = 1;
		if(IS_ON(DBUG))
		{
			blprt(reclevel);
			(void)printf("Looking for Single suffix rule.\n");
		}
		(void)concat(p->namep, "", prefix);
		pnamep = "";
		goto searchdir;
	}


/*
 *	THIRD SECTION -- LOOK FOR DEFAULT CONDITION OR DO COMMAND
 */
	if(errstat==0 && (ptime<tdep || (ptime==0 && tdep==0) ) )
	{
		if(savenamep)
		{
			setvar("@", archname);
			setvar("%", archmem);
		}
		else
		{
			setvar("@", p->namep);
		}
		setvar("?", mkqlist(qchain) );
		ballbat(p);
		if(explcom)
			errstat += docom(explcom);
		else if(implcom)
			errstat += docom(implcom);
		else if( (p->septype != SOMEDEPS && IS_OFF(MH_DEP)) ||
			 (p->septype == 0        && IS_ON(MH_DEP) )    )
/*
 *	OLD WAY OF DOING TEST is
 *		else if(p->septype == 0)
 *	notice above, a flag has been put in to get the murray hill version.
 *	the flag is "-b".
 */
			if(p1=srchname(".DEFAULT"))
			{
				if(IS_ON(DBUG))
				{
					blprt(reclevel);
					(void)printf("look for DEFAULT rule. %d \n", reclevel);
				}
				setvar("<", p->alias ? p->alias : p->namep);
				for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextline)
					if(implcom = lp2->shp)
					{
						errstat += docom(implcom);
					}
			}
			else if(IS_OFF(GET) ||
				  !get(p->namep, NOCD, (CHARSTAR)0) )
			{
				fatal1(" Don't know how to make %s", p->namep);
			}

		setvar("@", Nullstr);
		setvar("%", Nullstr);
		if(IS_ON(NOEX) || (ptime = exists(p)) == 0)
			ptime = prestime();
	}

	else if(errstat!=0 && reclevel==0)
		(void)printf("`%s' not remade because of errors\n", p->namep);

	else if(IS_OFF(QUEST) && reclevel==0  &&  didwork==NO)
		(void)printf("`%s' is up to date.\n", p->namep);

	if(IS_ON(QUEST) && reclevel==0)
		exit(ndocoms>0 ? -1 : 0);

	p->done = (errstat ? 3 : 2);
	ptime = max(ptime1, ptime);
	p->modtime = ptime;
	*tval = ptime;
	setvar("<", Nullstr);
	setvar("*", Nullstr);
	return(errstat);
}

docom(q)
SHBLOCK q;
{
	CHARSTAR s;
	extern CHARSTAR subst();
	int ign, nopr;
	char string[OUTMAX];
	char string2[OUTMAX];

	++ndocoms;
	if(IS_ON(QUEST))
		return(0);

	if(IS_ON(TOUCH))
	{
		s = varptr("@")->varval;
		if(IS_OFF(SIL))
			(void)printf("touch(%s)\n", s);
		if(IS_OFF(NOEX))
			touch(1,s);
	}

	else for( ; q!=0 ; q = q->nextsh )
	{
/*
 *	Allow recursive makes to execute only if the NOEX flag set
 */
		if(sindex(q->shbp, "$(MAKE)") != -1 && IS_ON(NOEX))
			Makecall = YES;
		else
			Makecall = NO;
		(void)subst(q->shbp,string2);
		fixname(string2, string);

		ign = IS_ON(IGNERR) ? YES : NO;
		nopr = NO;
		for(s = string ; *s==MINUS || *s==AT ; ++s)
			if(*s == MINUS)  ign = YES;
			else nopr = YES;

		if( docom1(s, ign, nopr) && !ign)
			if(IS_ON(KEEPGO))
				return(1);
			else	fatal((char *)0);
	}
	return(0);
}



docom1(comstring, nohalt, noprint)
register CHARSTAR comstring;
int nohalt, noprint;
{
	register int status;

	if(comstring[0] == '\0') return(0);

	if(IS_OFF(SIL) && (!noprint || IS_ON(NOEX)) )
	{
		CHARSTAR p1, ps;
		CHARSTAR pmt = prompt;

		ps = p1 = comstring;
		while(1)
		{
			while(*p1 && *p1 != NEWLINE) p1++;
			if(*p1)
			{
				*p1 = 0;
				(void)printf("%s%s\n", pmt, ps);
				*p1 = NEWLINE;
				ps = p1 + 1;
				p1 = ps;
			}
			else
			{
				(void)printf("%s%s\n", pmt, ps);
				break;
			}
		}

		(void)fflush(stdout);
	}

	if( status = dosys(comstring, nohalt) )
	{
		if( status>>8 )
			(void)printf("*** Error code %d", status>>8 );
		else
		{
			extern char *sys_siglist[];
			int coredumped;

			coredumped = status & 0200;
			status &= 0177;
			if (status > NSIG)
				(void)printf("*** Signal %d", status );
			else
				(void)printf("*** %s", sys_siglist[status] );
			if (coredumped)
				(void)printf(" - core dumped");
		}

		if(nohalt) (void)printf(" (ignored)\n");
		else	(void)printf("\n");
		(void)fflush(stdout);
	}

	return(status);
}


/*
 *	If there are any Shell meta characters in the name,
 *	search the directory, and if the search finds something
 *	replace the dependency in "p"'s dependency chain. srchdir
 *	produces a DEPBLOCK chain whose last member has a null
 *	nextdep pointer or the NULL pointer if it finds nothing.
 *	The loops below do the following: for each dep in each line
 *	if the dep->depname has a shell metacharacter in it and
 *	if srchdir succeeds, replace the dep with the new one
 *	created by srchdir. The Nextdep variable is to skip over
 *	the new stuff inserted into the chain.
*/

expand(p)
NAMEBLOCK p;
{
	register DEPBLOCK db;
	register DEPBLOCK Nextdep;
	register CHARSTAR s;
	register DEPBLOCK srchdb;
	register LINEBLOCK lp;



	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
		for(db=lp->depp ; db!=0 ; db=Nextdep )
		{
			Nextdep = db->nextdep;
			if(any( (s=db->depname->namep), STAR) ||
			   any(s, QUESTN) || any(s, LSQUAR) )
				if( srchdb = srchdir(s , YES) )
					dbreplace(p, db, srchdb);
		}
}
/*
 *	Replace the odb depblock in np's dependency list with the
 *	dependency chain defined by ndb. This is just a linked list insert
 *	problem. dbreplace assumes the last "nextdep" pointer in
 *	"ndb" is null.
 */
dbreplace(np, odb, ndb)
register NAMEBLOCK np;
register DEPBLOCK odb, ndb;
{
	register LINEBLOCK lp;
	register DEPBLOCK  db;
	register DEPBLOCK  enddb;

	for(enddb = ndb; enddb->nextdep; enddb = enddb->nextdep);

	for(lp = np->linep; lp; lp = lp->nextline)
		if(lp->depp == odb)
		{
			enddb->nextdep	= lp->depp->nextdep;
			lp->depp	= ndb;
			return;
		}
		else
		{
			for(db = lp->depp; db; db = db->nextdep)
				if(db->nextdep == odb)
				{
					enddb->nextdep	= odb->nextdep;
					db->nextdep	= ndb;
					return;
				}
		}
}


#define NPREDS 50

ballbat(np)
NAMEBLOCK np;
{
	static char ballb[200];
	register CHARSTAR p;
	register NAMEBLOCK npp;
	register int i;
	VARBLOCK vp;
	int npreds=0;
	NAMEBLOCK circles[NPREDS];


	if( *((vp=varptr("!"))->varval) == 0)
		vp->varval = ballb;
	p = ballb;
	p = copstr(p, varptr("<")->varval);
	p = copstr(p, " ");
	for(npp = np; npp; npp = npp->backname)
	{
		for(i = 0; i < npreds; i++)
		{
			if(npp == circles[i])
			{
				(void)fprintf(stderr,"$! nulled, predecessor circle\n");
				ballb[0] = CNULL;
				return;
			}
		}
		circles[npreds++] = npp;
		if(npreds >= NPREDS)
		{
			(void)fprintf(stderr, "$! nulled, too many predecessors\n");
			ballb[0] = CNULL;
			return;
		}
		p = copstr(p, npp->namep);
		p = copstr(p, " ");
	}
}

/*
 * Copy a command in s to d, checking each token to see if it is a file
 * name; if so, and it has an alias (i.e., it was actually found in a
 * directory other than the one its target was in), replace it with
 * its alias.
 */
fixname(s, d)
register CHARSTAR s, d;
{
	register CHARSTAR r, q;
	NAMEBLOCK pn;
	char name[BUFSIZ];

	while(*s)
	{
		if(isspace(*s))
			*d++ = *s++;
		else
		{
			r = name;
			while(*s && !isspace(*s))
				*r++ = *s++;
			*r = CNULL;
 		
			if((pn = srchname(name)) != 0 && pn->alias)
				q = pn->alias;
			else
				q = name;
	
			while(*q)
				*d++ = *q++;
		}
	}
	*d = CNULL;
}

/*
 *	PRINT n BLANKS WHERE n IS THE CURRENT RECURSION LEVEL.
 */
blprt(n)
register int n;
{
	while(n--)
		(void)printf("   ");
}
