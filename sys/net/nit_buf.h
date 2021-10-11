/*	@(#)nit_buf.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _net_nit_buf_h
#define _net_nit_buf_h

/*
 * Definitions for the streams NIT buffering module.
 *
 * The module gathers incoming (read-side) messages together into
 * "chunks" and passes completed chunks up to the next module in
 * line.  The gathering process is controlled by two ioctl-settable
 * parameters:
 *
 * timeout	The maximum delay after passing the previous chunk
 *		upward before passing the current one up, even if the
 *		chunk isn't full.  If the timeout value passed in is
 *		a null pointer, the timeout is infinite (as in the
 *		select system call); this is the default.
 * chunksize	The maximum size of a chunk of accumulated messages,
 *		unless a single message exceeds the chunksize, in
 *		which case it's passed up in a chunk containing only
 *		that message.  Note that a given message's size includes
 *		the length of any leading M_PROTO blocks it may have.
 *
 * There is one important side-effect: setting the timeout to zero
 * (polling) will force the chunksize to zero, regardless of its
 * previous setting.
 */

/*
 * Ioctls.
 */
#define NIOCSTIME	_IOW(p, 6, struct timeval)	/* set timeout info */
#define NIOCGTIME	_IOWR(p, 7, struct timeval)	/* get timeout info */
#define NIOCCTIME	_IO(p, 8)			/* clear timeout */
#define NIOCSCHUNK	_IOW(p, 9, u_int)		/* set chunksize */
#define NIOCGCHUNK	_IOWR(p, 10, u_int)		/* get chunksize */

/*
 * Default chunk size.
 */
#define NB_DFLT_CHUNK	8192	/* arbitrary */

/*
 * When adding a given message to an accumulating chunk, the module
 * first converts all leading M_PROTO data blocks to M_DATA data blocks.
 * It then constructs a nit_bufhdr (defined below), prepends it to
 * the message, and pads the result out to force its length to a
 * multiple of a machine-dependent alignment size guaranteed to be
 * at least sizeof (u_long).  It then adds the padded message to the
 * chunk.
 *
 * The first field of the header is length of the message after the
 * M_PROTO => M_DATA conversion, but before adding the header.
 *
 * The second header field is the total length of the message,
 * including both the header itself and the trailing padding bytes.
 */
struct nit_bufhdr {		/* see above commentary */
	u_int		nhb_msglen;
	u_int		nhb_totlen;
};

#endif /*!_net_nit_buf_h*/
