/*
 * Copyright (c) 1980, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980, 1986, 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ident "@(#)main.c 1.1 92/07/30 SMI"	/* from UCB 5.18 2/1/90 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <mntent.h>
#include <strings.h>
#include "fsck.h"

char	*rawname(), *unrawname(), *blockcheck();
void	catch(), catchquit();
extern	void addfilesys(), print_badlist();
extern	int dopreen();
int	returntosingle;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char *name;
	FILE *fstab;
	struct mntent *mnt;
	int maxrun = 0;

	setlinebuf(stdout);

	/*
	 * Write a single audit record
	 */
	audit_args(AU_ADMIN, argc, argv);
	sync();
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {

		case 'p':
			preen++;
			break;

		case 'b':
			if (argv[0][1] != '\0') {
				bflag = atoi(argv[0]+1);
			} else if (argc > 1) {
				bflag = atoi(*++argv);
				argc--;
			} else
				errexit("-b flag requires a number\n");
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'c':
			cvtflag++;
			break;

		case 'd':
			debug++;
			break;

		case 'f':	/* force checking regardless of "clean" flag */
			fflag++;
			break;

		case 'r':
			break;

		case 'w':	/* check only writable filesystems */
			wflag++;
			break;

		case 'l':
			if (argv[0][1] != '\0') {
				maxrun = atoi(argv[0]+1);
			} else if (argc > 1) {
				maxrun = atoi(*++argv);
				argc--;
			} else
				errexit("-l flag requires a number\n");
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			errexit("%c option?\n", **argv);
		}
	}
	if (nflag && preen && !debug)
		errexit("The -n and -p options are mutually exclusive\n");
	rflag++;  /* check raw devices */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	if (argc) {
		while (argc-- > 0) {
			if (wflag && !writable(*argv))
				argv++;
			else
				checkfilesys(*argv++);
		}
		exit(exitstat);
	}

	if ((fstab = setmntent(MNTTAB, "r")) == NULL)
		errexit("Can't open checklist file: %s\n", MNTTAB);
	while ((mnt = getmntent(fstab)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_42)) {
			continue;
		}
		if (wflag && hasmntopt(mnt, MNTOPT_RO)) {
			continue;
		}
		if (mnt->mnt_passno < 1) {
			continue;
		}
		name = blockcheck(mnt->mnt_fsname);
		if (name == NULL) {
			exitstat |= 8;
			continue;
		}
		if (preen == 0 || (fflag && mnt->mnt_passno == 1))
			checkfilesys(name);
		else
			addfilesys(name, mnt->mnt_fsname, mnt->mnt_dir);
	}
	endmntent(fstab);

	if (preen && exitstat == 0) {
		exitstat = dopreen(maxrun);
		if (exitstat)
			print_badlist();
	}
	if (exitstat)
		exit(exitstat & 4 ? 4 : 8);

	(void)endfsent();
	if (returntosingle)
		exit(2);
	exit(exitstat);
	/* NOTREACHED */
}

checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;

	hotroot = 0;
	mountedfs = 0;
	iscorrupt = 1;
	isconvert = 0;
	if ((devname = setup(filesys)) == 0) {
		if (iscorrupt == 0)
			return;
		exitstat |= 8;
		if (preen)
			errexit("CAN'T CHECK FILE SYSTEM.\n");
		return;
	}
	if (debug)
		printclean();
	iscorrupt = 0;
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		if (mountedfs)
			printf("** Currently Mounted on %s\n", sblock.fs_fsmnt);
		else
			printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	updateclean();
	if (debug)
		printclean();

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	pwarn("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
	if (preen)
		printf("\n");
	pwarn("(%d frags, %d blocks, %.1f%% fragmentation)\n",
	    n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	if (debug &&
	    (n_files -= maxino - ROOTINO - sblock.fs_cstotal.cs_nifree))
		printf("%d files missing\n", n_files);
	if (debug) {
		n_blks += sblock.fs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (n_blks -= maxfsblock - (n_ffree + sblock.fs_frag * n_bfree))
			printf("%d blocks missing\n", n_blks);
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %d,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = muldup = (struct dups *)0;
	inocleanup();
	if (isdirty) {
		if (FSOKAY == (fs_get_state(&sblock) + sblock.fs_time)) {
			(void)time(&sblock.fs_time);
			fs_set_state(&sblock, FSOKAY - sblock.fs_time);
		} else
			(void)time(&sblock.fs_time);
		sbdirty();
	}
	ckfini();
	free(blockmap);
	free(statemap);
	free((char *)lncntp);
	blockmap = statemap = (char *)lncntp = NULL;
	if (iscorrupt)
		exitstat |= 8;
	if (!fsmodified)
		return;
	if (!preen) {
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
		if (mountedfs || hotroot) {
			printf("\n***** REBOOT THE SYSTEM *****\n");
			exit(4);
		}
	}
	if (mountedfs || hotroot) {
		/* doesn't make sense to sync here w/ non-block devices */
		exitstat |= 4;
	}
}

char *
blockcheck(name)
	char *name;
{
	struct stat stblock, stchar;
	char *raw;
	int retried = 0;

retry:
	if (stat(name, &stblock) < 0) {
		printf("Can't stat %s\n", name);
		return (0);
	}
	if ((stblock.st_mode & S_IFMT) == S_IFBLK) {
		raw = rawname(name);
		if (stat(raw, &stchar) < 0) {
			printf("Can't stat %s\n", raw);
			return (0);
		}
		if ((stchar.st_mode & S_IFMT) == S_IFCHR) {
			if (!rflag)
				raw = unrawname(name);
			return (raw);
		} else {
			printf("%s is not a character device\n", raw);
			return (0);
		}
	} else if (((stblock.st_mode & S_IFMT) == S_IFCHR) && !retried) {
		name = unrawname(name);
		retried++;
		goto retry;
	}
	printf("Can't make sense out of name %s\n", name);
	return (0);
}

char *
unrawname(name)
	char *name;
{
	char *dp;
	struct stat stb;

	if ((dp = rindex(name, '/')) == 0)
		return (name);
	if (stat(name, &stb) < 0)
		return (name);
	if ((stb.st_mode & S_IFMT) != S_IFCHR)
		return (name);
	if (*++dp != 'r')
		return (name);
	while (*dp++ != 0)
		dp[-1] = dp[0];
	return (name);
}

char *
rawname(name)
	char *name;
{
	static char rawbuf[MAXPATHLEN];
	char *dp;

	if ((dp = rindex(name, '/')) == 0)
		return (name);
	*dp = 0;
	(void)strcpy(rawbuf, name);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp + 1);
	return (rawbuf);
}
