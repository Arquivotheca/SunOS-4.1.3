/*	@(#)audio_errno.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_audio_errno_h
#define	_multimedia_audio_errno_h

/*
 * libaudio error codes
 */
#define	AUDIO_SUCCESS		(0)		/* no error */
#define	AUDIO_UNIXERROR		(-1)		/* check errno for error code */

#define	AUDIO_ERR_BADHDR	(1)		/* bad audio header structure */
#define	AUDIO_ERR_BADFILEHDR	(2)		/* bad file header format */
#define	AUDIO_ERR_BADARG	(3)		/* bad subroutine argument */
#define	AUDIO_ERR_NOEFFECT	(4)		/* device control ignored */
#define	AUDIO_ERR_ENCODING	(5)		/* unknown encoding format */
#define	AUDIO_ERR_INTERRUPTED	(6)		/* operation was interrupted */

#endif /*!_multimedia_audio_errno_h*/
