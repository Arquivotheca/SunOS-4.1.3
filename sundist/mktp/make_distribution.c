#ifndef lint
static  char sccsid[] = "@(#)make_distribution.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * make distribution - actually write the distribution
 *
 */

#include <curses.h>
#include <signal.h>
#include <string.h>
#include "mktp.h"

extern char *sprintf();
extern int setbuf();

#define	NIL_COM		0
#define	MAKE_DIST	1
#define	EDIT_LOG	2
#define	ZAP_LOG		3
#define	EXIT		4

static char *ifile;
static char *xfile;
static char *logfile;

static char *none = "(none)";
static char *Load;
static char *Unload;
static char *Device;
static char *rname;

static int nvols;
static int volume;
static char *volsdone;
static FILE *stdlog;				/* mktp logfile */
static char *scriptpath = "bin/script_log";	/* script for logfile */
static char *script_log = "script_log";		/* script for logfile */
static char *xdrtoc = "XDRTOC";			/* encoded toc */

char *fsize();

main(argc,argv)
int argc;
char **argv;
{
	char *strcpy(), *strcat();
	register state;
	register char *p;
	FILE *zap_log();

	/*
	 * 1) Pick up input xdr file
	 */

	if(argc < 2)
	{
		(void) fprintf(stderr,"Usage: %s infile\n",*argv);
		exit(1);
	}


	xfile = newstring(argv[1]);

	if(!get_tables(xfile))
		exit(-1);
	else if((dinfo.dstate&READY_TO_GO) != READY_TO_GO)
	{
		(void) fprintf(stderr,
			"This distribution hasn't been made ready to go yet\n");
		exit(-1);
	}
	else
		dinfo.dstate = GONE;

	/*
	 * 2) Form log file name
	 */

	for(p = xfile; *p; p++)
		if(*p == '.' && *(p+1) && *(p+1) == 'x' && *(p+2) &&
				*(p+2) == 'd' && *(p+3) && *(p+3) == 'r')
			break;
	volume = *p;
	*p = 0;
	ifile = malloc((unsigned)strlen(xfile)+1);
	(void) strcpy(ifile,xfile);
	logfile = malloc((unsigned)strlen(ifile)+8+1);
	(void) strcpy(logfile,xfile);
	(void) strcat(logfile,".mktplog");
	*p = (char) volume;

	/*
	 * 3) perform other initialization
	 */

	volume = 1;
	mkvinfo(volume);

	if(vinfo.vaddr.dtype == DISK) { /* we don't do no stenking diskettes! */
		fprintf(stderr,"\n\007DISKETTES made by makefile \"diskette.mk\"\n");
		fprintf(stderr,"\n\007CDROM made by makefile \"cdrom.mk\"\n");
		fprintf(stderr,"main Makefile will invoke it automatically\n");
		fprintf(stderr,"press <return> to continue:");
		fflush(stderr);
		(void)fgetc(stdin);
		exit( 3 );
		/********/
	}

	volsdone = malloc((unsigned)(nvols = getnvols())+1);

	for(state = 0; state <= nvols; state++)
		volsdone[state] = 0;

	(void) make_write_command(&vinfo);

	(void) setbuf(stdout,(char *) 0);

	(void) zap_log("a");
	/*
	 * After this point, all error info should also be directed
	 * towards the log file (for record keeping purposes)
	 */
	stdlog = zap_log("a");
	log_start();

	/*
	 * 4) Drop into command loop.
	 *	gchoices() presents a screen and picks up a choice,
	 *	and turns that choice into an action which we decide
	 *	what to do with...
	 *
	 */

	while((state = gchoices()) != EXIT)
	{
		switch(state)
		{
		default:
		case NIL_COM:
			break;
		case MAKE_DIST:
			if(make_distribution(volume) < 0)
			{
				(void) fprintf(stderr,
					"make of volume %d failed\n",volume);
				sleep(2);
			}
			else
				volsdone[volume] = 1;
			break;
		case ZAP_LOG:
			if((stdlog = zap_log("w")) <= stderr)
			{
				(void) fprintf(stderr,
					"couldn't erase log file!\n");
				sleep(2);
			}
			else
				log_start();
			break;
		case EDIT_LOG:
			if(edit_log() < 0)
			{
				(void) fprintf(stderr,
					"couldn't edit log file!\n");
				sleep(2);
			}
			break;
		}
	}
	exit(0);
	/* NOTREACHED */
}
	
