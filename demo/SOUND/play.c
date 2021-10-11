#ifndef lint
static	char sccsid[] = "@(#)play.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>

#include <stropts.h>
#include <sys/ioctl.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>


#define	Error		(void) fprintf


/* Local variables */
char *prog;
char prog_desc[] = "Play an audio file";
char prog_opts[] = "Viv:d:?";		/* getopt() flags */

char		*Stdin = "stdin";
unsigned char	buf[1024 * 64];		/* size should depend on sample_rate */


#define	MAX_GAIN		(100)		/* maximum gain */

/*
 * This defines the tolerable sample rate error as a ratio between the
 * sample rates of the audio data and the audio device.
 */
#define	SAMPLE_RATE_THRESHOLD	(.01)


unsigned	Volume = ~0;		/* output volume */
double		Savevol;		/* saved volume level */
int		Verbose = FALSE;	/* verbose messages */
int		Immediate = FALSE;	/* don't hang waiting for device */
char		*Audio_dev = "/dev/audio";

int		Audio_fd = -1;		/* file descriptor for audio device */
Audio_hdr	Dev_hdr;		/* audio header for device */
char		*Ifile;			/* current filename */
Audio_hdr	File_hdr;		/* audio header for file */


/* Global variables */
extern int getopt();
extern int optind;
extern char *optarg;


usage()
{
	Error(stderr, "%s -- usage:\n\t%s ", prog_desc, prog);
	Error(stderr, "\t[-iV] [-v #] [-d dev] [file ...]\nwhere:\n");
	Error(stderr, "\t-i\tDon't hang if audio device is busy\n");
	Error(stderr, "\t-V\tPrint verbose warning messages\n");
	Error(stderr, "\t-v #\tSet output volume (0 - %d)\n", MAX_GAIN);
	Error(stderr, "\t-d dev\tSpecify audio device (default: /dev/audio)\n");
	Error(stderr, "\tfile\tList of files to play\n");
	Error(stderr, "\t\tIf no files specified, read stdin\n");
	exit(1);
}

void
sigint()
{
	/* flush output queues before exiting */
	if (Audio_fd >= 0) {
		(void) audio_flush_play(Audio_fd);
		if (Volume != ~0)
			(void) audio_set_play_gain(Audio_fd, &Savevol);
		(void) close(Audio_fd);			/* close output */
	}
	exit(1);
}

/*
 * Play a list of audio files.
 */
