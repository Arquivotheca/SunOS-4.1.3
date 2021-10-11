#ifndef lint
static	char sccsid[] = "@(#)str_conf.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/stream.h>

#include "nbuf.h"
#include "pf.h"
#include "kb.h"
#include "ms.h"
#include "tim.h"
#include "tirw.h"
#include "db.h"
#ifdef sun4c
#include "audioamd.h"
#else
#define	NAUDIOAMD	0
#endif

#if	NNBUF > 0
extern struct streamtab	nb_info;
#endif
#if	NPF > 0
extern struct streamtab	pf_info;
#endif
#if	NKB > 0
extern struct streamtab kbd_info;
#endif
#if	NMS > 0
extern struct streamtab ms_info;
#endif
#if	NTIM > 0
extern struct streamtab timinfo;
#endif
#if	NTIRW > 0
extern struct streamtab trwinfo;
#endif
#if	NDB > 0
extern struct streamtab dbinfo;
#endif
#if	NAUDIOAMD > 0
extern struct streamtab audio_79C30_info;
#endif

extern struct streamtab	ldtrinfo;
extern struct streamtab	ttycompatinfo;

struct	fmodsw fmodsw[] =
{
#if	NNBUF > 0
	{ "nbuf",	&nb_info },
#endif
#if	NPF > 0
	{ "pf",		&pf_info },
#endif
#if	NKB > 0
	{ "kb",		&kbd_info },
#endif
#if	NMS > 0
	{ "ms",		&ms_info },
#endif
#if	NTIM > 0
	{ "timod",	&timinfo },
#endif
#if	NTIRW > 0
	{ "tirdwr",	&trwinfo },
#endif
#if	NDB > 0
	{ "db",	 &dbinfo },
#endif
#if	NAUDIOAMD > 0
	{ "audioamd",	&audio_79C30_info },
#endif
	{ "ldterm",	&ldtrinfo },
	{ "ttcompat",	&ttycompatinfo },
#ifdef VDDRV
	{ "",		0 },	/* reserved for loadable modules */
	{ "",		0 },
	{ "",		0 },
	{ "",		0 },
	{ "",		0 },
	{ "",		0 },
	{ "",		0 },
	{ "",		0 },
#endif VDDRV
	{ "",		0 }
};

int	fmodcnt = sizeof (fmodsw) / sizeof (fmodsw[0]) - 1;
