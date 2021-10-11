/*	@(#)tcp_tli.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef	_TCP_TLI_
#define	_TCP_TLI_

/*
 * TLI automaton state definitions.
 *
 * They are expressed as bit masks to facilitate testing whether the
 * current automaton state is contained in a given set of states.
 */
#define	TL_UNINIT	0x0000	/* uninitialized */
#define	TL_UNBND	0x0002	/* unbound */
#define	TL_IDLE		0x0004	/* no connection established */
/*
 * outgoing connection pending for active user
 */
#define	TL_OUTCON	0x0008
/*
 * incoming connection pending for passive user
 */
#define	TL_INCON	0x0010
#define	TL_DATAXFER	0x0020	/* data transfer */
/*
 * outgoing orderly release (waiting for orderly release indication)
 */
#define	TL_OUTREL	0x0040
/*
 * incoming orderly release (waiting to send orderly release request)
 */
#define	TL_INREL	0x0080
/*
 * pseudo-state indicating disallowed transition (may end up unnecessary)
 */
#define	TL_ERROR	0x0100

/*
 * Max buffer size for each uio operation in tcptli_Ercv()
 */
#define	TT_BUFSIZE	2048

/*
 * Send and Recv size for socket operations
 */
#define	TT_SENDSIZE	24 * 1024
#define	TT_RECVSIZE	24 * 1024

/*
 * Max number of uio vectors for sosend and soreceive
 */
#define	TT_MAXUIO	10

/*
 * Flag to indicate that only part of the data buffer got sent
 */
#define	TT_INCOMPLETESEND	201

/*
 * Protocol options (socket options) supported by T_OPTMGMT_REQ
 */
struct	tt_soopt {
	int	tts_reuseaddr;	/* reuse a bound address */
	int	tts_keepalive;	/* keep connection alive */
	int	tts_sendsize;	/* socket send size */
	int	tts_recvsize;	/* socket recv size */
};

#define	TTS_BUFSIZE	4096	/* default socket send/recv size */
#define	TTS_DFLT_REUSEADDR	1
#define	TTS_DFLT_KEEPALIVE	1

#endif	_TCP_TLI_
