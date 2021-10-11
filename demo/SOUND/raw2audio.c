#ifndef lint
static	char sccsid[] = "@(#)raw2audio.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_filehdr.h>


#define	Error		(void) fprintf


/* Local variables */
char *prog;
char prog_desc[] = "Convert raw data to audio file format";
char prog_opts[] = "o:s:p:e:c:i:f?";	/* getopt() flags */

char		*Stdin = "stdin";
char		*Stdout = "stdout";
char		*Suffix = ".TMPAUDIO";
unsigned char	buf[1024 * 64];
char		tmpf[MAXPATHLEN];

char		*Info = NULL;		/* pointer to info data */
unsigned	Ilen = 0;		/* length of info data */
int		Force = 0;		/* rewrite file header, if present */
unsigned	Offset = 0;		/* byte offset into raw data */
Audio_hdr	Hdr;			/* audio header structure */
char		*ofile;			/* output temporary file name */


/* Global variables */
extern int getopt();
extern int optind;
extern char *optarg;


usage()
{
Error(stderr, "%s -- usage:\n\t%s ", prog_desc, prog);
Error(stderr, "%s\n\t\t%s", "[-f] [-s #] [-p #] [-e ULAW|LINEAR|FLOAT] [-c #]",
"  [-o off] [-i msg] [file ...]\nwhere:\n");
Error(stderr, "\t-f\tModify an existing sound file header, if present\n");
Error(stderr, "\t-s #\tSample rate (default: 8000)\n");
Error(stderr, "\t-p #\tData precision, in bits (default: 8)\n");
Error(stderr, "\t-e #\tEncoding type (default: ULAW)\n");
Error(stderr, "\t-c #\tNumber of interleaved channels (default: 1)\n");
Error(stderr, "\t-o off\tByte offset in data (default: 0)\n");
Error(stderr, "\t-i msg\tFile header information string\n");
Error(stderr, "\tfile\tList of files to modify\n");
Error(stderr, "\t\tIf no files specified, read stdin and write stdout\n");
exit(1);
}

void
sigint()
{
	/* try to remove temporary file before exiting */
	if ((ofile != NULL) && (ofile != Stdout))
		(void) unlink(ofile);
	exit(1);
}

/*
 * Add a sound file header to flat audio files.
 */
