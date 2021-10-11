#ifndef lint
static	char *sccsid = "@(#)kgmon.c 1.1 92/07/30 SMI"; /* from UCB 4.8 83/07/01 */
#endif

#include <sys/param.h>
#include <sys/vm.h>
#include <stdio.h>
#include <nlist.h>
#include <ctype.h>
#include <sys/gprof.h>
#include <kvm.h>
#include <fcntl.h>

#define	PROFILING_ON	0
#define PROFILING_OFF	3

/*
 * froms is actually a bunch of unsigned shorts indexing tos
 */
u_short	*froms;
struct	tostruct *tos;
char	*s_lowpc;
u_long	s_textsize;
int	ssiz;
off_t	sbuf;

struct nlist nl[] = {
#define N_FROMS		0
	{ "_froms" },
#define	N_PROFILING	1
	{ "_profiling" },
#define	N_S_LOWPC	2
	{ "_s_lowpc" },
#define	N_S_TEXTSIZE	3
	{ "_s_textsize" },
#define	N_SBUF		4
	{ "_sbuf" },
#define N_SSIZ		5
	{ "_ssiz" },
#define	N_TOS		6
	{ "_tos" },
	0,
};

char	*system = NULL;
char	*kmemf  = NULL;
kvm_t	*kd;
int	bflag, hflag, rflag, pflag;
int	debug = 0;

main(argc, argv)
	int argc;
	char *argv[];
{
	int mode, disp, openmode = O_RDONLY;
	char *cmdname;

	cmdname = argv[0];
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'b':
			bflag++;
			openmode = O_RDWR;
			break;
		case 'h':
			hflag++;
			openmode = O_RDWR;
			break;
		case 'r':
			rflag++;
			openmode = O_RDWR;
			break;
		case 'p':
			pflag++;
			openmode = O_RDWR;
			break;
		default:
			printf("Usage: kgmon [ -b -h -r -p system memory ]\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0) {
		system = *argv;
		argv++, argc--;
	}
	if (argc > 0)
		kmemf = *argv;

	kd = kvm_open(system, kmemf, NULL, openmode, NULL);
	if (kd == NULL) {
		openmode = O_RDONLY;
		kd = kvm_open(system, kmemf, NULL, openmode, cmdname);
		if (kd == NULL)
			exit(3);
		fprintf(stderr, "%s opened read-only\n", kmemf);
		if (rflag)
			fprintf(stderr, "-r supressed\n");
		if (bflag)
			fprintf(stderr, "-b supressed\n");
		if (hflag)
			fprintf(stderr, "-h supressed\n");
		rflag = 0;
		bflag = 0;
		hflag = 0;
	}
	if (kvm_nlist(kd, nl) < 0) {
		fprintf(stderr, "%s: no namelist\n", 
			system == NULL ? "/vmunix" : system);
		exit(2);
	}
	mode = kfetch(N_PROFILING);
	if (hflag)
		disp = PROFILING_OFF;
	else if (bflag)
		disp = PROFILING_ON;
	else
		disp = mode;
	if (pflag) {
		if (openmode == O_RDONLY && mode == PROFILING_ON)
			fprintf(stderr, "data may be inconsistent\n");
		dumpstate();
	}
	if (rflag)
		resetstate();
	turnonoff(disp);
	printf("kernel profiling is %s.\n", disp ? "off" : "running");
	exit(0);
	/* NOTREACHED */
}

