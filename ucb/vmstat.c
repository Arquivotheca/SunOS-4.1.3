/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)vmstat.c 1.1 92/07/30 SMI"; /* from UCB 5.4 5/17/86 */
#endif not lint

#include <stdio.h>
#include <ctype.h>
#include <nlist.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/vm.h>			/* has vmmeter.h */
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/dir.h>
#include <kvm.h>
#include <fcntl.h>

#ifdef vax
#include <sys/inode.h>
#include <sys/namei.h>
#endif

#ifdef sun
#include <sys/dnlc.h>
#include <sun/autoconf.h>
#endif sun

#ifdef vax

struct nlist nl[] = {
#define	X_CPTIME		0
	{ "_cp_time" },
#define	X_RATE			1
	{ "_rate" },
#define X_TOTAL			2
	{ "_total" },
#define	X_DEFICIT		3
	{ "_deficit" },
#define	X_FORKSTAT		4
	{ "_forkstat" },
#define X_SUM			5
	{ "_sum" },
#define	X_BOOTTIME		6
	{ "_boottime" },
#define	X_DKXFER		7
	{ "_dk_xfer" },
#define X_HZ			8
	{ "_hz" },
#define X_PHZ			9
	{ "_phz" },
#define X_NCHSTATS		10
	{ "_nchstats" },
#define	X_INTRNAMES		11
	{ "_intrnames" },
#define	X_EINTRNAMES		12
	{ "_eintrnames" },
#define	X_INTRCNT		13
	{ "_intrcnt" },
#define	X_EINTRCNT		14
	{ "_eintrcnt" },
#define	X_DK_NDRIVE		15
	{ "_dk_ndrive" },
#define	X_XSTATS		16
	{ "_xstats" },
#define X_MBDINIT		17
	{ "_mbdinit" },
#define X_UBDINIT		18
	{ "_ubdinit" },
#define X_REC			19
	{ "_rectime" },
#define X_PGIN			20
	{ "_pgintime" },
	{ "" },
};

#endif vax

#ifdef sun

