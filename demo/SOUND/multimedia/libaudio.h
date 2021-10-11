/*	@(#)libaudio.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_libaudio_h
#define	_multimedia_libaudio_h

#include "audio_errno.h"
#include "audio_hdr.h"

/* define various constants for general use */

/* Theoretical maximum length of hh:mm:ss.dd string */
#define	AUDIO_MAX_TIMEVAL	(32)

/* Theoretical maximum length of encoding information string */
#define	AUDIO_MAX_ENCODE_INFO	(80)


/* Why aren't these stupid values defined in a standard place?! */
#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif
#ifndef NULL
#define	NULL	0
#endif


/* Declare routines that return non-int values */

extern double	audio_bytes_to_secs(/* Audio_hdr *hp, unsigned cnt */);
extern unsigned	audio_secs_to_bytes(/* Audio_hdr *hp, double sec */);
extern double	audio_str_to_secs(/* char *str */);
extern char	*audio_secs_to_str(/* double sec, char *str, int precision */);


#endif /*!_multimedia_libaudio_h*/