dumpstate()
{
	int i;
	int fd;
	off_t kfroms, ktos;
	int fromindex, endfrom, fromssize, tossize;
	u_long frompc;
	int toindex;
	struct rawarc rawarc;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
	fd = creat("gmon.out", 0666);
	if (fd < 0) {
		perror("gmon.out");
		return;
	}
	ssiz = kfetch(N_SSIZ);
	sbuf = kfetch(N_SBUF);
	for (i = ssiz ; i > 0 ; i -= BUFSIZ, sbuf += BUFSIZ) {
		kvm_read(kd, (off_t)sbuf, buf, MIN(i, BUFSIZ));
		write(fd, buf, MIN(i, BUFSIZ));
	}
	s_textsize = kfetch(N_S_TEXTSIZE);
	fromssize = s_textsize / HASHFRACTION;
	froms = (u_short *)malloc(fromssize);
	kfroms = kfetch(N_FROMS);
	i = kvm_read(kd, kfroms, (char *)froms, fromssize);
	if (i != fromssize) {
		fprintf(stderr, "read froms: request %d, got %d", fromssize, i);
		perror("");
		exit(5);
	}
	tossize = (s_textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	tos = (struct tostruct *)malloc(tossize);
	ktos = kfetch(N_TOS);
	i = kvm_read(kd, ktos, (char *)tos, tossize);
	if (i != tossize) {
		fprintf(stderr, "read tos: request %d, got %d", tossize, i);
		perror("");
		exit(6);
	}
	s_lowpc = (char *)kfetch(N_S_LOWPC);
	if (debug)
		fprintf(stderr, "s_lowpc 0x%x, s_textsize 0x%x\n",
		    s_lowpc, s_textsize);
	endfrom = fromssize / sizeof(*froms);
	for (fromindex = 0; fromindex < endfrom; fromindex++) {
		if (froms[fromindex] == 0)
			continue;
		frompc = (u_long)s_lowpc +
		    (fromindex * HASHFRACTION * sizeof(*froms));
		for (toindex = froms[fromindex]; toindex != 0;
		   toindex = tos[toindex].link) {
			if (debug)
			    fprintf(stderr,
			    "[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			    frompc, tos[toindex].selfpc, tos[toindex].count);
			rawarc.raw_frompc = frompc;
			rawarc.raw_selfpc = (u_long)tos[toindex].selfpc;
			rawarc.raw_count = tos[toindex].count;
			write(fd, &rawarc, sizeof (rawarc));
		}
	}
	close(fd);
}

resetstate()
{
	int i;
	off_t kfroms, ktos;
	int fromssize, tossize;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
	bzero(buf, BUFSIZ);
	ssiz = kfetch(N_SSIZ);
	sbuf = kfetch(N_SBUF);
	ssiz -= sizeof(struct phdr);
	sbuf += sizeof(struct phdr);
	for (i = ssiz; i > 0; i -= BUFSIZ, sbuf += BUFSIZ)
		if (kvm_write(kd, sbuf, buf, MIN(i, BUFSIZ)) < 0) {
			perror("sbuf write");
			exit(7);
		}
	s_textsize = kfetch(N_S_TEXTSIZE);
	fromssize = s_textsize / HASHFRACTION;
	kfroms = kfetch(N_FROMS);
	for (i = fromssize; i > 0; i -= BUFSIZ, kfroms += BUFSIZ)
		if (kvm_write(kd, kfroms, buf, MIN(i, BUFSIZ)) < 0) {
			perror("kfroms write");
			exit(8);
		}
	tossize = (s_textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	ktos = kfetch(N_TOS);
	for (i = tossize; i > 0; i -= BUFSIZ, ktos += BUFSIZ)
		if (kvm_write(kd, ktos, buf, MIN(i, BUFSIZ)) < 0) {
			perror("ktos write");
			exit(9);
		}
}

turnonoff(onoff)
	int onoff;
{
	off_t off;

	if ((off = nl[N_PROFILING].n_value) == 0) {
		printf("profiling: not defined in kernel\n");
		exit(10);
	}
	kvm_write(kd, off, (char *)&onoff, sizeof (onoff));
}

kfetch(index)
	int index;
{
	off_t off;
	int value;

	if (nl[index].n_type == 0) {
		printf("kgmon: %s: not defined in kernel\n", nl[index].n_name);
		exit(11);
	}
	off = (off_t)nl[index].n_value;
	if (kvm_read(kd, off, (char *)&value, sizeof (value)) != sizeof (value)) {
		fprintf(stderr, "kgmon: kernel read error\n");
		exit(13);
	}
	return (value);
}