struct nlist nl[] = {
#define	X_CPTIME		0
	{ "_cp_time" },
#define	X_RATE			1
	{ "_rate" },
#define	X_TOTAL			2
	{ "_total" },
#define	X_DEFICIT		3
	{ "_deficit" },
#define	X_FORKSTAT		4
	{ "_forkstat" },
#define	X_SUM			5
	{ "_sum" },
#define	X_BOOTTIME		6
	{ "_boottime" },
#define	X_DKXFER		7
	{ "_dk_xfer" },
#define	X_HZ			8
	{ "_hz" },
#define	X_PHZ			9
	{ "_phz" },
#define	X_NCHSTATS		10
	{ "_ncstats" },
#define	X_MBDINIT		11
	{ "_mbdinit" },
#define	X_AV2INTRNAMES		12
	{ "_level2_names" },
#define	X_AV3INTRNAMES		13
	{ "_level3_names" },
#define	X_AV4INTRNAMES		14
	{ "_level4_names" },
#define	X_AVNAMETAB		15
	{ "_av_nametab" },
#define	X_AVEND			16
	{ "_av_end" },
#define	X_AV2INTRCNT		17
	{ "_level2_intcnt" },
#define	X_AV3INTRCNT		18
	{ "_level3_intcnt" },
#define	X_AV4INTRCNT		19
	{ "_level4_intcnt" },
#define	X_CLOCKINT		20
	{ "_clk_intr" },
#define	X_INTRNAMES		21
	{ "_intrnames" },
#define	X_EINTRNAMES		22
	{ "_eintrnames" },
#define	X_INTRCNT		23
	{ "_intrcnt" },
#define	X_EINTRCNT		24
	{ "_eintrcnt" },
#define	X_DK_NDRIVE		25
	{ "_dk_ndrive" },
#define	X_BSTATS		26
	{ "_bstats" },
#define	X_NBUF			27
	{ "_nbuf" },
#define	X_BUFALLOC		28
	{ "_bufalloc" },
#define	X_BUFNHEAD		29
	{ "_bufnhead" },
#define	X_REC			30
	{ "_rectime" },
#define	X_PGIN			31
	{ "_pgintime" },
#define	X_FLUSHSUM		32
	{ "_flush_sum" },
#define	X_VAC			33
	{ "_vac" },
#define	X_DK_IVEC		34
	{ "_dk_ivec" },

/*
 * Sun-4c (and Sun-4m "other") interrupt chains
 */

#define	X_AVLEVEL1		35
	{"_level1"},
#define	X_AVLEVEL2		36
	{"_level2"},
#define	X_AVLEVEL3		37
	{"_level3"},
#define	X_AVLEVEL4		38
	{"_level4"},
#define	X_AVLEVEL5		39
	{"_level5"},
#define	X_AVLEVEL6		40
	{"_level6"},
#define	X_AVLEVEL7		41
	{"_level7"},
#define	X_AVLEVEL8		42
	{"_level8"},
#define	X_AVLEVEL9		43
	{"_level9"},
#define	X_AVLEVEL10		44
	{"_level10"},
#define	X_AVLEVEL11		45
	{"_level11"},
#define	X_AVLEVEL12		46
	{"_level12"},
#define	X_AVLEVEL13		47
	{"_level13"},
#define	X_AVLEVEL14		48
	{"_level14"},
#define	X_AVLEVEL15		49
	{"_level15"},

/*
 * Sun-4m "soft" interrupt chains
 */

#define	X_AVXLVL1		50
	{"_xlvl1"},
#define	X_AVXLVL2		51
	{"_xlvl2"},
#define	X_AVXLVL3		52
	{"_xlvl3"},
#define	X_AVXLVL4		53
	{"_xlvl4"},
#define	X_AVXLVL5		54
	{"_xlvl5"},
#define	X_AVXLVL6		55
	{"_xlvl6"},
#define	X_AVXLVL7		56
	{"_xlvl7"},
#define	X_AVXLVL8		57
	{"_xlvl8"},
#define	X_AVXLVL9		58
	{"_xlvl9"},
#define	X_AVXLVL10		59
	{"_xlvl10"},
#define	X_AVXLVL11		60
	{"_xlvl11"},
#define	X_AVXLVL12		61
	{"_xlvl12"},
#define	X_AVXLVL13		62
	{"_xlvl13"},
#define	X_AVXLVL14		63
	{"_xlvl14"},
#define	X_AVXLVL15		64
	{"_xlvl15"},

/*
 * Sun-4m "onboard" interrupt chains
 */

#define	X_AVOLVL1		65
	{"_olvl1"},
#define	X_AVOLVL2		66
	{"_olvl2"},
#define	X_AVOLVL3		67
	{"_olvl3"},
#define	X_AVOLVL4		68
	{"_olvl4"},
#define	X_AVOLVL5		69
	{"_olvl5"},
#define	X_AVOLVL6		70
	{"_olvl6"},
#define	X_AVOLVL7		71
	{"_olvl7"},
#define	X_AVOLVL8		72
	{"_olvl8"},
#define	X_AVOLVL9		73
	{"_olvl9"},
#define	X_AVOLVL10		74
	{"_olvl10"},
#define	X_AVOLVL11		75
	{"_olvl11"},
#define	X_AVOLVL12		76
	{"_olvl12"},
#define	X_AVOLVL13		77
	{"_olvl13"},
#define	X_AVOLVL14		78
	{"_olvl14"},
#define	X_AVOLVL15		79
	{"_olvl15"},

/*
 * Sun-4m "S"-Bus interrupt chains
 */

#define	X_AVSLVL1		80
	{"_slvl1"},
#define	X_AVSLVL2		81
	{"_slvl2"},
#define	X_AVSLVL3		82
	{"_slvl3"},
#define	X_AVSLVL4		83
	{"_slvl4"},
#define	X_AVSLVL5		84
	{"_slvl5"},
#define	X_AVSLVL6		85
	{"_slvl6"},
#define	X_AVSLVL7		86
	{"_slvl7"},

/*
 * Sun-4m "VME" interrupt chains
 */

#define	X_AVVLVL1		87
	{"_vlvl1"},
#define	X_AVVLVL2		88
	{"_vlvl2"},
#define	X_AVVLVL3		89
	{"_vlvl3"},
#define	X_AVVLVL4		90
	{"_vlvl4"},
#define	X_AVVLVL5		91
	{"_vlvl5"},
#define	X_AVVLVL6		92
	{"_vlvl6"},
#define	X_AVVLVL7		93
	{"_vlvl7"},
#define X_CACHE			94
	{"_cache"},
	{ "" },
};

#endif sun


char	**dr_name;
int	*dr_select;
int	dk_ndrive;
int	ndrives = 0;
#ifdef vax
char	*defdrives[] = { "hp0", "hp1", "hp2",  0 };
#else
char	*defdrives[] = { 0 };
#endif
double	stat1();
int	hz;
int	phz;
int	HERTZ;

#define	INTS(x)	((x) - (hz + phz))

struct {
	int	busy;
	long	time[CPUSTATES];
	long	*xfer;
	struct	vmmeter Rate;
	struct	vmtotal	Total;
	struct	vmmeter Sum;
	struct	forkstat Forkstat;
	unsigned rectime;
	unsigned pgintime;
} s, s1, z;
#define	rate		s.Rate
#define	total		s.Total
#define	sum		s.Sum
#define	forkstat	s.Forkstat

#define pgtok(a) ((a)*pagesize/1024)

struct bstats bstats;

struct	vmmeter osum;
int	deficit;
double	etime;
time_t	now, boottime;
int	printhdr();
int	lines = 1;
extern	char *calloc();
int	swflag = 0;
int	pagesize;

