#ifndef lint
static	char sccsid[] = "@(#)zs_conf.c 1.1 92/07/30 SMI";
#endif

#include "zs.h"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/ttycom.h>
#include <sys/tty.h>
#include <sundev/zsreg.h>
#include <sundev/zscom.h>
#include <machine/param.h>	/* for OPENPROMS, if present */

#ifdef	OPENPROMS

char *zssoftCAR;
int nzs = 0;
struct zscom *zscom;
struct zsaline *zsaline;

#else	OPENPROMS

#define	NZSLINE	(NZS*2)
char zssoftCAR[NZSLINE];
int nzs = NZSLINE;
struct zscom zscom[NZSLINE];
struct zsaline zsaline[NZSLINE];

#endif	OPENPROMS
