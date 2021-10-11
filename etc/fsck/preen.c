/*
 * Copyright 1990, by Sun Microsystems, Inc.
 */
#ident	"@(#)preen.c 1.1 92/07/30 SMI"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sun/dkio.h>
#include <ufs/inode.h>
#include "fsck.h"

extern void voidquit();

/*
 * UNITMASK is used to mask off partition bits from device numbers.
 * We assume that NDKMAP is a power of two (viz., 8).
 */
#define	UNITMASK	~(NDKMAP - 1)
/*
 * WORDSIZE is the number of bits in a word, for use in bit-map operations.
 */
#define	WORDSIZE	(NBBY * sizeof (u_int))
/*
 * DEVMAPINCR is the number of entries that we add to devmap[] when it
 * becomes full; more or less arbitrary.
 */
#define	DEVMAPINCR	128

struct filesys {
	struct filesys *next;		/* Link to next record */
	char *fsname;			/* Name of file system mount dir */
	char *rawname;			/* Name of file system raw device */
	char *blockname;		/* Name of file system block device */
	u_int ndev;			/* No. of physical devices */
	dev_t *devlist;			/* List of device numbers */
	u_int mapsize;			/* Size of bit map */
	u_int *map;			/* Bit map of physical devices */
	pid_t pid;			/* pid of child fsck for this fs */
};

static struct filesys *unchecked, *badlist;
static struct filesys **badnext = &badlist;
static struct filesys **checknext = &unchecked;
static dev_t *devmap;
static u_int devmapsize;
static int ndevice;
static int nfilesys;

static void get_devlist(), simple_device(), walkinfotree();
static void build_bitmap(), startfilesys();
static int count_leaves(), lookup_dev(), wait_for_fs();
static dev_t *list_devices();
static struct filesys *get_runnable();

void
addfilesys(rawname, blockname, fsname)
	char *rawname, *blockname, *fsname;
{
	struct filesys *fs;

	fs = (struct filesys *)malloc(sizeof (*fs));
	if (fs == NULL)
		errexit("out of memory in addfilesys\n");
	fs->rawname = strdup(rawname);
	fs->blockname = strdup(blockname);
	fs->fsname = strdup(fsname);
	if (fs->rawname == NULL || fs->blockname == NULL || fs->fsname == NULL)
		errexit("out of memory in addfilesys\n");
	get_devlist(fs);
	build_bitmap(fs);
	*checknext = fs;
	checknext = &fs->next;
	fs->next = NULL;
	nfilesys++;
}

static void
get_devlist(fs)
	struct filesys *fs;
{
	fs->ndev = 0;
	fs->devlist = NULL;
	simple_device(fs);
}

static void
simple_device(fs)
	struct filesys *fs;
{
	struct stat s;

	if (stat(fs->blockname, &s) == -1) {
		printf("Can't stat %s (%s)\n",
		    fs->blockname, fs->fsname);
		perrout(fs->blockname);
		return;
	}
	fs->devlist = (dev_t *) malloc(sizeof (dev_t));
	if (fs->devlist == NULL)
		errexit("out of memory in simple_device\n");
	fs->ndev = 1;
	*fs->devlist = s.st_rdev;
}

static void
build_bitmap(fs)
	struct filesys *fs;
{
	int i, n;
	u_int newsize;

	fs->mapsize = 0;
	for (i = 0; i != fs->ndev; i++) {
		n = lookup_dev(fs->devlist[i]);
		newsize = howmany(n + 1, WORDSIZE);
		if (fs->mapsize < newsize) {
			fs->map = (fs->mapsize == 0) ?
			    (u_int *) malloc(newsize * sizeof (u_int)) :
			    (u_int *) realloc((char *)fs->map,
				newsize * sizeof (u_int));
			if (fs->map == NULL)
				errexit("out of memory in build_bitmap\n");
			bzero((char *)&fs->map[fs->mapsize],
			    (int)((newsize - fs->mapsize) * sizeof (u_int)));
			fs->mapsize = newsize;
		}
		fs->map[n / WORDSIZE] |= (1 << (n % WORDSIZE));
	}
}

static int
lookup_dev(devno)
	dev_t devno;
{
	int i;

	devno &= UNITMASK;
	for (i = 0; i != ndevice; i++) {
		if (devmap[i] == devno)
			return (i);
	}
	if (ndevice == devmapsize) {
		devmapsize += DEVMAPINCR;
		devmap = (devmap == NULL) ?
		    (dev_t *) malloc(devmapsize * sizeof (dev_t)) :
		    (dev_t *) realloc((char *) devmap,
			devmapsize * sizeof (dev_t));
		if (devmap == NULL)
			errexit("out of memory in lookup_dev\n");
	}
	devmap[ndevice] = devno;
	return (ndevice++);
}

