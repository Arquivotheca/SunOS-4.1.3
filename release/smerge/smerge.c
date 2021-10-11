#include <stdio.h>
#include <sys/param.h>
#include "smerge.h"
#include "ssum.h"
#include "defs.h"

#ifdef sparc
#include <alloca.h>
#endif

#define TRUE	1
#define FALSE	0

char	*calloc(),
	*malloc(),
	*sccsname(),
	*getword(),
	*skipwhite(),
	*getenv(),
	*strcpy(),
	*strcat(),
	*alloca(),
	*copys(),
	**split(),
	*getname(),
	**pathopt();
void	compare(),
	setvar();
delta_t	**do_file();

static char genlist[] ="\
(\
	( cd %s;/usr/bin/find . -name 's.*' -print );\
	( cd %s;/usr/bin/find . -name 's.*' -print );\
) | /usr/bin/sort -u";

extern int errno;
extern int abortflag;
char	*myname;
int	debug;
int	quickflag = FALSE,
	branchok = FALSE,
	Lflag = FALSE,
	examine_graph = FALSE; /* if delta graph is different, old.old
				* becomes new.new
				*/
	examine_comments = FALSE; /* TRUE => No change deltas are changes */
int	Mflags;
int	sccserrok = FALSE;

blankline(line)
	register char	*line;
{
	line = skipwhite(line);
	if( *line == '\0' || *line == '#' )
		return(1);
	return(0);
}

int
filenames(f, output)
	FILE	*f,
		*output;
{
	char	ifname[MAXPATHLEN];
	int	didwork = FALSE;

	while( fgets(ifname, sizeof(ifname), f) != NULL )
	{
		int	len;

		if( output )
			fprintf(output, ifname);

		if( blankline(ifname) )
			continue;
		len = strlen(ifname) - 1;
		if( ifname[len] == '\n' )
			ifname[len] = '\0';
		if( !do_name(ifname) )
			exit(errno);
		else
			didwork = TRUE;
	}

	if( output )
	{
		(void)pclose(f);
		(void)fclose(f);
	}
	else
		(void)fclose(f);
	return(didwork);
}

