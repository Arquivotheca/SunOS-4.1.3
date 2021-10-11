#ident   "@(#)audbri_globals.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Main file for the DBRI/MMCODEC audio test.  It has the all the 
 * Sundiag interface routines.  The DBRI and MMCODEC devices are 
 * used on 'Campus2' and 'Synergy' platforms and Sbus 'Batphone' card.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/asynch.h>
/* #include <sun/audioio.h> ...not there yet... */
#include "audioio.h" 

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"

#include "audbri.h"
#include "audbri_msg.h"

/*
 *  Test control flags.
 */
int   calibration_test = FALSE;
int   controls_test = FALSE;
int   crystal_test = FALSE;
int   dump_signals = FALSE;
int   loop_test = FALSE;
int   music_test = FALSE;
int   ltype = LOOP_SBMIC;

char	*ioctldev_name = IOCTLDEV_NAME;
int	audiofd = -1;
int	audioctlfd = -1;

int	duration = 1;		/* signal duration in seconds */
int	gain_step = 4;		/* used during calibration */

/*
 *  After changing ref_size be sure to verify that calc_offset() will
 * still operate properly for all sample rates.
 */
int	ref_size = FFT_SIZE;	/* sample count for signal processing  */

int	play_size = FFT_SIZE*32; /* block size for writes */
int	music_gain = 220;
char	*music_file = "music.au";

double	aucrystal_tol = .03;	/* tolerance of audio crystal */
double	aumag_tol = 10;		/* tolerance of FFT magnitudes (db) */
/*double	aurms_tol = 5;*/		/* tolerance of RMS (db) */
double	aurms_tol = 10;		/* tolerance of RMS (db) */
double	bandrms_tol = 40;	/* diff between fund and non-fund */

int	aioto_cnt = 5;		/* wait N * signal duration for aioxxx() */

ref_data_t	*refptr = NULL;
char		*refname = "                                  ";
int		ref_mapsize = 0;

char		*encodestr[] = {"None", "u-law", "a-law", "linear"};


/*
 *  Band data used to verify signals during loopback testing.  The
 * numbers here represent frequency rab\nges to check and are closely
 * tied to the sample rate and test signal frequency as specified in
 * the au_data_t tables above (sbmic_audata, loli_audata and hdli_audata).
 *
 *  The boundaries around the fundamental band (the band which contains
 * the test signal frequency) are carefully choosen such that the
 * "skirts" of the fundamental frequency response are avoided.
 *
 *  The first band of bdata_8000 goes from 0-100 hz to encompass the
 * DC "skirt" in that band.  The following band_data_t structure 
 * scale 0-100 up by sample_rate/8000 * 100, e.g. bdata_9600's first 
 * band is 0-120, where 9600/8000 * 100 = 120.
 */
