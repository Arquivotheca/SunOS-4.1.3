# include	"../hdr/defines.h"
# include	"../hdr/had.h"
# include	<sys/wait.h>
# include	<values.h>
# include	<vfork.h>

SCCSID(@(#)delta.c 1.1 92/07/30 SMI); /* from System III 5.1 */

/*
 * H_THRESHOLD_DEF is an experimentally determined threshold number of 
 * lines above which diff becomes slow and diff -h is used instead.
 * It can be over-ruled by the value of the h option
 */
#define H_THRESHOLD_DEF	2560	

USXALLOC();

char	Diffpgm[]   =   "/bin/diff";
FILE	*Diffin;
int	Debug = 0;
struct packet gpkt;
struct sid sid;
int	num_files;
char	*ilist, *elist, *glist;
char	*Comments, *Mrs;
int	Domrs;
int	verbosity;
int	Did_id;
long	Szqfile;
char	Pfilename[FILESIZE], *fgets();
FILE	*Xiop;
int	Xcreate;
static int lines();
int 	Newlines, Oldlines;
static	int	diffh_threshold;

extern char	*auxf();
extern FILE	*open_sccs_log();

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	int testmore;
	extern delta();
	extern	char	*savecmt();
	extern int Fcnt;

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'g':
				glist = p;
				break;
			case 'y':
				Comments = p;
				break;
			case 'm':
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'p':
			case 'n':
			case 's':
			case 'f': /* force delta without p. file (NSE only) */
				testmore++;
				break;
			case 'h': /* allow diffh for large files (NSE only) */
				if (*p ) {
					diffh_threshold = atoi(p);
				} else {
					diffh_threshold = H_THRESHOLD_DEF;
				}
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

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
						"value after %c arg (cm7)",c);
					fatal(Error);
				}
			}
			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[i] = 0;
		}
		else num_files++;

	if ((HADF || HADH) && !HADQ) {
		fatal("unknown key letter (cm1)");
	}
	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (!HADS)
		verbosity = -1;
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,delta);
	exit(Fcnt ? 1 : 0);
	/* NOTREACHED */
}


static long gfile_mtime;

