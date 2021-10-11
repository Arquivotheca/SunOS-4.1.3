/*	@(#)audio_filehdr.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_audio_filehdr_h
#define	_multimedia_audio_filehdr_h

#include "archdep.h"

/*
 * Define an on-disk audio file header.
 *
 * This structure should not be arbitrarily imposed over a stream of bytes,
 * since the byte orders could be wrong.
 *
 * Note that there is an 'info' field that immediately follows this
 * structure in the file.
 *
 * The hdr_size field is problematic in the general case because the
 * field is really "data location", which does not ensure that all
 * the bytes between the header and the data are really 'info'.
 * Further, there are no absolute guarantees that the 'info' is ASCII text,
 * (non-ASCII info may eventually be far more useful anyway).
 *
 * When audio files are passed through pipes, the 'data_size' field may
 * not be known in advance.  In such cases, the 'data_size' should be
 * set to AUDIO_UNKNOWN_SIZE.
 */
typedef struct {
	u_32		magic;		/* magic number */
	u_32		hdr_size;	/* size of this header */
	u_32		data_size;	/* length of data (optional) */
	u_32		encoding;	/* data encoding format */
	u_32		sample_rate;	/* samples per second */
	u_32		channels;	/* number of interleaved channels */
} Audio_filehdr;


/* Define the magic number */
#define	AUDIO_FILE_MAGIC		((u_32)0x2e736e64)

/* Define the encoding fields */
#define	AUDIO_FILE_ENCODING_MULAW_8	(1)	/* 8-bit ISDN u-law */
#define	AUDIO_FILE_ENCODING_LINEAR_8	(2)	/* 8-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_16	(3)	/* 16-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_24	(4)	/* 24-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_32	(5)	/* 32-bit linear PCM */
#define	AUDIO_FILE_ENCODING_FLOAT	(6)	/* 32-bit IEEE floating point */
#define	AUDIO_FILE_ENCODING_DOUBLE	(7)	/* 64-bit IEEE floating point */

#endif /*!_multimedia_audio_filehdr_h*/
