# include	"../hdr/defines.h"
# include	"../hdr/had.h"

SCCSID(@(#)get.c 1.1 92/07/30 SMI); /* from System III 5.1 */
USXALLOC();

/*
 * Get is now much more careful than before about checking
 * for write errors when creating the g- l- p- and z-files.
 * However, it remains deliberately unconcerned about failures
 * in writing statistical information (e.g., number of lines
 * retrieved).
 */

int	Debug = 0;
int	had_pfile;
struct packet gpkt;
struct sid sid;
unsigned	Ser;
int	num_files;
char	Pfilename[FILESIZE];
char	*ilist, *elist, *lfile;
long	cutoff = 0X7FFFFFFFL;	/* max positive long */
int verbosity;
char	Gfile[FILESIZE], *fgets(), *trans();
char	*Type;
int	Did_id;
FILE	*fdfopen();

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	int testmore;
	extern int Fcnt;
	extern get();

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

			case 'a':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				Ser = patoi(p);
				break;
			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				if ((sid.s_rel < MINR) ||
					(sid.s_rel > MAXR))
					fatal("r out of range (ge22)");
				break;
			case 'c':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				if (date_ab(p,&cutoff))
					fatal("bad date/time (cm5)");
				break;
			case 'l':
				lfile = p;
				break;
			case 'i':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				ilist = p;
				break;
			case 'x':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				elist = p;
				break;
			case 'G':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				copy(p, Gfile);
				break;
			case 'b':
			case 'g':
			case 'e':
			case 'p':
			case 'k':
			case 'm':
			case 'n':
			case 's':
			case 't':
				testmore++;
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
						"value after %c arg (cm8)",c);
					fatal(Error);
				}
			}
			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (HADE && HADM)
		fatal("e not allowed with m (ge3)");
	if (HADE)
		HADK = 1;
	if (!HADS)
		verbosity = -1;
	if (HADE && ! logname())
		fatal("User ID not in password file (cm9)");
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,get);
	exit(Fcnt ? 1 : 0);
	/* NOTREACHED */
}


extern char *Sflags[];