delta(file)
{
	static int first = 1;
	register char *p;
	int n, linenum;
	char type;
	register int ser;
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	char dfilename[FILESIZE];
	char gfilename[FILESIZE];
	char line[LINESIZE];
	FILE *gin, *fdfopen(), *dodiff();
	struct stats stats;
	struct pfile *pp, *rdpfile();
	struct stat sbuf;
	int inserted, deleted, orig;
	int newser;
	int i;
	union wait status;
	FILE *efp;

	gin = NULL;
	gfile_mtime = 0;
	if (setjmp(Fjmp)) {
		if (gin != NULL) {
			fclose(gin);
		}
		return;
	}
	sinit(&gpkt,file,1);
	if (first) {
		first = 0;
		dohist(file);
	}
	if (lockit(auxf(gpkt.p_file,'z'),2,getpid()))
		fatal("cannot create lock file (cm4)");
	gpkt.p_reopen = 1;
	gpkt.p_stdout = stdout;
	copy(auxf(gpkt.p_file,'g'),gfilename);
	gin = xfopen(gfilename,0);
	if (dodelt(&gpkt,&stats,0,0) == 0)
		fmterr(&gpkt);
	if ((HADF) && !exists(auxf(gpkt.p_file,'p'))) {
		/* if no p. file exists delta can still happen if
		 * -f flag given (in NSE mode) - uses the same logic
		 * as get -e to assign a new SID */
		gpkt.p_reqsid.s_rel = 0;
		gpkt.p_reqsid.s_lev = 0;
		gpkt.p_reqsid.s_br = 0;
		gpkt.p_reqsid.s_seq = 0;
		gpkt.p_cutoff = time(0);
		ilist = 0;
		elist = 0;
		ser = getser(&gpkt, 0, 0);
		newsid(&gpkt, 0, 0);
	} else {	
		pp = rdpfile(&gpkt,&sid);
		gpkt.p_cutoff = pp->pf_date;
		ilist = pp->pf_ilist;
		elist = pp->pf_elist;
		if ((ser = sidtoser(&pp->pf_gsid,&gpkt)) == 0 ||
			sidtoser(&pp->pf_nsid,&gpkt))
				fatal("invalid sid in p-file (de3)");
		move(&pp->pf_nsid,&gpkt.p_reqsid,sizeof(gpkt.p_reqsid));
	}
	if (HADQ && stat(gfilename, &sbuf) == 0) {
		/* In NSE mode, the mtime of the clear file is remembered for
		 * use as delta time. Sccs is thus now vulnerable to clock
		 * skew between NFS server and host machine and to a mis-set
		 * clock when file is last changed.
		 */
		gfile_mtime = sbuf.st_mtime;
	}

	doie(&gpkt,ilist,elist,glist);
	setup(&gpkt,ser);
	finduser(&gpkt);
	doflags(&gpkt);
	permiss(&gpkt);
	flushto(&gpkt,EUSERTXT,1);
	gpkt.p_chkeof = 1;
	/* if encode flag is set, encode the g-file before diffing it
	 * with the s.file
	 */
	if (Sflags[ENCODEFLAG - 'a'] &&
	    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
		efp = xfcreat(auxf(gpkt.p_file,'e'),0644);
		encode(gin,efp);
		fclose(efp);
		fclose(gin);
		gin = xfopen(auxf(gpkt.p_file,'e'),0);
	}
	copy(auxf(gpkt.p_file,'d'),dfilename);
	gpkt.p_gout = xfcreat(dfilename,0444);
	while(readmod(&gpkt)) {
		chkid(gpkt.p_line);
		if (fputs(gpkt.p_line, gpkt.p_gout) == EOF)
			xmsg(dfilename, "delta");
	}
	if (fflush(gpkt.p_gout) == EOF)
		xmsg(dfilename, "delta");
	if (fsync(fileno(gpkt.p_gout)) < 0)
		xmsg(dfilename, "delta");
	if (fclose(gpkt.p_gout) == EOF)
		xmsg(dfilename, "delta");
	orig = gpkt.p_glnno;
	gpkt.p_glnno = 0;
	gpkt.p_verbose = verbosity;
	Did_id = 0;
	while (fgets(line,sizeof(line),gin) != NULL && !chkid(line))
		;
	fclose(gin);
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		(void) fprintf(gpkt.p_stdout, "\n%s:\n",
				nse_file_trim(gpkt.p_file, 0));
	if (!Did_id && !HADQ)
		if (Sflags[IDFLAG - 'a'])
			fatal("no id keywords (cm6)");
		else if (gpkt.p_verbose)
			(void) fprintf(stderr, "No id keywords (cm7)\n");

	/*
	Execute 'diff' on g-file and d-file.
	*/
	inserted = deleted = 0;
	gpkt.p_glnno = 0;
	gpkt.p_upd = 1;
	gpkt.p_wrttn = 1;
	getline(&gpkt);
	gpkt.p_wrttn = 1;
	if (HADF) {
		newser = mkdelt(&gpkt, &gpkt.p_reqsid, &gpkt.p_gotsid, orig);
	} else {
		newser = mkdelt(&gpkt,&pp->pf_nsid,&pp->pf_gsid, orig);
	}
	flushto(&gpkt,EUSERTXT,0);
	if (Sflags[ENCODEFLAG - 'a'] &&
	    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0))
		Diffin = dodiff(auxf(gpkt.p_file,'e'),dfilename);
	else
		Diffin = dodiff(auxf(gpkt.p_file,'g'),dfilename);
	while (n = getdiff(&type,&linenum)) {
		if (type == INS) {
			inserted += n;
			insert(&gpkt,linenum,n,newser);
		}
		else {
			deleted += n;
			delete(&gpkt,linenum,n,newser);
		}
	}
	fclose(Diffin);
	if (gpkt.p_iop)
		while (readmod(&gpkt))
			;
	wait(&status);
	if (status.w_termsig || status.w_retcode > 1) {	 /* diff trouble */
		/*
		Check exit code of child.
		*/
		switch (status.w_retcode) {

		case 32: 	/* 'execl' failed */
			sprintf(Error,
				"cannot execute '%s' (de12)",Diffpgm);
			fatal(Error);
			break;

		default:	/* Diff had trouble, or other */
			sprintf(Error,
			  "trouble executing '%s', status %d (de12)",
				Diffpgm, status);
			fatal(Error);
			break;
		}
	}
	if (Sflags[ENCODEFLAG - 'a'] &&
	    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
		fgetchk(auxf(gpkt.p_file,'e'),&gpkt);
		unlink(auxf(gpkt.p_file,'e'));
	}
	else
		fgetchk(gfilename,&gpkt);
	unlink(dfilename);
	stats.s_ins = inserted;
	stats.s_del = deleted;
	stats.s_unc = orig - deleted;
	if (gpkt.p_verbose) {
		(void) fprintf(gpkt.p_stdout, "%u inserted\n", stats.s_ins);
		(void) fprintf(gpkt.p_stdout, "%u deleted\n", stats.s_del);
		(void) fprintf(gpkt.p_stdout, "%u unchanged\n", stats.s_unc);
	}
	flushline(&gpkt,&stats);
	stat(gpkt.p_file,&sbuf);
	rename(auxf(gpkt.p_file,'x'),gpkt.p_file);
	chown(gpkt.p_file,sbuf.st_uid,sbuf.st_gid);
	if (!(HADF)) {
		if (Szqfile)
			rename(auxf(gpkt.p_file,'q'),Pfilename);
		else {
			xunlink(Pfilename);
			xunlink(auxf(gpkt.p_file,'q'));
		}
	}
	clean_up(0);
	if (!HADN) {
		(void) fflush(gpkt.p_stdout);
		(void) fflush(stderr);
		if ((i = vfork()) < 0)
			fatal("cannot fork, try again");
		if (i == 0) {
			setuid(getuid());
			unlink(gfilename);
			_exit(0);
		}
		else {
			wait(&status);
		}
	}
}


