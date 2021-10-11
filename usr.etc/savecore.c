/*
 * Copyright (c) 1980,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980,1986 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)savecore.c 1.1 7/30/92 SMI"; /* from UCB 5.8 5/26/86 */
#endif not lint

/*
 * savecore
 */

#include <stdio.h>
#include <nlist.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/dumphdr.h>
#include <sys/param.h>
#include <kvm.h>
#ifdef COMPAT
#include <setjmp.h>		/* XXX Compatibility */
#include <errno.h>		/* XXX Compatibility */
#endif /* COMPAT */

#define	DAY	(60L*60L*24L)
#define	LEEWAY	(3*DAY)

#define eq(a,b) (!strcmp(a,b))

struct nlist current_nl[] = {	/* namelist for currently running system */
#define X_VERSION	0
	{ "_version" },
	{ "" },
};

char	*system;
char	*dirname;			/* directory to save dumps in */
char	*ddname = "/dev/dump";		/* name of dump device */
#ifdef COMPAT
char	*oldsavecore = "/usr/etc/savecore.old";	/* XXX compatibility */
#endif /* COMPAT */
char	*find_dev();
int	dumpsize;			/* amount of memory dumped */
time_t	now;				/* current date */
char 	*progname;			/* argv[0] */
char 	**args;				/* saved argv XXX */
char	*path();
char    *malloc();
char	*ctime();
time_t	time();
#define	VERS_SIZE	160
char	vers[VERS_SIZE];
int	Verbose;
kvm_t	*Kvm_open();
void	Kvm_nlist();
void	Kvm_read();
void	Kvm_write();
extern	int errno;
int	dfd;				/* dump file descriptor */
struct dumphdr	*dhp;			/* pointer to dump header */
int	pagesize;			/* dump pagesize */
#ifdef COMPAT
jmp_buf	tryold;				/* XXX compatibility */
#endif /* COMPAT */
int	fflag;

