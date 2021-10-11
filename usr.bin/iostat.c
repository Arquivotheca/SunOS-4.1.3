#ifndef lint
static  char sccsid[] = "@(#)iostat.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * rewritten from UCB 4.13 83/09/25
 */

/*
 * iostat
 */
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <signal.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/time.h>

struct nlist nl[] = {
	{ "_dk_busy" },
#define	X_DK_BUSY	0
	{ "_dk_time" },
#define	X_DK_TIME	1
	{ "_dk_xfer" },
#define	X_DK_XFER	2
	{ "_dk_wds" },
#define	X_DK_WDS	3
	{ "_tk_nin" },
#define	X_TK_NIN	4
	{ "_tk_nout" },
#define	X_TK_NOUT	5
	{ "_dk_seek" },
#define	X_DK_SEEK	6
	{ "_cp_time" },
#define	X_CP_TIME	7
	{ "_dk_bps" },
#define	X_DK_BPS	8
	{ "_hz" },
#define	X_HZ		9
	{ "_phz" },
#define	X_PHZ		10
	{ "_dk_ndrive" },
#define	X_DK_NDRIVE	11
	{ "_dk_read" },
#define X_DK_READ	12
	{ "_ncpu" },
#define	X_NCPU		13

#define X_COMMON	13

#ifdef vax
	{ "_mbdinit" },
#define X_MBDINIT	X_COMMON+1
	{ "_ubdinit" },
#define X_UBDINIT	X_COMMON+2
#endif

#ifdef sun
	{ "_mbdinit" },
#define X_MBDINIT	X_COMMON+1
	{ "_dk_ivec" },
#define	X_DK_IVEC	X_COMMON+2
	{ "_dk_hexunit" },
#define X_DK_HEXUNIT	X_COMMON+3
#endif

	{ 0 },
};

char	**dr_name;
int	*dr_select;
long	*dk_bps;
u_int	dk_ndrive;
int 	ncpu;

struct {
	int	dk_busy;
	long	cp_time[CPUSTATES];
	long	*dk_time;
	long	*dk_wds;
	long	*dk_seek;
	long	*dk_xfer;
	long	*dk_read;
	long	tk_nin;
	long	tk_nout;
} s, s1;

struct timeval curr_time_tv;
struct timezone curr_time_tz;

int	kfd;		/* kernel file descriptor */
int	hz;
int	phz;
int	etime_base;
double	etime;		/* elapsed time */
double	eutime;		
long	oldtime;
long	oldutime;
long	ftime = 1;
int	tohdr = 1;
char	*cmdname;	/* name by which command was invoked */

#define USECS_PER_SEC	1000000 /* microseconds per second - for rollover */
#define TICK_IN_USECS   10000   /* ticks in usec base */

#define SIZEDKWDS	64.	/* number of characters per dk_wds */
#define	KILO		1024.	/* size of kilobyte */
#define MILLE		1000.	/* scale factor seconds to milliseconds */
#define	PERCENT		100.	/* scale factor fraction to percent */

#define LINES4HDR	19	/* number of lines to emit after header */
#define DISKHDRLEN	80	/* maximum disk header length */
#define NAMESIZE	10	/* size of device names */
#define MAXINTSIZE	12	/* sizeof "-2147483648" + 1 */

#define DISK_OLD	1
#define DISK_NEW	2

int	do_tty;
int	do_disk;
int	do_cpu;
int	do_interval = 0;
char	disk_header[DISKHDRLEN];
int	limit = 4;		/* limit for drive display */

void	printhdr();
char	*calloc();
off_t	lseek();
char	*sprintf();

char    *rindex();
#define setbasename(target, source) \
do {char *s = source, *t = rindex(s,'/'); target = t ? t + 1 : s;} while (0)

#define FETCH(index, variable) \
	k_index(index, (char *)&variable, sizeof (variable))
#define FETCH_RA(index, var) \
 	k_index(index, (char *)var, dk_ndrive*sizeof (long));

