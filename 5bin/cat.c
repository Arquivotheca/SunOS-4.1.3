/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)cat.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.14 */
#endif

/*
**	Concatenate files.
*/


#include	<stdio.h>
#include	<ctype.h>
#include	<locale.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mman.h>

#define	IDENTICAL(A,B)	((A).st_dev==(B).st_dev && (A).st_ino==(B).st_ino)
#define ISBLK(A)	(((A).st_mode & S_IFMT) == S_IFBLK)
#define ISCHR(A)	(((A).st_mode & S_IFMT) == S_IFCHR)
#define ISDIR(A)	(((A).st_mode & S_IFMT) == S_IFDIR)
#define ISFIFO(A)	(((A).st_mode & S_IFMT) == S_IFIFO)
#define ISREG(A)	(((A).st_mode & S_IFMT) == S_IFREG)

int	silent = 0;		/* s flag */
int	visi_mode = 0;		/* v flag */
int	visi_tab = 0;		/* t flag */
int	visi_newline = 0;	/* e flag */

int	ibsize;
int	obsize;

main(argc, argv)
int    argc;
char **argv;
{
	register FILE *fi;
	register char *filenm;
	register int c;
	extern	int optind;
	int	errflg = 0;
	int	stdinflg = 0;
	int	status = 0;
	struct stat source, target;

#ifdef STANDALONE
	/*
	 * If the first argument is NULL,
	 * discard arguments until we find cat.
	 */
	if (argv[0][0] == '\0')
		argc = getargv ("cat", &argv, 0);
#else
	(void) setlocale(LC_ALL, "");
#endif

	/*
	 * Process the options for cat.
	 */

	while( (c=getopt(argc,argv,"usvte")) != EOF ) {
		switch(c) {

		case 'u':

			/*
			 * If not standalone, set stdout to
	 		 * completely unbuffered I/O when
			 * the 'u' option is used.
			 */

#ifndef	STANDALONE
			setbuf(stdout, (char *)NULL);
#endif
			continue;

		case 's':
		
			/*
			 * The 's' option requests silent mode
			 * where no messages are written.
			 */

			silent++;
			continue;

		case 'v':
			
			/*
			 * The 'v' option requests that non-printing
			 * characters (with the exception of newlines,
			 * form-feeds, and tabs) be displayed visibly.
			 *
			 * Control characters are printed as "^x".
			 * DEL characters are printed as "^?".
			 * Non-printable  and non-contrlol characters with the
			 * 8th bit set are printed as "M-x".
			 */

			visi_mode++;
			continue;

		case 't':

			/*
			 * When in visi_mode, this option causes tabs
			 * to be displayed as "^I".
			 */

			visi_tab++;
			continue;

		case 'e':

			/*
			 * When in visi_mode, this option causes newlines
			 * and form-feeds to be displayed as "$" at the end
			 * of the line prior to the newline.
			 */

			visi_newline++;
			continue;

		case '?':
			errflg++;
			break;
		}
		break;
	}

	if (errflg) {
		if (!silent)
			fprintf(stderr,"usage: cat -usvte [-|file] ...\n");
		exit(2);
	}

	/*
	 * Stat stdout to be sure it is defined.
	 */

	if(fstat(fileno(stdout), &target) < 0) {
		if(!silent) {
			fprintf(stderr, "cat: cannot stat standard output: ");
			perror("");
		}
		exit(2);
	}
	obsize = target.st_blksize;

	/*
	 * If no arguments given, then use stdin for input.
	 */

	if (optind == argc) {
		argc++;
		stdinflg++;
	}

	/*
	 * Process each remaining argument,
	 * unless there is an error with stdout.
	 */


	for (argv = &argv[optind];
	     optind < argc && !ferror(stdout); optind++, argv++) {
		filenm = *argv;

		/*
		 * If the argument was '-' or there were no files
		 * specified, take the input from stdin.
		 */

		if (stdinflg || (filenm[0]=='-' && filenm[1]=='\0')) {
			filenm = "standard input";
			fi = stdin;
		} else {
			/*
			 * Attempt to open each specified file.
			 */

			if ((fi = fopen(filenm, "r")) == NULL) {
				if (!silent) {
				   fprintf(stderr, "cat: cannot open %s: ",
								filenm);
				   perror("");
				}
				status = 2;
				continue;
			}
		}
		
		/*
		 * Stat source to make sure it is defined.
		 */

		if(fstat(fileno(fi), &source) < 0) {
			if(!silent) {
			   fprintf(stderr, "cat: cannot stat %s: ", filenm);
			   perror("");
			}
			status = 2;
			continue;
		}


		/*
		 * If the source is not a character special file or a
		 * block special file, make sure it is not identical
		 * to the target.
		 */
	
		if (!ISCHR(target)
		 && !ISBLK(target)
		 && IDENTICAL(target, source)) {
			if(!silent)
			   fprintf(stderr, "cat: input/output files '%s' identical\n",
						stdinflg?"-": *argv);
			if (fclose(fi) != 0 ) {
				fprintf(stderr, "cat: close error: ");
				perror("");
			}
			status = 2;
			continue;
		}
		ibsize = source.st_blksize;

		/*
		 * If in visible mode, use vcat; otherwise, use cat.
		 */

		if (visi_mode)
			status = vcat(fi, filenm);
		else
			status = cat(fi, &source, filenm);

		/*
		 * If the input is not stdin, flush stdout.
		 */

		if (fi!=stdin) {
			fflush(stdout);
			
			/* 
			 * Attempt to close the source file.
			 */

			if (fclose(fi) != 0) 
				if (!silent) {
					fprintf(stderr, "cat: close error: ");
					perror("");
				}
		}
	}
	
	/*
	 * When all done, flush stdout to make sure data was written.
	 */

	fflush(stdout);
	
	/*
	 * Display any error with stdout operations.
	 */

	if (ferror(stdout)) {
		if (!silent) {
			fprintf (stderr, "cat: output error: ");
			perror("");
		}
		status = 2;
	}
	exit(status);
	/* NOTREACHED */
}