/*
 * gchoices - paint screen, get a choice, turn choice into action...
 */


static int
gchoices()
{
	void (*isav)(), (*qsav)();
	register choice, i;

	(void) initscr();
        isav = signal(SIGINT,SIG_IGN);
        qsav = signal(SIGQUIT,SIG_IGN);
        nonl();         /* turn off CR-LF remap */
        crmode();       /* set cbreak...        */
        noecho();       /* turn off echoing     */

	clear();

	/*
	 * Put up basic info
	 */

	(void) mvprintw(0,10,"make_distribution version %s of %s","1.22","89/09/19");

	(void) mvprintw(2,14,"Prototype File: %s",ifile);
	(void) mvprintw(3,14,"  Working File: %s",xfile);
	(void) mvprintw(4,14,"      Log File: %s",logfile);
	(void) mvprintw(5,14,"  Load Command: %s",(Load)?Load:none);
	(void) mvprintw(6,14,"Unload Command: %s",(Unload)?Unload:none);

	(void) mvprintw(7,14,"   Device Type: ");
	(void) mvprintw(8,14,"   Device Name: ");
	(void) mvprintw(9,14,"   Device Size: ");
	if(vinfo.vaddr.dtype == TAPE)
	{
		(void) mvprintw(7,30,"tape");
		(void) mvprintw(8,30,"/dev/r%s",
			vinfo.vaddr.Address_u.tape.dname);
		(void) mvprintw(9,30,"%s",
			fsize(vinfo.vaddr.Address_u.tape.volsize));
	}
	else if(vinfo.vaddr.dtype == DISK)
	{
		(void) mvprintw(7,30,"disk");
		(void) mvprintw(8,30,"/dev/r%s",
			vinfo.vaddr.Address_u.disk.dname);
		(void) mvprintw(9,30,"%s",
			fsize(vinfo.vaddr.Address_u.disk.volsize));
	}

	for(;;)
	{
		(void) mvprintw(12,(COLS/2)-10,"Choices");

		(void) mvprintw(14,COLS/3,"[1] Quit");
		(void) mvprintw(15,COLS/3,"[2] View Log File");
		(void) mvprintw(16,COLS/3,"[3] Erase Log File");

		if (vinfo.vaddr.dtype == TAPE)
		{
		    for(i = 0; i < nvols; i++)
		    {
			(void) mvprintw(i+17,COLS/3,
				"[%1d] Make Volume %d",i+4,i+1);
			if(volsdone[i+1])
				(void) printw(" (has been done)");
		    }
		}

		(void) mvprintw(LINES-1,7,"What would you like? ");

		refresh();

		if((choice = getch()) == ERR)
		{
			(void) signal(SIGINT,isav);
			(void) signal(SIGQUIT,qsav);
			move(LINES,0);
			refresh();
			endwin();
			return(NIL_COM);
		}

		addch(choice);

		choice -= '0';

		if(choice < 1 || choice > nvols+3)
		{
			if(choice+'0' > ' ' && choice+'0' < 0177)
				errprint("Illegal choice '%c'",choice+'0');
			else
				errprint("Illegal choice 0%o",choice+'0');

			infoprint("Try again........");
		}
		else
			break;
	}

        (void) signal(SIGINT,isav);
        (void) signal(SIGQUIT,qsav);
	clear();
        move(LINES,0);
        refresh();
        endwin();

	if(choice == 1)
		return(EXIT);
	else if(choice == 2)
		return(EDIT_LOG);
	else if(choice == 3)
		return(ZAP_LOG);

	choice -= 3;
	if(choice > nvols)
		return(NIL_COM);
	if(vinfo.vaddr.dtype == TAPE)
		volume = choice;
	return(MAKE_DIST);
}