kvm_t	*kd;				/* kvm desciptor */

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *ctime();
	register i;
	int iter, nintv, iflag = 0;
	long t;
	char *arg, **cp, buf[BUFSIZ];

	pagesize = getpagesize();

	kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "vmstat");
	if ((int)kd == 0) {
		exit (1);
	}
	if (kvm_nlist(kd, nl) == 0) {
		printf("couldn't read name list\n");
		exit (1);
	}

	iter = 0;
	argc--, argv++;
	while (argc>0 && argv[0][0]=='-') {
		char *cp = *argv++;
		argc--;
		while (*++cp) switch (*cp) {

		case 'S':
			swflag = !swflag;
			break;

		case 't':
			dotimes();
			okexit();
			break;

		case 'z':
			kd = kvm_open(NULL, NULL, NULL, O_RDWR, "vmstat: ");
			if ((int)kd == -1) {
				printf("couldn't open kvm for writing\n");
				exit (1);
			}
			kvm_write(kd, (long)nl[X_SUM].n_value,
					&z.Sum, sizeof z.Sum);
			okexit();
			break;

		case 'f':
			doforkst();
			okexit();
			break;

		case 's':
			dosum();
			okexit();
			break;

		case 'i':
			iflag++;	/* per-device interrupts */
			break;

		case 'c':
			docachestats(argc, argv); /* cache flushing stats */
			okexit();
			break;

		case 'b':
			dobufstats();	/* undocumented for now */
			okexit();
			break;

		default:
			fprintf(stderr,
			    "usage: vmstat [ -fsSic ] [ interval ] [ count]\n");
			exit(1);
		}
	}

	kvm_read(kd, (long)nl[X_BOOTTIME].n_value, &boottime, sizeof boottime);
	kvm_read(kd, (long)nl[X_HZ].n_value, &hz, sizeof hz);
	if (nl[X_PHZ].n_value != 0)
		kvm_read(kd, (long)nl[X_PHZ].n_value, &phz, sizeof phz);
	HERTZ = phz ? phz : hz;
	if (nl[DK_NDRIVE].n_value == 0) {
		fprintf(stderr, "dk_ndrive undefined in system\n");
		kvm_close(kd);
		exit(1);
	}
	kvm_read(kd, nl[X_DK_NDRIVE].n_value, &dk_ndrive, sizeof (dk_ndrive));
	if (dk_ndrive <= 0) {
		fprintf(stderr, "dk_ndrive %d\n", dk_ndrive);
		kvm_close(kd);
		exit(1);
	}
	dr_select = (int *)calloc(dk_ndrive, sizeof (int));
	dr_name = (char **)calloc(dk_ndrive, sizeof (char *));

#define	allocate(e, t) \
    s./**/e = (t *)calloc(dk_ndrive, sizeof (t)); \
    s1./**/e = (t *)calloc(dk_ndrive, sizeof (t));
	allocate(xfer, long);
	for (arg = buf, i = 0; i < dk_ndrive; i++) {
		dr_name[i] = arg;
		sprintf(dr_name[i], "dk%d", i);
		arg += strlen(dr_name[i]) + 1;
	}
	read_names();
	time(&now);
	nintv = now - boottime;
	if (nintv <= 0 || nintv > 60*60*24*365*10) {
		fprintf(stderr,
		    "Time makes no sense... namelist must be wrong.\n");
		kvm_close(kd);
		exit(1);
	}
	if (iflag) {
		dointr(nintv);
		okexit();
	}

	/*
	 * Choose drives to be displayed.  Priority
	 * goes to (in order) drives supplied as arguments,
	 * default drives.  If everything isn't filled
	 * in and there are drives not taken care of,
	 * display the first few that fit.
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
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		for (cp = defdrives; *cp; cp++)
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				ndrives++;
				break;
			}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		ndrives++;
	}
	if (argc > 1)
		iter = atoi(argv[1]);
	if (iter < 0)  {
		fprintf(stderr, "usage: vmstat [ -fsSic ] [ interval ] [ count] count must be positive\n");
		exit(1);
	}
	signal(SIGCONT, printhdr);
loop:
	if (--lines == 0)
		printhdr();
 	kvm_read(kd, (long)nl[X_CPTIME].n_value, s.time, sizeof s.time);
	kvm_read(kd, (long)nl[X_DKXFER].n_value,
				s.xfer, dk_ndrive * sizeof (long));
	if (nintv != 1)
		kvm_read(kd, (long)nl[X_SUM].n_value, &rate, sizeof rate);
	else
		kvm_read(kd, (long)nl[X_RATE].n_value, &rate, sizeof rate);
	kvm_read(kd, (long)nl[X_TOTAL].n_value, &total, sizeof total);
	osum = sum;
	kvm_read(kd, (long)nl[X_SUM].n_value, &sum, sizeof sum);
	kvm_read(kd, (long)nl[X_DEFICIT].n_value, &deficit, sizeof deficit);
	etime = 0;
	for (i=0; i < dk_ndrive; i++) {
		t = s.xfer[i];
		s.xfer[i] -= s1.xfer[i];
		s1.xfer[i] = t;
	}
	for (i=0; i < CPUSTATES; i++) {
		t = s.time[i];
		s.time[i] -= s1.time[i];
		s1.time[i] = t;
		etime += s.time[i];
	}
	if(etime == 0.)
		etime = 1.;
	printf("%2d%2d%2d", total.t_rq, total.t_dw+total.t_pw, total.t_sw);
	printf("%6d%6d", pgtok(total.t_avm), pgtok(total.t_free));
	printf("%4d%3d",
	    swflag ?
		sum.v_swpin-osum.v_swpin :
		(rate.v_pgrec - (rate.v_xsfrec+rate.v_xifrec))/nintv,
	    swflag ?
		sum.v_swpout-osum.v_swpout :
	    	(rate.v_xsfrec+rate.v_xifrec)/nintv);
	printf("%4d", pgtok(rate.v_pgpgin)/nintv);
	printf("%4d%4d%4d%4d", pgtok(rate.v_pgpgout)/nintv,
	    pgtok(rate.v_dfree)/nintv, pgtok(deficit), rate.v_scan/nintv);
	etime /= (float)HERTZ;
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			stats(i);
	printf("%4d%4d%4d", INTS(rate.v_intr/nintv), rate.v_syscall/nintv,
	    rate.v_swtch/nintv);
	for(i=0; i<CPUSTATES; i++) {
		float f = stat1(i);
		if (i == 0) {		/* US+NI */
			i++;
			f += stat1(i);
		}
		printf("%3.0f", f);
	}
	printf("\n");
	fflush(stdout);
	nintv = 1;
	if (--iter &&argc > 0) {
		sleep(atoi(argv[0]));
		goto loop;
	}
	okexit();
	/* NOTREACHED */
}