main(argc, argv)
	int  argc;
	char *argv[];
{
	extern char *ctime();
	register  i;
	int ndrives;
	int iter;
	long t;			/* temporary */
	u_int interval;		/* delay between display */
	char ch;


	setbasename(cmdname, *argv);
	argc--, argv++;

	while (argc && **argv == '-') {
		register char *cp;

		for (cp = *argv + 1; *cp; cp++)
		switch (*cp) {
		case 't':
			do_tty++;
			break;
		case 'd':
			do_disk = DISK_OLD;
			break;
		case 'D':
			do_disk = DISK_NEW;
			break;
		case 'c':
			do_cpu++;
			break;
		case 'I':
			do_interval++;
			break;
		case 'l':
			if (argc > 1) {
				limit = atoi(argv[1]);
				if (limit < 1)
					usage();
				if (limit > DK_NDRIVE)
					limit = DK_NDRIVE;
				argc--, argv++;
				break;
			} else
				usage();
		default:
			usage(); 
		}
	argc--, argv++;
	}

	/* if no output classes explicity specified, use defaults */
	if (do_tty == 0 && do_disk == 0 && do_cpu == 0)
		do_tty = do_cpu = 1, do_disk = DISK_OLD;

	
	if (nlist("/vmunix", nl) < 0)
		fail("nlist", "/vmunix", 0);

	kfd = open("/dev/kmem", 0);
	if(kfd < 0)
		fail("open", "/dev/kmem", 1);

	iter = 0;
	interval = 0;

	ch = do_interval ? 'i' : 's';
	(void)sprintf(disk_header, " %cp%c %cp%c %s ",
	    do_disk == DISK_OLD ? 'b' : 'r', ch,
	    do_disk == DISK_OLD ? 't' : 'w', ch,
	    do_disk == DISK_OLD ? "msps" : "util");

	if ( !(k_index_ncpu( X_NCPU, (char *)&ncpu, sizeof (ncpu)))) 
		ncpu = 1;

	FETCH(X_HZ, hz);
	FETCH(X_PHZ, phz);
	if (phz)
		hz = phz;

	
	if (do_disk) {
		init_disk();
		/*
		 * Choose drives to be displayed.  Priority
		 * goes to (in order) drives supplied as arguments,
		 * then any other active drives that fit.
		 */
		ndrives = 0;
		while (argc > 0 && !isdigit(argv[0][0])) {
			for (i = 0; i < dk_ndrive; i++) {
				if (strcmp(dr_name[i], argv[0]))
					continue;
				dr_select[i] = 1;
				ndrives++;
			}
			argc--, argv++;
		}
		for (i = 0; i < dk_ndrive && ndrives < limit; i++) {
			if (dr_select[i] || dk_bps[i] == 0)
				continue;
			dr_select[i] = 1;
			ndrives++;
		}
	}

	if (argc > 0) {
		interval = atoi(argv[0]);
		if (interval <= 0)
			fail("negative interval;", argv[0], 0);
		argc--, argv++;
	}
	if (argc > 0) {
		iter = atoi(argv[0]);
		if (iter <= 0)
			fail("negative count;", argv[0], 0);
		argc--, argv++;
	}

	if (argc != 0)
		usage();

	(void)signal(SIGCONT, printhdr);


	for (;;) {
		if (--tohdr == 0)
			printhdr();

		/* 
		 * DODELTA:
		 *    s  gets the delta  AND
		 *    s1 gets the current value. 
		 */
#define DODELTA(fld,ndx)   t = s.fld/**/ndx; \
			   s.fld/**/ndx -= s1.fld/**/ndx; \
			   s1.fld/**/ndx = t
#define DELi(fld) 	DODELTA(fld, [i])
#define DEL(fld) 	DODELTA(fld, )


		if (do_disk) {
			FETCH(X_DK_BUSY, s.dk_busy);
			FETCH_RA(X_DK_XFER, s.dk_xfer);
			FETCH_RA(X_DK_READ, s.dk_read);
			FETCH_RA(X_DK_SEEK, s.dk_seek);
			FETCH_RA(X_DK_WDS, s.dk_wds);
			FETCH_RA(X_DK_TIME, s.dk_time);

			for (i = 0; i < dk_ndrive; i++) {
				if (!dr_select[i])
					continue;
				DELi(dk_xfer);
				DELi(dk_read);
				DELi(dk_seek);
				DELi(dk_wds);
				DELi(dk_time);
			}
		}

		if (do_tty) {
			FETCH(X_TK_NIN, s.tk_nin);
			FETCH(X_TK_NOUT, s.tk_nout);
			DEL(tk_nin);	
			DEL(tk_nout);	
		}

		k_index(X_CP_TIME, (char *)s.cp_time, sizeof s.cp_time);

		etime = 0;
		for (i=0; i<CPUSTATES; i++) {
			DELi(cp_time);
			if ( ftime )
				etime += s.cp_time[i];
		}

		(void)gettimeofday(&curr_time_tv, &curr_time_tz);
		
		if ( ftime ) {
			ftime--;
			oldtime = curr_time_tv.tv_sec;
			oldutime = curr_time_tv.tv_usec;
			etime_base = hz*ncpu;
		} else {	
			if ( oldutime > curr_time_tv.tv_usec) {
				eutime = ((double)curr_time_tv.tv_usec 
					+ USECS_PER_SEC
					- oldutime)/USECS_PER_SEC;
				etime = (curr_time_tv.tv_sec - oldtime - 1)
					+ eutime;
			} else {
				eutime = ((double)curr_time_tv.tv_usec 
					- oldutime)/USECS_PER_SEC;
				etime = (curr_time_tv.tv_sec - oldtime)
					+ eutime;
			}
			oldtime = curr_time_tv.tv_sec;
			oldutime = curr_time_tv.tv_usec;
			etime_base = 1;
		}
		if ( etime == 0.0)
			etime = 1.0;
		etime /= etime_base;

#undef DEL
#undef DELi
#undef DODELTA
	
		if (do_tty) {
			(void)printf("%4.0f%5.0f", 
				s.tk_nin/etime, s.tk_nout/etime);
		}
		if (do_disk) {
			for (i=0; i<dk_ndrive; i++)
				if (dr_select[i])
					dk_stat(i);
		}
		if (do_cpu) {
			for (i=0; i<CPUSTATES; i++)
				cpu_stat(i);
		}	

		(void)printf("\n");
		(void)fflush(stdout);
	
		if (--iter && interval > 0) 
			sleep(interval);
		else
			exit(0); 
	} /*for*/
}