/*
 * edit (read-only) a log file
 */

static int
edit_log()
{
	auto char combuf[128];
	return(system(sprintf(combuf,"view %s",logfile)));
}

/*
 * zap (erase) a log file
 *
 * also a dirty way to capture stderr from subprograms
 *
 */

static FILE *
zap_log(how)
char *how;
{
	register FILE *op;
	(void) fflush(stderr);
	(void) setbuf(stderr,(char *) 0);

	/* if we are erasing the log file,
	 * then close it before we reopen it
	 */
	if(stdlog > stderr)
		(void) fclose(stdlog);

 	if((op = fopen(logfile,how)) == NULL)
		return(stderr);
#ifdef notdef
	fileno(stderr) = dup2(fileno(op),fileno(stderr));
	(void) fclose(op);
#endif
	(void) setbuf(op,(char *) 0);
	return(op);
}

static
setoutput()
{
	register FILE *op;
	(void) fflush(stdout);
 	if((op = fopen(Device,"a")) == NULL)
		return(-1);
	fileno(stdout) = dup2(fileno(op),fileno(stdout));
	(void) fclose(op);

 	/* a dirty way to capture stderr from subprograms
	 */
	(void) fflush(stdlog);
	fileno(stderr) = dup2(fileno(stdlog),fileno(stderr));
	(void) fclose(stdlog);
	return(0);
}

static
mkvinfo(volno)
{
	static char env_volstring[16], env_nvolstring[16];
	sprintf(env_volstring,"VOLUME=%d",volno);
	(void) putenv(env_volstring);
	sprintf(env_nvolstring,"NVOLS=%d",getnvols());
	(void) putenv(env_nvolstring);
	if(vinfo.vaddr.dtype == TAPE)
		mkvinfo_tape(volno);
	else
		mkvinfo_disk();
}

static
mkvinfo_tape(volno)
{
	register toc_entry *eptr;
	char local[16];
	tapeaddress *tp = &vinfo.vaddr.Address_u.tape;

	tp->volno = volno;
	tp->label = newstring(sprintf(local,"VOLUME %d",volno));
	strcpy(tp->label,local);
	for (eptr = entries; eptr < ep; eptr++)
	{
		if(IS_TOC(eptr) && Volume_of_Entry(eptr) == volno)
		{
			tp->file_number = Entry_in_Volume(eptr);
			tp->volsize = Size_of_Volume(eptr);
		}
	}
	rname = newstring(tp->dname);
}

static
mkvinfo_disk()
{
	register toc_entry *eptr;
	char local[16];
	diskaddress *dp = &vinfo.vaddr.Address_u.disk;

	rname = newstring(dp->dname);
	for (eptr = entries; eptr < ep; eptr++)
	{
		if( IS_TOC(eptr) ) {
			dp->volno = eptr->Where.Address_u.disk.volno;
			dp->label = newstring(sprintf(local,
				"VOLUME %d",dp->volno));
			strcpy(dp->label,local);
			/* this element being used as diskette number */
			dp->offset = eptr->Where.Address_u.disk.offset;
			dp->volsize = Size_of_Volume(eptr);
			break;
		}
	}
}

/*
 * make a distribution volume
 */