okexit() {
	kvm_close(kd);
	exit (0);
}

printhdr()
{
	register int i, j;

	printf(" procs     memory              page           ");
	i = ndrives * 3;		/* width of drives field */
	if (i >= 6) {
		j = (i - 4) / 2;
		while (j--)
			putchar(' ');
		printf("disk");
		j = i - ((i - 4) / 2) - 4;
		while (j--)
			putchar(' ');
	} else {
		while (i-- > 0)
			putchar(' ');
	}
	printf("   faults     cpu\n");
	if (swflag)
		printf(" r b w   avm   fre  si so  pi  po  fr  de  sr ");
	else
		printf(" r b w   avm   fre  re at  pi  po  fr  de  sr ");
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			printf("%c%c ", dr_name[i][0], dr_name[i][2]);	
	printf(" in  sy  cs us sy id\n");
	lines = 19;
}

dotimes()
{

	kvm_read(kd, (long)nl[X_REC].n_value, &s.rectime, sizeof s.rectime);
	kvm_read(kd, (long)nl[X_PGIN].n_value, &s.pgintime, sizeof s.pgintime);
	kvm_read(kd, (long)nl[X_SUM].n_value, &sum, sizeof sum);
	printf("%d reclaims, %d total time (usec)\n", sum.v_pgrec, s.rectime);
	printf("average: %d usec / reclaim\n", s.rectime/sum.v_pgrec);
	printf("\n");
	printf("%d page ins, %d total time (msec)\n",sum.v_pgin, s.pgintime/10);
	printf("average: %8.1f msec / page in\n", s.pgintime/(sum.v_pgin*10.0));
}

/* SHOULD BE AVAILABLE IN <sys/text.h> */
/*
 * Statistics
 */
#ifdef vax
struct xstats {
	u_long	alloc;			/* calls to xalloc */
	u_long	alloc_inuse;		/*	found in use/sticky */
	u_long	alloc_cachehit;		/*	found in cache */
	u_long	alloc_cacheflush;	/*	flushed cached text */
	u_long	alloc_unused;		/*	flushed unused cached text */
	u_long	free;			/* calls to xfree */
	u_long	free_inuse;		/*	still in use/sticky */
	u_long	free_cache;		/*	placed in cache */
	u_long	free_cacheswap;		/*	swapped out to place in cache */
};
#endif vax
/* END SHOULD BE AVAILABLE... */