mkdelt(pkt,sp,osp,orig_nlines)
struct packet *pkt;
struct sid *sp, *osp;
int orig_nlines;
{
	extern long Timenow;
	struct deltab dt;
	char str[BUFSIZ];
	int newser;
	extern char *Sflags[];
	register char *p;
	int ser_inc, opred, nulldel;
#ifdef LOGGING
	FILE	*fopen(), *logfile;
	char	*getwd(), pathname[1024];

	logfile = open_sccs_log();

	if (logfile != NULL) {
		fprintf(logfile, "%s/%s\n", getwd(pathname), pkt->p_file);
	}
#endif

	if (pkt->p_verbose) {
		sid_ba(sp,str);
		(void) fprintf(pkt->p_stdout, "%s\n", str);
		(void) fflush(pkt->p_stdout);
		(void) fflush(stderr);
	}
	sprintf(str,"%c%c00000\n",CTLCHAR,HEAD);
	putline(pkt,str);
	newstats(pkt,str,"0");
	move(sp,&dt.d_sid,sizeof(dt.d_sid));

	/*
	Check if 'null' deltas should be inserted
	(only if 'null' flag is in file and
	releases are being skipped) and set
	'nulldel' indicator appropriately.
	*/
	if (Sflags[NULLFLAG - 'a'] && (sp->s_rel > osp->s_rel + 1) &&
			!sp->s_br && !sp->s_seq &&
			!osp->s_br && !osp->s_seq)
		nulldel = 1;
	else
		nulldel = 0;
	/*
	Calculate how many serial numbers are needed.
	*/
	if (nulldel)
		ser_inc = sp->s_rel - osp->s_rel;
	else
		ser_inc = 1;
	/*
	Find serial number of the new delta.
	*/
	newser = dt.d_serial = maxser(pkt) + ser_inc;
	/*
	Find old predecessor's serial number.
	*/
	opred = sidtoser(osp,pkt);
	if (nulldel)
		dt.d_pred = newser - 1;	/* set predecessor to 'null' delta */
	else
		dt.d_pred = opred;
	dt.d_datetime = Timenow;
	/* Since the NSE always preserves the clear file after delta and
	 * makes it read only (no get is required since keywords are not
	 * supported), the delta time is set to be the mtime of the clear
	 * file.
	 */
	if (HADQ && (gfile_mtime != 0)) {
		dt.d_datetime = gfile_mtime;
	}
	substr(logname(),dt.d_pgmr,0,LOGSIZE-1);
	dt.d_type = 'D';
	del_ba(&dt,str);
	putline(pkt,str);
#ifdef LOGGING
	if (logfile != NULL) {
		fputs(str, logfile);
	}
#endif
	if (ilist)
		mkixg(pkt,INCLUSER,INCLUDE);
	if (elist)
		mkixg(pkt,EXCLUSER,EXCLUDE);
	if (glist)
		mkixg(pkt,IGNRUSER,IGNORE);
	if (Mrs) {
		if (!(p = Sflags[VALFLAG - 'a']))
			fatal("MRs not allowed (de8)");
		if (*p && valmrs(pkt,p))
			fatal("invalid MRs (de9)");
		putmrs(pkt);
	}
	else if (Sflags[VALFLAG - 'a'])
		fatal("MRs required (de10)");
	sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
	putline(pkt,str);
#ifdef LOGGING
	if (logfile != NULL) {
		fputs(str, logfile);
	}
#endif
	sprintf(str,"%s",savecmt(Comments));
	putline(pkt,str);
#ifdef LOGGING
	if (logfile != NULL) {
		fputs(str, logfile);
	}
#endif
	putline(pkt,"\n");
#ifdef LOGGING
	if (logfile != NULL) {
		fputs("\n", logfile);
	}
#endif
	sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
	putline(pkt,str);
	if (nulldel)			/* insert 'null' deltas */
		while (--ser_inc) {
			putline(pkt,sprintf(str,"%c%c %s/%s/%05u\n",
				CTLCHAR, STATS,
				"00000", "00000", orig_nlines));
			dt.d_sid.s_rel -= 1;
			dt.d_serial -= 1;
			if (ser_inc != 1)
				dt.d_pred -= 1;
			else
				dt.d_pred = opred;	/* point to old pred */
			del_ba(&dt,str);
			putline(pkt,str);
			sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
			putline(pkt,str);
			putline(pkt,"AUTO NULL DELTA\n");
			sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
			putline(pkt,str);
		}
#ifdef LOGGING
	if (logfile != NULL) {
		fputs("\n", logfile);
		fclose(logfile);
	}
#endif
	return(newser);
}


