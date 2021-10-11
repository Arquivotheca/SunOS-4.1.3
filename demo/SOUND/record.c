#ifndef lint
static	char sccsid[] = "@(#)record.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>

#include <stropts.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>


#define	Error		(void) fprintf


/* Local variables */
char *prog;
char prog_desc[] = "Record an audio file";
char prog_opts[] = "aft:v:d:i:?";	/* getopt() flags */

char		*Stdout = "stdout";

/* XXX - the input buffer size should depend on sample_rate */
unsigned char	buf[1024 * 64];


#define	MAX_GAIN		(100)		/* maximum gain */


char		*Info = NULL;		/* pointer to info data */
unsigned	Ilen = 0;		/* length of info data */
unsigned	Volume = ~0;		/* record volume */
double		Savevol;		/* saved record volume */
int		Append = FALSE;		/* append to output file */
int		Force = FALSE;		/* ignore rate differences on append */
double		Time = -1.;		/* recording time */
unsigned	Limit = AUDIO_UNKNOWN_SIZE;	/* recording limit */
char		*Audio_dev = "/dev/audio";

int		Audio_fd = -1;		/* file descriptor for audio device */
Audio_hdr	Dev_hdr;		/* audio header for device */
char		*Ofile;			/* current filename */
Audio_hdr	File_hdr;		/* audio header for file */
int		Cleanup = FALSE;	/* SIGINT sets this flag */
unsigned	Size = 0;		/* Size of output file */
unsigned	Oldsize = 0;		/* Size of input file, if append */


/* Global variables */
extern int getopt();
extern int optind;
extern char *optarg;


usage()
{
	Error(stderr, "%s -- usage:\n\t%s ", prog_desc, prog);
	Error(stderr, "\t[-a] [-v #] [-t #] [-i msg] [-d dev] [file]\n");
	Error(stderr, "where:\n\t-a\tAppend to output file\n");
	Error(stderr, "\t-f\tIgnore sample rate differences on append\n");
	Error(stderr, "\t-v #\tSet record volume (0 - %d)\n", MAX_GAIN);
	Error(stderr, "\t-t #\tSpecify record time (hh:mm:ss.dd)\n");
	Error(stderr, "\t-i msg\tSpecify a file header information string\n");
	Error(stderr, "\t-d dev\tSpecify audio device (default: /dev/audio)\n");
	Error(stderr, "\tfile\tRecord to named file\n");
	Error(stderr, "\t\tIf no file specified, write to stdout\n");
	Error(stderr, "\t\tIf no -t specified, recording continues until ^C\n");
	exit(1);
}

void
sigint()
{
	/* If this is the first ^C, set a flag for the main loop */
	if (!Cleanup && (Audio_fd >= 0)) {
		/* flush input queues before exiting */
		Cleanup = TRUE;
		if (audio_pause_record(Audio_fd) == AUDIO_SUCCESS)
			return;
		Error(stderr, "%s: could not flush input buffer\n", prog);
	}

	/* If double ^C, really quit */
	if ((Volume != ~0) && (Audio_fd >= 0)) {
		(void) audio_set_record_gain(Audio_fd, &Savevol);
	}
	exit(1);
}

/*
 * Record from the audio device to a file.
 */