main(argc, argv)
	int	argc;
	char	**argv;
{
	register int
		i,
		c;
	char	*smergefile = NULL,
		*filelist = NULL;
	extern int	optind;
	extern char	*optarg;
	int	errflg = 0,
		didwork = FALSE;

	myname = *argv;

	setlinebuf(stderr);
	setlinebuf(stdout);

	initfunny();	/* so that is_meta() works elsewhere */

	abortflag = 0;	/* retain control on SCCS file errors */

	while( (c = getopt(argc, argv, "bCcdeGIiknqsf:L:l:")) != EOF)
		switch( c )
		{
		case 'b':	/* consider branch deltas in decisions */
			++branchok;
			break;
		case 'c':
			examine_comments = FALSE;
			break;
		case 'C':
			examine_comments = TRUE;
			break;
		case 'd':	/* turn on debugging */
			++debug;
			TURNON(DBUG);
			break;
		case 'e':	/* ?? */
			TURNON(ENVOVER);
			break;
		case 'f':	/* specify rules file */
			if( smergefile != NULL )
				++errflg;
			smergefile = optarg;
			break;
		case 'G':
			examine_graph = TRUE;
			break;
		case 'I':	/* ignore SCCS errors */
			sccserrok = TRUE;
			break;
		case 'i':	/* ignore errors */
			TURNON(IGNERR);
			break;
		case 'k':	/* skip to next file when error encountered */
			TURNON(KEEPGO);
			break;
		case 'L':
			Lflag = TRUE;
			/* fall through */
		case 'l':	/* specify file containing list of file names */
			if( filelist != NULL )
				++errflg;
			filelist = optarg;
			break;
		case 'n':	/* don't execute any shell commands */
			TURNON(NOEX);
			break;
		case 'q':	/* take a few short cuts */
			++quickflag;
			break;
		case 's':	/* don't echo any shell commands */
			TURNON(SIL);
			break;
		case '?':
			++errflg;
			break;
		}

	if( errflg )
	{
		fprintf(stderr,
			"usage: %s [-bdeIiknqs] [-f smergefile] [-l nameslistfile | -L nameslistfile] [files]\n",
			myname);
		exit(1);
	}

	if( IS_ON(KEEPGO) && IS_ON(IGNERR) )
		fprintf(stderr, "%s: -k flag ignored\n", myname);

	/*
	 * Read environment args.  Let file args which follow override.
	 * unless 'e' in MAKEFLAGS variable is set. 
	 */
	if (any((varptr(Smergeflags))->varval, 'e'))
		TURNON(ENVOVER);
	if (IS_ON(DBUG))
		(void) printf("Reading environment.\n");
	TURNON(EXPORT);
	readenv();
	TURNOFF(EXPORT | ENVOVER);

	setvar("$", "$");	/* so that $$ expands to $ in smergefile */

	/*
	 * Read command line "=" type args and make them readonly. 
	 */
	TURNON(INARGS | EXPORT);
	if (IS_ON(DBUG))
		(void) printf("Reading \"=\" type args on command line.\n");
	for (i = 1; i < argc; ++i)
		if (argv[i] && argv[i][0] != '-' && (eqsign(argv[i]) == YES))
			argv[i] = 0;
	TURNOFF(INARGS | EXPORT);

	if( smergefile == NULL )
	{
		if( access("smergefile", 0) == 0 )
			smergefile = "smergefile";
		else if( access("Smergefile", 0) == 0 )
			smergefile = "Smergefile";
	}

	if( smergefile != NULL )
	{
		FILE	*sfile;

		if( (sfile = fopen(smergefile, "r")) == NULL )
		{
			perror(smergefile);
			exit(errno);
		}
		fcallyacc(sfile);
	}

	if( filelist )
	{
		FILE	*f,
			*listout = (FILE *)NULL;

		if( !Lflag )
		{
			if( (f = fopen(filelist, "r")) == (FILE *)NULL )
			{
				perror(filelist);
				exit(errno);
			}
		}
		else
		{
			char	*cmdbuf;
			int	z;
			char	*tname1, *tname2;

			tname1 = getname(VARTREE1, REQUIRED);
			tname2 = getname(VARTREE2, REQUIRED);

			listout = fopen(filelist, "w");
			
			z = sizeof(genlist)+strlen(tname1)+strlen(tname1)+1;
			cmdbuf = alloca(z);

			(void)sprintf(cmdbuf, genlist, tname1, tname2);
			if( (f = popen(cmdbuf, "r")) == (FILE *)NULL )
			{
				perror("Cannot open pipe");
				exit(errno);
			}
		}
		didwork = filenames(f, listout);
	}
	else
		for( i = optind; i < argc; i++)
		{
			if( argv[i] != NULL )
			{
				if( !do_name(argv[i]) )
					exit(errno ? errno : 1 );
				else
					didwork = TRUE;
			}
		}

	if( !didwork )
	{
		fprintf(stderr, "%s: no files specified\n", myname);
		exit(1);
	}
	return(0);
}

void
setdirandfile(name)
	char	*name;
{
	char	**namev;
	char	namebuf[MAXPATHLEN];
	int	i,
		n,
		ndir;

	namev = pathopt(split(name));

	for( n = 0; namev[n]; ++n )
		continue;

	if( n > 1 )
	{
		if( strcmp(namev[n-2], "SCCS") == 0 )
			ndir = n - 2;
		else
			ndir = n - 1;
	}
	else
		ndir = 0;


	if( *name == '/' )
	{
		namebuf[0] = '/';
		namebuf[1] = '\0';
	}
	else
		namebuf[0] = '\0';

	for(i = 0; i < ndir; ++i)
	{
		(void)strcat(namebuf, namev[i]);
		if( i + 1 != ndir )
			(void)strcat(namebuf, "/");
	}

	if( namebuf[0] == '\0' )
		setvar(VARDIR, ".");
	else
		setvar(VARDIR, namebuf);

	if( n == 0 )
		setvar(VARFILE, NULL);
	else if( strncmp("s.", namev[n-1], 2) == 0 )
		setvar(VARFILE, namev[n-1]+2);
	else
		setvar(VARFILE, namev[n-1]);
	freev(namev);
}

char *
getname(name, required)
	char	*name;
	int	required;
{
	VARBLOCK	v;

	v = srchvar(name);
	if((v==NULL || v->varval == NULL || v->varval[0] == '\0') && required )
		fatal1("$(%s) not set", name);
	return( v ? v->varval : NULL );
}

