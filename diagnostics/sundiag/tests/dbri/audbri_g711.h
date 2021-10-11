#ident   "@(#)audbri_g711.h 1.1 92/07/30 SMI"

/*
 * Derived from:
 *	#ident "@(#)audio_encode.h      1.1     92/02/14 SMI"
 */

/* Copyright (c) 1992 by Sun Microsystems, Inc. */

/*
 * u-law, A-law and linear PCM conversion tables and macros.
 */

/* PCM linear <-> a-law conversion tables */
extern short		_alaw2linear[];		/* 8-bit a-law to 16-bit PCM */
extern unsigned char	*_linear2alaw;		/* 13-bit PCM to 8-bit a-law */

/* PCM linear <-> u-law conversion tables */
extern short		_ulaw2linear[];		/* 8-bit u-law to 16-bit PCM */
extern unsigned char	*_linear2ulaw;		/* 14-bit PCM to 8-bit u-law */

/* A-law <-> u-law conversion tables */
extern unsigned char	_alaw2ulaw[];		/* 8-bit A-law to 8-bit u-law */
extern unsigned char	_ulaw2alaw[];		/* 8-bit u-law to 8-bit A-law */

/* PCM linear <-> a-law conversion macros */

/* a-law to 8,16,32-bit linear */
#define	audio_a2c(X)	((char)(_alaw2linear[(unsigned char) (X)] >> 8))
#define	audio_a2s(X)	(_alaw2linear[(unsigned char) (X)])
#define	audio_a2l(X)	(((long)_alaw2linear[(unsigned char) (X)]) << 16)

/* 8,16,32-bit linear to a-law */
#define	audio_c2a(X)	(_linear2alaw[((short)(X)) << 5])
#define	audio_s2a(X)	(_linear2alaw[((short)(X)) >> 3])
#define	audio_l2a(X)	(_linear2alaw[((long)(X)) >> 19])

/* PCM linear <-> u-law conversion macros */

/* u-law to 8,16,32-bit linear */
#define	audio_u2c(X)	((char)(_ulaw2linear[(unsigned char) (X)] >> 8))
#define	audio_u2s(X)	(_ulaw2linear[(unsigned char) (X)])
#define	audio_u2l(X)	(((long)_ulaw2linear[(unsigned char) (X)]) << 16)

/* 8,16,32-bit linear to u-law */
#define	audio_c2u(X)	(_linear2ulaw[((short)(X)) << 6])
#define	audio_s2u(X)	(_linear2ulaw[((short)(X)) >> 2])
#define	audio_l2u(X)	(_linear2ulaw[((long)(X)) >> 18])

/* A-law <-> u-law conversion macros */

#define audio_a2u(X)	(_alaw2ulaw[(unsigned char)(X)])
#define audio_u2a(X)	(_ulaw2alaw[(unsigned char)(X)])