get(file)
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp, *idsubst(), *auxf();
	struct stats stats;
	char	str[32];
	int	l_rel;
	int i,status;
	static first = 1;

	Fflags |= FTLMSG;
	if (setjmp(Fjmp))
		return;
	if (HADE) {
		had_pfile = 1;
		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		*/
		sinit(&gpkt,file,0);
		if (lockit(auxf(file,'z'),10,getpid()))
			fatal("cannot create lock file (cm4)");
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(&gpkt,file,1);
	gpkt.p_ixuser = (HADI | HADX);
	gpkt.p_reqsid.s_rel = sid.s_rel;
	gpkt.p_reqsid.s_lev = sid.s_lev;
	gpkt.p_reqsid.s_br = sid.s_br;
	gpkt.p_reqsid.s_seq = sid.s_seq;
	gpkt.p_verbose = verbosity;
	gpkt.p_stdout = (HADP ? stderr : stdout);
	gpkt.p_cutoff = cutoff;
	gpkt.p_lfile = lfile;
	if (Gfile[0] == 0 || !first)
		copy(auxf(gpkt.p_file,'g'),Gfile);
	first = 0;

	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		(void) fprintf(gpkt.p_stdout, "\n%s:\n",
				nse_file_trim(gpkt.p_file, 0));
	if (dodelt(&gpkt,&stats,0,0) == 0)
		fmterr(&gpkt);
	finduser(&gpkt);
	doflags(&gpkt);
	if (!HADA)
		ser = getser(&gpkt, Sflags[DEFTFLAG - 'a'], HADT);
	else {
		if ((ser = Ser) > maxser(&gpkt))
			fatal("serial number too large (ge19)");
		move(&gpkt.p_idel[ser].i_sid, &gpkt.p_gotsid, sizeof(sid));
		if (HADR && sid.s_rel != gpkt.p_gotsid.s_rel) {
			zero(&gpkt.p_reqsid, sizeof(gpkt.p_reqsid));
			gpkt.p_reqsid.s_rel = sid.s_rel;
		}
		else
			move(&gpkt.p_gotsid, &gpkt.p_reqsid, sizeof(sid));
	}
	doie(&gpkt,ilist,elist,0);
	setup(&gpkt,ser);
	if (!(Type = Sflags[TYPEFLAG - 'a']))
		Type = Null;
	if (!(HADP || HADG))
		if (exists(Gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(Error,"writable `%s' exists (ge4)",Gfile);
			fatal(Error);
		}
	if (gpkt.p_verbose) {
		sid_ba(&gpkt.p_gotsid,str);
		(void) fprintf(gpkt.p_stdout, "%s\n", str);
	}
	if (HADE) {
		if (HADC || gpkt.p_reqsid.s_rel == 0)
			move(&gpkt.p_gotsid, &gpkt.p_reqsid, sizeof(sid));
		newsid(&gpkt,Sflags[BRCHFLAG - 'a'] && HADB,
			Sflags[JOINTFLAG - 'a']);
		permiss(&gpkt);
		if (exists(auxf(gpkt.p_file,'p')))
			mk_qfile(&gpkt);
		else had_pfile = 0;
		wrtpfile(&gpkt,ilist,elist);
	}
	if (HADG) {
		if (HADL)
			gen_lfile(&gpkt);
		(void) fclose(gpkt.p_iop);
	}
	else {
		(void) fflush(gpkt.p_stdout);
		(void) fflush(stderr);
		if ((i = fork()) < 0)
			fatal("cannot fork, try again");
		if (i == 0) {
			Fflags |= FTLEXIT;
			Fflags &= ~FTLJMP;
			setuid(getuid());
			if (HADL)
				gen_lfile(&gpkt);
			flushto(&gpkt,EUSERTXT,1);
			idsetup(&gpkt);
			gpkt.p_chkeof = 1;
			Did_id = 0;
			/*
			call `xcreate' which deletes the old `g-file' and
			creates a new one except if the `p' keyletter is set in
			which case any old `g-file' is not disturbed.
			The mode of the file depends on whether or not the `k'
			keyletter has been set.
			*/

			if (gpkt.p_gout == 0) {
				if (HADP)
					gpkt.p_gout = stdout;
				else
					gpkt.p_gout = xfcreat(Gfile,
							HADK ? 0644 : 0444);
			}
			if (Sflags[ENCODEFLAG - 'a'] &&
			    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
				while(readmod(&gpkt)) {
					decode(gpkt.p_line,gpkt.p_gout);
				}
			}
			else {
				while(readmod(&gpkt)) {
					prfx(&gpkt);
					p = idsubst(&gpkt,gpkt.p_line);
					if (fputs(p, gpkt.p_gout) == EOF)
						xmsg(Gfile, "get");
				}
			}
			if (fflush(gpkt.p_gout) == EOF)
				xmsg(Gfile, "get");
			(void) fflush(stderr);
			if (gpkt.p_gout && gpkt.p_gout != stdout) {
				/*
				 * Force g-file to disk and verify
				 * that it actually got there.
				 */
				if (fsync(fileno(gpkt.p_gout)) < 0)
					xmsg(Gfile, "get");
				if (fclose(gpkt.p_gout) == EOF)
					xmsg(Gfile, "get");
			}
			if (gpkt.p_verbose)
				(void) fprintf(gpkt.p_stdout, "%u lines\n",
					gpkt.p_glnno);
			if (!Did_id && !HADK && !HADQ)
				if (Sflags[IDFLAG - 'a'])
					fatal("no id keywords (cm6)");
				else if (gpkt.p_verbose)
					(void) fprintf(stderr,
						"No id keywords (cm7)\n");
			exit(0);
		}
		else {
			wait(&status);
			if (status) {
				Fflags &= ~FTLMSG;
				fatal();
			}
		}
	}
	if (gpkt.p_iop)
		(void) fclose(gpkt.p_iop);
	/*
	remove 'q'-file because it is no longer needed
	*/
	unlink(auxf(gpkt.p_file,'q'));
	xfreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
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
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"%s\n",str);
	ap = &pkt->p_apply[n];
	switch(ap->a_code) {

	case EMPTY:
		if (ch == INCLUDE)
			condset(ap,APPLY,INCLUSER);
		else
			condset(ap,NOAPPLY,EXCLUSER);
		break;
	case APPLY:
		sid_ba(sidp,str);
		sprintf(Error,"%s already included (ge9)",str);
		fatal(Error);
		break;
	case NOAPPLY:
		sid_ba(sidp,str);
		sprintf(Error,"%s already excluded (ge10)",str);
		fatal(Error);
		break;
	default:
		fatal("internal error in get/enter() (ge11)");
		break;
	}
}