static
make_distribution(volume)
int volume;
{
	register toc_entry *eptr;
	register rval;

	if (vinfo.vaddr.dtype == TAPE)
		mkvinfo(volume);

	if(Get_Vol_Mounted(volume) < 0)
		return(-1);


	/*
	 * Unload Volume (use this step to make sure tape is ready)
	 */

	if(Unload)
	{
		(void) fprintf(stdlog,"Unloading volume %d\n",volume);
		(void) fprintf(stderr,"Unloading volume %d\n",volume);
		if(system(Unload) != 0)
		{
			(void) fprintf(stdlog,
				"Error in unloading volume %d\n",volume);
			(void) fprintf(stderr,
				"Error in unloading volume %d\n",volume);
			return(-1);
		}
	}
	if(Load)
	{
		(void) fprintf(stdlog,"Loading volume %d\n",volume);
		(void) fprintf(stderr,"Loading volume %d\n",volume);
		if(system(Load) != 0)
		{
			(void) fprintf(stdlog,
				"Error in loading volume %d\n",volume);
			(void) fprintf(stderr,
				"Error in loading volume %d\n",volume);
			return(-1);
		}
	}

	/*
	 * Percolate thru entire toc structure.
	 * For any entry, we have 4 choices as to what to
	 * do with it: 1) It doesn't belong to the volume we
	 * are dumping, 2) it's an XDR toc, 3) it's the ascii
	 * TOC, and 4) it is a normal entry.
	 *	
	 */

	if (vinfo.vaddr.dtype == TAPE)
	{
	    for(rval = -1, eptr = entries; eptr < ep; eptr++)
	    {
		if(Volume_of_Entry(eptr) != volume)
			continue;
		else if(IS_TOC(eptr))
		{
			if((rval = send_xdr_toc(eptr)) < 0)
				break;
		}
		else if((rval = send_reg(eptr)) < 0)
			break;
	    }
	}

	if(Unload)
	{
		(void) fprintf(stdlog,"Unloading volume %d\n",volume);
		(void) fprintf(stderr,"Unloading volume %d\n",volume);
		if(system(Unload) != 0)
		{
			(void) fprintf(stdlog,
				"Error in unloading volume %d\n",volume);
			(void) fprintf(stderr,
				"Error in unloading volume %d\n",volume);
			return(-1);
		}
	}

	return(rval);
}

static int
send_xdr_toc(this_ep)
toc_entry *this_ep;
{
	register FILE *op;
	static char local[128];
	FILE *popen();


#ifdef	lint
	this_ep = this_ep;
#endif
	if (vinfo.vaddr.dtype == TAPE) {
	    (void) sprintf(local,"tapeblock %s 512",Device);
	    if((op = popen(local,"w")) == NULL)
	    {
		(void) fprintf(stdlog,"...Could not open device '%s'\n",Device);
		(void) fprintf(stderr,"...Could not open device '%s'\n",Device);
		return(-1);
	    }

	    (void) fprintf(stdlog,"...transferring XDR TOC entry..");
	    (void) fprintf(stderr,"...transferring XDR TOC entry..");

	    if(!write_xdr_toc(op,&dinfo,&vinfo,entries,ep-entries))
	    {
		(void) fprintf(stdlog,"Error writing xdr table of contents\n");
		(void) fprintf(stderr,"Error writing xdr table of contents\n");
		return(-1);
	    }
	    else if(pclose(op) != 0)
	    {
		(void) fprintf(stdlog,"Error closing xdr table of contents\n");
		(void) fprintf(stderr,"Error closing xdr table of contents\n");
		return(-1);
	    }
	}
	(void) fprintf(stdlog,"..done\n");
	(void) fprintf(stderr,"..done\n");

	return(0);
}

/*
 * send_reg - send out a regular file...
 *
 */