dosum()
{
#ifdef vax
	struct nchstats nchstats;
#endif vax
#ifdef sun
	struct ncstats ncstats;
#endif sun
#ifdef vax
	struct xstats  xstats;
#endif vax
	long nchtotal;

	kvm_read(kd, (long)nl[X_SUM].n_value, &sum, sizeof sum);
	printf("%9d swap ins\n", sum.v_swpin);
	printf("%9d swap outs\n", sum.v_swpout);
	printf("%9d pages swapped in\n", sum.v_pswpin / CLSIZE);
	printf("%9d pages swapped out\n", sum.v_pswpout / CLSIZE);
	printf("%9d total address trans. faults taken\n", sum.v_faults);
	printf("%9d page ins\n", sum.v_pgin);
	printf("%9d page outs\n", sum.v_pgout);
	printf("%9d pages paged in\n", sum.v_pgpgin);
	printf("%9d pages paged out\n", sum.v_pgpgout);
	printf("%9d sequential process pages freed\n", sum.v_seqfree);
	printf("%9d total reclaims (%d%% fast)\n", sum.v_pgrec,
	    (sum.v_fastpgrec * 100) / (sum.v_pgrec == 0 ? 1 : sum.v_pgrec));
	printf("%9d reclaims from free list\n", sum.v_pgfrec);
	printf("%9d intransit blocking page faults\n", sum.v_intrans);
	printf("%9d zero fill pages created\n", sum.v_nzfod / CLSIZE);
	printf("%9d zero fill page faults\n", sum.v_zfod / CLSIZE);
	printf("%9d executable fill pages created\n", sum.v_nexfod / CLSIZE);
	printf("%9d executable fill page faults\n", sum.v_exfod / CLSIZE);
	printf("%9d swap text pages found in free list\n", sum.v_xsfrec);
	printf("%9d inode text pages found in free list\n", sum.v_xifrec);
	printf("%9d file fill pages created\n", sum.v_nvrfod / CLSIZE);
	printf("%9d file fill page faults\n", sum.v_vrfod / CLSIZE);
	printf("%9d pages examined by the clock daemon\n", sum.v_scan);
	printf("%9d revolutions of the clock hand\n", sum.v_rev);
	printf("%9d pages freed by the clock daemon\n", sum.v_dfree / CLSIZE);
	printf("%9d cpu context switches\n", sum.v_swtch);
	printf("%9d device interrupts\n", sum.v_intr);
	/* printf("%9d software interrupts\n", sum.v_soft); XX missing */
#ifdef vax
	printf("%9d pseudo-dma dz interrupts\n", sum.v_pdma);
#endif
	printf("%9d traps\n", sum.v_trap);
	printf("%9d system calls\n", sum.v_syscall);
#define	nz(x)	((x) ? (x) : 1) /* XX */
#ifdef vax
	kvm_read(kd, (long)nl[X_NCHSTATS].n_value, &nchstats, sizeof nchstats);
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_badhits +
	    nchstats.ncs_falsehits + nchstats.ncs_miss + nchstats.ncs_long;
	printf("%9d total name lookups", nchtotal);
	printf(" (cache hits %d%% system %d%% per-process)\n",
	    nchstats.ncs_goodhits * 100 / nz(nchtotal),
	    nchstats.ncs_pass2 * 100 / nz(nchtotal));
	printf("%9s badhits %d, falsehits %d, toolong %d\n", "",
	    nchstats.ncs_badhits, nchstats.ncs_falsehits, nchstats.ncs_long);
#endif vax

#ifdef sun
	kvm_read(kd, (long)nl[X_NCHSTATS].n_value, &ncstats, sizeof ncstats);
	nchtotal = ncstats.hits + ncstats.misses + ncstats.long_look;
	printf("%9d total name lookups", nchtotal);
	printf(" (cache hits %d%%  per-process)\n",
	    ncstats.hits * 100 / nz(nchtotal));
	printf("%9s toolong %d\n", "", ncstats.long_enter+ncstats.long_look);
#endif sun

#ifdef vax
	kvm_read(kd, (long)nl[X_XSTATS].n_value, &xstats, sizeof xstats);
	printf("%9d total calls to xalloc (cache hits %d%%)\n",
	    xstats.alloc, xstats.alloc_cachehit * 100 / nz(xstats.alloc));
	printf("%9s sticky %d flushed %d unused %d\n", "",
	    xstats.alloc_inuse, xstats.alloc_cacheflush, xstats.alloc_unused);
	printf("%9d total calls to xfree", xstats.free);
	printf(" (sticky %d cached %d swapped %d)\n",
	    xstats.free_inuse, xstats.free_cache, xstats.free_cacheswap);
#endif vax

}

doforkst()
{

	kvm_read(kd, (long)nl[X_FORKSTAT].n_value, &forkstat, sizeof forkstat);
	printf("%d forks, %d pages, average=%.2f\n",
		forkstat.cntfork, forkstat.sizfork,
		(float) forkstat.sizfork / forkstat.cntfork);
	printf("%d vforks, %d pages, average=%.2f\n",
		forkstat.cntvfork, forkstat.sizvfork,
		(float)forkstat.sizvfork / forkstat.cntvfork);
}


dobufstats()
{
	int nbuf;
	int totreads;
	int hits;
	int hitratio;
	int cachesize;
	int headers;

	kvm_read(kd, (long)nl[X_BSTATS].n_value, &bstats, sizeof(struct bstats));
	kvm_read(kd, (long)nl[X_NBUF].n_value, &nbuf, sizeof(int));
	kvm_read(kd, (long)nl[X_BUFALLOC].n_value, &cachesize, sizeof(int));
	kvm_read(kd, (long)nl[X_BUFNHEAD].n_value, &headers, sizeof(int));
	totreads = bstats.n_bread;
	hits = bstats.n_bread_hits;
	hitratio = (totreads == 0) ? 0 : hits * 100 / totreads;
	printf("nbuf<%d> headers<%d> cachesize<%d> reads<%d> hits<%d> hit ratio<%d>\n",
	  nbuf, headers, cachesize, totreads, hits, hitratio);
	printf("cache allocation: AGE<%d> LRU<%d> SLEEP<%d> Total:<%d>\n",
		bstats.n_ages, bstats.n_lrus, bstats.n_sleeps,
		bstats.n_ages + bstats.n_lrus + bstats.n_sleeps + cachesize);
}

