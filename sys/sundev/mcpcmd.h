/* @(#)mcpcmd.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 */

#ifndef _sundev_mcpcmd_h
#define _sundev_mcpcmd_h

#include <sys/ioccom.h>

#define MCPIOSPR	_IOWR(M, 1, u_char)	/* ioctl to set printer diag
						 * mode */
#define MCPIOGPR	_IOR(M, 1, u_char)	/* ioctl to set printer diag
						 * mode */
#define MCPRIGNSLCT	0x02	/* set = ignore hardware slct sig */
#define MCPRDIAG	0x04	/* set = activate diag mode */
#define MCPRVMEINT	0x08	/* set = VME interrupts enabled */
#define MCPRINTPE	0x10	/* set = interrupt on pe false */
#define MCPRINTSLCT	0x20	/* set = interrupt on slct false */
#define MCPRPE		0x40	/* set = printer ok, clr = paper out */
#define MCPRSLCT	0x80	/* set = printer online */

#endif /*!_sundev_mcpcmd_h*/
