# include "../hdr/defines.h"
# include "../hdr/had.h"

SCCSID(@(#)rmchg.c 1.1 92/07/30 SMI); /* from System III 5.1 */

/*
	Program to remove a specified delta from an SCCS file,
	when invoked as 'rmdel',
	or to change the MRs and/or comments of a specified delta,
	when invoked as 'cdc'.
	(The program has two links to it, one called 'rmdel', the
	other 'cdc'.)

	The delta to be removed (or whose MRs and/or comments
	are to be changed) is specified via the
	r argument, in the form of an SID.

	If the delta is to be removed, it must be the most recent one
	in its branch in the delta tree (a so-called 'leaf' delta).
	For either function, the delta being processed must not
	have any 'delivered' MRs, and the user must have basically
	the same permissions as are required to make deltas.

	If a directory is given as an argument, each SCCS file
	within the directory will be processed as if it had been
	specifically named. If a name of '-' is given, the standard
	input will be read for a list of names of SCCS files to be
	processed. Non SCCS files are ignored.
*/

# define COPY 0
# define NOCOPY 1

struct sid sid;
int num_files;
char D_type;
char	*Mrs;
char	*Comments;
char	*Darg[NVARGS];
char	*Earg[NVARGS];
char	*NVarg[NVARGS];
int D_serial;

main(argc,argv)
int argc;
char *argv[];
{
	register int i;
	register char *p;
	char c;
	extern rmchg();
	extern	char *savecmt();
	extern int Fcnt;

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine, and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;

	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c = argv[i][1])) {
			p = &argv[i][2];
			switch (c) {

			case 'r':
				if (!(*p))
					fatal("r has no sid (rc11)");
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'm':	/* MR entry */
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'y':	/* comment line */
				Comments = p;
				break;
			case 'q': /* enable NSE mode */
				if (*p) {
					nsedelim = p;
				} else {
					nsedelim = (char *) 0;
				}
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[i] = 0;
		}
		else
			num_files++;

	if(num_files == 0)
		fatal("missing file arg (cm3)");

	if (*(p = sname(argv[0])) == 'n')
		p++;
	if (equal(p,"rmdel"))
		D_type = 'R';		/* invoked as 'rmdel' */
	else if (equal(p,"cdc"))
		D_type = 'D';		/* invoked as 'cdc' */
	else
		fatal("bad invocation (rc10)");
	if (! logname())
		fatal("User ID not in password file (cm9)");

	setsig();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'rmchg' routine for each file argument.
	*/
	for (i=1; i<argc; i++)
		if (p = argv[i])
			do_file(p,rmchg);

	exit(Fcnt ? 1 : 0);
	/* NOTREACHED */
}


/*
	Routine that actually causes processing of the delta.
	Processing on the file takes place on a
	temporary copy of the SCCS file (the x-file).
	The name of the x-file is the same as that of the
	s-file (SCCS file) with the 's.' replaced by 'x.'.
	At end of processing, the s-file is removed
	and the x-file is renamed with the name of the old s-file.

	This routine makes use of the z-file to lock out simultaneous
	updates to the SCCS file by more than one user.
*/

struct packet gpkt;	/* see file s.h */
char line[BUFSIZ];
int Domrs;

USXALLOC();

rmchg(file)
char *file;
{
	static int first_time = 1;
	struct deltab dt;	/* see file s.defines.h */
	struct stats stats;	/* see file s.defines.h */
	extern char *Sflags[];
	int n, numdelts;
	char *p, *cp;
	int keep, found;
	extern char Pgmr[LOGSIZE], *getline();
	int fowner, downer, user;

	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of rmchg */

	if (!HADR)
		fatal("missing r (rc1)");

	if (D_type == 'D' && first_time) {
		first_time = 0;
		dohist(file);
	}

	if (!exists(file)) {
		sprintf(Error,"file %s does not exist (rc2)",
			nse_file_trim(file, 1));
		fatal(Error);
	}

	/*
	Lock out any other user who may be trying to process
	the same file.
	*/
	if (lockit(auxf(file,'z'),2,getpid()))
		fatal("cannot create lock file (cm4)");

	sinit(&gpkt,file,1);	/* initialize packet and open s-file */

	/*
	Flag for 'putline' routine to tell it to open x-file
	and allow writing on it.
	*/
	gpkt.p_upd = 1;

	/*
	Save requested SID for later checking of
	permissions (by 'permiss').
	*/
	move(&sid,&gpkt.p_reqsid,sizeof(gpkt.p_reqsid));

	/*
	Now read-in delta table. The 'dodelt' routine
	will read the table and change the delta entry of the
	requested SID to be of type 'R' if this is
	being executed as 'rmdel'; otherwise, for 'cdc', only
	the MR and comments sections will be changed 
	(by 'escdodelt', called by 'dodelt').
	*/
	if (dodelt(&gpkt,&stats,&sid,D_type) == 0)
		fmterr(&gpkt);

	/*
	Get serial number of requested SID from
	delta table just processed.
	*/
	D_serial = sidtoser(&gpkt.p_reqsid,&gpkt);

	/*
	If SID has not been zeroed (by 'dodelt'),
	SID was not found in file.
	*/
	if (sid.s_rel != 0)
		fatal("nonexistent sid (rc3)");
	/*
	Replace 'sid' with original 'sid'
	requested.
	*/
	move(&gpkt.p_reqsid,&sid,sizeof(gpkt.p_reqsid));

	/*
	Now check permissions.
	*/
	finduser(&gpkt);
	doflags(&gpkt);
	permiss(&gpkt);

	/*
	Check that user is either owner of file or
	directory, or is one who made the delta.
	*/
	fstat(fileno(gpkt.p_iop),&Statbuf);
	fowner = Statbuf.st_uid;
	copy(gpkt.p_file,line);		/* temporary for dname() */
	if (stat(dname(line),&Statbuf))
		downer = -1;
	else
		downer = Statbuf.st_uid;
	user = getuid();
	if (user != fowner && user != downer)
		if (!equal(Pgmr,logname())) {
			sprintf(Error,
				"you are neither owner nor '%s' (rc4)",Pgmr);
			fatal(Error);
		}

	/*
	For 'rmdel', check that delta being removed is a
	'leaf' delta, and if ok,
	process the body.
	*/
	if (D_type == 'R') {
		struct idel *ptr;
		for (n = maxser(&gpkt); n > D_serial; n--) {
			ptr = &gpkt.p_idel[n];
			if (ptr->i_pred == D_serial)
				fatal("not a 'leaf' delta (rc5)");
		}

		/*
		   For 'rmdel' check that the sid requested is
		   not contained in p-file, should a p-file
		   exist.
		*/

		if (exists(auxf(gpkt.p_file,'p')))
			rdpfile(&gpkt,&sid);

		flushto(&gpkt,EUSERTXT,COPY);

		keep = YES;
		found = NO;
		numdelts = 0;  		/* keeps count of number of deltas */
		gpkt.p_chkeof = 1;		/* set EOF is ok */
		while ((p = getline(&gpkt)) != NULL) {
			if (*p++ == CTLCHAR) {
				cp = p++;
				NONBLANK(p);
				/*
				Convert serial number to binary.
				*/
				if (*(p = satoi(p,&n)) != '\n')
					fmterr(&gpkt);
				if (n == D_serial) {
					gpkt.p_wrttn = 1;
					if (*cp == INS) {
						found = YES;
						keep = NO;
					}
					else
						keep = YES;
				}
				if (*cp == INS)
					/*
					   keep track of number of deltas  -
					   do not remove if it's the last one
					*/
					numdelts++;
			}
			else
				if (keep == NO)
					gpkt.p_wrttn = 1;
		}
	}
	else {
		numdelts = 2;    /* dummy - needed for rmdel, not cdc */
		/*
		This is for invocation as 'cdc'.
		Check MRs.
		*/
		if (Mrs) {
			if (!(p = Sflags[VALFLAG - 'a']))
				fatal("MRs not allowed (rc6)");
			if (*p && valmrs(&gpkt,p))
				fatal("invalid MRs (rc7)");
		}
		else
			if (Sflags[VALFLAG - 'a'])
				fatal("MRs required (rc8)");

		/*
		Indicate that EOF at this point is ok, and
		flush rest of s-file to x-file.
		*/
		gpkt.p_chkeof = 1;
		while (getline(&gpkt))
			;
	}

	flushline(&gpkt,0);
	if (numdelts < 2 && found == YES)
		/* they tried to delete the last remaining delta */
		fatal("may not remove the last delta (rc13)");
		

	/*
	Delete old s-file, change x-file name to s-file.
	*/
	rename(auxf(&gpkt,'x'),&gpkt);

	clean_up();
}


escdodelt(pkt)
struct packet *pkt;
{
	static int first_time = 1;
	extern int First_esc;
	extern int First_cmt;
	extern int CDid_mrs;
	char	*p, *mr_ptr;
	extern long Timenow;

	if (first_time) {
		/*
		 * if first time calling `escdodelt'
		 * then split the MR list from mrfixup.
		*/

		split_mrs();
		first_time = 0;
	}
	if (D_type == 'D' && First_esc) {	/* cdc, first time */
		First_esc = 0;
		if (Mrs) {
			/*
			 * if adding MRs then put out the new list
			 * from `split_mrs' (if any)
			*/

			putmrs(pkt);
			CDid_mrs = 1;	/* indicate that some MRs were read */
		}
	}

	if (pkt->p_line[1] == MRNUM) {		/* line read is MR line */
		p = pkt->p_line;
		while (*p)
			p++;
		if (*(p - 2) == DELIVER)
			fatal("delta specified has delivered MR (rc9)");
		if (!CDid_mrs)	/* if no MRs list from `dohist' then return */
			return;
		else
			/*
			 * check to see if this MR should be removed
			*/

			ckmrs(pkt,pkt->p_line);
	}
	else if (D_type == 'D') {		/* this line is a comment */
		if (First_cmt) {		/* first comment encountered */
			First_cmt = 0;
			/*
			 * if comments were read by `dohist' then
			 * put them out.
			*/

			if (*Comments) {
				sprintf(line,"%c%c %s\n",CTLCHAR,
					COMMENTS,savecmt(Comments));
				putline(pkt,line);
			}

			/*
			 * if any MRs were deleted, print them out
			 * as comments for this invocation of `cdc'
			*/

			put_delmrs(pkt);
			/*
			 * if comments were read by `dohist' then
			 * notify user that comments were CHANGED.
			*/

			if (*Comments) {
				sprintf(line,"%c%c ",CTLCHAR,COMMENTS);
				putline(pkt,line);
				putline(pkt,"*** CHANGED *** ");
				/* get date and time */
				date_ba(&Timenow,line);	
				putline(pkt,line);
				sprintf(line," %s\n",logname());
				putline(pkt,line);
			}
		}
		else return;
	}
}


extern char **Varg, *stalloc();

split_mrs()
{
	register char **argv;
	char	**dargv;
	char	**nargv;
	char	*ap;

	dargv = &Darg[VSTART];
	nargv = &NVarg[VSTART];
	for (argv = &Varg[VSTART]; *argv; argv++)
		if (*argv[0] == '!') {
			*argv += 1;
			ap = *argv;
			copy(ap,*dargv++ = stalloc(size(ap)));
			*dargv = 0;
			continue;
		}
		else {
			copy(*argv,*nargv++ = stalloc(size(*argv)));
			*nargv = 0;
		}
	Varg = NVarg;
}

putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char	str[64];

	for(argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}


clean_up()
{
	xrm(&gpkt);
	if (exists(auxf(gpkt.p_file,'x')))
		xunlink(auxf(gpkt.p_file,'x'));
	xfreeall();
	if (gpkt.p_file[0])
		unlockit(auxf(gpkt.p_file,'z'),getpid());
}


rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	struct pfile pf;
	char line[BUFSIZ];
	FILE *in, *fdfopen();

	in = xfopen(auxf(pkt->p_file,'p'),0);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		if (sp->s_rel == pf.pf_gsid.s_rel &&
			sp->s_lev == pf.pf_gsid.s_lev &&
			sp->s_br == pf.pf_gsid.s_br &&
			sp->s_seq == pf.pf_gsid.s_seq) {
				fclose(in);
				fatal("being edited -- sid is in p-file (rc12)");
		}
	}
	fclose(in);
	return;
}


ckmrs(pkt,p)
struct packet *pkt;
char *p;
{
	register char **argv, **eargv;
	char mr_no[BUFSIZ];
	char *mr_ptr;

	eargv = &Earg[VSTART];
	copy(p,mr_no);
	mr_ptr = mr_no;
	repl(mr_ptr,'\n','\0');
	mr_ptr += 3;
	for (argv = &Darg[VSTART]; *argv; argv++)
		if (equal(mr_ptr,*argv)) {
			pkt->p_wrttn = 1;
			copy(mr_ptr,*eargv++ = stalloc(size(mr_ptr)));
			*eargv = 0;
		}
}


put_delmrs(pkt)
struct	packet	*pkt;
{

	register char	**argv;
	register int first;
	char	str[64];

	first = 1;
	for(argv = &Earg[VSTART]; *argv; argv++) {
		if (first) {
			putline(pkt,"c *** LIST OF DELETED MRS ***\n");
			first = 0;
		}
		sprintf(str,"%c%c %s\n",CTLCHAR,COMMENTS,*argv);
		putline(pkt,str);
	}
}
