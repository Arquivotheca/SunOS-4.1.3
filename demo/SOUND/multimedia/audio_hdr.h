/*	@(#)audio_hdr.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_audio_hdr_h
#define	_multimedia_audio_hdr_h

/*
 * Define an in-core audio data header.
 *
 * This is different that the on-disk file header.
 * The fields defined here are preliminary at best.
 */

/*
 * The audio header contains the following fields:
 *
 *	sample_rate		Number of samples per second (per channel).
 *
 *	samples_per_unit	This field describes the number of samples
 *				  represented by each sample unit (which, by
 *				  definition, are aligned on byte boundaries).
 *				  Audio samples may be stored individually
 *				  or, in the case of compressed formats
 *				  (e.g., ADPCM), grouped in algorithm-
 *				  specific ways.  If the data is bit-packed,
 *				  this field tells the number of samples
 *				  needed to get to a byte boundary.
 *
 *	bytes_per_unit		Number of bytes stored for each sample unit
 *
 *	channels		Number of interleaved sample units.
 *				   For any given time quantum, the set
 *				   consisting of 'channels' sample units
 *				   is called a sample frame.  Seeks in
 *				   the data should be aligned to the start
 *				   of the nearest sample frame.
 *
 *	encoding		Data encoding format.
 *
 *	data_size		Number of bytes in the data.
 *				   This value is advisory only, and may
 *				   be set to the value AUDIO_UNKNOWN_SIZE
 *				   if the data size is unknown (for
 *				   instance, if the data is being
 *				   recorded or generated and piped
 *				   to another process).
 *
 * The first four values are used to compute the byte offset given a
 * particular time, and vice versa.  Specifically:
 *
 *	seconds = offset / C
 *	offset = seconds * C
 * where:
 *	C = (channels * bytes_per_unit * sample_rate) / samples_per_unit
 */
typedef struct {
	unsigned	sample_rate;		/* samples per second */
	unsigned	samples_per_unit;	/* samples per unit */
	unsigned	bytes_per_unit;		/* bytes per sample unit */
	unsigned	channels;		/* # of interleaved channels */
	unsigned	encoding;		/* data encoding format */

	unsigned	data_size;		/* length of data (optional) */
} Audio_hdr;

/*
 * Define the possible encoding types.
 * Note that the names that overlap the encodings in <sun/audioio.h>
 * must have the same values.
 */
#define	AUDIO_ENCODING_ULAW	(1)	/* ISDN u-law */
#define	AUDIO_ENCODING_ALAW	(2)	/* ISDN A-law */
#define	AUDIO_ENCODING_LINEAR	(3)	/* PCM 2's-complement (0-center) */
#define	AUDIO_ENCODING_FLOAT	(4)	/* IEEE float (-1. <-> +1.) */


/* Value used for indeterminate size (e.g., data passed through a pipe) */
#define	AUDIO_UNKNOWN_SIZE	((unsigned)(~0))


/* Define conversion macros for integer<->floating-point conversions */

/* double to 8,16,32-bit linear */
#define	audio_d2c(X)		((X) >= 1. ? 127 : (X) <= -1. ? -127 :	\
				    (char)(irint((X) * 127.)))
#define	audio_d2s(X)		((X) >= 1. ? 32767 : (X) <= -1. ? -32767 :\
				    (short)(irint((X) * 32767.)))
#define	audio_d2l(X)		((X) >= 1. ? 2147483647 : (X) <= -1. ?	\
				    -2147483647 :			\
				    (long)(irint((X) * 2147483647.)))
				    

/* 8,16,32-bit linear to double */
#define	audio_c2d(X)		(((unsigned char)(X)) == 0x80 ? -1. :	\
				    ((double)((char)(X))) / 127.)
#define	audio_s2d(X)		(((unsigned short)(X)) == 0x8000 ? -1. :\
				    ((double)((short)(X))) / 32767.)
#define	audio_l2d(X)		(((unsigned long)(X)) == 0x80000000 ? -1. :\
				    ((double)((long)(X))) / 2147483647.)


#endif /*!_multimedia_audio_hdr_h*/
