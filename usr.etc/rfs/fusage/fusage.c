/*	@(#)fusage.c 1.1 92/07/30 SMI					*/

/*	Copyright (c) 1987 Sun Microsystems			*/
/*	ported from System V.3.1				*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fusage:fusage.c	1.15.3.1"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <mntent.h>
#include <rfs/rfs_misc.h>
#include <rfs/nserve.h>
#include <rfs/rfs_mnt.h>
#include <ctype.h>
#include <rfs/fumount.h>
#include <rfs/rfsys.h>
#include <errno.h>

#define ADVTAB "/etc/advtab"

char *malloc();

#ifndef SZ_PATH
#define SZ_PATH 128
#endif
struct advlst {
	char resrc[SZ_RES+1];
	char dir[SZ_PATH+1];
} *advlst;

/* The block size in RFS is 1K instead of 8K. */
#ifndef RFS_BKSIZE
#define RFS_BKSIZE	1024
#endif RFS_BKSIZE

struct clnts *client;
struct mnttab *mnttab;
extern int errno;

main(argc, argv)
	int argc;
	char *argv[];
{
	char str[SZ_RES+SZ_MACH+SZ_PATH+20]; /* hold strings from adv table */
	int nadv, clntsum, advsum, local, prtflg, fswant, i, j, k;
	int exitcode = 0;
	FILE *atab, *mntfilep;
	struct mntent *mntentp;
	struct stat stbuf;
	char myname[32];	/* Host names are limited to 31 chars. */

	for (i = 0; i < argc; i++)
		if (argv[i][0] == '-') {
			fprintf(stderr, "Usage: %s [mounted file system]\n", argv[0]);
			fprintf(stderr, "          [advertised resource]\n");
			fprintf(stderr, "          [mounted block special device]\n");

			exit(1);
		}
	/* command fails when RFS not installed in system */
	if ((rfsys(RF_RUNSTATE) == -1) && errno == RFS_ENOPKG) {
		perror("fusage");
		exit(1);
	}
	gethostname(myname, 31);
	myname[31] = '\0';	/* in case name is longer than 31 chars */
	printf("\nFILE USAGE REPORT FOR %.8s\n\n", myname);

	/* load resource and directory names from /etc/advtab */
	if (stat(ADVTAB, &stbuf) == -1) {
		nadv = 0;
		goto noadv;
	}
	if ((atab = fopen(ADVTAB, "r")) == 0) {
		fprintf(stderr, "fusage: cannot open %s", ADVTAB);
		perror("fopen");
		exit(1);
	}
	nadv = 0;	/* count lines in advtab */
	while (getline(str, sizeof(str), atab) != 0) 
		nadv++;
	if ((nadv != 0) &&
	    ((advlst = (struct advlst *)malloc(nadv * sizeof(struct advlst))) 
	    == 0)) {
		fprintf(stderr, "fusage: cannot get memory for advtab\n");
		exit(1);
	}
	freopen(ADVTAB, "r", atab);	/* rewind */
	/*
	 * load advlst from advtab.
	 * We are not going to store the complete line for each entry
	 * in /etc/advtab.  Only the resource name and the path name
	 * are important here (client names are unnecessary).
	 */
	i = 0;
	while ((getline(str, sizeof(str), atab) != 0) && (i < nadv)) {
		loadadvl(str, &advlst[i++]);
	}
	if (nlload()) 
		exit(1);

	/*
	 * Get mount information by calling getmntent until if returns NULL.
	 * If the mount is a local file system, find advertised
	 * resources that are within it.  Collect and print data 
	 * on each remote, then print the info for the file system.
	 * Even though we may be requested to print a subset of the
	 * mounted resources, we must always execute all of the loop 
	 * in order to get data for items that are printed.
	 */
noadv:
	mntfilep = setmntent(MOUNTED, "r");
	while (mntentp = getmntent(mntfilep)) {
		if (strcmp(mntentp->mnt_type, MNTTYPE_42) != 0)
			continue;
		if (wantprint(argc, argv, mntentp->mnt_fsname,
		    mntentp->mnt_dir)) {
			printf("\n\t%-15s      %s\n", mntentp->mnt_fsname, 
			    mntentp->mnt_dir);
			fswant++;
		} else
			fswant = 0;
		advsum = 0;
		if (stat(mntentp->mnt_dir, &stbuf) < 0)
			perror("stat");
		for (j = 0; j < nadv; j++) {
			if (isinfs(mntentp, advlst[j].dir)) {
				prtflg = wantprint(argc, argv, advlst[j].resrc, 
								advlst[j].dir);
				if (prtflg)
					printf("\n\t%15s", advlst[j].resrc);

					/* get client list */
				switch(getnodes(advlst[j].resrc, 0)) {
				case 1:
					if (prtflg)
						printf(" (%s) not in kernel advertise table\n",
							advlst[j].dir);
					continue;
					break;
				case 2:
					if (prtflg)
						printf(
						" (%s) ...bad data\n",
							advlst[j].dir);
					continue;
					break;
				case 3:
					if (prtflg)
						printf(
						" (%s) ...no clients\n",
							advlst[j].dir);
					continue;
				}
				if (prtflg)
					printf("      %s\n", advlst[j].dir);
				clntsum = 0;
				for (k = 0; client[k].flags != EMPTY; k++) {
					if (prtflg)
						prdat(client[k].node, 
						    client[k].bcount *
						    RFS_BKSIZE);
					clntsum += client[k].bcount;
				}
				if (prtflg)
					prdat("Sub Total",
					    clntsum * RFS_BKSIZE);
				advsum += clntsum;
			}
		}
		if (fswant){
			printf("\n\t%15s      %s\n", "",  mntentp->mnt_dir);
			if ((local = getcount(mntentp->mnt_dir)) != -1)
				prdat(myname, local * stbuf.st_blksize);
		}
		if (!wantprint(argc, argv, mntentp->mnt_fsname,
		    mntentp->mnt_dir))
			continue;
		if (advsum > 0) {
			prdat("Clients", advsum * RFS_BKSIZE);
			prdat("TOTAL",
			    local * stbuf.st_blksize + advsum * RFS_BKSIZE);
		}
	}
	endmntent(mntfilep);	/* close MOUNTED */
	for (i = 1; i < argc; i++)
		if (argv[i][0] != '\0') {
			exitcode = 2;
			printf("'%s' not found\n", argv[i]);
		}
	exit(exitcode);
	/* NOTREACHED */
}