gen_lfile(pkt)
register struct packet *pkt;
{
	char *n;
	int reason;
	char str[32];
	char line[BUFSIZ];
	struct deltab dt;
	FILE *in;
	FILE *out;
	char *outname = "stdout";

#define	OUTPUTC(c) \
	if (putc((c), out) == EOF) \
		xmsg(outname, "gen_lfile");

	in = xfopen(pkt->p_file,0);
	if (*pkt->p_lfile)
		out = stdout;
	else {
		outname = auxf(pkt->p_file, 'l');
		out = xfcreat(outname, 0444);
	}
	fgets(line,sizeof(line),in);
	while (fgets(line,sizeof(line),in) != NULL &&
	    line[0] == CTLCHAR && line[1] == STATS) {
		fgets(line,sizeof(line),in);
		del_ab(line,&dt);
		if (dt.d_type == 'D') {
			reason = pkt->p_apply[dt.d_serial].a_reason;
			if (pkt->p_apply[dt.d_serial].a_code == APPLY) {
				OUTPUTC(' ');
				OUTPUTC(' ');
			}
			else {
				OUTPUTC('*');
				if (reason & IGNR) {
					OUTPUTC(' ');
				}
				else {
					OUTPUTC('*');
				}
			}
			switch (reason & (INCL | EXCL | CUTOFF)) {

			case INCL:
				OUTPUTC('I');
				break;
			case EXCL:
				OUTPUTC('X');
				break;
			case CUTOFF:
				OUTPUTC('C');
				break;
			default:
				OUTPUTC(' ');
				break;
			}
			OUTPUTC(' ');
			sid_ba(&dt.d_sid,str);
			if (fprintf(out, "%s\t", str) == EOF)
				xmsg(outname, "gen_lfile");
			date_ba(&dt.d_datetime,str);
			if (fprintf(out, "%s %s\n", str, dt.d_pgmr) == EOF)
				xmsg(outname, "gen_lfile");
		}
		while ((n = fgets(line,sizeof(line),in)) != NULL)
			if (line[0] != CTLCHAR)
				break;
			else {
				switch (line[1]) {

				case EDELTAB:
					break;
				default:
					continue;
				case MRNUM:
				case COMMENTS:
					if (dt.d_type == 'D')
						if (fprintf(out, "\t%s",
						    &line[3]) == EOF)
							xmsg(outname,
							    "gen_lfile");
					continue;
				}
				break;
			}
		if (n == NULL || line[0] != CTLCHAR)
			break;
		OUTPUTC('\n');
	}
	(void) fclose(in);
	if (out != stdout) {
		if (fsync(fileno(out)) < 0)
			xmsg(outname, "gen_lfile");
		if (fclose(out) == EOF)
			xmsg(outname, "gen_lfile");
	}

#undef	OUTPUTC
}


char	Curdate[18];
char	*Curtime;
char	Gdate[9];
char	Chgdate[18];
char	*Chgtime;
char	Gchgdate[9];
char	Sid[32];
char	Mod[FILESIZE];
char	Olddir[BUFSIZ];
char	Pname[BUFSIZ];
char	Dir[BUFSIZ];
char	*Qsect;

idsetup(pkt)
register struct packet *pkt;
{
	extern long Timenow;
	register int n;
	register char *p;

	date_ba(&Timenow,Curdate);
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate,Gdate);
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n)
		date_ba(&pkt->p_idel[n].i_datetime,Chgdate);
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate,Gchgdate);
	sid_ba(&pkt->p_gotsid,Sid);
	if (p = Sflags[MODFLAG - 'a'])
		copy(p,Mod);
	else
		copy(auxf(gpkt.p_file,'g'), Mod);
	if (!(Qsect = Sflags[QSECTFLAG - 'a']))
		Qsect = Null;
}