stats(dn)
{

	if (dn >= dk_ndrive) {
		printf("  0");
		return;
	}
	printf("%3.0f", s.xfer[dn]/etime);
}

double
stat1(row)
{
	double t;
	register i;

	t = 0;
	for(i=0; i<CPUSTATES; i++)
		t += s.time[i];
	if(t == 0.)
		t = 1.;
	return(s.time[row]*100./t);
}

#ifdef vax
dointr(nintv)
{
	int nintr, inttotal;
	long *intrcnt;
	char *intrname, *malloc();

	nintr = (nl[X_EINTRCNT].n_value - nl[X_INTRCNT].n_value) / sizeof(long);
	intrcnt = (long *) malloc(nl[X_EINTRCNT].n_value -
		nl[X_INTRCNT].n_value);
	intrname = malloc(nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	if (intrcnt == NULL || intrname == NULL) {
		fprintf(stderr, "vmstat: out of memory\n");
		kvm_close(kd);
		exit(9);
	}
	kvm_read(kd, (long)nl[X_INTRCNT].n_value,
					 intrcnt, nintr * sizeof (long));
	kvm_read(kd, (long)nl[X_INTRNAMES].n_value, intrname,
		 nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	printf("interrupt      total      rate\n");
	inttotal = 0;
	while (nintr--) {
		if (*intrcnt)
			printf("%-12s %8ld %8ld\n", intrname,
			    *intrcnt, *intrcnt / nintv);
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	printf("Total        %8ld %8ld\n", inttotal, inttotal / nintv);
}
#endif vax

#ifdef sun

dointr(nintv)
{
	int avcnt;
	int veccnt;

	printf("interrupt      total      rate\n");
	avcnt = avintr(nintv);		/* display autovectored interrupts */
	veccnt = vecintr(nintv);	/* display vectored interrupts */
	printf("-----------------------------------\n");
	printf("Total        %8ld %8ld\n",
	  avcnt + veccnt, (avcnt + veccnt) / nintv);
}

#define	PRINTCOUNTS(cnt)						\
	for (i = 0; ; i++) {						\
		if (correspond/**/cnt/**/[i] == 0)			\
			break;						\
		printf("%-12s %8ld %8ld\n",				\
		  cornames[correspond/**/cnt/**/[i]-1], count/**/cnt/**/[i],\
		  count/**/cnt/**/[i] / nintv);				\
		inttotal += count/**/cnt/**/[i];				\
	}

#define READCORRESP(cnt)						\
	kvm_read(kd, (long)nl[X_AV/**/cnt/**/INTRNAMES].n_value, 	\
		 correspond/**/cnt/**/, sizeof(int) * NVECT);

#define	READCOUNTS(cnt)							\
	kvm_read(kd, (long)nl[X_AV/**/cnt/**/INTRCNT].n_value, 		\
		 count/**/cnt/**/, NVECT*sizeof(int));

/* display autovectored interrupts */
int
avintr(nintv)
{
	char *malloc();
	char *av_nametab;
	char *av_end;
	char *nametab;
	int size;
	char *cornames[100];
	register int i;
	int correspond2[NVECT];
	int correspond3[NVECT];
	int correspond4[NVECT];
	int count2[NVECT];
	int count3[NVECT];
	int count4[NVECT];
	int clockintr;
	int inttotal = 0;

	/* read in the name table */
	if (
	  (nl[X_AV2INTRNAMES].n_value == 0) ||
	  (nl[X_AV3INTRNAMES].n_value == 0) ||
	  (nl[X_AV4INTRNAMES].n_value == 0) ||
	  (nl[X_AV2INTRCNT].n_value == 0) ||
	  (nl[X_AV3INTRCNT].n_value == 0) ||
	  (nl[X_AV4INTRCNT].n_value == 0)) {
		return (autovec_intr(nintv));
	  }

	if (nl[X_AVEND].n_value == 0)
		return (0);
	kvm_read(kd, (long)nl[X_AVEND].n_value, &av_end, sizeof(char *));
	kvm_read(kd, (long)av_end, &av_end, sizeof(char *));
	if (nl[X_AVNAMETAB].n_value == 0)
		return (0);
	kvm_read(kd, (long)nl[X_AVNAMETAB].n_value,
				 &av_nametab, sizeof(char *));
	size = (unsigned)av_end - (unsigned)av_nametab;
	nametab = malloc(size);
	kvm_read(kd, (long)av_nametab, nametab, size);

	printf("----------------------------------- autovectored interrupts\n");

	/* store pointers to nametable strings */
	for (i = 0; size > 0; i++) {
		cornames[i] = nametab;
		size -= (strlen(nametab) + 1);
		nametab += (strlen(nametab) + 1);
	}

	/* read in the correspondence tables */
	READCORRESP(2);
	READCORRESP(3);
	READCORRESP(4);

	/* read in the interrupt counts */
	READCOUNTS(2);
	READCOUNTS(3);
	READCOUNTS(4);

	/* print out the interrupt counts */
	PRINTCOUNTS(2);
	PRINTCOUNTS(3);
	PRINTCOUNTS(4);

	/* get clock interrupts */
	if (nl[X_CLOCKINT].n_value == 0)
		return (inttotal);
	kvm_read(kd, (long)nl[X_CLOCKINT].n_value, &clockintr, sizeof(char *));
	printf("%-12s %8ld %8ld\n", "clock", clockintr, clockintr / nintv);
	inttotal += clockintr;

	return (inttotal);
}

/*
 * here we deal with the 'new' autoconfig as used in the sun4c and sun4m
 */
int
autovec_intr(nintv)
{
	int clockintr;
	int inttotal = 0;
	register int ix, i;
	char avname[20];	/* XXX max name length?!? */
	struct autovec av[NVECT];

	if ( (nl[X_AVLEVEL1].n_value == 0) ||
	     (nl[X_AVLEVEL2].n_value == 0) ||
	     (nl[X_AVLEVEL3].n_value == 0) ||
	     (nl[X_AVLEVEL4].n_value == 0) ||
	     (nl[X_AVLEVEL5].n_value == 0) ||
	     (nl[X_AVLEVEL6].n_value == 0) ||
	     (nl[X_AVLEVEL7].n_value == 0) ||
	     (nl[X_AVLEVEL8].n_value == 0) ||
	     (nl[X_AVLEVEL9].n_value == 0) ||
	     (nl[X_AVLEVEL13].n_value == 0) ) 
		return(0);

	printf("----------------------------------- autovectored interrupts\n");

	for (ix = X_AVLEVEL1; ix <= X_AVVLVL7; ix++) {
		if (nl[ix].n_type != 0) {
			kvm_read(kd, (long)nl[ix].n_value, av, sizeof(av));
			for (i = 0; i < NVECT; i++) {
				if (av[i].av_name == (char *)0)
					break;
				/*
				 * XXX
				 * read enought to get the entire name.
				 * We want to avoid reading a byte at a time and
				 * looking for the null terminator
				 */
				kvm_read(kd, (long)av[i].av_name, avname,sizeof(avname));
				printf("%-12s %8ld %8ld\n",
				       avname,
				       av[i].av_intcnt,
				       av[i].av_intcnt / nintv);
				inttotal += av[i].av_intcnt;
			}
		}
	}

	/* get clock interrupts */
	if (nl[X_CLOCKINT].n_type == 0)
		return (inttotal);
	kvm_read (kd, (long)nl[X_CLOCKINT].n_value,
		(caddr_t) &clockintr, sizeof(clockintr));
	printf("%-12s %8ld %8ld\n", "clock", clockintr, clockintr / nintv);
	inttotal += clockintr;

	return (inttotal);
}

/* display vectored interrupts */
int
vecintr(nintv)
{
	int nintr, inttotal;
	long *intrcnt;
	char *intrname, *malloc();

	nintr = (nl[X_EINTRCNT].n_value - nl[X_INTRCNT].n_value) / sizeof(long);
	if (nintr == 0)
		return (0);
	intrcnt = (long *) malloc(nl[X_EINTRCNT].n_value -
		nl[X_INTRCNT].n_value);
	intrname = malloc(nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	if (intrcnt == NULL || intrname == NULL) {
		fprintf(stderr, "vmstat: out of memory\n");
		kvm_close(kd);
		exit(9);
	}
	kvm_read(kd, (long)nl[X_INTRCNT].n_value, intrcnt,
						 nintr * sizeof (long));
	kvm_read(kd, (long)nl[X_INTRNAMES].n_value,
		 intrname, nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	printf("----------------------------------- vectored interrupts\n");
	inttotal = 0;
	while (nintr--) {
		if (*intrcnt)
			printf("%-12s %8ld %8ld\n", intrname,
			    *intrcnt, *intrcnt / nintv);
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	return (inttotal);
}

docachestats(argc, argv)
	int argc;
	char **argv;
{
	register int interval, count, forever;
	register int i;
	register int totonly = 1;
	register struct flushmeter *last, *current;
	int vacval;

       if (nl[X_VAC].n_value == 0)
                no_vac_warning();
        else {
                kvm_read(kd, (long)nl[X_VAC].n_value, &vacval, sizeof vacval);
                if (vacval == 0) {
			if (nl[X_CACHE].n_value == 0)
				no_vac_warning();
			else {
                		kvm_read(kd, (long)nl[X_CACHE].n_value, 
							&vacval, sizeof vacval);
				if (vacval == 0)
                        		no_vac_warning();
				else
					printf("Physical Address Cache is ON\n");
			}
		} else {
			printf("Virtual Address Cache is ON\n");
		}
        }

	if (nl[X_FLUSHSUM].n_value == 0) {
		printf("couldn't get flush statistics\n");
		kvm_close(kd);
		exit(1);
	} 
	last = (struct flushmeter *) malloc(sizeof (struct flushmeter) * 2);
	current = last + sizeof (struct flushmeter);

	/* read orignal flush statistics */
	kvm_read(kd, (long)nl[X_FLUSHSUM].n_value,
				 last, sizeof (struct flushmeter));
	interval = 5;		/* default values */
	count = 1;
	if (argc) {
		interval = atoi(argv[0]);
		forever = 1;
		totonly = 0;
	}
	if (argc > 1) {
		count = atoi(argv[1]);
		forever = 0;
	}
	printf("flush statistics: (%s)\n",
		totonly ? "totals" : "interval based" );
	if (totonly) {
		printf("%8s%8s%8s%8s%8s%8s\n", 
			"usr", "ctx", "rgn", "seg", "pag", "par");
		printf("%8d%8d%8d%8d%8d%8d\n",
			last->f_usr, last->f_ctx, last->f_region,
			last->f_segment, last->f_page, last->f_partial);
		okexit();
	}
	for (;count;) {
		printf("%8s%8s%8s%8s%8s%8s\n", 
			"usr", "ctx", "rgn", "seg", "pag", "par");
		for (i = 0; i < 20; i--) {
			/* reread flush statistics */
			kvm_read(kd, (long)nl[X_FLUSHSUM].n_value,
					current, sizeof (struct flushmeter));
			/* print new numbers */
			printf("%8d%8d%8d%8d%8d%8d\n",
				current->f_usr - last->f_usr,
				current->f_ctx - last->f_ctx,
				current->f_region - last->f_region,
				current->f_segment - last->f_segment,
				current->f_page - last->f_page,
				current->f_partial- last->f_partial);
			last->f_usr = current->f_usr;
			last->f_ctx = current->f_ctx;
			last->f_region = current->f_region;
			last->f_segment = current->f_segment;
			last->f_page = current->f_page;
			last->f_partial = current->f_partial;
			sleep(interval);
			if (forever) continue;
			if (--count) continue;
			else okexit();
		}
	}
}

no_vac_warning()
{
	printf("warning: cache appears to be off or nonexistant\n");
}

#endif sun

#define steal(where, var) \
	kvm_read(kd, where, &var, sizeof var);
/*
 * Read the drive names out of kmem.
 */
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
		fprintf(stderr, "vmstat: Disk init info not in namelist\n");
		kvm_close(kd);
		exit(1);
	}
	if (mp) for (;;) {
		steal(mp++, mdev);
		if (mdev.mi_driver == 0)
			break;
		if (mdev.mi_dk < 0 || mdev.mi_alive == 0)
			continue;
		steal(mdev.mi_driver, mdrv);
		steal(mdrv.md_dname, two_char);
		sprintf(dr_name[mdev.mi_dk], "%c%c%d",
		    cp[0], cp[1], mdev.mi_unit);
	}
	for (;;) {
		steal(up++, udev);
		if (udev.ui_driver == 0)
			break;
		if (udev.ui_dk < 0 || udev.ui_alive == 0)
			continue;
		steal(udev.ui_driver, udrv);
		steal(udrv.ud_dname, two_char);
		sprintf(dr_name[udev.ui_dk], "%c%c%d",
		    cp[0], cp[1], udev.ui_unit);
	}
}
#endif

#ifdef sun

#include <sundev/mbvar.h>

read_names()
{
	struct mb_device mdev;
	register struct mb_device *mp;
	struct mb_driver mdrv;
	short two_char;
	char *cp = (char *) &two_char;

	mp = (struct mb_device *) nl[X_MBDINIT].n_value;
	if (mp == 0) {
		read_devinfo_names();
	} else for (;;) {
		steal(mp++, mdev);
		if (mdev.md_driver == 0)
			break;
		if (mdev.md_dk < 0 || mdev.md_alive == 0)
			continue;
		steal(mdev.md_driver, mdrv);
		steal(mdrv.mdr_dname, two_char);
		sprintf(dr_name[mdev.md_dk], "%c%c%d", cp[0], cp[1], mdev.md_unit);
	}
}

read_devinfo_names()
{
	register i;
	struct dk_ivec dk_ivec[DK_NDRIVE], *dk_ivp;
	char cp[2];

	dk_ivp = (struct dk_ivec *) nl[X_DK_IVEC].n_value;
	if (dk_ivp == 0) {
		fprintf(stderr, "vmstat: Disk init info not in namelist\n");
		kvm_close(kd);
		exit(1);
	}
	kvm_read (kd, (long) dk_ivp, (caddr_t) dk_ivec,
		DK_NDRIVE * (sizeof (struct dk_ivec)));

	for (dk_ivp = dk_ivec, i = 0; i < DK_NDRIVE; i++, dk_ivp++) {
		if (dk_ivp->dk_name == (char *) 0)
			continue;
		kvm_read (kd, (long) dk_ivp->dk_name, cp, 2);
		sprintf(dr_name[dk_ivp->dk_unit], "%c%c%d", cp[0], cp[1], 
			(long) dk_ivp->dk_unit);
	}
}


#endif sun
