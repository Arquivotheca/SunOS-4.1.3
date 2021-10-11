#ident   "@(#)audbri.h 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Defines for DBRI/MMCODEC audio tests.
 */

#define MARKER		send_message(0, DEBUG, marker_msg, \
			__LINE__, __FILE__);

#define	DEVICE_NAME	"/dev/audio"
#define	IOCTLDEV_NAME	"/dev/audioctl"

#define	TOTAL_PASS	1	/* number of test loops */
#define ERROR_LIMIT	5	/* max errors allowed if run_on_error */

#define	LOOP_SBMIC	0	/* loopback modes */
#define	LOOP_LOLI	1
#define	LOOP_HDLI	2

#define	SYSMSG		errmsg(errno)	/* pointer to system message */

#define MAXPCM		32767		/* max value for 16 bit PCM */
#define MAXALAW		32256		/* max value for alaw (in 16 bits) */
#define MAXULAW		32124		/* max value for alaw (in 16 bits) */
#define SCALE		(double) 32767	/* scale signal to 16 bit */

#define STEREO		2
#define MONO		1
#define	LEFT		0
#define	RIGHT		1

/*
 *  Band data defines.
 */
#define END_FREQ	-1

#define BAND_FUND	1	/* fundamental band */
#define BAND_PREFUND	2	/* pre-fundamental band */
#define BAND_POSTFUND	3	/* post-fundamental band */
#define BAND_NORM	4	/* any other band */
#define BAND_IGNORE	5	/* don't check this band */

#define	FFT_SIZE	1024	/* Size of FFT */


/*
 *  How the threshold data's laid out in calc_thresh().
 */
typedef struct {
	double	*lrms;
	double	*rrms;
	double  *lmag;
	double  *rmag;
} thresh_data_t;

/*
 *  How the data's laid out in a "reference" file.
 */
typedef struct {
	int     play_gain;
	int     record_gain;
	double	rms;
	double  mag;
} ref_data_t;

/*
 *  FFT frequency band info, used for signal validation
 */
typedef struct {
	int	type;
	int	start_freq;
	int	end_freq;
} band_data_t;

/*
 *  Audio configuration data for loopback tests (sb/mic, lo/li, hd/li).
 */
typedef struct {
	int		sample_rate;
	int		lfreq;
	int		rfreq;
	int		channels;
	int     	cal_pgain_max;
	int     	cal_pgain_min;
	int     	cal_rgain;
	int		precision;
	int		encoding;
	band_data_t	*bdata;
} au_data_t;

/*
 *  Loopback configuration data (sb/mic, lo/li, hd/li).
 */
typedef struct {
	int		play_port;
	int		record_port;
	au_data_t	*audata;
	char		*refname;
} loop_data_t;


extern au_data_t	crystal_audata[];

extern int	errno;			/* system error code */
extern int	process_test_args();
extern int	routine_usage();

extern int	calibration_test;
extern int	controls_test;
extern int	crystal_test;
extern int	dump_signals;
extern int	loop_test;
extern int	music_test;
extern int	ltype;

extern char	*ioctldev_name;
extern int	audiofd;
extern int	audioctlfd;

extern int	duration;
extern int	gain_step;

extern int	ref_size;

extern int	music_gain;
extern int	play_size;
extern char	*music_file;

extern double	aucrystal_tol;
extern double	aumag_tol;
extern double	aurms_tol;
extern double	bandrms_tol;

extern int	aioto_cnt;

extern ref_data_t	*refptr;
extern char		*refname;
extern int		ref_mapsize;

extern char		*encodestr[];

extern loop_data_t	ldata[];

extern band_data_t	bdata[];
extern int		bdata_cnt;

extern audio_info_t	audio_info;

extern int	dc_flag;

extern char	*aufname();