main(argc, argv)
	int		argc;
	char		**argv;
{
	int		i;
	int		cnt;
	int		err;
	int		ofd;
	double		vol;
	struct stat	st;
	struct sigvec	vec;
	struct pollfd	pfd;


	prog = argv[0];		/* save program initiation name */

	err = 0;
	while ((i = getopt(argc, argv, prog_opts)) != EOF)  switch (i) {
	case 'v':
		if (parse_unsigned(optarg, &Volume, "-v")) {
			err++;
		} else if (Volume > MAX_GAIN) {
			Error(stderr, "%s: invalid value for -v\n", prog);
			err++;
		}
		break;
	case 't':
		Time = audio_str_to_secs(optarg);
		if ((Time == HUGE_VAL) || (Time < 0.)) {
			Error(stderr, "%s: invalid value for -t\n", prog);
			err++;
		}
		break;
	case 'd':
		Audio_dev = optarg;
		break;
	case 'f':
		Force = TRUE;
		break;
	case 'a':
		Append = TRUE;
		break;
	case 'i':
		Info = optarg;		/* set information string */
		Ilen = strlen(Info);
		break;
	case '?':
		usage();
/*NOTREACHED*/
	}
	if (Append && (Info != NULL)) {
		Error(stderr, "%s: cannot specify -a and -i\n", prog);
		err++;
	}
	if (err > 0)
		exit(1);

	argc -= optind;		/* update arg pointers */
	argv += optind;

	/* Open the output file */
	if (argc <= 0) {
		Ofile = Stdout;
	} else {
		Ofile = *argv++;
		argc--;

		/* Interpret "-" filename to mean stdout */
		if (strcmp(Ofile, "-") == 0)
			Ofile = Stdout;
	}

	if (Ofile == Stdout) {
		ofd = fileno(stdout);
		Append = FALSE;
		if (isatty(ofd)) {
			Error(stderr,
	    "%s: Specify a filename or redirect stdout to a file\n", prog);
			exit(1);
		}
	} else {
		ofd = open(Ofile,
		    (O_RDWR | O_CREAT | (Append ? 0 : O_TRUNC)), 0666);
		if (ofd < 0) {
			Error(stderr, "%s: cannot open ", prog);
			perror(Ofile);
			exit(1);
		}
		if (Append) {
			/*
			 * Check to make sure we're appending to an audio file.
			 * It must be a regular file (if zero-length, simply
			 * write it from scratch).  Also, its file header
			 * must match the input device configuration.
			 */
			if ((fstat(ofd, &st) < 0) || (!S_ISREG(st.st_mode))) {
				Error(stderr, "%s: %s is not a regular file\n",
				    prog, Ofile);
				exit(1);
			}
			if (st.st_size == 0) {
				Append = FALSE;
				goto openinput;
			}

			err = audio_read_filehdr(ofd, &File_hdr,
			    (char *)NULL, 0);

			if (err != AUDIO_SUCCESS) {
				Error(stderr,
				    "%s: %s is not a valid audio file\n",
				    prog, Ofile);
				exit(1);
			}
			/* Get the current size, if possible */
			Oldsize = File_hdr.data_size;
			if ((Oldsize == AUDIO_UNKNOWN_SIZE) &&
			    ((err = (int)lseek(ofd, 0L, L_INCR)) >= 0))
				Oldsize = st.st_size - err;

			/* Seek to end to start append */
			if ((int)lseek(ofd, st.st_size, L_SET) < 0) {
				Error(stderr,
				    "%s: cannot find end of %s\n", prog, Ofile);
				exit(1);
			}
		}
	}
openinput:
	/* Validate and open the audio device */
	err = stat(Audio_dev, &st);
	if (err < 0) {
		Error(stderr, "%s: cannot open ", prog);
		perror(Audio_dev);
		exit(1);
	}
	if (!S_ISCHR(st.st_mode)) {
		Error(stderr, "%s: %s is not an audio device\n", prog,
		    Audio_dev);
		exit(1);
	}
	Audio_fd = open(Audio_dev, O_RDONLY | O_NDELAY);
	if (Audio_fd < 0) {
		if (errno == EBUSY) {
			Error(stderr, "%s: %s is busy\n", prog, Audio_dev);
		} else {
			Error(stderr, "%s: error opening ", prog);
			perror(Audio_dev);
		}
		exit(1);
	}
	if (audio_get_record_config(Audio_fd, &Dev_hdr) != AUDIO_SUCCESS) {
		Error(stderr, "%s: %s is not an audio device\n",
		    prog, Audio_dev);
		exit(1);
	}


	/* If appending to an existing file, check the configuration */
	if (Append) {
		char	msg[AUDIO_MAX_ENCODE_INFO];

		switch (audio_cmp_hdr(&Dev_hdr, &File_hdr)) {
		case 0:			/* configuration matches */
			break;
		case 1:			/* all but sample rate matches */
			if (Force) {
				Error(stderr,
		    "%s: WARNING: appending %.3fkHz data to %s (%.3fkHz)\n",
				prog, ((double)Dev_hdr.sample_rate / 1000.),
				Ofile, ((double)File_hdr.sample_rate / 1000.));
				break;
			}		/* if not -f, fall through */

		default:		/* encoding mismatch */
			(void) audio_enc_to_str(&Dev_hdr, msg);
			Error(stderr,
			    "%s: device encoding [%s]\n", prog, msg);
			(void) audio_enc_to_str(&File_hdr, msg);
			Error(stderr,
			    "\tdoes not match file encoding [%s]\n", msg);
			exit(1);
		}
	} else {
		if (audio_write_filehdr(ofd, &Dev_hdr, Info, Ilen) !=
		    AUDIO_SUCCESS) {
			Error(stderr, "%s: error writing header for \n", prog);
			perror(Ofile);
			exit(1);
		}
	}

	/* If -v flag, set the record volume now */
	if (Volume != ~0) {
		vol = (double) Volume / (double) MAX_GAIN;
		(void) audio_get_record_gain(Audio_fd, &Savevol);
		err = audio_set_record_gain(Audio_fd, &vol);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    "%s: could not set record volume for %s\n",
			    prog, Audio_dev);
			exit(1);
		}
	}

	/* Set the device up for non-blocking reads */
	(void) fcntl(Audio_fd, F_SETFL, fcntl(Audio_fd, F_GETFL, 0) | O_NDELAY);

	/* Set up SIGINT handler so that final buffers may be flushed */
	vec.sv_handler = sigint;
	vec.sv_mask = 0;
	vec.sv_flags = 0;
	(void) sigvec(SIGINT, &vec, (struct sigvec *)NULL);


	/*
	 * At this point, we're (finally) ready to copy the data.
	 * Init a poll() structure, to use when there's nothing to read.
	 */
	if (Time > 0)
		Limit = audio_secs_to_bytes(&Dev_hdr, Time);
	pfd.fd = Audio_fd;
	pfd.events = POLLIN;
	while ((Limit == AUDIO_UNKNOWN_SIZE) || (Limit != 0)) {
		/* Fill the buffer or read to the time limit */
		cnt = read(Audio_fd, (char *)buf,
		    ((Limit != AUDIO_UNKNOWN_SIZE) && (Limit < sizeof (buf)) ?
		    (int)Limit : sizeof (buf)));

		if (cnt == 0)		/* normally, eof can't happen */
			break;

		/* If error, probably have to wait for input */
		if (cnt < 0) {
			if (Cleanup)
				break;		/* done if ^C seen */
			switch (errno) {
			case EAGAIN:
			case EWOULDBLOCK:
				(void) poll(&pfd, 1L, -1);
				break;
			default:
				Error(stderr, "%s: error reading ", prog);
				perror(Audio_dev);
				exit(1);
			}
			continue;
		}
		err = write(ofd, (char *)buf, cnt);
		if (err != cnt) {
			Error(stderr, "%s: error writing ", prog);
			perror(Ofile);
			break;
		}
		Size += cnt;
		if (Limit != AUDIO_UNKNOWN_SIZE)
			Limit -= cnt;
	}

	/* Attempt to rewrite the data_size field of the file header */
	if (!Append || (Oldsize != AUDIO_UNKNOWN_SIZE)) {
		if (Append)
			Size += Oldsize;
		(void) audio_rewrite_filesize(ofd, Size);
	}

	(void) close(ofd);			/* close input file */


	/* Check for error during record */
	if (audio_get_record_error(Audio_fd, (unsigned *)&err) != AUDIO_SUCCESS)
		Error(stderr, "%s: error reading device status\n", prog);
	else if (err)
		Error(stderr, "%s: WARNING: Data overflow occurred\n", prog);

	/* Reset record volume */
	if (Volume != ~0)
		(void) audio_set_record_gain(Audio_fd, &Savevol);
	(void) close(Audio_fd);

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
