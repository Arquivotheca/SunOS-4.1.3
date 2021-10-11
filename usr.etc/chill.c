#ifndef lint
static  char sccsid[] = "@(#)chill.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

  /*
  **
  **  NOTE: The original version (which allocated virtmem == maxmem
  **  and ran around cycle times) which was designed to flush out all
  **  other pages.  But subsequent paging behavior indicates that not
  **  all pages were flushed.  One failure mode I can see is as
  **  follows.
  **
  **  Let all of main memory be represented as ten page slots,
  **  numbered 0 to 9.  Let the virtual memory pages in order be
  **  labeled a, b, ..., j.  Let the suspected unflushed page be page
  **  0.  If the paging daemon hand were at page 1, and all but page 0
  **  were free, and the free pages were in reverse order, we would
  **  have the following picture after the first 9 accesses, none of
  **  which would cause a page fault:
  **
  **		+ hand
  **		v
  **	    0   1   2   3   4   5   6   7   8   9
  **	    X   i   h   g   f   e   d   c   b   a
  **
  **	On the 10th access, we get
  **
  **		    + hand
  **		    v
  **	    0   1   2   3   4   5   6   7   8   9
  **	    X   j   h   g   f   e   d   c   b   a
  **
  **	We restart the cycle, and cause no faults on a..h.
  **	Access 9 yields
  **
  **			+ hand
  **			v
  **	    0   1   2   3   4   5   6   7   8   9
  **	    X   j   i   g   f   e   d   c   b   a
  **
  **	and access 10 does not cause a page fault.
  **
  **  As you can see, page 0 is still, X, the unknown; it has not been
  **  forced out.
  **
  **  In view of this, the algorithm has been changed.  The new
  **  algorithm is to allocate 2X maxmem, and cycle through it once.
  **
  **  version:	@(#)chill.c 1.1 92/07/30 hacked from page1.c 1.6 87/06/20
  */

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <nlist.h>

char	*rindex();
char	*malloc();
off_t	lseek();

#define	setbasename(target, source) \
do {char *s = source, *t = rindex(s,'/'); target = t ? t + 1 : s;} while (0)

#define streq(s, t)	(strcmp(s, t) == 0)

char *Usage="Usage: %s [-f n] [-c n] [-m n]\n";
char *cmdname;
static int pagesize;
static int cycles = 1;
static int factor = 2;

main(argc, argv)
	int argc;
	char **argv;
{
	char *ptr;
	int i;
	int cycle;
	int memory = 0;
	int number;

	setbasename(cmdname, argv[0]);
	pagesize = getpagesize();

	argc--, argv++;
	while (*argv) {
		if (argv[0][0] == '-' && argv[0][2] == 0 && argc > 1) {
			number = atoi(argv[1]);
			if (number <= 0)
				usage();

			switch (argv[0][1]) {
			case 'c':
				cycles = number;
				break;
			case 'm':
				memory = number;
				break;
			case 'f':
				factor = number;
				break;
			default:
				usage();
				break;
			}
			argc--, argv++;
		} else
			usage();

		argc--, argv++;
	}

	if (memory == 0)
		memory = getmaxmem() * pagesize;

	memory *= factor;

	ptr = malloc((unsigned)memory);
	if (ptr == 0) {
		(void)fprintf(stderr, "%s: can't malloc %d -- exiting\n",
		    cmdname, memory);
		exit(1);
	}

	for (cycle = 0; cycle < cycles; cycle++)
		for (i = 0; i < memory; i+=pagesize)
			ptr[i] = 1;

	_exit(0);
	/*NOTREACHED*/
}

usage()
{
	(void)fprintf(stderr, Usage, cmdname);
	exit(1);
}



static struct nlist nl[] = {
	{"_maxmem" }, {""}, 
};

getmaxmem()
{
	int fd, maxmem;
	
	(void)nlist("/vmunix", nl);
	if (nl[0].n_value == 0) {
		(void)fprintf(stderr, "%s: bad namelist -- exiting\n",
		    cmdname);
		exit(1);
	}
	if ((fd = open("/dev/kmem", 0)) < 0) {
		(void)fprintf(stderr, "%s: open: ", cmdname);
	    	perror("/dev/kmem");
		exit(1);
	}
	if (lseek(fd, (off_t)nl[0].n_value, 0) == -1) {
		(void)fprintf(stderr, "%s: ", cmdname);
		perror("lseek");
		exit(1);
	}
	if (read(fd, (char *)&maxmem, sizeof(maxmem)) != sizeof (maxmem)) {
		(void)fprintf(stderr, "%s: ", cmdname);
		perror("read");
		exit(1);
	}
	return (maxmem);
}