do_name(name)
	char	*name;
{
	register delta_t **a1 = (delta_t **)NULL,
			**a2 = (delta_t **)NULL;
	register char	*sname1,
			*sname2;
	int	state1 = NONE,
		state2 = NONE;
	FILE	*file1,
		*file2;

	clearnames();
	setdirandfile(name);	 /* set dir and file */
	sname1 = copys(sccsname(getname(VARTREE1, REQUIRED), name, 1));
	sname2 = copys(sccsname(getname(VARTREE2, REQUIRED), name, 1));
	file1 = fopen(sname1, "r");
	file2 = fopen(sname2, "r");

	if( file1 == (FILE *)NULL || file2 == (FILE *)NULL )
	{
		if( file1 )
		{
			if( quickflag )
				state1 = OLD;
			else if((a1=do_file(file1, sname1)) != (delta_t **)NULL)
				state1 = OLD;
			else
				state1 = ERRSTATE;
		}
		else if( file2 )
		{
			if( quickflag )
				state2 = OLD;
			else if((a2=do_file(file2, sname2)) != (delta_t **)NULL)
				state2 = OLD;
			else
				state2 = ERRSTATE;
		}
	}
	else	/* both files exist */
	{
		char	line1[80],
			line2[80];
		char	*p = NULL,
			*q = NULL;

		if( quickflag )
		{
			if( (p = fgets(line1, sizeof(line1), file1)) != NULL )
				rewind(file1);
			if( (q = fgets(line2, sizeof(line2), file2)) != NULL )
				rewind(file2);
		}

		if(    quickflag
		    && p != (char *)NULL	/* file 1 has 1st line */
		    && q != (char *)NULL	/* file 2 has 1st line */
		    && line1[0] == CTLCHAR	/* first line is SCCS cmd */
		    && line1[1] == HEAD		/* cmd is SCCS header */
						/* same SCCS checksum? */
		    && strncmp(line1, line2, sizeof(line1)) == 0
		    )
		{
			state1 = OLD;
			state2 = OLD;
		}
		else
		{
			if( (a1 = do_file(file1, sname1)) != (delta_t **)NULL )
				state1 = OLD;
			else
				state1 = ERRSTATE;

			if( (a2 = do_file(file2, sname2)) != (delta_t **)NULL )
				state2 = OLD;
			else
				state2 = ERRSTATE;

			if( state1 != ERRSTATE && state2 != ERRSTATE )
				compare(a1, a2, &state1, &state2);
		}
	}
	if( a1 )
		freearray(a1);
	if( a2 )
		freearray(a2);
	if( file1 )
		(void)fclose(file1);
	if( file2 )
		(void)fclose(file2);
	free(sname1);
	free(sname2);

	decision(state1, state2);	/* execute commands */

	if(state1 == ERRSTATE || state2 == ERRSTATE )
	{
		printf("SCCS file format error%s\n",
			sccserrok ? ", (ignored)" : "");
		if( !sccserrok )
			exit(1);
	}

	return(1);
}

#define EQUAL(d1,d2) (d1->sum == d2->sum && d1->bytes == d2->bytes)

calcbase(a1,a2,bases)
	register delta_t **a1;
	register delta_t **a2;
	int	*bases;
{
	int	max1,
		max2,
		i,
		j;
	
	bases[0] = 0;		/* set bases to zero in case there is no
				 * common base
				 */
	bases[1] = 0;

	if( debug )
		(void)fprintf(stderr, "calcbase(%x,%x,%x)\n", a1,a2,bases);

	max1 = (long)a1[0];
	max2 = (long)a2[0];
	for( i = max1; i > 0; --i )
	{
		register delta_t *a1subi = a1[i];

		if( a1subi == (delta_t *)NULL
		    || ( !branchok && a1subi->sid.s_br != 0 ))
			continue;
		
		if( examine_comments && i > 1 && EQUAL(a1subi, a1[i-1]))
			continue;

		for( j = max2; j > 0; --j )
		{
			if(   a2[j] == (delta_t *)NULL
			   || ( !branchok && a2[j]->sid.s_br != 0 ))
				continue;

			if( !EQUAL(a1subi, a2[j]) )
				continue;
			
			if(examine_comments && j > 1 && EQUAL(a1subi,a2[j-1]))
				continue;

			bases[0] = i;
			bases[1] = j;
			return;
		}
	}
}