void
printhdr()
{
	register int i;

	if (do_tty)
		(void)printf("      tty");
	if (do_disk) {
		for (i = 0; i < dk_ndrive; i++)
			if (dr_select[i])
				(void)printf(" %12s ", dr_name[i]);
	}
	if (do_cpu)
		(void)printf("         cpu");
	(void)printf("\n");

	if (do_tty)
		(void)printf(" tin tout");
	if (do_disk)
		for (i = 0; i < dk_ndrive; i++)
			if (dr_select[i])
				(void)printf(disk_header);
	if (do_cpu)
		(void)printf(" us ni sy id");
	(void)printf("\n");

	tohdr = LINES4HDR;
}

dk_stat(dn)
{
	double atime;	/* active time */
	double bytes;
	double xtime;	/* transfer time */
	double itime;	/* active not transfer => seek time */
	double elapsed;
	long nread;
	long nwrite;

	elapsed = etime;
	if (do_interval)
		elapsed = 1;

	if (dk_bps[dn] == 0) {
		(void)printf("%4.0f%4.0f%5.1f ", 0.0, 0.0, 0.0);
		return;
	}

	atime = s.dk_time[dn];
	atime /= hz;
	bytes = s.dk_wds[dn]*SIZEDKWDS;	/* number of bytes transferred */
	xtime = bytes/dk_bps[dn];
	itime = atime - xtime;
	if (xtime < 0)
		itime += xtime, xtime = 0;
	if (itime < 0)
		xtime += itime, itime = 0;

	nread = s.dk_read[dn];
	nwrite = s.dk_xfer[dn] - nread;

	if (do_disk == DISK_OLD) {
		(void)printf("%4.0f", bytes/KILO/elapsed);
		/* 1K blocks per second */
		(void)printf("%4.0f", s.dk_xfer[dn]/elapsed);
		/* transfers per second */
		(void)printf("%5.1f ",
		    s.dk_seek[dn] ? itime*MILLE/s.dk_seek[dn] : 0.0);
		    /* milliseconds per seek */
	}

	if (do_disk == DISK_NEW) {
		(void)printf("%4.0f", nread/elapsed);
		/* reads per second */
		(void)printf("%4.0f", nwrite/elapsed);
		/* writes per second */
		(void)printf("%5.1f ", atime * PERCENT / etime);
		/* percent utilization */
	}
}