makgdate(old,new)
register char *old, *new;
{
	if ((*new = old[3]) != '0')
		new++;
	*new++ = old[4];
	*new++ = '/';
	if ((*new = old[6]) != '0')
		new++;
	*new++ = old[7];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	*new = 0;
}


static char Zkeywd[5] = "@(#)";
char *
idsubst(pkt,line)
register struct packet *pkt;
char line[];
{
	static char tline[BUFSIZ];
	static char str[32];
	register char *lp, *tp;
	extern char *Type;
	extern char *Sflags[];

	if (HADK || !any('%',line))
		return(line);

	tp = tline;
	for(lp=line; *lp != 0; lp++) {
		if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%') {
			switch(*++lp) {

			case 'M':
				tp = trans(tp,Mod);
				break;
			case 'Q':
				tp = trans(tp,Qsect);
				break;
			case 'R':
				sprintf(str,"%u",pkt->p_gotsid.s_rel);
				tp = trans(tp,str);
				break;
			case 'L':
				sprintf(str,"%u",pkt->p_gotsid.s_lev);
				tp = trans(tp,str);
				break;
			case 'B':
				sprintf(str,"%u",pkt->p_gotsid.s_br);
				tp = trans(tp,str);
				break;
			case 'S':
				sprintf(str,"%u",pkt->p_gotsid.s_seq);
				tp = trans(tp,str);
				break;
			case 'D':
				tp = trans(tp,Curdate);
				break;
			case 'H':
				tp = trans(tp,Gdate);
				break;
			case 'T':
				tp = trans(tp,Curtime);
				break;
			case 'E':
				tp = trans(tp,Chgdate);
				break;
			case 'G':
				tp = trans(tp,Gchgdate);
				break;
			case 'U':
				tp = trans(tp,Chgtime);
				break;
			case 'Z':
				tp = trans(tp,Zkeywd);
				break;
			case 'Y':
				tp = trans(tp,Type);
				break;
			case 'W':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Mod);
				*tp++ = '\t';
			case 'I':
				tp = trans(tp,Sid);
				break;
			case 'P':
				copy(pkt->p_file,Dir);
				dname(Dir);
				if(curdir(Olddir) != 0)
					fatal("curdir failed (ge20)");
				if(chdir(Dir) != 0)
					fatal("cannot change directory (ge21)");
				if(curdir(Pname) != 0)
					fatal("curdir failed (ge20)");
				if(chdir(Olddir) != 0)
					fatal("cannot change directory (ge21)");
				tp = trans(tp,Pname);
				*tp++ = '/';
				tp = trans(tp,(sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp,pkt->p_file);
				break;
			case 'C':
				sprintf(str,"%u",pkt->p_glnno);
				tp = trans(tp,str);
				break;
			case 'A':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Type);
				*tp++ = ' ';
				tp = trans(tp,Mod);
				*tp++ = ' ';
				tp = trans(tp,Sid);
				tp = trans(tp,Zkeywd);
				break;
			default:
				*tp++ = '%';
				*tp++ = *lp;
				continue;
			}
			lp++;
		}
		else
			*tp++ = *lp;
	}

	*tp = 0;
	return(tline);
}

char *
trans(tp,str)
register char *tp, *str;
{
	Did_id = 1;
	while(*tp++ = *str++)
		;
	return(tp-1);
}


prfx(pkt)
register struct packet *pkt;
{
	char str[32];

	if (HADN)
		if (fprintf(pkt->p_gout, "%s\t", Mod) == EOF)
			xmsg(Gfile, "prfx");
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		if (fprintf(pkt->p_gout, "%s\t", str) == EOF)
			xmsg(Gfile, "prfx");
	}
}


clean_up(n)
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);
	if (gpkt.p_gout)
		fflush(gpkt.p_gout);
		fflush(stderr);
	if (gpkt.p_gout && gpkt.p_gout != stdout) {
		fclose(gpkt.p_gout);
		unlink(Gfile);
	}
	if (HADE) {
		if (! had_pfile) {
			unlink(auxf(gpkt.p_file,'p'));
		}
		else if (exists(auxf(gpkt.p_file,'q'))) {
			copy(auxf(gpkt.p_file,'p'),Pfilename);
			rename(auxf(gpkt.p_file,'q'),Pfilename);
		     }
	}
	xfreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
}