mkixg(pkt,reason,ch)
struct packet *pkt;
int reason;
char ch;
{
	int n;
	char str[LINESIZE];

	sprintf(str,"%c%c",CTLCHAR,ch);
	putline(pkt,str);
	for (n = maxser(pkt); n; n--) {
		if (pkt->p_apply[n].a_reason == reason) {
			sprintf(str," %u",n);
			putline(pkt,str);
		}
	}
	putline(pkt,"\n");
}


putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[64];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}


static char ambig[] = "ambiguous `r' keyletter value (de15)";

struct pfile *
rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	char *user;
	struct pfile pf;
	static struct pfile goodpf;
	char line[BUFSIZ];
	int cnt, uniq;
	FILE *in, *out, *fdfopen();
	char *logname();
	char *outname;

	uniq = cnt = -1;
	user = logname();
	zero(&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'),0);
	outname = auxf(pkt->p_file, 'q');
	out = xfcreat(outname, 0644);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		pf.pf_date = MAXLONG;
		if (equal(pf.pf_user,user)||getuid()==0) {
			if (sp->s_rel == 0) {
				if (++cnt) {
					if (fflush(out) == EOF)
						xmsg(outname, "rdpfile");
					if (fsync(fileno(out)) < 0)
						xmsg(outname, "rdpfile");
					if (fclose(out) == EOF)
						xmsg(outname, "rdpfile");
					fclose(in);
					fatal("missing -r argument (de1)");
				}
				move(&pf,&goodpf,sizeof(pf));
				continue;
			}
			else if ((sp->s_rel == pf.pf_nsid.s_rel &&
			    sp->s_lev == pf.pf_nsid.s_lev &&
			    sp->s_br == pf.pf_nsid.s_br &&
			    sp->s_seq == pf.pf_nsid.s_seq) ||
			    (sp->s_rel == pf.pf_gsid.s_rel &&
			    sp->s_lev == pf.pf_gsid.s_lev &&
			    sp->s_br == pf.pf_gsid.s_br &&
			    sp->s_seq == pf.pf_gsid.s_seq)) {
				if (++uniq) {
					if (fflush(out) == EOF)
						xmsg(outname, "rdpfile");
					if (fsync(fileno(out)) < 0)
						xmsg(outname, "rdpfile");
					if (fclose(out) == EOF)
						xmsg(outname, "rdpfile");
					fclose(in);
					fatal(ambig);
				}
				move(&pf,&goodpf,sizeof(pf));
				continue;
			}
		}
		if (fputs(line, out) == EOF)
			xmsg(outname, "rdpfile");
	}
	if (fflush(out) == EOF)
		xmsg(outname, "rdpfile");
	(void) fflush(stderr);
	fstat(fileno(out),&Statbuf);
	Szqfile = Statbuf.st_size;
	copy(auxf(pkt->p_file,'p'),Pfilename);
	if (fsync(fileno(out)) < 0)
		xmsg(outname, "rdpfile");
	if (fclose(out) == EOF)
		xmsg(outname, "rdpfile");
	fclose(in);
	if (!goodpf.pf_user[0])
		fatal("login name or SID specified not in p-file (de2)");
	return(&goodpf);
}