main(argc, argv)
	int		argc;
	char		**argv;
{
	int		i;
	unsigned	ui;
	int		cnt;
	unsigned	offcnt;
	int		err;
	int		ifd;
	int		ofd;
	char		*realfile;
	char		*ifile;
	char		*bufp;
	Audio_hdr	thdr;
	struct stat	st;
	struct sigvec	vec;

	prog = argv[0];		/* save program initiation name */

	/* Initialize the audio header to default values */
	Hdr.sample_rate = 8000;		/* 8kHz */
	Hdr.samples_per_unit = 1;	/* normal data packing */
	Hdr.bytes_per_unit = 1;		/* 8-bit */
	Hdr.channels = 1;		/* monophonic data */
	Hdr.encoding = AUDIO_ENCODING_ULAW;
	Hdr.data_size = AUDIO_UNKNOWN_SIZE;

	err = 0;
	while ((i = getopt(argc, argv, prog_opts)) != EOF)  switch (i) {
	case 's':
		if (parse_unsigned(optarg, &Hdr.sample_rate, "-s"))
			err++;
		break;
	case 'p':
		if (parse_unsigned(optarg, &ui, "-p"))
			err++;
		switch (ui) {
		case 8: case 16: case 24: case 32: case 64:
			Hdr.bytes_per_unit = ui / 8;
			break;
		default:
			Error(stderr,
			    "%s: precision must be one of: 8, 16, 24, 32, 64\n",
			    prog);
			err++;
			break;
		}
		break;
	case 'c':
		if (parse_unsigned(optarg, &Hdr.channels, "-c"))
			err++;
		break;
	case 'e':
		if (parse_encoding(optarg, &Hdr.encoding))
			err++;
		break;
	case 'o':
		if (parse_unsigned(optarg, &Offset, "-o"))
			err++;
		break;
	case 'i':
		Info = optarg;		/* set information string */
		Ilen = strlen(Info);
		break;
	case 'f':
		Force++;		/* force header rewrite */
		break;
	case '?':
		usage();
/*NOTREACHED*/
	}
	/* Check interactions of flag values */
	if (err == 0) {
		switch (Hdr.encoding) {
		case AUDIO_ENCODING_ULAW:
			if (Hdr.bytes_per_unit != 1)
				err++;
			break;
		case AUDIO_ENCODING_LINEAR:
			if (Hdr.bytes_per_unit > 4)
				err++;
			break;
		case AUDIO_ENCODING_FLOAT:
			if (Hdr.bytes_per_unit < 4)
				err++;
			break;
		}
		if (err > 0) {
			Error(stderr, "%s: encoding/precision mismatch\n",
			    prog);
		}
	}
	if (err > 0)
		exit(1);

	argc -= optind;		/* update arg pointers */
	argv += optind;

	/* Set up SIGINT handler to clean up temporary files */
	ofile = NULL;		/* flag no output file open */
	vec.sv_handler = sigint;
	vec.sv_mask = 0;
	vec.sv_flags = 0;
	(void) sigvec(SIGINT, &vec, (struct sigvec *)NULL);

	/* If no filenames, copy stdin to stdout */
	if (argc <= 0) {
		ifile = Stdin;
		ofile = Stdout;
	} else {
		ifile = *argv++;
		argc--;
	}

	/* Main processing loop */
	do {
		if (ifile == Stdin) {
			ifd = fileno(stdin);
			ofd = fileno(stdout);
		} else {
			/* If file is a sym-link, find the underlying file */
			realfile = ifile;
			err = 0;
			while (err == 0) {
				err = lstat(realfile, &st);
				if (!err && S_ISLNK(st.st_mode)) {
					err = readlink(realfile, tmpf,
					    (sizeof (tmpf) - 1));
					if (err > 0) {
						tmpf[err] = '\0';
						realfile = tmpf;
						err = 0;
					}
				} else {
					break;
				}
			}

			if ((err < 0) ||
			    ((ifd = open(realfile, O_RDONLY, 0)) < 0)) {
				Error(stderr, "%s: cannot open ", prog);
				perror(ifile);
				goto nextfile;
			}
			Hdr.data_size = st.st_size;
		}
		/*
		 * Read the beginning of the file and check to see if it
		 * already has a header.  If so, only write a new header
		 * if -f was set.  If no -f, leave the file alone
		 * (if reading stdin, copy it unchanged to stdout).
		 */
		cnt = read(ifd, (char *)buf, sizeof (Audio_filehdr));
		if (cnt == 0) {
			Error(stderr, "%s: WARNING: %s is empty\n",
			    prog, ifile);
			goto closeinput;
		}
		if (cnt < 0) {
readerror:
			Error(stderr, "%s: error reading ", prog);
			perror(ifile);
			goto closeinput;
		}

		if (audio_decode_filehdr(buf, &thdr, &ui) == AUDIO_SUCCESS) {
			/*
			 * The file header looks ok; also check to see that
			 * the data_size is consistent.  If data_size is
			 * unknown, this is fine.  Otherwise, check the size
			 * only if this is a regular file.  If the sizes
			 * are wrong but the -f flag is set, try to rewrite
			 * the file anyway (assume that hdr_size is ok).
			 */
			err = 0;
			if ((thdr.data_size != AUDIO_UNKNOWN_SIZE) &&
			    S_ISREG(st.st_mode) && (st.st_size !=
			    (ui + thdr.data_size + sizeof (Audio_filehdr))))
				err++;

			/* This *really is* an audio file. */
			if (!Force) {
				if (err)
					goto corruptfile;
				Error(stderr,
		    "%s: WARNING: %s is already an audio file\n", prog, ifile);
				if (ifile == Stdin)
					goto copydata;
				goto closeinput;
			}

			/* Subtract the old header size */
			if (Hdr.data_size != AUDIO_UNKNOWN_SIZE)
				Hdr.data_size -= (ui + sizeof (Audio_filehdr));

			/* Skip the info part of the old header */
			while (ui != 0) {
				cnt = MIN(ui, sizeof (buf));
				err = read(ifd, (char *)buf, cnt);
				if (err < 0)
					goto readerror;

				/* Premature EOF means a bad audio header */
				if (err != cnt) {
corruptfile:
					Error(stderr,
					    "%s: audio header corrupt in %s\n",
					    prog, ifile);
					goto closeinput;
				}
				ui -= cnt;
			}
			cnt = 0;	/* no residual data in buffer */
		}

		if (ifile != Stdin) {
			/* If the file is read-only, give up */
			if (access(realfile, W_OK)) {
				Error(stderr, "%s: cannot rewrite ", prog);
				perror(ifile);
				goto closeinput;
			}

			/* Create a temporary output file */
			i = strlen(realfile) + strlen(Suffix) + 1;
			ofile = malloc((unsigned)i);
			if (ofile == NULL) {
				perror(prog);
				exit(1);
			}
			(void) sprintf(ofile, "%s%s", realfile, Suffix);
			ofd = open(ofile, O_WRONLY | O_CREAT, 0666);
			if (ofd < 0) {
				Error(stderr, "%s: cannot create ", prog);
				perror(ofile);
				goto closefree;
			}
		}

		/* Write the audio file header first. */
		if (Hdr.data_size != AUDIO_UNKNOWN_SIZE)
			Hdr.data_size -= Offset;
		err = audio_write_filehdr(ofd, &Hdr, Info, Ilen);
		if (err != AUDIO_SUCCESS)
			goto writeerror;

copydata:
		/*
		 * At this point, we're all ready to copy the data.
		 * 'cnt' contains the number of bytes in 'buf'
		 * that must be written first ('cnt' may be zero).
		 * 'Offset' bytes must be skipped.
		 */
		offcnt = Offset;
		while (cnt >= 0) {
			bufp = (char *)buf;
			if (offcnt != 0) {
				if (cnt > offcnt) {
					bufp += offcnt;
					cnt -= offcnt;
					offcnt = 0;
				} else {
					offcnt -= cnt;
					cnt = 0;
				}
			}
			if (cnt > 0) {
				err = write(ofd, bufp, cnt);
				if (err != cnt) {
writeerror:
					Error(stderr, "%s: error writing ",
					    prog);
					perror(ofile);
					goto closeall;
				}
			}
			cnt = read(ifd, (char *)buf, sizeof (buf));
			if (cnt == 0)
				break;
		}
		if (cnt < 0) {
			Error(stderr, "%s: error reading ", prog);
			perror(ifile);
			goto closeall;
		}
		if (offcnt != 0) {
			Error(stderr, "%s: premature eof on %s\n", prog, ifile);
			goto closeall;
		}

		/* The file transfer is complete.  Rename the file. */
		if (ifile != Stdin) {
			if (rename(ofile, realfile) < 0) {
				Error(stderr, "%s: error renaming ", prog);
				perror(ofile);
				(void) close(ofd);
				goto closefree;
			}
			/* Set the permissions to match the original */
			if (fchmod(ofd, (int)st.st_mode) < 0) {
				Error(stderr,
			    "%s: WARNING: could not reset mode of ", prog);
				perror(realfile);
			}
		}

closeall:
		(void) close(ofd);		/* close output file */
		if (ifile != Stdin)
			(void) unlink(ofile);	/* in case of error */
closefree:
		if (ifile != Stdin)
			(void) free(ofile);	/* release temp name */
		ofile = NULL;
closeinput:
		(void) close(ifd);		/* close input file */
nextfile:;
	} while ((argc > 0) && (argc--, (ifile = *argv++) != NULL));
	exit(0);
/*NOTREACHED*/
}


/* Parse an unsigned integer */
parse_unsigned(str, dst, flag)
	char		*str;
	unsigned	*dst;
	char		*flag;
{
	char		x;

	if (sscanf(str, "%u%c", dst, &x) != 1) {
		Error(stderr, "%s: invalid value for %s\n", prog, flag);
		return (1);
	}
	return (0);
}

/* Parse an encoding type */
parse_encoding(str, dst)
	char		*str;
	unsigned	*dst;
{
	char		*p;
	int		i;

	p = str;
	while (*p != '\0') {
		if (islower(*p))
			*p = toupper(*p);
		p++;
	}
	i = p - str;

	if (strncmp(str, "ULAW", i) == 0)
		*dst = AUDIO_ENCODING_ULAW;
	else if (strncmp(str, "LINEAR", i) == 0)
		*dst = AUDIO_ENCODING_LINEAR;
	else if (strncmp(str, "FLOAT", i) == 0)
		*dst = AUDIO_ENCODING_FLOAT;
	else {
		Error(stderr,
		    "%s: encoding must be one of: ULAW, LINEAR, FLOAT\n", prog);
		return (1);
	}
	return (0);
}