#define	MAXMAPSIZE	(1024*1024)	/* map at most 1MB */

int
cat(fi, statp, filenm)
	FILE *fi;
	register struct stat *statp;
	char *filenm;
{
	register int nitems;
	register int nwritten;
	register int offset;
	register int fi_desc;
	register int buffsize;
	register char *bufferp;
	int mapsize, munmapsize;
	register off_t filesize;
	register off_t mapoffset;
	extern char *mmap();
	extern char *malloc();

	if (ISREG(*statp)) {
		mapsize = MAXMAPSIZE;
		if (statp->st_size < mapsize)
			mapsize = statp->st_size;
		munmapsize = mapsize;

		/*
		 * Mmap time!
		 */
		bufferp = mmap((caddr_t)NULL, mapsize, PROT_READ, MAP_SHARED,
		    fileno(fi), (off_t)0);
		if (bufferp == (caddr_t)-1)
			mapsize = 0;	/* I guess we can't mmap today */
	} else
		mapsize = 0;		/* can't mmap non-regular files */

	if (mapsize != 0) {
		mapoffset = 0;
		filesize = statp->st_size;
		for (;;) {
			if (write(fileno(stdout), bufferp, mapsize) < 0) {
				if (!silent)
					perror("cat: output error");
				(void) munmap(bufferp, munmapsize);
				return(2);
			}
			filesize -= mapsize;
			if (filesize == 0)
				break;
			mapoffset += mapsize;
			if (filesize < mapsize)
				mapsize = filesize;
			if (mmap(bufferp, mapsize, PROT_READ,
			    MAP_SHARED|MAP_FIXED, fileno(fi),
			    mapoffset) == (caddr_t)-1) {
				if (!silent)
					perror("cat: mmap error");
				(void) munmap(bufferp, munmapsize);
				return(1);
			}
		}
		(void) munmap(bufferp, munmapsize);
	} else {
		if (obsize)
			buffsize = obsize;	/* common case, use output blksize */
		else if (ibsize)
			buffsize = ibsize;
		else
			buffsize = BUFSIZ;

		if ((bufferp = malloc(buffsize)) == NULL) {
			perror("cat: no memory");
			return (1);
		}

		fi_desc = fileno(fi);

		/*
		 * While not end of file, copy blocks to stdout. 
		 */

		while ((nitems=read(fi_desc,bufferp,buffsize))  > 0) {
			offset = 0;
			/*
			 * Note that on some systems (V7), very large writes
			 * to a pipe return less than the requested size of
			 * the write.  In this case, multiple writes are
			 * required.
			 */
			do {
				nwritten = write(1,bufferp+offset,
				    (unsigned)nitems);
				if (nwritten < 0) {
					if (!silent) {
						fprintf(stderr, "cat: output error: ");
						perror("");
					}
					free(bufferp);
					return(2);
				}
				offset += nwritten;
			} while ((nitems -= nwritten) > 0);
		}
		free(bufferp);
		if (nitems < 0) {
			fprintf(stderr, "cat: input error on %s: ", filenm);
			perror("");
		}
	}

	return(0);
}


vcat(fi, filenm)
	FILE *fi;
	char *filenm;
{
	register int c;

	while ((c = getc(fi)) != EOF) 
	{
		if ( (c == '\t') || (c == '\f') ) {
			/*
			 * Display tab as "^I" and formfeed as ^L
			 * if visi_tab set
			 */

			if (! visi_tab)
				putchar(c);
			else {
				putchar('^');
				putchar(c^0100);
			}
		} else if ( c == '\n') {
			/*
			 * Display newlines as "$<newline>"
			 * if visi_newline set
			 */
			if (visi_newline) 
				putchar('$');
			putchar(c);
		} else {
			/*
			 * For non-printable and non-cntrl chars,
			 * use the "M-x" notation.
			 */
			if (c >= 0200 && !isprint(c) && !iscntrl(c)) {
				putchar('M');
				putchar('-');
				c &= 0177;
			}
			if (isprint(c))
				putchar(c);
			else if (c < ' ')
				printf("^%c", c+'@');
			else if (c == 0177)
				printf("^?");
			else
				putchar(c);
		}
	}
	if (ferror(fi)) {
		fprintf(stderr, "cat: input error on %s: ", filenm);
		perror("");
	}
	return(0);
}