static	char	warn[] = "WARNING: being edited: `%s' (ge18)\n";

wrtpfile(pkt,inc,exc)
register struct packet *pkt;
char *inc, *exc;
{
	char line[64], str1[32], str2[32];
	char *user;
	FILE *in, *out;
	struct pfile pf;
	register char *p;
	int fd;
	int i;
	extern long Timenow;

	user = logname();
	if (exists(p = auxf(pkt->p_file,'p'))) {
		fd = xopen(p,2);
		in = fdfopen(fd,0);
		while (fgets(line,sizeof(line),in) != NULL) {
			p = line;
			p[length(p) - 1] = 0;
			pf_ab(p,&pf,0);
			if (!(Sflags[JOINTFLAG - 'a'])) {
				if ((pf.pf_gsid.s_rel == pkt->p_gotsid.s_rel &&
     				   pf.pf_gsid.s_lev == pkt->p_gotsid.s_lev &&
				   pf.pf_gsid.s_br == pkt->p_gotsid.s_br &&
				   pf.pf_gsid.s_seq == pkt->p_gotsid.s_seq) ||
				   (pf.pf_nsid.s_rel == pkt->p_reqsid.s_rel &&
				   pf.pf_nsid.s_lev == pkt->p_reqsid.s_lev &&
				   pf.pf_nsid.s_br == pkt->p_reqsid.s_br &&
				   pf.pf_nsid.s_seq == pkt->p_reqsid.s_seq)) {
					fclose(in);
					sprintf(Error,
					     "being edited: `%s' (ge17)",line);
					fatal(Error);
				}
				if (!equal(pf.pf_user,user))
					fprintf(stderr,warn,line);
			}
			else fprintf(stderr,warn,line);
		}
		out = fdfopen(dup(fd),1);
		fclose(in);
	}
	else
		out = xfcreat(p,0644);
	if (fseek(out, 0L, 2) == EOF)
		xmsg(p, "wrtpfile");
	sid_ba(&pkt->p_gotsid,str1);
	sid_ba(&pkt->p_reqsid,str2);
	date_ba(&Timenow,line);
	if (fprintf(out, "%s %s %s %s", str1, str2, user, line) == EOF)
		xmsg(p, "wrtpfile");
	if (inc)
		if (fprintf(out, " -i%s", inc) == EOF)
			xmsg(p, "wrtpfile");
	if (exc)
		if (fprintf(out, " -x%s", exc) == EOF)
			xmsg(p, "wrtpfile");
	if (fprintf(out, "\n") == EOF)
		xmsg(p, "wrtpfile");
	if (fflush(out) == EOF)
		xmsg(p, "wrtpfile");
	if (fsync(fileno(out)) < 0)
		xmsg(p, "wrtpfile");
	if (fclose(out) == EOF)
		xmsg(p, "wrtpfile");
	if (pkt->p_verbose)
		if (HADQ)
			(void) fprintf(pkt->p_stdout,
					"new version %s\n", str2);
		else
			(void) fprintf(pkt->p_stdout, "new delta %s\n", str2);
}

/* Null routine to satisfy external reference from dodelt() */

escdodelt()
{
}

mk_qfile(pkt)
register struct	packet *pkt;
{
	FILE	*in, *qout;
	char	line[BUFSIZ];
	char	*qfile = auxf(pkt->p_file, 'q');

	in = xfopen(auxf(pkt->p_file,'p'),0);
	qout = xfcreat(qfile, 0644);

	while ((fgets(line,sizeof(line),in) != NULL))
		if (fputs(line, qout) == EOF)
			xmsg(qfile, "mk_qfile");
	(void) fclose(in);
	if (fflush(qout) == EOF)
		xmsg(qfile, "mk_qfile");
	if (fsync(fileno(qout)) < 0)
		xmsg(qfile, "mk_qfile");
	if (fclose(qout) == EOF)
		xmsg(qfile, "mk_qfile");
}