main(argc, argv)
	char **argv;
	int argc;
{
	char *cp;

	progname = argv[0];
	args = argv;			/* save args in case we're not it */
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'v':
			Verbose++;
			break;

		case 'f':	/* Undocumented! */
			if (argc < 2) {
				(void) fprintf(stderr,
					"-f requires dumpfile name");
				exit(1);
			}
			argc--, argv++;
			ddname = argv[0];
			fflag++;
			break;

		default:
		usage:
			(void) fprintf(stderr,
			    "usage: savecore [-v] dirname [ system ]\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc != 1 && argc != 2)
		goto usage;
#ifdef COMPAT
	if (setjmp(tryold)) {		/* XXX compatibility */
		args[0] = oldsavecore;
		if (Verbose)
			(void) fprintf(stderr,
				"savecore: not new-style, trying old-style\n");
		execv(oldsavecore, args);
		if (Verbose)
			perror(oldsavecore);
		exit(1);
	}
#endif /* COMPAT */
	dirname = argv[0];
	if (argc == 2)
		system = argv[1];
	openlog("savecore", LOG_ODELAY, LOG_AUTH);
	if (access(dirname, W_OK) < 0) {
		int oerrno = errno;

		perror(dirname);
		errno = oerrno;
		syslog(LOG_ERR, "%s: %m", dirname);
		exit(1);
	}
	if (!read_dumphdr()) {
		if (Verbose)
			(void)fprintf(stderr, "savecore: No dump exists.\n");
		exit(0);
	}
	read_kmem();
	(void) time(&now);
	check_kmem();
	if (*dump_panicstring(dhp))
		syslog(LOG_CRIT, "reboot after panic: %s",
			dump_panicstring(dhp));
	else
		syslog(LOG_CRIT, "reboot");
	if (!get_crashtime() || !check_space())
		exit(1);
	if (save_core())
		clear_dump();
	close(dfd);
	exit(0);
}

read_dumphdr()
{
	struct dumphdr	dumphdr1;
	struct dumphdr	*dhp1 = &dumphdr1;

	pagesize = getpagesize();		/* first guess */

	dfd = Open(ddname, fflag ? O_RDONLY : O_RDWR);	/* dump */
	Lseek(dfd, -pagesize, L_XTND);
	Read(dfd, dhp1, sizeof (*dhp1));	/* read the header */
	dhp = dhp1;				/* temporarily */

	/* Check that this is a valid header */
	if (!good_dumphdr(dhp1))
		goto bad;

	/*
	 * So far, so good.  Reposition file, read real header, and
	 * compare.
	 */
	pagesize = dhp1->dump_pagesize;		/* get dump pagesize */
	dhp = (struct dumphdr *)malloc(dhp1->dump_headersize);
	if (dhp == NULL) {
		(void) fprintf(stderr,
			"savecore: Can't allocate dumphdr buffer.\n");
		return (0);
	}
	Lseek(dfd, -(dhp1->dump_dumpsize), L_XTND);
	Read(dfd, dhp, dhp1->dump_headersize);
	if (bcmp(dhp, dhp1, sizeof(*dhp)))
		goto bad;

	if (good_dumphdr(dhp))
		return (1);

bad:
#ifdef COMPAT
	/* Compatibility: if not new-style try oldstyle */
	if (dhp->dump_magic != DUMP_MAGIC) {
		longjmp(tryold, 1);
		/*NOTREACHED*/
	}
#endif /* COMPAT */

	/* good_dumphdr will have said what the problem is */
	return (0);
}

/*
 * Check for a valid header:
 * 1. Valid magic number
 * 2. A version number we understand.
 * 3. "dump valid flag" on
 */
good_dumphdr(dhp)
	struct dumphdr	*dhp;
{

	if (dhp->dump_magic != DUMP_MAGIC) {
		if (Verbose)
			printf("magic number mismatch: 0x%x != 0x%x\n",
				dhp->dump_magic, DUMP_MAGIC);
		return (0);
	}

	if (dhp->dump_version > DUMP_VERSION) {
		printf(
"Warning: savecore version (%d) is older than dumpsys version (%d)\n",
			DUMP_VERSION, dhp->dump_version);
	}

	if (dhp->dump_pagesize != pagesize) {
		printf("Warning: dump pagesize (%d) != system pagesize (%d)\n",
			dhp->dump_pagesize, pagesize);
	}


	if ((dhp->dump_flags & DF_VALID) == 0) {
		if (Verbose)
			printf("dump already processed by savecore.\n");
		return (0);
	}

	/* no fatal errors */
	return (1);
}

int	cursyms[] =
    { X_VERSION, -1 };

read_kmem()
{
	int i;
	kvm_t	*lkd;

	lkd = Kvm_open(NULL,   NULL,   NULL, O_RDONLY); /* live */
	Kvm_nlist(lkd, current_nl);

	/*
	 * Check that all the names we need are there
	 */
	for (i = 0; cursyms[i] != -1; i++)
		if (current_nl[cursyms[i]].n_value == 0) {
			(void) fprintf(stderr, "/vmunix: %s not in namelist",
			    current_nl[cursyms[i]].n_name);
			syslog(LOG_ERR, "/vmunix: %s not in namelist",
			    current_nl[cursyms[i]].n_name);
			exit(1);
		}
	if (system == NULL) {
		Kvm_read(lkd, current_nl[X_VERSION].n_value, vers, sizeof (vers));
		nullterm(vers, sizeof (vers));
	}
	(void)kvm_close(lkd);
}

nullterm(s, len)
	register char *s;
	int len;
{

	s[len - 1] = '\0';
	while (*s++ != '\n') {
		if (*s == '\0')
			return;
	}
	*s = '\0';
}

check_kmem()
{

	nullterm(dump_versionstr(dhp), strlen(dump_versionstr(dhp)) + 1);
	if (!eq(vers, dump_versionstr(dhp)) && system == 0) {
		(void) fprintf(stderr,
		   "Warning: vmunix version mismatch:\n\t%sand\n\t%s",
		   vers, dump_versionstr(dhp));
		syslog(LOG_WARNING,
		   "Warning: vmunix version mismatch: %s and %s",
		   vers, dump_versionstr(dhp));
	}
}

get_crashtime()
{

	if (dhp->dump_crashtime.tv_sec == 0) {
		if (Verbose)
			(void) printf("Dump time not found.\n");
		return (0);
	}
	(void) printf("System went down at %s",
			ctime(&dhp->dump_crashtime.tv_sec));
	if (dhp->dump_crashtime.tv_sec < now - LEEWAY ||
	    dhp->dump_crashtime.tv_sec > now + LEEWAY) {
		(void) printf("dump time is unreasonable\n");
		return (0);
	}
	return (1);
}

char *
path(file)
	char *file;
{
	register char *cp = malloc((u_int)(strlen(file) + strlen(dirname) + 2));

	(void) strcpy(cp, dirname);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

check_space()
{
	struct statfs fsb;
	int spacefree;

	if (statfs(dirname, &fsb) < 0) {
		int oerrno = errno;

		perror(dirname);
		errno = oerrno;
		syslog(LOG_ERR, "%s: %m", dirname);
		exit(1);
	}
	spacefree = fsb.f_bavail * fsb.f_bsize / 1024;
	if (spacefree < read_number("minfree")) {
		(void) fprintf(stderr,
		   "savecore: Dump omitted, not enough space on device\n");
		syslog(LOG_WARNING,
		    "Dump omitted, not enough space on device");
		return (0);
	}
	if (fsb.f_bavail * fsb.f_bsize <
	    dhp->dump_nchunks * dhp->dump_chunksize * dhp->dump_pagesize) {
		(void) fprintf(stderr,
		    "savecore: Dump will be saved, but free space threshold will be crossed\n");
		syslog(LOG_WARNING,
		    "Dump will be saved, but free space threshold will be crossed");
	}
	return (1);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	fp = fopen(path(fn), "r");
	if (fp == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		(void) fclose(fp);
		return (0);
	}
	(void) fclose(fp);
	return (atoi(lin));
}

save_core()
{
	register char *cp;
	register int ifd, ofd, bounds;
	register FILE *fp;
	int saved = 0;
	int	offset;
	int	nchunks;
	register int	i, n;
	int	bitmapsize;

	cp = malloc((u_int)pagesize);
	if (cp == 0) {
		(void) fprintf(stderr,
			"savecore: Can't allocate i/o buffer.\n");
		return (0);
	}
	bounds = read_number("bounds");
	ifd = Open(system ? system : "/vmunix", O_RDONLY);
	(void) sprintf(cp, "vmunix.%d", bounds);
	ofd = Create(path(cp), 0644);
	while((n = Read(ifd, cp, pagesize)) > 0)
		Write(ofd, cp, n);
	(void) close(ifd);
	Fsync(ofd);
	(void) close(ofd);

	/*
	 * Calculate some constants
	 */
	nchunks = dhp->dump_nchunks;
	dumpsize = nchunks * dhp->dump_chunksize;
	bitmapsize = howmany(dhp->dump_bitmapsize, pagesize);

	/*
	 * We'll save everything, including the bit map
	 */
	ifd = dfd;
	Lseek(ifd, -(dhp->dump_dumpsize), L_XTND);

	(void)sprintf(cp, "vmcore.%d", bounds);
	ofd = Create(path(cp), 0644);
	(void)printf("Saving %d pages of image in vmcore.%d\n",
			dumpsize, bounds);
	syslog(LOG_NOTICE, "Saving %d pages of image in vmcore.%d\n",
		dumpsize, bounds);

	/* Save the first header */
	if (!save_a_page(ifd, ofd, cp))
		goto error;

	/* Save the bitmap */
	for ( ; bitmapsize > 0 ; bitmapsize--) {
		if (!save_a_page(ifd, ofd, cp))
			break;
	}

	/* Save the chunks */
	for ( ; dumpsize > 0 ; dumpsize--) {
		if (!save_a_page(ifd, ofd, cp))
			break;
		if ((++saved & 07) == 0) {/* every 8 pages */
			(void) printf("%6d pages saved\r",
					saved);
			(void) fflush(stdout);
		}
	}

	/* Save the last header */
	(void) save_a_page(ifd, ofd, cp);

error:
	(void) printf("%6d pages saved.\n", saved);

	Fsync(ofd);
	(void) close(ofd);
	free(cp);

#ifdef SAVESWAP
	/*
	 * Now save the swapped out uareas into vmswap.N
	 */
	save_swap(bounds);
#endif SAVESWAP

	fp = fopen(path("bounds"), "w");
	if (fp == NULL)
		perror("savecore: cannot update \"bounds\" file");
	else {
		(void) fprintf(fp, "%d\n", bounds+1);
		(void) fclose(fp);
	}

	/* success! */
	return (1);
}

save_a_page(ifd, ofd, cp)
	register char *cp;
	register int ifd, ofd;
{
	register int	n;

	n = Read(ifd, cp, pagesize);
	if (n == 0) {
		syslog(LOG_WARNING, "WARNING: vmcore may be incomplete\n");
		(void) printf( "WARNING: vmcore may be incomplete\n");
		return (0);
	}
	if (n <= 0) {
		syslog(LOG_WARNING,
		    "WARNING: Dump area was too small - %d pages not saved",
			dumpsize);
		(void) printf(
		    "WARNING: Dump area was too small - %d pages not saved\n",
			dumpsize);
		return (0);
	}
	Write(ofd, cp, n);
	return (1);
}

#ifdef SAVESWAP
#include <sys/proc.h>
#include <sys/user.h>
#include <vm/anon.h>
#include <machine/seg_u.h>

save_swap(bounds)
	int	bounds;
{
	int	ifd, ofd;
	char	*cp;
	char	*vmunix, *vmcore, *vmswap;
	kvm_t	*kd;
	struct proc	*proc;
	int	nuptes;		/* number of ptes to map uarea */

	nuptes = roundup(sizeof (struct user), pagesize) / pagesize;

	cp = malloc(pagesize);		/* get a buffer to work with */
	(void) sprintf(cp, "vmunix.%d", bounds);
	vmunix = path(cp);
	(void) sprintf(cp, "vmcore.%d", bounds);
	vmcore = path(cp);
	(void) sprintf(cp, "vmswap.%d", bounds);
	vmswap = path(cp);

	ifd = Open("/dev/drum", O_RDONLY);
	ofd = Create(vmswap, 0644);
	kd = Kvm_open(vmunix, vmcore, NULL, O_RDONLY);

	/*
	 * For each proc that is swapped out,
	 * copy its user pages from /dev/drum to vmswap.N.
	 * Note that this code is liberally borrowed from kvmgetu.c in
	 * libkvm, and even uses an internal libkvm function!
	 */
	/*XXX This is all wrong for 4.1; needs to be changed XXX*/
	for (kvm_setproc(kd); proc = kvm_nextproc(kd); /*void*/) {
		struct segu_data sud;
		struct anon **app = &sud.su_swaddr[0];
		int cc;

		/* Skip loaded processes */
		if ((proc->p_flag & SLOAD) != 0)
			continue;

		/*
		 * u-area information lives in the segu data structure
		 * pointed to by proc->p_useg.  Obtain the contents of
		 * this structure before worrying about whether or not
		 * the u-area is swapped out.
		 */
		if (kvm_read(kd, (u_long)proc->p_useg, (char *)&sud, sizeof sud)
		    != sizeof sud) {
			(void) fprintf(stderr,
			   "savecore: couldn't read seg_u for pid %d\n",
			   proc->p_pid);
			syslog(LOG_WARNING,
			    "couldn't read seg_u for pid %d\n",
			    proc->p_pid);
			continue;
		}

		/*
		 * u-area is swapped out.  proc->p_useg->su_swaddr[i]
		 * contains an anon pointer for the swap space holding
		 * the corresponding u page.  Note that swap space for
		 * a given u-area is not guaranteed to be contiguous.
		 */

		for (cc = 0; cc < nuptes; cc++) {
			long swapoffset;
			addr_t /* really a (struct vnode *) */ vp;
			u_int off;

			/* XXX _anonoffset is an internal libkvm function! */
			swapoffset = _anonoffset(kd, app[cc], &vp, &off);
			if (swapoffset == -1) {
				(void) fprintf(stderr,
				   "savecore: couldn't find uarea for pid %d\n",
				   proc->p_pid);
				syslog(LOG_WARNING,
				    "couldn't find uarea for pid %d\n",
				    proc->p_pid);
				continue;
			}
			lseek(ifd, swapoffset, 0);
			lseek(ofd, swapoffset, 0);
			read(ifd, cp, pagesize);
			write(ofd, cp, pagesize);
		}
	}
	/* End of stolen code */

	(void) close(ifd);
	Fsync(ofd);
	(void) close(ofd);
	free(cp);
	free(vmunix);
	free(vmcore);
	free(vmswap);
	kvm_close(kd);
}
#endif SAVESWAP

clear_dump()
{

	if (fflag)		/* Don't clear if overridden */
		return;
	Lseek(dfd, -pagesize, L_XTND);	/* Seek to last header */
	dhp->dump_flags &= ~DF_VALID;	/* turn off valid bit */
	Write(dfd, dhp, sizeof (*dhp));	/* re-write the header */
}

/*
 * Versions of kvm routines that exit on error.
 */
kvm_t *
Kvm_open(namelist, corefile, swapfile, flag)
	char *namelist, *corefile, *swapfile;
	int flag;
{
	kvm_t *kd;

	kd = kvm_open(namelist, corefile, swapfile, flag,
		Verbose ? progname : NULL);
	if (kd == NULL) {
		if (Verbose)
			(void)fprintf(stderr, "savecore: kvm_open failed\n");
		exit(1);
	}
	return kd;
}

void
Kvm_nlist(kd, nl)
	kvm_t *kd;
	struct nlist nl[];
{
	if (kvm_nlist(kd, nl) < 0) {
		(void) fprintf(stderr, "savecore: no namelist\n");
		syslog(LOG_ERR, "no namelist");
		exit(1);
	}
}

void
Kvm_read(kd, addr, buf, nbytes)
	kvm_t *kd;
	unsigned long addr;
	char *buf;
	unsigned nbytes;
{
	if (kvm_read(kd, addr, buf, nbytes) != nbytes) {
		(void) fprintf(stderr, "savecore: kernel read error\n");
		syslog(LOG_ERR, "kernel read error");
		exit(1);
	}
}

void
Kvm_write(kd, addr, buf, nbytes)
	kvm_t *kd;
	unsigned long addr;
	char *buf;
	unsigned nbytes;
{
	if (kvm_write(kd, addr, buf, nbytes) != nbytes) {
		(void) fprintf(stderr, "savecore: kernel write error\n");
		syslog(LOG_ERR, "kernel write error");
		exit(1);
	}
}

/*
 * Versions of std routines that exit on error.
 */
Open(name, rw)
	char *name;
	int rw;
{
	int fd;

	fd = open(name, rw);
	if (fd < 0) {
		int oerrno = errno;

		(void) fprintf(stderr, "savecore: ");
		errno = oerrno;
		perror(name);
		errno = oerrno;
		syslog(LOG_ERR, "%s: %m", name);
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
	return (fd);
}

Read(fd, buff, size)
	int fd, size;
	char *buff;
{
	int ret;

	ret = read(fd, buff, size);
	if (ret < 0) {
		int oerrno = errno;

#ifdef COMPAT
		if (errno == EINVAL)
			longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		perror("savecore: read");
		errno = oerrno;
		syslog(LOG_ERR, "read: %m");
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
	return (ret);
}

Create(file, mode)
	char *file;
	int mode;
{
	register int fd;

	fd = creat(file, mode);
	if (fd < 0) {
		int oerrno = errno;

		(void) fprintf(stderr, "savecore: ");
		errno = oerrno;
		perror(file);
		errno = oerrno;
		syslog(LOG_ERR, "%s: %m", file);
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
	return (fd);
}

Write(fd, buf, size)
	int fd, size;
	char *buf;
{

	if (write(fd, buf, size) < size) {
		int oerrno = errno;

		perror("savecore: write");
		errno = oerrno;
		syslog(LOG_ERR, "write: %m");
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
}

Fsync(fd)
	int fd;
{
	if (fsync(fd) < 0) {
		int oerrno = errno;

		perror("savecore: write");
		errno = oerrno;
		syslog(LOG_ERR, "write: %m");
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
}

Lseek(fd, offset, whence)
	int	fd, whence;
	off_t	offset;
{
	int ret;

	ret = lseek(fd, offset, whence);
	if (ret == -1) {
		int oerrno = errno;

		perror("savecore: lseek");
		errno = oerrno;
		syslog(LOG_ERR, "lseek: %m");
#ifdef COMPAT
		longjmp(tryold, 1);	/* XXX Compatibility */
#endif /* COMPAT */
		exit(1);
	}
	return (ret);
}