cpu_stat(o)
{
	register i;
	double time = 0;	

	for(i=0; i<CPUSTATES; i++)
		time += s.cp_time[i];
	if (time == 0.0 )
		time = 1.0;
	(void)printf("%3.0f", PERCENT*s.cp_time[o]/time);
}

#define STEAL(where, var, what) \
    k_addr((int)where, (char *)&var, sizeof (var), what);

#ifdef vax
#include <vaxuba/ubavar.h>
#include <vaxmba/mbavar.h>

read_names()
{
	struct mba_device mdev;
	register struct mba_device *mp;
	struct mba_driver mdrv;
	short two_char;
	char *cp = (char *) &two_char;
	struct uba_device udev, *up;
	struct uba_driver udrv;

	mp = (struct mba_device *) nl[X_MBDINIT].n_value;
	up = (struct uba_device *) nl[X_UBDINIT].n_value;
	if (up == 0) {
		(void)fprintf(stderr,
		    "%s: Disk init info not in namelist\n", cmdname);
		exit(2);
	}
	if (mp) for (;;) {
		STEAL(mp++, mdev, "mba_device");
		if (mdev.mi_driver == 0)
			break;
		if (mdev.mi_dk < 0 || mdev.mi_alive == 0)
			continue;
		STEAL(mdev.mi_driver, mdrv, "mi_driver");
		STEAL(mdrv.md_dname, two_char, "md_dname");
		(void)sprintf(dr_name[mdev.mi_dk], "%c%c%d",
		    cp[0], cp[1], mdev.mi_unit);
	}
	if (up) for (;;) {
		STEAL(up++, udev, "uba_device");
		if (udev.ui_driver == 0)
			break;
		if (udev.ui_dk < 0 || udev.ui_alive == 0)
			continue;
		STEAL(udev.ui_driver, udrv, "ui_driver");
		STEAL(udrv.ud_dname, two_char, "ud_dname");
		(void)sprintf(dr_name[udev.ui_dk], "%c%c%d",
		    cp[0], cp[1], udev.ui_unit);
}
#endif vax

#ifdef sun
#include <sundev/mbvar.h>

read_devinfo_names()
{
	register i;
	struct dk_ivec dk_ivec[DK_NDRIVE], *dk_ivp;
	short two_char;
	char *cp = (char *) &two_char;

	dk_ivp = (struct dk_ivec *) nl[X_DK_IVEC].n_value;
	if (dk_ivp == 0) {
		(void)fprintf(stderr,
		    "%s: Disk init info not in namelist\n", cmdname);
		exit(2);
	}
	for (i = 0; i < DK_NDRIVE; i++) {
		STEAL(dk_ivp++, dk_ivec[i], "dk_ivec");
	};
	for (dk_ivp = dk_ivec, i = 0; i < DK_NDRIVE; dk_ivp++, i++) {
		if (dk_ivp->dk_name == (char *) 0)
			continue;
		STEAL(dk_ivp->dk_name, two_char, "dk_name");
		sprintf(dr_name[i], "%c%c%d", cp[0], cp[1], dk_ivp->dk_unit);
	}
}

read_names()
{
	struct mb_device mdev;
	register struct mb_device *mp;
	struct mb_driver mdrv;
	short two_char;
	long dk_hexunit;
	char *cp = (char *) &two_char;

	mp = (struct mb_device *) nl[X_MBDINIT].n_value;
	if (mp == 0) {
		read_devinfo_names();
		return;
	}
	dk_hexunit = 0;
	if (nl[X_DK_HEXUNIT].n_value)
		STEAL(nl[X_DK_HEXUNIT].n_value, dk_hexunit, "dk_hexunit");
	for (;;) {
		STEAL(mp++, mdev, "mb_device");
		if (mdev.md_driver == 0)
			break;
		if (mdev.md_dk < 0 || mdev.md_alive == 0)
			continue;
		STEAL(mdev.md_driver, mdrv, "md_driver");
		STEAL(mdrv.mdr_dname, two_char, "mdr_dname");
		if (dk_hexunit & (1 << mdev.md_dk))
			(void)sprintf(dr_name[mdev.md_dk], "%c%c%3.3x",
				cp[0], cp[1], mdev.md_unit);
		else
			(void)sprintf(dr_name[mdev.md_dk], "%c%c%d",
				cp[0], cp[1], mdev.md_unit);
	}
}
#endif sun

/* issue failure message and exit */
fail(operation, value, doperror)
        char *operation;
	char *value;
	int doperror;
{
	if (doperror) {
		(void)fprintf(stderr, "%s: ", cmdname);
		perror(operation);
	}
        (void)fprintf(stderr,
	    "%s: %s failed with %s -- exiting\n",
	    cmdname, operation, value);
        exit(2);
}

/* fetch a variable with index index from the kernel */
k_index(index, buffer, size)
	int index;
	char *buffer;
	u_int size;
{
	int value;

	value = nl[index].n_value;

	if (value == 0)
		fail("symbol lookup", nl[index].n_name, 0);
	k_addr(value, buffer, size, nl[index].n_name);
}

int
k_index_ncpu(index, buffer, size)
	int index;
	char *buffer;
	u_int size;
{
	int value;

	value = nl[index].n_value;

	if (value != 0)  
		return (k_addr(value, buffer, size, nl[index].n_name));
	else
		return 0;
}

/* fetch a variable with address addr from the kernel */
k_addr(addr, buffer, size, name)
	int addr;
	char *buffer;
	u_int size;
	char *name;
{
	char msg[80];
	char *fmt = "%.*s (0x%x)";
	int precision;

	/* avoid overrunning the msg buffer */
	precision = sizeof(msg) - strlen(fmt) - MAXINTSIZE;

	(void)sprintf(msg, fmt, precision, name, addr);

	if (lseek(kfd, (off_t)addr, L_SET) == -1)
		fail("lseek", msg, 1);
	if (read(kfd, buffer, (int)size) != size)
		fail("read", msg, 1);
}

/* print out the usage line and quit */
usage()
{
	(void)fprintf(stderr,
	    "Usage: %s [-tdDcI] [-l n] [disk ...] [interval [count]]\n",
	    cmdname);
	exit(1);
}

/* initialize the disk related info */
init_disk()
{
	char *buf;		/* pointer into device name space */
	register i;

	if(nl[X_DK_BUSY].n_type == 0)
		fail("nlist", "dk_busy", 0);

	if (nl[X_DK_NDRIVE].n_value == 0) {
		fail("symbol lookup", nl[X_DK_NDRIVE].n_name, 0);
	}
	FETCH(X_DK_NDRIVE, dk_ndrive);
	if (0 == dk_ndrive ) {
		char buf[MAXINTSIZE];
		(void)sprintf(buf, "%d", dk_ndrive);
		fail("kernel dk_ndrive set to 0;", buf, 0);
	}

	dr_select = (int *)calloc(dk_ndrive, sizeof (int));
	dr_name = (char **)calloc(dk_ndrive, sizeof (char *));
	buf = (char *)calloc(dk_ndrive, NAMESIZE);
	dk_bps = (long *)calloc(dk_ndrive, sizeof (*dk_bps));
#define	allocate(e, t) \
    s./**/e = (t *)calloc(dk_ndrive, sizeof (t)); \
    s1./**/e = (t *)calloc(dk_ndrive, sizeof (t))
	allocate(dk_time, long);
	allocate(dk_wds, long);
	allocate(dk_seek, long);
	allocate(dk_xfer, long);
	allocate(dk_read, long);
#undef allocate

	k_index(X_DK_BPS, (char *)dk_bps, dk_ndrive*sizeof (dk_bps));

	for (i = 0; i < dk_ndrive; i++) {
		dr_name[i] = buf;
		(void)sprintf(buf, "dk%d", i);
		buf += NAMESIZE;
	}

	read_names();
}