#define EQ(X, Y) !strcmp(X, Y)

/*
 * wantprint returns nonzero if either r1 or r2 is requested to be
 * printed in argv[].  It also returns nonzero if argc == 1
 * (no argument means to print everything).
 */
wantprint(argc, argv, r1, r2)
	int argc;
	char *argv[], *r1, *r2;
{
	int	found, i;

	found = 0;
	if (argc == 1)
		return (1);	/* the default is "print everything" */
	for (i = 0; i < argc; i++)
		if (EQ(r1, argv[i]) || EQ(r2, argv[i])) {
			argv[i][0] = '\0';	/* done with this arg */
			found++;		/* continue scan to find */
		}				/* duplicate requests */
	return (found);
}

/*
 * prdat print string s followed by number of kilobytes in nbyte.
 */
prdat(s, nbyte)
	char *s;
	long nbyte;
{
	printf("\t\t\t%15s %10d KB\n", s, nbyte / 1024);
}

/*
 * isinfs returns 1 if advdir is in the file system mntentp.
 */
isinfs(mntentp, advdir)
	struct mntent *mntentp;
	char *advdir;
{
			   
	struct stat mpstat, advstat;

	stat(mntentp->mnt_dir, &mpstat);
	stat(advdir, &advstat);
	if (advstat.st_dev == mpstat.st_dev)
		return (1);
	return (0);
}

/*
 * getline reads up to 'len' characters from the file fp (/etc/advtab)
 * and toss remaining characters up to a newline.  If the last line
 * is not terminated with a '\n', the funcion returns failure (0)
 * even if some data was read.  Since the adv(1M) command always
 * terminates the lines with a '\n', this is not a problem.
 */
getline(str, len, fp)
	char *str;
	int len;
	FILE *fp;
{
	int i, c;
	char *s;

	s = str; 
	i = 1;
	for (;;) {
		c = getc(fp);
		switch (c) {
		case EOF:
			*s = '\0';
			return (0);
		case '\n':
			*s = '\0';
			return (1);
		default:
			if (i < len)
				*s++ = c;
			i++;
		}
	}
}

/*
 * loadadvl loads resource and directory fields from string s to
 * (struct advlst *)advx.
 */
loadadvl(s, advx)
	char *s;
	struct advlst *advx;
{
	int i;

	i = 0;
	while (isspace(*s)) s++;
	while (!isspace(*s) && (i < SZ_RES)) advx->resrc[i++] = *s++;
	advx->resrc[i] = '\0';

	i = 0;
	while (isspace(*s)) s++;
	while (!isspace(*s) && (i < SZ_MACH+SZ_PATH+1)) advx->dir[i++] = *s++;
	advx->dir[i] = '\0';
}