void
settop(name, array, topp, statep)
	register char	*name;
	register delta_t	**array;
	int	*statep,
		*topp;
{
	register int	i;

	if( array != (delta_t **)NULL )
	{
		for(	i = (long)array[0];
			i>0 && (!array[i] || (!branchok && array[i]->sid.s_br));
			--i)
			continue;
		if( i > 0 )
		{
			*topp = i;
			setsidname(name, array, *topp);
			return;
		}
	}
	setvar(name, "NONE");
	*topp = 0;
	*statep = NONE;
}

void
setincludes(deltas, dptr, limit)
	int	*deltas;
	register delta_t	*dptr;
	int	limit;
{
	register ixglist_t	*ixg;

	if( dptr == (delta_t *)NULL || dptr->sid.s_serial <= limit )
		return;

	setincludes(deltas, dptr->parent, limit);

	for( ixg = dptr->ixglist; ixg; ixg = ixg->next )
		if( ixg->action == INCLUDE )
			setincludes(deltas, ixg->deltap, limit);

	for( ixg = dptr->ixglist; ixg; ixg = ixg->next )
		if( ixg->action != INCLUDE )
			deltas[ixg->delta] = FALSE;

	deltas[dptr->sid.s_serial] = TRUE;
}

void
setnewsid(name, array, top, base)
	char	*name;
	delta_t	**array;
	int	top,
		base;
{
	int	*deltas,
		i;
	char	buffer[1024];

	if(!(deltas=(int *)calloc(1, (unsigned)(sizeof(int) * (long)array[0]))))
		nomem();

	setincludes(deltas, array[top], base);

	for( i = base+1, buffer[0] = '\0'; i <= top; ++i )
		if( deltas[i] )
		{
			strcat(buffer, sidstr(&array[i]->sid));
			if( i < top )
				strcat(buffer, " ");
		}

	free((char *)deltas);
	setvar(name, buffer);
}

/* graphcalc - compare two delta_t arrays. If the sccs delta graph is
 *	different set return 1, else return 0
 */
int
graphcalc(a1, a2)
	delta_t **a1,
		**a2;
{
	int	i;

	if( (long)a1[0] != (long)a2[0] )	/* can't possibly match */
		return(1);

	for( i = (long)a1[i]; i > 0; --i )
		if( !EQUAL(a1[i], a2[i]) )
			return(1);
		else
		{
			int	bad = 0;
			register ixglist_t
				*ixg1,
				*ixg2;
			
			/* !! Is the order of exclusion, inclusion, and
			 * ignorance in delta order? (nope)
			 */
			for(ixg1=a1[i]->ixglist; ixg1; ixg1=ixg1->next)
			{
				for(ixg2=a2[i]->ixglist; ixg2; ixg2=ixg2->next)
				{
					if(   ixg1->delta == ixg2->delta
					   && ixg1->action == ixg2->action)
					{
						/* ugly flag hack: */
						ixg2->action |= 0200;
						break;
					}
				}
				if( !ixg2 )
					bad = 1;
			}

			for(ixg2=a2[i]->ixglist; ixg2; ixg2=ixg2->next)
			{
				if( ixg2->action & 0200 )
					ixg2->action &= ~0200;
				else
					bad = 1;
			}

			if( bad )
				return(1);
		}

	return(0);
}

/* compare - compare two sccs files
 *	side effects:	sets s1 and s2
 *			sets vars base1, base2, top1 and top2
 */
