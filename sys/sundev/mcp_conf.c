#ifndef lint
static char	sccsid[] = "@(#)mcp_conf.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Sun MCP Multiprotocol Communications Processor
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 */

#include "mcp.h"
#include "mcpa.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/buf.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/clist.h>

#include <sundev/mbvar.h>
#include <sundev/zsreg.h>
#include <sundev/mcpreg.h>
#include <sundev/mcpcom.h>

#define NMCPLINE	(NMCP*16)
int		nmcp = NMCP;
int		nmcpline = NMCPLINE;

struct mcpcom	mcpcom[NMCPLINE + NMCP];
struct mb_device *mcpinfo[NMCP];
struct dma_chip dma_chip[NMCP * NDMA_CHIPS];
int		mcpsoftCAR[NMCP];

extern int	mcpprobe();
extern int	mcpattach();
extern int	mcpintr();
struct mb_driver mcpdriver = {
	mcpprobe, 0, mcpattach, 0, 0, mcpintr,
	sizeof(struct mcp_device), "mcp", mcpinfo, 0, 0, 0
};

#if NMCPA > 0

struct mcpaline mcpaline[NMCPLINE + NMCP];
extern struct mcpops mcpops_async;

#else NMCPA

int
mcpr_attach() { }

#endif NMCPA