band_data_t	bdata_8000[] = {
  {BAND_NORM, 0, 100},
  {BAND_NORM, 101, 900},
  {BAND_FUND, 901, 1099},
  {BAND_NORM, 1100, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_9600[] = {
  {BAND_NORM, 0, 120},
  {BAND_NORM, 121, 1080},
  {BAND_FUND, 1081, 1319},
  {BAND_NORM, 1320, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_16000[] = {
  {BAND_NORM, 0, 200},
  {BAND_NORM, 201, 1800},
  {BAND_FUND, 1801, 2199},
  {BAND_NORM, 2200, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_18900[] = {
  {BAND_NORM, 0, 236},
  {BAND_NORM, 237, 2126},
  {BAND_FUND, 2127, 2597},
  {BAND_NORM, 2598, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_32000[] = {
  {BAND_NORM, 0, 400},
  {BAND_NORM, 401, 3600},
  {BAND_FUND, 3601, 4399},
  {BAND_NORM, 4400, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_37800[] = {
  {BAND_NORM, 0, 473},
  {BAND_NORM, 474, 4253},
  {BAND_FUND, 4254, 5196},
  {BAND_NORM, 5197, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_44100[] = {
  {BAND_NORM, 0, 551},
  {BAND_NORM, 552, 4961},
  {BAND_FUND, 4962, 6062},
  {BAND_NORM, 6063, END_FREQ},
  {NULL, NULL, NULL}
};

band_data_t	bdata_48000[] = {
  {BAND_NORM, 0, 600},
  {BAND_NORM, 601, 5400},
  {BAND_FUND, 5401, 6599},
  {BAND_NORM, 6600, END_FREQ},
  {NULL, NULL, NULL}
};


/*
 *  The following data specifies the audio configuration data 
 * each of the loopback configurations.  Adding another line here
 * will add one more pass to the loopback test for the loopback
 * configuration in question.
 *
 *  The format of each line is as follows:
 *
 *	sample_rate, lfreq, rfreq, channels, cal_pgain_max, cal_pgain_min, 
 *		cal_rgain, precision, encoding, band_data_pointer
 *
 *  Only the lfreq field is used if it's a MONO signal.
 *  Make sure that entries are legal otherwise SETINFO will fail
 * (i.e., you can't have STEREO and a-law).
 *
 */

#define AEA	AUDIO_ENCODING_ALAW
#define AEL	AUDIO_ENCODING_LINEAR
#define AEU	AUDIO_ENCODING_ULAW
/*
 *  Speaker / Microphone audio data.
 */
#define SPGAIN	200
au_data_t sbmic_audata[] = {
  {8000, 1000, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_8000},
  {9600, 1200, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_9600},
  {16000, 2000, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_16000},
  {18900, 2362, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_18900},
  {32000, 4000, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_32000},
/*	Commented out due to problem in Crystal MMCodec
  {37800, 4725, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_37800},
*/
  {44100, 5512, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_44100},
  {48000, 6000, 0, MONO, SPGAIN, 50, 0, 16, AEL, bdata_48000},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/*
 *  Line Out / Line In audio data.
 */
#define LOGAIN	210
au_data_t	loli_audata[] = {
  {8000, 1000, 0, MONO, LOGAIN, 150, 200, 8, AEU, bdata_8000},
  {8000, 1000, 0, MONO, LOGAIN, 150, 200, 8, AEA, bdata_8000},
  {8000, 1000, 1000, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_8000},
  {8000, 0, 1000, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_8000},
  {8000, 1000, 0, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_8000},
  {48000, 6000, 6000, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_48000},
  {48000, 0, 6000, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_48000},
  {48000, 6000, 0, STEREO, LOGAIN, 150, 200, 16, AEL, bdata_48000},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/*
 *  Headphone / Line In audio data.
 */
#define HDGAIN	210
au_data_t	hdli_audata[] = {
  {8000, 1000, 1000, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_8000},
  {8000, 0, 1000, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_8000},
  {8000, 1000, 0, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_8000},
  {48000, 6000, 6000, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_48000},
  {48000, 0, 6000, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_48000},
  {48000, 6000, 0, STEREO, HDGAIN, 150, 200, 16, AEL, bdata_48000},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/*
 *  Crystal test audio data.
 */
#define XSPGAIN	140
au_data_t	crystal_audata[] = {
  {8000, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {9600, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {16000, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {18900, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {32000, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {37800, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {44100, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {48000, 200, 0, MONO, XSPGAIN, 130, 0, 16, AEL, NULL},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


/*
 *  The following data specifies the audio ports, config data and 
 * reference files for each of the loopback configurations.
 */
loop_data_t	ldata[] = {

	{ /* sb/mic data */
		AUDIO_SPEAKER,			/* play port */
		AUDIO_MICROPHONE,		/* record port */
		sbmic_audata,			/* audio config data */
		"audbri_sbmic.data"		/* ref filename */
	},

	{ /* lo/li data */
		AUDIO_LINE_OUT,			/* play port */
		AUDIO_LINE_IN,			/* record port */
		loli_audata,			/* audio config data */
		"audbri_loli.data"		/* ref filename */
	},

	{ /* hd/li data */
		AUDIO_HEADPHONE,		/* play port */
		AUDIO_LINE_IN,			/* record port */
		hdli_audata,			/* audio config data */
		"audbri_hdli.data"		/* ref filename */
	}
};


audio_info_t	audio_info;

int	dc_flag;		/* state flag for controls test */
