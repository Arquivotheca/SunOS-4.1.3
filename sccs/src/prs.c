/*************************************************************************/
/*									 */
/*	prs [-d<dataspec>] [-r<sid>] [-l] [-e] [-a] file ...		 */
/*									 */
/*************************************************************************/

/*
	Program to print parts or all of an SCCS file
	in user supplied format.
	Arguments to the program may appear in any order
	and consist of keyletters, which begin with '-',
	and named files.

	If a direcory is given as an argument, each
	SCCS file within the directory is processed as if
	it had been specifically named. If a name of '-'
	is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.
*/

# include "../hdr/defines.h"
# include "../hdr/had.h"

SCCSID(@(#)prs.c 1.12 88/05/03 SMI); /* from System III 5.1 */

USXALLOC();		/* use of xalloc instead of alloc */

char	Getpgm[]   =   "/usr/sccs/get";
static char defline[]     =     ":Dt:\t:DL:\nMRs:\n:MR:COMMENTS:\n:C:";
char	Sid[32];
char	Mod[FILESIZE];
char	Olddir[BUFSIZ];
char	Pname[BUFSIZ];
char	Dir[BUFSIZ];
char	*Type;
char	*Qsect;
char	Deltadate[18];
char	*Deltatime;
char	*getline();
char	tempskel[]   =   "/tmp/prXXXXXX";	/* used to generate temp
						   file names
						*/
char	untmp[32], uttmp[32], cmtmp[32];
char	mrtmp[32], bdtmp[32];
FILE	*UNiop;
FILE	*UTiop;
FILE	*CMiop;
FILE	*MRiop;
FILE	*BDiop;
FILE	*fdfopen();
char	line[BUFSIZ];
int	num_files;
int	HAD_CM, HAD_MR, HAD_FD, HAD_BD, HAD_UN;
char	dt_line[BUFSIZ];
char	*dataspec = &defline[0];
char	iline[BUFSIZ], xline[BUFSIZ], gline[BUFSIZ];
FILE	*maket();
struct	packet	gpkt;
struct	sid	sid;
struct	tm	*Dtime;
long 	Date_time;

main(argc,argv)
int argc;
char *argv[];
{
	register int j;
	register char *p;
	char c;
	extern process();
	extern int Fcnt;

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine, and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/
	for (j = 1; j < argc; j++)
		if (argv[j][0] == '-' && (c = argv[j][1])) {
			p = &argv[j][2];
			switch (c) {
			case 'r':	/* specified SID */
				if (*p) {
					if (invalid(p))
						fatal("invalid sid (co8)");
					sid_ab(p,&sid);
				}
				break;
			case 'c':	/* cutoff date[time] */
				if ((*p) && (date_ab(p, &Date_time) != -1))
					break;
				else
					fatal("invalid cutoff date");
			case 'l':	/* later than specified SID */
			case 'e':	/* earlier than specified SID */
			case 'a':	/* print all delta types (R or D) */
			case 'q': /* enable NSE mode */
				if (*p) {
					nsedelim = p;
				} else {
					nsedelim = (char *) 0;
				}
				break;
			case 'd':	/* dataspec line */
				if (*p)
					dataspec = p;
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[j] = 0;
		}
		else
			num_files++;

	if (num_files == 0)
		fatal("missing file arg (cm3)");

	if (HADR && HADC) 
                fatal("can't specify cutoff date and SID");
        if (HADC && (!HADL) && (!HADE)) 
                fatal("must specify -e or -l with -c"); 

	/*
	check the dataspec line and determine if any tmp files
	need be created
	*/
	ck_spec(dataspec);

	setsig();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'process' routine for each file argument.
	*/
	for (j = 1; j < argc; j++)
		if (p = argv[j])
			do_file(p,process);

	exit(Fcnt ? 1 : 0);
	/* NOTREACHED */
}


/*
 * This procedure opens the SCCS file and calls all subsequent
 * modules to perform 'prs'.  Once the file is finished, process
 * returns to 'main' to process any other possible files.
*/
process(file)
register	char	*file;
{
	static	int pr_fname = 0;
	extern	char	had_dir, had_standinp;

	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of 'process' */

	sinit(&gpkt,file,1);	/* init packet and open SCCS file */

	/*
	move value of global sid into gpkt.p_reqsid for
	later comparision.
	*/

	move(&sid,&gpkt.p_reqsid,sizeof(sid));

	gpkt.p_reopen = 1;	/* set reopen flag to 1 for 'getline' */

	/*
	Read delta table entries checking for format error and
	setting the value for the SID if none was specified.
	Also check to see if SID specified does in fact exists.
	*/

	deltblchk(&gpkt);
	/*
	create auxiliary file for User Name Section
	*/

	if (HAD_UN)
		aux_create(UNiop,untmp,EUSERNAM);
	else read_to(EUSERNAM,&gpkt);

	/*
	store flags (if any) into global array called 'Sflags'
	*/

	doflags(&gpkt);

	/*
	create auxiliary file for the User Text section
	*/

	if (HAD_FD)
		aux_create(UTiop,uttmp,EUSERTXT);
	else read_to(EUSERTXT,&gpkt);

	/*
	indicate to 'getline' that EOF is okay
	*/

	gpkt.p_chkeof = 1;

	/*
	read body of SCCS file and create temp file for it
	*/

	while(read_mod(&gpkt))
		;

	/*
	Here, file has already been re-opened (by 'getline' after
	EOF was encountered by 'read_mod' calling 'getline')
	*/

	getline(&gpkt);		/* skip over header line */

	if (!HADD && !HADR && !HADE && !HADL)
		HADE = pr_fname = 1;
	if (pr_fname)
		printf("%s:\n\n",nse_file_trim(file, 0));

	/*
	call 'dodeltbl' to read delta table entries
	and determine which deltas are to be considered
	*/

	dodeltbl(&gpkt);

	/*
	call 'clean_up' to remove any temporary file created
	during processing of the SCCS file passed as an argument from
	'do_file'
	*/

	clean_up();

	return;		/* return to caller of 'process' */
}


/*
 * This procedure actually reads the delta table entries and
 * substitutes pre-defined strings and pointers with the information
 * needed during the scanning of the 'dataspec' line
*/
dodeltbl(pkt)
register struct packet *pkt;
{
	char	*n;
	int	stopdel;
	int	found;
	long	dcomp;
	struct	deltab	dt;
	struct	stats	stats;

	/*
	flags used during determination of deltas to be
	considered
	*/

	found = stopdel = 0;

	/*
	Read entire delta table.
	*/
	while (getstats(pkt,&stats) && !stopdel) {
		if (getadel(pkt,&dt) != BDELTAB)
			fmterr(pkt);

		/*
		ignore 'removed' deltas if !HADA keyletter
		*/

		if (!HADA && dt.d_type != 'D') {
			read_to(EDELTAB,pkt);
			continue;
		}

		/*
		determine whether or not to consider current delta
		*/

        	if (HADC) {
	                dcomp = Date_time - dt.d_datetime;
	                if (HADE && !HADL && (dcomp < 0)) {
	                        read_to(EDELTAB, pkt);
	                        continue;
	                }
	                else if (HADL && !HADE && (dcomp > 0)) {
	                       stopdel = 1;
	                       continue;
	                }
	        }
	        else if (HADE && HADL) stopdel = 0;
	        else if (!(eqsid(&gpkt.p_reqsid, &dt.d_sid)) && !found) {
			/*
			if !HADL keyletter skip delta entry
			*/
			if (!HADL) {
				read_to(EDELTAB,pkt);
				continue;
			}
		}
		else {
			found = 1;
			stopdel = 1;
		}
		/*
		if HADE keyletter read remainder of delta table entries
		*/
		if (HADE && stopdel)
			stopdel = 0;
		/*
		create temp file for MRs and comments
		*/
		if (HAD_MR)
			MRiop = maket(mrtmp);
		if (HAD_CM)
			CMiop = maket(cmtmp);
		/*
		Read rest of delta entry. 
		*/
		while ((n = getline(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch (pkt->p_line[1]) {
				case INCLUDE:
					getit(iline,n);
					continue;
				case EXCLUDE:
					getit(xline,n);
					continue;
				case IGNORE:
					getit(gline,n);
					continue;
				case MRNUM:
					if (HAD_MR)
						putmr(n);
					continue;
				case COMMENTS:
					if (HAD_CM)
						putcom(n);
					continue;
				case EDELTAB:
					/*
					close temp files for MRs and comments
					*/
					if (HAD_MR)
						fclose(MRiop);
					if (HAD_CM)
						fclose(CMiop);
					scanspec(dataspec,&dt,&stats);
					/*
					remove temp files for MRs and comments
					*/
					unlink(mrtmp);
					unlink(cmtmp);
					break;
				default:
					fmterr(pkt);
				}
				break;
			}
		if (n == NULL || pkt->p_line[0] != CTLCHAR)
			fmterr(pkt);
	}
}


/*
 * The scanspec procedure scans the dataspec searching for ID keywords.
 * When a keyword is found the value is replaced and printed on the
 * standard output. Any character that is not an ID keyword is printed
 * immediately.
*/

extern	char	*Sflags[];
static	char	Zkywd[5]   =   "@(#)";

scanspec(spec,dtp,statp)
char spec[];
struct	deltab	*dtp;
struct	stats	*statp;
{

	register char *lp;
	register char	*k;
	register	char	c;
	register int istr;

	/*
	call 'idsetup' to set certain data keywords for
	'scanspec' substitution
	*/
	idsetup(&dtp->d_sid,&gpkt,&dtp->d_datetime);

	/*
	scan 'dataspec' line
	*/
	for(lp = spec; *lp != 0; lp++) {
		if(lp[0] == ':' && lp[1] != 0 && lp[2] == ':') {
			c = *++lp;
			switch (c) {
			case 'I':	/* SID */
				printf("%s",Sid);
				break;
			case 'R':	/* Release number */
				printf("%u",dtp->d_sid.s_rel);
				break;
			case 'L':	/* Level number */
				printf("%u",dtp->d_sid.s_lev);
				break;
			case 'B':	/* Branch number */
				if (dtp->d_sid.s_br != 0)
					printf("%u",dtp->d_sid.s_br);
				break;
			case 'S':	/* Sequence number */
				if (dtp->d_sid.s_seq != 0)
					printf("%u",dtp->d_sid.s_seq);
				break;
			case 'D':	/* Date delta created */
				printf("%s",Deltadate);
				break;
			case 'T':	/* Time delta created */
				printf("%s",Deltatime);
				break;
			case 'P':	/* Programmer who created delta */
				printf("%s",dtp->d_pgmr);
				break;
			case 'C':	/* Comments */
				if (exists(cmtmp))
					printfile(cmtmp);
				break;
			case 'Y':	/* Type flag */
				printf("%s",Type);
				break;
			case 'Q':	/* csect flag */
				printf("%s",Qsect);
				break;
			case 'J':	/* joint edit flag */
				if (Sflags[JOINTFLAG - 'a'])
					printf("yes");
				else printf("no");
				break;
			case 'M':	/* Module name */
				printf("%s",Mod);
				break;
			case 'W':	/* Form of what string */
				printf("%s%s\t%s",Zkywd,Mod,Sid);
				break;
			case 'A':	/* Form of what string */
				printf("%s%s %s %s%s",Zkywd,Type,Mod,Sid,Zkywd);
				break;
			case 'Z':	/* what string constructor */
				printf("%s",Zkywd);
				break;
			case 'F':	/* File name */
				printf("%s",sname(gpkt.p_file));
				break;
			default:
				putchar(':');
				--lp;
				continue;
			}
			lp++;
		}
		else if(lp[0]==':' && lp[1]!=0 && lp[2]!=0 && lp[3]==':') {
			if (lp[1] == ':') {
				putchar(':');
				continue;
			}
			istr = (256 * ((int)(lp[2]))) + ((int)(lp[1]));
			lp += 2;
			switch (istr) {
			case 256*'L'+'D':	/* :DL: Delta line statistics */
				printf("%05d",statp->s_ins);
				putchar('/');
				printf("%05d",statp->s_del);
				putchar('/');
				printf("%05d",statp->s_unc);
				break;
			case 256*'i'+'L':	/* :Li: Lines inserted by delta */
				printf("%05d",statp->s_ins);
				break;
			case 256*'d'+'L':	/* :Ld: Lines deleted by delta */
				printf("%05d",statp->s_del);
				break;
			case 256*'u'+'L':	/* :Lu: Lines unchanged by delta */
				printf("%05d",statp->s_unc);
				break;
			case 256*'T'+'D':	/* :DT: Delta type */
				printf("%c",dtp->d_type);
				break;
			case 256*'y'+'D':	/* :Dy: Year delta created */
				printf("%02d",Dtime->tm_year);
				break;
			case 256*'m'+'D':	/* :Dm: Month delta created */
				printf("%02d",(Dtime->tm_mon + 1));
				break;
			case 256*'d'+'D':	/* :Dd: Day delta created */
				printf("%02d",Dtime->tm_mday);
				break;
			case 256*'h'+'T':	/* :Th: Hour delta created */
				printf("%02d",Dtime->tm_hour);
				break;
			case 256*'m'+'T':	/* :Tm: Minutes delta created */
				printf("%02d",Dtime->tm_min);
				break;
			case 256*'s'+'T':	/* :Ts: Seconds delta created */
				printf("%02d",Dtime->tm_sec);
				break;
			case 256*'S'+'D':	/* :DS: Delta sequence number */
				printf("%d",dtp->d_serial);
				break;
			case 256*'P'+'D':	/* :DP: Predecessor delta sequence number */
				printf("%d",dtp->d_pred);
				break;
			case 256*'I'+'D':	/* :DI: Deltas included,excluded,ignored */
				printf("%s",iline);
				if (length(xline))
					printf("/%s",xline);
				if (length(gline))
					printf("/%s",gline);
				break;
			case 256*'n'+'D':	/* :Dn: Deltas included */
				printf("%s",iline);
				break;
			case 256*'x'+'D':	/* :Dx: Deltas excluded */
				printf("%s",xline);
				break;
			case 256*'g'+'D':	/* :Dg: Deltas ignored */
				printf("%s",gline);
				break;
			case 256*'K'+'L':	/* :LK: locked releases */
				if (k = Sflags[LOCKFLAG - 'a'])
					printf("%s",k);
				else printf("none");
				break;
			case 256*'R'+'M':	/* :MR: MR numbers */
				if (exists(mrtmp))
					printfile(mrtmp);
				break;
			case 256*'N'+'U':	/* :UN: User names */
				if (exists(untmp))
					printfile(untmp);
				break;
			case 256*'F'+'M':	/* :MF: MR validation flag */
				if (Sflags[VALFLAG - 'a'])
					printf("yes");
				else printf("no");
				break;
			case 256*'P'+'M':	/* :MP: MR validation program */
				if (!(k = Sflags[VALFLAG - 'a']))
					printf("none");
				else printf("%s",k);
				break;
			case 256*'F'+'K':	/* :KF: Keyword err/warn flag */
				if (Sflags[IDFLAG - 'a'])
					printf("yes");
				else printf("no");
				break;
		         case 256*'V'+'K':  /*:KV: Keyword validation string*/
		                if (!(k = Sflags[IDFLAG - 'a']))
		                        printf("none");
		                else printf("%s",k);
				break;
			case 256*'F'+'B':	/* :BF: Branch flag */
				if (Sflags[BRCHFLAG - 'a'])
					printf("yes");
				else printf("no");
				break;
			case 256*'B'+'F':	/* :FB: Floor Boundry */
				if (k = Sflags[FLORFLAG - 'a'])
					printf("%s",k);
				else printf("none");
				break;
			case 256*'B'+'C':	/* :CB: Ceiling Boundry */
				if (k = Sflags[CEILFLAG - 'a'])
					printf("%s",k);
				else printf("none");
				break;
			case 256*'s'+'D':	/* :Ds: Default SID */
				if (k = Sflags[DEFTFLAG - 'a'])
					printf("%s",k);
				else printf("none");
				break;
			case 256*'D'+'N':	/* :ND: Null delta */
				if (Sflags[NULLFLAG - 'a'])
					printf("yes");
				else printf("no");
				break;
			case 256*'D'+'F':	/* :FD: File descriptive text */
				if (exists(uttmp))
					printfile(uttmp);
				break;
			case 256*'D'+'B':	/* :BD: Entire file body */
				if (exists(bdtmp))
					printfile(bdtmp);
				break;
			case 256*'B'+'G':	/* :GB: Gotten body from 'get' */
				getbody(&dtp->d_sid,&gpkt);
				break;
			case 256*'N'+'P':	/* :PN: Full pathname of File */
				copy(gpkt.p_file,Dir);
				dname(Dir);
				if(curdir(Olddir) != 0)
					fatal("curdir failed (prs2)");
				if(chdir(Dir) != 0)
					fatal("cannot change directory (prs3)");
				if(curdir(Pname) != 0)
					fatal("curdir failed (prs2)");
				if(chdir(Olddir) != 0)
					fatal("cannot change directory (prs3)");
				printf("%s/",Pname);
				printf("%s",sname(gpkt.p_file));
				break;
			case 256*'L'+'F':	/* :FL: Flag descriptions (as in 'prt') */
				printflags();
				break;
			case 256*'t'+'D':	/* :Dt: Whole delta table line */
				/*
				replace newline with null char to make
				data keyword simple format
				*/
				repl(dt_line,'\n','\0');
				k = dt_line;
				/*
				skip control char, line flag, and blank
				*/
				k += 3;
				printf("%s",k);
				break;
			default:
				putchar(':');
				lp -= 2;
				continue;
			}
			lp++;
		}
		else {
			c = *lp;
			if (c == '\\') {
				switch(*++lp) {
				case 'n':	/* for newline */
					putchar('\n');
					break;
				case ':':	/* for wanted colon */
					putchar(':');
					break;
				case 't':	/* for tab */
					putchar('\t');
					break;
				case 'b':	/* for backspace */
					putchar('\b');
					break;
				case 'r':	/* for carriage return */
					putchar('\r');
					break;
				case 'f':	/* for form feed */
					putchar('\f');
					break;
				case '\\':	/* for backslash */
					putchar('\\');
					break;
				case '\'':	/* for single quote */
					putchar('\'');
					break;
				default:	/* unknown case */
					putchar('\\');
					putchar(*lp);
					break;
				}
			}
			else putchar(*lp);
		}
	}
	/*
	zero out first char of global string lines in case
	a value is not gotten in next delta table entry
	*/
	iline[0] = xline[0] = gline[0] = 0;
	putchar('\n');
	return;
}


/*
 * This procedure cleans up all temporary files created during
 * 'process' that are used for data keyword substitution
*/
clean_up()
{
	if (gpkt.p_iop)		/* if SCCS file is open, close it */
		fclose(gpkt.p_iop);
	xrm(&gpkt);	      /* remove the 'packet' used for this SCCS file */
	unlink(mrtmp);		/* remove all temporary files from /tmp */
	unlink(cmtmp);		/*			"		*/
	unlink(untmp);		/*			"		*/
	unlink(uttmp);		/*			"		*/
	unlink(bdtmp);		/*			"		*/
}


/* This function takes as it's argument the SID inputed and determines
 * whether or not it is valid (e. g. not ambiguous or illegal).
*/
invalid(i_sid)
register char	*i_sid;
{
	register int count;
	register int digits;
	count = digits = 0;
	if (*i_sid == '0' || *i_sid == '.')
		return (1);
	i_sid++;
	digits++;
	while (*i_sid != '\0') {
		if (*i_sid++ == '.') {
			digits = 0;
			count++;
			if (*i_sid == '0' || *i_sid == '.')
				return (1);
		}
		digits++;
		if (digits > 5)
			return (1);
	}
	if (*(--i_sid) == '.' )
		return (1);
	if (count == 1 || count == 3)
		return (0);
	return (1);
}


/*
 * This procedure checks the delta table entries for correct format.
 * It also checks to see if the SID specified by the -r keyletter
 * is contained in the file.  If no SID was specified assumes the top
 * delta created (last in time).
*/
deltblchk(pkt)
register struct packet *pkt;
{
	char	*n;
	int	have;
	int	found;
	struct	deltab	dt;
	struct	stats	stats;

	have = found = 0;
	/*
	Read entire delta table.
	*/
	while (getstats(pkt,&stats)) {
		if (getadel(pkt,&dt) != BDELTAB)
			fmterr(pkt);

		/*
		if no SID was specified, get top delta
		*/
		if (pkt->p_reqsid.s_rel == 0 && !have) {
			/*
			ignore if "removed" delta 
			*/
			if (!HADA && dt.d_type != 'D') {
				read_to(EDELTAB,pkt);
				continue;
			}
			/*
			move current SID into SID to look at
			*/
			move(&dt.d_sid, &gpkt.p_reqsid, sizeof(sid));
			found = have = 1;
		}
		/*
		if SID was specified but not located yet check
		to see if this SID is the one
		*/
		if (pkt->p_reqsid.s_rel != 0 && !found)
			if (eqsid(&gpkt.p_reqsid, &dt.d_sid))
				found = 1;
		/*
		Read rest of delta entry. 
		*/
		while ((n = getline(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch (pkt->p_line[1]) {
				case EDELTAB:
					break;
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
				case MRNUM:
				case COMMENTS:
					continue;
				default:
					fmterr(pkt);
				}
				break;
			}
		if (n == NULL || pkt->p_line[0] != CTLCHAR)
			fmterr(pkt);
	}
	/*
	if not at the beginning of the User Name section
	there is an internal error
	*/
	if (pkt->p_line[1] != BUSERNAM)
		fmterr(pkt);
	/*
	if SID did not exist (the one specified by -r keyletter)
	then there exists an error
	*/
	if (!found)
		fatal("nonexistent SID (prs1)");
}


/*
 * This procedure reads the stats line from the delta table entry
 * and places the statisitics into a structure called "stats".
*/
getstats(pkt,statp)
register struct packet *pkt;
register struct stats *statp;
{
	register char *p;

	p = pkt->p_line;
	if (getline(pkt) == NULL || *p++ != CTLCHAR || *p++ != STATS)
		return(0);
	NONBLANK(p);
	p = satoi(p,&statp->s_ins);
	p = satoi(++p,&statp->s_del);
	satoi(++p,&statp->s_unc);
	return(1);
}


/*
 * This procedure reads a delta table entry line from the delta
 * table entry and places the contents of the line into a structure
 * called "deltab".
*/
getadel(pkt,dt)
register struct packet *pkt;
register struct deltab *dt;
{
	if (getline(pkt) == NULL)
		fmterr(pkt);
	copy(pkt->p_line,dt_line);  /* copy delta table line for :Dt: keywd */
	return(del_ab(pkt->p_line,dt,pkt));
}


/*
 * This procedure creates the temporary file used during the
 * "process" subroutine.  The skeleton defined at the beginning
 * of the program is filled in in this function
*/
FILE *
maket(file)
char	*file;
{
	FILE *iop;

	copy(tempskel,file);	/* copy file name into the skeleton */
	iop = xfcreat(mktemp(file),0644);

	return(iop);
}


/*
 * This procedure prints (on the standard output) the contents of any
 * temporary file that may have been created during "process".
*/
printfile(file)
register	char	*file;
{
	register	char	*p;
	FILE	*iop;

	iop = xfopen(file,0);
	while ((p = fgets(line,sizeof(line),iop)) != NULL)
		printf("%s",p);
	fclose(iop);
}


/*
 * This procedure reads the body of the SCCS file from beginning to end.
 * It also creates the temporary file /tmp/prbdtmp which contains
 * the body of the SCCS file for data keyword substitution.
*/
read_mod(pkt)
register struct packet *pkt;
{
	register char *p;
	int ser;
	int iod;
	register struct apply *ap;

	if (HAD_BD)
		BDiop = maket(bdtmp);
	while (getline(pkt) != NULL) {
		p = pkt->p_line;
		if (HAD_BD)
			fputs(p,BDiop);
		if (*p++ != CTLCHAR)
			continue;
		else {
			if (!((iod = *p++) == INS || iod == DEL || iod == END))
				fmterr(pkt);
			NONBLANK(p);
			satoi(p,&ser);
/*
			if (iod == END)
				remq(pkt,ser);
			else if ((ap = &pkt->p_apply[ser])->a_code == APPLY)
				addq(pkt,ser,iod == INS ? YES : NO,iod,ap->a_reason & USER);
			else
				addq(pkt,ser,iod == INS ? NO : NULL,iod,ap->a_reason & USER);
*/
		}
	}
	if (HAD_BD)
		fclose(BDiop);
	if (pkt->p_q)
		fatal("premature eof (co5)");
	return(0);
}


/*
 * This procedure is only called if the :GB: data keyword is specified.
 * It forks and creates a child process to invoke 'get' with the '-p'
 * and '-s' options for the SID currently being processed.  Upon
 * completion, control of the program is returned to 'prs'.
*/
getbody(gsid,pkt)
struct	sid	*gsid;
struct packet *pkt;
{
	int	i;
	int	status;
	extern	char	Getpgm[];
	char	str[128];
	char	rarg[20];
	char	filearg[80];

	sid_ba(gsid,str);
	sprintf(rarg,"-r%s",str);
	sprintf(filearg,"%s",pkt->p_file);
	/*
	fork here so 'getbody' can execute 'get' to
	print out gotten body :GB:
	*/
	if ((i = fork()) < 0)
		fatal("cannot fork, try again");
	if (i == 0) {
		/*
		perform 'get' and redirect output
		to standard output
		*/
		execl(Getpgm,Getpgm,"-s","-p",rarg,filearg,(char *)0);
		sprintf(Error,"cannot execute '%s'",Getpgm);
		fatal(Error);
	}
	else {
		wait(&status);
		return;
	}
}


/*
 * This procedure places the line read in "dodeltbl" into a global string
 * 'str'.  This procedure is only called for include, exclude or ignore
 * lines.
*/
getit(str,cp)
register	char	*str, *cp;
{
	cp += 2;
	NONBLANK(cp);
	cp[length(cp) - 1] = '\0';
	sprintf(str,"%s",cp);
}


/*
 * This procedure creates an auxiliary file for the iop passed as an argument
 * for the file name also passed as an argument.  If no text exists for the
 * named file, an auxiliary file is still created with the text "(none)".
*/
aux_create(iop,file,delchar)
FILE	*iop;
char	*file;
char	delchar;
{

	char	*n;
	int	text;
	/*
	create auxiliary file for the named section
	*/

	text = 0;
	iop = maket(file);
	while ((n = getline(&gpkt)) != NULL && gpkt.p_line[0] != CTLCHAR) {
		text = 1;
		fputs(n,iop);
	}
	/*
	check to see that delimiter found is correct
	*/
	if (n == NULL || gpkt.p_line[0] != CTLCHAR || gpkt.p_line[1] != delchar)
		fmterr(&gpkt);
	if (!text)
		fprintf(iop,"(none)\n");
	fclose(iop);
}


/*
 * This procedure sets the values for certain data keywords which are
 * either shared by more than one data keyword or because substitution
 * here would be easier than doing it in "scanspec" (more efficient etc.)
*/
idsetup(gsid,pkt,bdate)
struct	sid	*gsid;
struct	packet	*pkt;
long	*bdate;
{

	register	char	*p;
	extern	struct	tm	*localtime();

	date_ba(bdate,Deltadate);
	Deltatime = &Deltadate[9];
	Deltadate[8] = 0;
	sid_ba(gsid,Sid);
	tzsetwall();	/* always use the machine's local time */
	Dtime = localtime(bdate);
	if (p = Sflags[MODFLAG - 'a'])
		copy(p,Mod);
	else sprintf(Mod,"%s",auxf(pkt->p_file,'g'));
	if (!(Type = Sflags[TYPEFLAG - 'a']))
		Type = Null;
	if (!(Qsect = Sflags[QSECTFLAG - 'a']))
		Qsect = Null;
}


/*
 * This procedure places any MRs that are found in the delta table entry
 * into the temporary file created for that express purpose (/tmp/prmrtmp).
*/
putmr(cp)
register char	*cp;
{

	cp += 3;

	if (!(*cp) || (*cp == '\n')) {
		fclose(MRiop);
		unlink(mrtmp);
		return;
	}

	fputs(cp,MRiop);
}


/*
 * This procedure is the same as "putmr" except it is used for the comment
 * section of the delta table entries.
*/
putcom(cp)
register char	*cp;
{

	cp += 3;

	fputs(cp,CMiop);

}


/*
 * This procedure reads through the SCCS file until a line is found
 * containing the character passed as an argument in the 2nd position
 * of the line.
*/
read_to(ch,pkt)
register char	ch;
register struct packet *pkt;
{
	register char *p;

	while ((p = getline(pkt)) &&
			!(*p++ == CTLCHAR && *p == ch))
		;
	return;
}


/*
 * This procedure prints a list of all the flags that are present in the
 * SCCS file.  The format is the same as 'prt' except the flag description
 * is not preceeded by a "tab".
*/
printflags()
{
	register	char	*k;

	if (Sflags[BRCHFLAG - 'a'])	/* check for 'branch' flag */
		printf("branch\n");
	if ((k = (Sflags[CEILFLAG - 'a'])))	/* check for 'ceiling flag */
		printf("ceiling\t%s\n",k);
	if ((k = (Sflags[DEFTFLAG - 'a'])))  /* check for 'default SID' flag */
		printf("default SID\t%s\n",k);
	if ((k = (Sflags[ENCODEFLAG - 'a']))) {	/* check for 'encoded' flag */
		/*
		 * The semantics of this flag are unusual: report its
		 * existence only if an operand with value '1' is present.
		 * Yes, the test below is sloppy...
		 */
		if (*k == '1')
			printf("encoded\n");
	}
	if ((k = (Sflags[FLORFLAG - 'a'])))	/* check for 'floor' flag */
		printf("floor\t%s\n",k);
	if (Sflags[IDFLAG - 'a'])	/* check for 'id err/warn' flag */
		printf("id keywd err/warn\n");
	if (Sflags[JOINTFLAG - 'a'])	/* check for joint edit flag */
		printf("joint edit\n");
	if ((k = (Sflags[LOCKFLAG - 'a'])))	/* check for 'lock' flag */
		printf("locked releases\t%s\n",k);
	if ((k = (Sflags[MODFLAG - 'a'])))	/* check for 'module' flag */
		printf("module\t%s\n",k);
	if (Sflags[NULLFLAG - 'a'])	/* check for 'null delta' flag */
		printf("null delta\n");
	if ((k = (Sflags[QSECTFLAG - 'a'])))	/* check for 'qsect' flag */
		printf("csect name\t%s\n",k);
	if ((k = (Sflags[TYPEFLAG - 'a'])))	/* check for 'type' flag */
		printf("type\t%s\n",k);
	if (Sflags[VALFLAG - 'a']) {	/* check for 'MR valid' flag */
		printf("validate MRs\t");
		/*
		check for MR validating program
		(optional)
		*/
		if (k = (Sflags[VALFLAG - 'a']))
			printf("%s\n",k);
		else putchar('\n');
	}
	return;
}

/*
 * This procedure checks the `dataspec' (if user defined) and determines
 * if any temporary files need be created for future keyword replacement
*/
ck_spec(p)
register char *p;
{
	if (sindex(p,":C:") != -1)	/* check for Comment keyword */
		HAD_CM = 1;
	if (sindex(p,":MR:") != -1)	/* check for MR keyword */
		HAD_MR = 1;
	if (sindex(p,":UN:") != -1)	/* check for User name keyword */
		HAD_UN = 1;
	if (sindex(p,":FD:") != -1)	/* check for descriptive text kyword */
		HAD_FD = 1;
	if (sindex(p,":BD:") != -1)	/* check for body keyword */
		HAD_BD = 1;
}