static int
send_reg(eptr)
toc_entry *eptr;
{
	int status, pid;
	char cmdstring[BUFSIZ], *tarfile,  *archstring, *getenv();
		

	(void) fprintf(stdlog,"...transferring entry '%s'..",eptr->Name);
	(void) fprintf(stderr,"...transferring entry '%s'..",eptr->Name);

	if((pid = fork()))
	{
		while(wait((int *)&status) != pid)
			;
		if(status)
		{
			(void) fprintf(stdlog,"failed\n");
			(void) fprintf(stderr,"failed\n");
		}
		else
		{
			(void) fprintf(stdlog,"done\n");
			(void) fprintf(stderr,"done\n");
		}
		return(status);
	}
	/* Tape media */
	if (setoutput()) {
		fprintf(stderr, "setoutput failed \n");
		fprintf(stdlog, "setoutput failed \n");
		exit (1);
	}
	if (eptr->Info.kind == INSTALLABLE) {
		if ((tarfile = getenv("TARFILES")) == NULL) {
			fprintf(stderr, "No environment variable TARFILES \n");
			fprintf(stdlog, "No environment variable TARFILES \n");
			exit (1);
		}
		if ((archstring = getenv("ARCH")) == NULL)  {
			fprintf(stderr, "No environment variable ARCH \n"); 	
			fprintf(stdlog, "No environment variable ARCH \n"); 	
			exit (1);
		}
		/* blocking factor doesn't matter - going to stdout */
		if (eptr->Type == TARZ)
			sprintf(cmdstring, "dd if=%s/%s.Z/%s ibs=1b conv=sync obs=20b\n", tarfile,
			    archstring, eptr->Name);
		else
			sprintf(cmdstring, "dd if=%s/%s/%s bs=20b\n", tarfile,
			    archstring, eptr->Name);
	 	exit(system(cmdstring));	
	} else {
		exit(system(eptr->Command));
	}
}

/*
 * send_all - send out all non-required entries to an archive file...
 *
 * This function appends a macro definition ie. CLUSTERS="sys net debug..."
 * to the given makefile command string 
 *
 */

static int
send_archive(eptr)
toc_entry *eptr;
{
	register toc_entry *entp;
	register char *token, *token2;
	toc_entry ent;
	char clusters[BUFSIZ];
	char cmdstring[BUFSIZ];
	char *strtok();

	ent = *eptr;
	(void) strcpy(clusters, " CLUSTERS=\"");
	for (entp = eptr; !IS_TOC(eptr) && (entp < ep); entp++) {
		(void) strcpy(cmdstring, entp->Command);
		token = strtok(cmdstring,"=`;$ ");
		while (token != NULL) {
			if (strcmp(token, "CLUSTER") == 0) {
				token2 = strtok(NULL," ;`$=");
				break;
			}
			token = strtok(NULL," ;`$=");
		}
		if (token2) {
			(void) strcat(clusters, token2);
			(void) strcat(clusters, " ");
		}
	}
	(void) strcat(clusters, "\"");
	(void) strcpy(cmdstring, eptr->Command);
	(void) strcat(cmdstring, clusters);
	ent.Command = cmdstring;
	return(send_reg(&ent));
}

static char *
fsize(amt)
unsigned int amt;
{
	auto char local[32];

	if(amt < 1024)
		return(newstring(sprintf(local,"%u bytes",amt)));
	else if(amt < 1024*1024)
		return(newstring(sprintf(local,"%ukb",(amt+((1<<10)-1))>>10)));
	else
		return(newstring(sprintf(local,"%umb",(amt+((1<<20)-1))>>20)));
}
/*
 * make_write_command
 *
 *	FIX ME FIX ME - I'll only handle local tape devices for
 *	the moment!!!
 */


static
make_write_command(vp)
volume_info *vp;
{
	auto char tmp[128];

	Load = Device = Unload = (char *) 0;
/*
 * We had also better be just tapes around here!!!
 */
	/* Diskette support now added!!!
	 */
	if (vp->vaddr.dtype == TAPE) {
		Load = newstring(sprintf(tmp,"mt -f /dev/r%s rewind",rname));
		Unload = newstring(sprintf(tmp,"mt -f /dev/r%s weof",rname));
		Device = newstring(sprintf(tmp,"/dev/nr%s",rname));
	}
}




static
log_start()
{
	auto time_t tv;
	extern time_t time();
	extern char *ctime();

	(void) time(&tv);
	(void) fprintf(stdlog,
		"make_distribution: LOGGING STARTED %s",ctime(&tv));
}
