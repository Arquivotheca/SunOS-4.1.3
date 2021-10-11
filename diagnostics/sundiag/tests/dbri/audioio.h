#ident   "@(#)audioio.h 1.1 92/07/30 SMI"

/*
 * Derived from:
 * @(#) audioio.h 4.7@(#) Copyright 1991 Sun Microsystems, Inc.
 */

/*
 * These are the ioctl calls for generic audio devices, including
 * the SPARCstation 1 audio device.
 *
 * You are encouraged to design your code in a modular fashion so that
 * future changes to the interface can be incorporated with little trouble.
 */

#ifndef _sun_audioio_h
#define	_sun_audioio_h

/*
 * This structure contains state information for audio device IO streams.
 */
typedef struct audio_prinfo {
	/*
	 * The following values describe the audio data encoding.
	 * They are read-only for SPARCstation audio, but may be
	 * dynamically configurable for other audio devices.
	 */
	unsigned int	sample_rate;	/* sample frames per second */
	unsigned int	channels;	/* number of interleaved channels */
	unsigned int	precision;	/* bits per sample */
	unsigned int	encoding;	/* data encoding method */

	/* The following values control audio device configuration */
	unsigned int	gain;		/* gain level: 0 - 255 */
	unsigned int	port;		/* selected I/O port (see below) */
	unsigned int	avail_ports;	/* available I/O ports (see below) */
	unsigned int	_xxx[3];	/* Reserved for future use */

	/* The following values describe driver state */
	unsigned int	samples;	/* number of samples converted */
	unsigned int	eof;		/* number of EOF records (play only) */

	unsigned char	pause;		/* TRUE to pause, FALSE to resume */
	unsigned char	error;		/* TRUE if overflow/underflow */
	unsigned char	waiting;	/* TRUE if a process wants access */
	unsigned char	_ccc[1];	/* Reserved for future use */

	unsigned short	minordev;

	/* The following values are read-only state flags */
	unsigned char	open;		/* TRUE if access requested at open */
	unsigned char	active;		/* TRUE if HW I/O or D-channel active */
} audio_prinfo_t;

/*
 * This structure describes the current state of the audio device.
 */
typedef struct audio_info {
	audio_prinfo_t	play;		/* output status information */
	audio_prinfo_t	record;		/* input status information */
	unsigned int	monitor_gain;	/* input to output mix: 0 - 255 */
	unsigned char	speaker_muted;	/* TRUE if speaker is muted */
	unsigned char	_xxx[3];	/* Reserved for future use */
	unsigned int	_yyy[3];	/* Reserved for future use */
} audio_info_t;


/* Audio encoding types */
#define	AUDIO_ENCODING_NONE	(0)	/* no encoding assigned */
#define	AUDIO_ENCODING_ULAW	(1)	/* u-law encoding */
#define	AUDIO_ENCODING_ALAW	(2)	/* A-law encoding */
#define	AUDIO_ENCODING_LINEAR	(3)	/* Linear PCM encoding */

/* These ranges apply to record, play, and monitor gain values */
#define	AUDIO_MIN_GAIN	(0)		/* minimum gain value */
#define	AUDIO_MAX_GAIN	(255)		/* maximum gain value */

/* Define some convenient names for SPARCstation audio ports */
	/* output ports */
#define	AUDIO_SPEAKER		0x01	/* output to built-in speaker */
#define	AUDIO_HEADPHONE		0x02	/* output to headphone jack */
#define	AUDIO_LINE_OUT		0x04	/* output to line out */
	/* input ports */
#define	AUDIO_MICROPHONE	0x01	/* input from microphone */
#define	AUDIO_LINE_IN		0x02	/* input from line in */

/*
 * This macro initializes an audio_info structure to 'harmless' values.
 * Note that (~0) might not be a harmless value for a flag that was signed int.
 */
#define	AUDIO_INITINFO(i)	{					\
	unsigned int	*__x__;						\
	for (__x__ = (unsigned int *)(i);				\
	    (char *) __x__ < (((char *)(i)) + sizeof (audio_info_t));	\
	    *__x__++ = ~0);						\
}


/*
 * Ioctl calls for the audio device.
 */

/*
 * AUDIO_GETINFO retrieves the current state of the audio device.
 *
 * AUDIO_SETINFO copies all fields of the audio_info structure whose values
 * are not set to the initialized value (-1) to the device state.  It performs
 * an implicit AUDIO_GETINFO to return the new state of the device.  Note that
 * the record.samples and play.samples fields are set to the last value before
 * the AUDIO_SETINFO took effect.  This allows an application to reset the
 * counters while atomically retrieving the last value.
 *
 * AUDIO_DRAIN suspends the calling process until the write buffers are empty.
 *
 * AUDIO_GETDEV returns an integer (list below) indicating the currently
 * in use audio hardware configuration
 */
#define	AUDIO_GETINFO		_IOR(A, 1, audio_info_t)
#define	AUDIO_SETINFO		_IOWR(A, 2, audio_info_t)
#define	AUDIO_DRAIN		_IO(A, 3)
#define	AUDIO_GETDEV		_IOR(A, 4, int)

/* Define possible audio hardware configurations for AUDIO_GETDEV ioctl */
#define	AUDIO_DEV_UNKNOWN	(0)
#define	AUDIO_DEV_AMD		(1)
#define	AUDIO_DEV_SPEAKERBOX	(2)
#define	AUDIO_DEV_CODEC		(3)

#endif /* !_sun_audioio_h */