main(argc, argv)
	int		argc;
	char		**argv;
{
	int		i;
	int		cnt;
	int		err;
	int		ifd;
	int		stdinseen;
	double		vol;
	struct stat	st;
	struct sigvec	vec;

	prog = argv[0];		/* save program initiation name */

	err = 0;
	while ((i = getopt(argc, argv, prog_opts)) != EOF) switch (i) {
	case 'v':
		if (parse_unsigned(optarg, &Volume, "-v")) {
			err++;
		} else if (Volume > MAX_GAIN) {
			Error(stderr, "%s: invalid value for -v\n", prog);
			err++;
		}
		break;
	case 'd':
		Audio_dev = optarg;
		break;
	case 'V':
		Verbose = TRUE;
		break;
	case 'i':
		Immediate = TRUE;
		break;
	case '?':
		usage();
/*NOTREACHED*/
	}
	if (err > 0)
		exit(1);

	argc -= optind;		/* update arg pointers */
	argv += optind;

	/* Validate and open the audio device */
	err = stat(Audio_dev, &st);
	if (err < 0) {
		Error(stderr, "%s: cannot stat ", prog);
		perror(Audio_dev);
		exit(1);
	}
	if (!S_ISCHR(st.st_mode)) {
		Error(stderr, "%s: %s is not an audio device\n", prog,
		    Audio_dev);
		exit(1);
	}

	/* Try it quickly, first */
	Audio_fd = open(Audio_dev, O_WRONLY | O_NDELAY);
	if ((Audio_fd < 0) && (errno == EBUSY)) {
		if (Immediate) {
			Error(stderr, "%s: %s is busy\n", prog, Audio_dev);
			exit(1);
		}
		if (Verbose) {
			Error(stderr, "%s: waiting for %s...", prog, Audio_dev);
			(void) fflush(stderr);
		}
		/* Now hang until it's open */
		Audio_fd = open(Audio_dev, O_WRONLY);
		if (Verbose)
			Error(stderr, "%s\n", (Audio_fd < 0) ? "" : "open");
	}
	if (Audio_fd < 0) {
		Error(stderr, "%s: error opening ", prog);
		perror(Audio_dev);
		exit(1);
	}

	/* Get the device output encoding configuration */
	if (audio_get_play_config(Audio_fd, &Dev_hdr) != AUDIO_SUCCESS) {
		Error(stderr, "%s: %s is not an audio device\n",
		    prog, Audio_dev);
		exit(1);
	}

	/* If -v flag, set the output volume now */
	if (Volume != ~0) {
		vol = (double) Volume / (double) MAX_GAIN;
		(void) audio_get_play_gain(Audio_fd, &Savevol);
		err = audio_set_play_gain(Audio_fd, &vol);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    "%s: could not set output volume for %s\n",
			    prog, Audio_dev);
			exit(1);
		}
	}

	/* Set up SIGINT handler to flush output */
	vec.sv_handler = sigint;
	vec.sv_mask = 0;
	vec.sv_flags = 0;
	(void) sigvec(SIGINT, &vec, (struct sigvec *)NULL);


	/* If no filenames, read stdin */
	stdinseen = FALSE;
	if (argc <= 0) {
		Ifile = Stdin;
	} else {
		Ifile = *argv++;
		argc--;
	}

	/* Loop through all filenames */
	do {
		/* Interpret "-" filename to mean stdin */
		if (strcmp(Ifile, "-") == 0)
			Ifile = Stdin;

		if (Ifile == Stdin) {
			if (stdinseen) {
				Error(stderr, "%s: stdin already processed\n",
				    prog);
				goto nextfile;
			}
			stdinseen = TRUE;
			ifd = fileno(stdin);
		} else {
			if ((ifd = open(Ifile, O_RDONLY, 0)) < 0) {
				Error(stderr, "%s: cannot open ", prog);
				perror(Ifile);
				goto nextfile;
			}
		}

		/* Check to make sure this is an audio file */
		err = audio_read_filehdr(ifd, &File_hdr, (char *)NULL, 0);

		if (err != AUDIO_SUCCESS) {
			Error(stderr, "%s: %s is not a valid audio file\n",
			    prog, Ifile);
			goto closeinput;
		}

		/* Check the device configuration */
		if (audio_cmp_hdr(&Dev_hdr, &File_hdr) != 0) {
			/*
			 * The device does not match the input file.
			 * Wait for any old output to drain, then attempt
			 * to reconfigure the audio device to match the
			 * input data.
			 */
			if (audio_drain(Audio_fd, FALSE) != AUDIO_SUCCESS) {
				Error(stderr, "%s: ", prog);
				perror("AUDIO_DRAIN error");
				exit(1);
			}
			if (!reconfig())
				goto closeinput;
		}

		/*
		 * At this point, we're all ready to copy the data.
		 */
		while ((cnt = read(ifd, (char *)buf, sizeof (buf))) >= 0) {
			/* If input EOF, write an eof marker */
			err = write(Audio_fd, (char *)buf, cnt);

			if (err != cnt) {
				Error(stderr, "%s: output error: ", prog);
				perror("");
				break;
			}
			if (cnt == 0)
				break;
		}
		if (cnt < 0) {
			Error(stderr, "%s: error reading ", prog);
			perror(Ifile);
		}

closeinput:
		(void) close(ifd);		/* close input file */
nextfile:;
	} while ((argc > 0) && (argc--, (Ifile = *argv++) != NULL));

	/*
	 * Though drain is implicit on close(), it's performed here
	 * for the sake of completeness, and to ensure that the volume
	 * is reset after all output is complete.
	 */
	(void) audio_drain(Audio_fd, FALSE);
	if (Volume != ~0) {
		(void) audio_set_play_gain(Audio_fd, &Savevol);
	}
	
	(void) close(Audio_fd);			/* close output */
	exit(0);
/*NOTREACHED*/
}


/*
 * Try to reconfigure the audio device to match the file encoding.
 * If this fails, we should attempt to make the input data match the
 * device encoding.  For now, we give up on this file.
 *
 * Returns TRUE if successful.  Returns FALSE if not.
 */
reconfig()
{
	int	err;
	char	msg[AUDIO_MAX_ENCODE_INFO];

	Dev_hdr = File_hdr;
	err = audio_set_play_config(Audio_fd, &Dev_hdr);

	switch (err) {
	case AUDIO_SUCCESS:
		return (TRUE);

	case AUDIO_ERR_NOEFFECT:
		/*
		 * Couldn't change the device.
		 * Check to see if we're nearly compatible.
		 * audio_cmp_hdr() returns >0 if only sample rate difference.
		 */
		if (audio_cmp_hdr(&Dev_hdr, &File_hdr) > 0) {
			double	ratio;

			ratio = (double) abs((int)
			    (Dev_hdr.sample_rate - File_hdr.sample_rate)) /
			    (double) File_hdr.sample_rate;
			if (ratio <= SAMPLE_RATE_THRESHOLD) {
				if (Verbose) {
					Error(stderr,
			    "%s: WARNING: %s sampled at %d, playing at %d\n",
					    prog, Ifile, File_hdr.sample_rate,
					    Dev_hdr.sample_rate);
				}
				return (TRUE);
			}
			Error(stderr, "%s: %s sample rate %d not available\n",
			    prog, Ifile, File_hdr.sample_rate);
			return (FALSE);
		}
		(void) audio_enc_to_str(&File_hdr, msg);
		Error(stderr, "%s: %s encoding not available: %s\n",
		    prog, Ifile, msg);
		return (FALSE);

	default:
		Error(stderr, "%s: i/o error (set config)\n");
		exit(1);
/*NOTREACHED*/
	}
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
