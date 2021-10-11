/*	@(#)mcpbuf.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sundev_mcpbuf_h
#define _sundev_mcpbuf_h

struct zs_dmabuf {
	u_char *d_baddr;  /* base addr of the buf */
	short   d_wc;     /* current word count */
};

#endif /*!_sundev_mcpbuf_h*/
