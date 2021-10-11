/*	@(#)ulaw2linear.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_ulaw2linear_h
#define	_multimedia_ulaw2linear_h

/* PCM linear <-> u-law conversion tables */
extern short		_ulaw2linear[];		/* 8-bit u-law to 16-bit PCM */
extern unsigned char	*_linear2ulaw;		/* 13-bit PCM to 8-bit u-law */


/* PCM linear <-> u-law conversion macros */

/* u-law to 8,16,32-bit linear */
#define	audio_u2c(X)	((char)(_ulaw2linear[(unsigned char) (X)] >> 8))
#define	audio_u2s(X)	(_ulaw2linear[(unsigned char) (X)])
#define	audio_u2l(X)	(((long)_ulaw2linear[(unsigned char) (X)]) << 16)

/* 8,16,32-bit linear to u-law */
#define	audio_c2u(X)	(_linear2ulaw[(((short)(X)) << 5) + 0xf])
#define	audio_s2u(X)	(_linear2ulaw[((short)(X)) >> 3])
#define	audio_l2u(X)	(_linear2ulaw[((long)(X)) >> 19])

#endif /*!_multimedia_ulaw2linear_h*/