void
compare(a1, a2, s1, s2)
	register delta_t **a1;
	register delta_t **a2;
	int	*s1,
		*s2;
{
	int	a1base[2],
		a2base[2],
		top1 = 0,
		top2 = 0;

	settop(VARTOP1, a1, &top1, s1);
	settop(VARTOP2, a2, &top2, s2);

	/* none.none, none.old, old.none */
	if( !a1 || !a2 )
	{
		if( a1 )
		{
			setnewsid(VARNSID1, a1, top1, a1base[0]);
			setsidname(VARBASE1, a1, a2base[1]);
		}
		if( a2 )
		{
			setnewsid(VARNSID2, a2, top2, a2base[0]);
			setsidname(VARBASE2, a2, a1base[1]);
		}
		return;
	}

	/* The base SID is always what the other side thinks it is.
	 * Normally, it doesn't matter how one calculates the base,
	 * but in the case where a change is backed out by laying an
	 * old version on top of a new version it does matter.
	 */
	calcbase(a1,a2,a1base);
	calcbase(a2,a1,a2base);

	if( a1base[0] == 0 )	/* nobase */
	{
		/* there is no base delta, since every delta is needed
		 * to update the other side, set the base delta to 1.1
		 * and record every delta in $(newsid[1-2])
		 */
		setvar(VARBASE1, "");
		setvar(VARBASE2, "");
		setnewsid(VARNSID1, a1, top1, 0);
		setnewsid(VARNSID2, a2, top2, 0);
		*s1 = NOBASE;
		*s2 = NOBASE;
	}
	else	/* old.old, old.new, new.old, new.new, ambiguous */
	{
		if( a1base[0] == a2base[1] && a1base[1] == a2base[0] )
		{
			/* everyone agrees on base delta */
			if( top1 != a2base[1] )	/* no change? */
				*s1 = NEW;
			if( top2 != a1base[1] )	/* no change? */
				*s2 = NEW;
			if( examine_graph && *s1 == OLD && *s2 == OLD )
			{
				if( graphcalc(a1, a2, a1base, a2base) )
				{
					*s1 = NEW;
					*s2 = NEW;
				}
			}
		}
		else /* base is different depending on which side you're on. */
			*s1 = *s2 = AMBIGUOUS;

		setsidname(VARBASE1, a1, a2base[1]);
		setsidname(VARBASE2, a2, a1base[1]);
		setnewsid(VARNSID1, a1, top1, a2base[1]);
		setnewsid(VARNSID2, a2, top2, a1base[1]);
	}
}

delta_t **
do_file(file, sname)
	FILE	*file;
	char	*sname;
{
	delta_t	*tree = NULL;
	register delta_t	**array;

	if(	   (tree = readhead(file, sname)) == (delta_t *)NULL
		|| (array = buildtree(&tree, sname)) == (delta_t **)NULL
		|| readtext(file, tree, sname, 0, array) == ERROR
	       )
	{
		if( array )
			freearray(array);
		return((delta_t **)NULL);
	}
	return(array);
}

nomem()
{
	(void)fprintf(stderr, "%s: out of memory\n", myname);
	abort();
}

STATEBLOCK
findstate(name)
	char	*name;
{
	register STATEBLOCK	sb;

	for( sb = firststate; sb; sb = sb->next )
		if( equal(sb->name, name) )
			return(sb);
	return((STATEBLOCK)0);
}

/* VARARGS1 */
fatal1(s, t1, t2, t3)
	char	*s;
{
	char	buf[BUFSIZ];

	(void) sprintf(buf, s, t1, t2, t3);
	fatal(buf);
}

fatal(s)
	char	*s;
{
	if (s)
		(void) fprintf(stderr, "Smerge: %s.  Stop.\n", s);
	else
		(void) fprintf(stderr, "\nStop.\n");
	exit(1);
}

setsidname(name, array, delta)
	char	*name;
	delta_t	**array;
	int	delta;
{
	char	buf[80];
	sid_t	*sid;

	sid = &array[delta]->sid;
	if( sid->s_br != 0 )
		(void)sprintf(buf, "%d.%d.%d.%d",
			sid->s_rel, sid->s_lev, sid->s_br, sid->s_seq);
	else
		(void)sprintf(buf, "%d.%d", sid->s_rel, sid->s_lev);
	setvar(name, buf);
}

clearnames()
{
	setvar(VARBASE1, (char *)NULL);/* common base sid, "skew", or "none" */
	setvar(VARBASE2, (char *)NULL);/* common base sid, "skew", or "none" */
	setvar(VARDIR, (char *)NULL);	/* dir part of path from tree to file */
	setvar(VARFILE, (char *)NULL);	/* last path component of filename */
	setvar(VARTOP1, (char *)NULL);	/* sid of top sccs delta of file 1 */
	setvar(VARTOP2, (char *)NULL);	/* sid of top sccs delta of file 2 */
	setvar(VARNSID1, (char *)NULL);/* new sid's in file 1 */
	setvar(VARNSID2, (char *)NULL);/* new sid's in file 2 */
}
