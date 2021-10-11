#ifndef lint
static  char sccsid[] = "@(#)mcp_proto.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 */

#include "mcp.h"
#include "mcph.h"
#include "mcpa.h"
#include "mcps.h"
#include "mcpb.h"

#if NMCP > 0
#include <sys/types.h>
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/buf.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/clist.h>

#include <sundev/zsreg.h>
#include <sundev/mcpreg.h>
#include <sundev/mcpcom.h>


#if NMCPA > 0
extern struct mcpops  mcpops_async;
#endif

#if NMCPH > 0
extern struct mcpops  mcpops_sdlc;
#endif

#if NMCPS > 0
extern struct mcpops  mcpops_isdlc;
#endif

#if NMCPB > 0
extern struct mcpops  mcpops_bsc;
#endif

struct  mcpops *mcp_proto[] = {
#if NMCPA > 0
        &mcpops_async,
#endif
#if NMCPH > 0
        &mcpops_sdlc,
#endif

#if NMCPS > 0
        &mcpops_isdlc,
#endif

#if NMCPB > 0
        &mcpops_bsc,
#endif

        0,     /* must be last */
};
#endif