FILE *
dodiff(newf,oldf)
char *newf, *oldf;
{
	register int i;
	int pfd[2];
	FILE *iop, *fdfopen();
	extern char Diffpgm[];
	char num[10];
	char message[2000];

	if ((Newlines = lines(newf)) < 0) {
		sprintf(message, "cannot open %s", newf);
		fatal(message);
	}
	if ((Oldlines = lines(oldf)) < 0) {
		sprintf(message, "cannot open %s", oldf);
		fatal(message);
	}
	xpipe(pfd);
	if ((i = vfork()) < 0) {
		close(pfd[0]);
		close(pfd[1]);
		fatal("cannot fork, try again (de11)");
	}
	else if (i == 0) {
		/* Child */
		close(pfd[0]);
		close(1);
		dup(pfd[1]);
		close(pfd[1]);
		for (i = 5; i < getdtablesize(); i++)	/* Why 5? --sun!gnu */
			close(i);
		if (!HADH ||
		    (Newlines < diffh_threshold && Oldlines < diffh_threshold))
			execl(Diffpgm,Diffpgm,oldf,newf,0);
		else 
			execl(Diffpgm,Diffpgm,"-h",oldf,newf,0);
		close(1);
		_exit(32);	/* tell parent that 'execl' failed */
	}
	else {
		/* Parent */
		close(pfd[1]);
		iop = fdfopen(pfd[0],0);
		return(iop);
	}
}

static int
lines(fname)
	char		*fname;
{
	FILE		*fp;
	int		i;
	int		nlines;
	long		nchars;
	long		lineend;

	if ((fp = fopen(fname, "r")) == NULL)
		return -1;
	nlines = 0;
	nchars = 0;
	lineend = 0;
	while ((i = getc(fp)) != EOF) {
		nchars++;
		if ((char) i == '\n') {
			nlines++;
			lineend = nchars;
		}
	}
	if (nchars > lineend)
		nlines++;
	fclose(fp);
	return nlines;
}

getdiff(type,plinenum)
register char *type;
register int *plinenum;
{
	char line[LINESIZE];
	register char *p;
	int num_lines;
	static int chg_num, chg_ln;
	int lowline, highline;
	char *linerange(), *rddiff();

	if ((p = rddiff(line,sizeof(line))) == NULL)
		return(0);

	if (*p == '-') {
		*type = INS;
		*plinenum = chg_ln;
		num_lines = chg_num;
	}
	else {
		p = linerange(p,Oldlines,&lowline,&highline);
		*plinenum = lowline;

		switch(*p++) {
		case 'd':
			num_lines = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;

		case 'a':
			linerange(p,Newlines,&lowline,&highline);
			num_lines = highline - lowline + 1;
			*type = INS;
			break;

		case 'c':
			chg_ln = lowline;
			num_lines = highline - lowline + 1;
			linerange(p,Newlines,&lowline,&highline);
			chg_num = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;
		}
	}

	return(num_lines);
}