int
dopreen(maxrun)
	int maxrun;
{
	int nrun = 0;		/* # of active runs */
	int status = 0;		/* combined exit status */
	u_int *busydev;		/* set of busy devices */
	struct filesys *active;	/* list of active file systems */
	struct filesys *fs;
	int i;

	if (maxrun <= 0)
		maxrun = nfilesys;
	active = NULL;
	busydev = (u_int *)
	    calloc(howmany(devmapsize, WORDSIZE), sizeof (u_int));
	if (busydev == NULL)
		errexit("out of memory in dopreen\n");

	while (unchecked != NULL) {
		while (nrun == maxrun ||
		    (fs = get_runnable(&unchecked, busydev)) == NULL) {
			assert(nrun > 0);
			status |= wait_for_fs(&active, busydev);
			nrun--;
		}
		nrun++;
		fs->next = active;
		active = fs;
		for (i = 0; i != fs->mapsize; i++) {
			assert((busydev[i] & fs->map[i]) == 0);
			busydev[i] |= fs->map[i];
		}
		startfilesys(fs);
	}
	while (nrun-- != 0)
		status |= wait_for_fs(&active, busydev);

	return (status);
}

static struct filesys *
get_runnable(fslist, busy)
	struct filesys **fslist;
	u_int *busy;
{
	struct filesys *last = NULL, *fs;
	int i;

	for (fs = *fslist; fs != NULL; last = fs, fs = fs->next) {
		for (i = 0; i != fs->mapsize; i++) {
			if ((busy[i] & fs->map[i]) != 0)
				break;
		}
		if (i == fs->mapsize)
			break;
	}
	if (fs != NULL) {
		if (last != NULL)
			last->next = fs->next;
		else
			*fslist = fs->next;
	}
	return (fs);
}

static int
wait_for_fs(fslist, busy)
	struct filesys **fslist;
	u_int *busy;
{
	int wstatus, i, pid, xstat;
	struct filesys *last = NULL, *fs;

	pid = wait(&wstatus);
	if (pid == -1) {
		errexit("wait unexpectedly returned -1\n");
	}
	for (fs = *fslist; fs != NULL; last = fs, fs = fs->next) {
		if (fs->pid == pid)
			break;
	}
	if (fs == NULL) {
		errexit("wait returned %d - not found\n", pid);
	}
	if (last != NULL)
		last->next = fs->next;
	else
		*fslist = fs->next;
	for (i = 0; i != fs->mapsize; i++) {
		assert((busy[i] & fs->map[i]) == fs->map[i]);
		busy[i] &= ~fs->map[i];
	}

	if (WIFSIGNALED(wstatus)) {
		printf("%s (%s): EXITED WITH SIGNAL %d\n",
		    fs->rawname, fs->fsname, WTERMSIG(wstatus));
	}
	xstat = WEXITSTATUS(wstatus);
	if (xstat >= 8) {
		fs->next = NULL;
		*badnext = fs;
		badnext = &fs->next;
	}
	if (debug) {
		printf("finished %s (%s); status = %d\n",
		    fs->rawname, fs->fsname, xstat);
	}
	return (xstat);
}

void
print_badlist()
{
	struct filesys *fs;
	int x, len;

	if (badlist == NULL)
		return;

	printf(
"\nTHE FOLLOWING FILE SYSTEM%s HAD AN UNEXPECTED INCONSISTENCY:\n   ",
	    badlist->next != NULL ? "S" : "");
	for (fs = badlist, x = 3; fs != NULL; fs = fs->next) {
		len = strlen(fs->rawname) + strlen(fs->fsname) + 5;
		x += len;
		if (x >= 80) {
			printf("\n    ");
			x = len + 3;
		} else {
			printf(" %s (%s)%s", fs->rawname, fs->fsname,
			fs->next != NULL ? "," : "\n");
		}
	}
}

static void
startfilesys(fs)
	struct filesys *fs;
{
	if (debug) {
		int i;
		printf("checking %s (%s); devlist = ",
		    fs->rawname, fs->fsname);
		for (i = 0; i != fs->ndev; i++)
			printf("%s(%d, %d)", (i != 0) ? ", " : "",
			    major(fs->devlist[i]), minor(fs->devlist[i]));
		putchar('\n');
	}
	fs->pid = fork();
	if (fs->pid == -1) {
		perrout("fork");
		exit(8);
	}
	if (fs->pid == 0) {
		exitstat = 0;
		(void)signal(SIGQUIT, voidquit);
		checkfilesys(fs->rawname);
		exit(exitstat);
	}
}