insert(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
register int n;
int ser;
{
	char str[LINESIZE];

	after(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,INS,ser);
	putline(pkt,str);
	for (++n; --n; ) {
		rddiff(str,sizeof(str));
		putline(pkt,&str[2]);
	}
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


delete(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
int n;
register int ser;
{
	char str[LINESIZE];

	before(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,DEL,ser);
	putline(pkt,str);
	after(pkt,linenum + n - 1);
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


after(pkt,n)
register struct packet *pkt;
register int n;
{
	before(pkt,n);
	if (pkt->p_glnno == n)
		putline(pkt,0);
}


before(pkt,n)
register struct packet *pkt;
register int n;
{
	while (pkt->p_glnno < n) {
		if (!readmod(pkt))
			break;
	}
}

char *
linerange(cp,maxlines,low,high)
register char *cp;
int maxlines;
register int *low, *high;
{
	if (*cp == '$') {
		*low = maxlines;
		cp++;
	}
	else
		cp = satoi(cp,low);
	if (*cp == ',') {
		cp++;
		if (*cp == '$') {
			*high = maxlines;
			cp++;
		}
		else
			cp = satoi(cp,high);
	}
	else
		*high = *low;

	return(cp);
}


skipline(lp,num)
register char *lp;
register int num;
{
	for (++num;--num;)
		rddiff(lp,LINESIZE);
}


char *
rddiff(s,n)
register char *s;
register int n;
{
	register char *r;

	if ((r = fgets(s,n,Diffin)) != NULL) {
		if (s[strlen(s)-1] != '\n') {
			fclose(Diffin);
			fatal("line too long (de18)");
		}
		if (HADP)
			fputs(s,gpkt.p_stdout);
	}
	return(r);
}


enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char str[32];
	register struct apply *ap;

	sid_ba(sidp,str);
	ap = &pkt->p_apply[n];
	if (pkt->p_cutoff > pkt->p_idel[n].i_datetime)
		switch(ap->a_code) {

		case EMPTY:
			switch (ch) {
			case INCLUDE:
				condset(ap,APPLY,INCLUSER);
				break;
			case EXCLUDE:
				condset(ap,NOAPPLY,EXCLUSER);
				break;
			case IGNORE:
				condset(ap,EMPTY,IGNRUSER);
				break;
			}
			break;
		case APPLY:
			fatal("internal error in delta/enter() (de5)");
			break;
		case NOAPPLY:
			if (ch == 'g' && (ap->a_reason & EXCLUSER))
				fatal("-g option conflicts with -x option used at edit-time");
			else
				fatal("internal error in delta/enter() (de6)");
			break;
		default:
			fatal("internal error in delta/enter() (de7)");
			break;
		}
}


escdodelt()	/* dummy routine for dodelt() */
{
}


clean_up(n)
{
	if (mylock(auxf(gpkt.p_file,'z'),getpid())) {
		if (gpkt.p_iop)
			fclose(gpkt.p_iop);
		if (Xiop) {
			fclose(Xiop);
			unlink(auxf(gpkt.p_file,'x'));
		}
		unlink(auxf(gpkt.p_file,'d'));
		unlink(auxf(gpkt.p_file,'q'));
		xrm(&gpkt);
		xfreeall();
		unlockit(auxf(gpkt.p_file,'z'),getpid());
	}
}


static char bd[] = "leading SOH char in line %d of file `%s' not allowed (de14)";

fgetchk(file,pkt)
register char	*file;
register struct packet *pkt;
{
	FILE	*iptr, *fdfopen();
	char	line[BUFSIZ];
	register int k;

	iptr = xfopen(file,0);
	for (k = 1; fgets(line,sizeof(line),iptr); k++)
		if (*line == CTLCHAR) {
			fclose(iptr);
			sprintf(Error,bd,k,auxf(pkt->p_file,'g'));
			fatal(Error);
		}
	fclose(iptr);
}
