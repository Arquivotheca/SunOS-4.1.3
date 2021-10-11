# ifndef lint
static char sccsid[] = "@(#)rex_xdr.c 1.1 92/07/30 Copyr 1985 Sun Micro";
# endif lint

/*
 * rex_xdr - remote execution external data representations
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include "rex.h"

/*
 * xdr_rex_start - process the command start structure
 */
xdr_rex_start(xdrs, rst)
	XDR *xdrs;
	struct rex_start *rst;
{
	return
		xdr_argv(xdrs, &rst->rst_cmd) &&
		xdr_string(xdrs, &rst->rst_host, 1024) &&
		xdr_string(xdrs, &rst->rst_fsname, 1024) &&
		xdr_string(xdrs, &rst->rst_dirwithin, 1024) &&
		xdr_argv(xdrs, &rst->rst_env) &&
		xdr_u_short(xdrs, &rst->rst_port0) &&
		xdr_u_short(xdrs, &rst->rst_port1) &&
		xdr_u_short(xdrs, &rst->rst_port2) &&
		xdr_u_long(xdrs, &rst->rst_flags);
}

xdr_argv(xdrs, argvp)
	XDR *xdrs;
	char ***argvp;
{
	register char **argv = *argvp;
	register char **ap;
	int i, count;

	/*
	 * find the number of args to encode or free
	 */
	if ((xdrs->x_op) != XDR_DECODE)
		for (count = 0, ap = argv; *ap != 0; ap++)
			count++;
	/* XDR the count */
	if (!xdr_u_int(xdrs, (unsigned *) &count))
		return (FALSE);

	/*
	 * now deal with the strings
	 */
	if (xdrs->x_op == XDR_DECODE) {
		*argvp = argv = (char **)mem_alloc((unsigned)(count+1)*sizeof (char **));
		for (i = 0; i <= count; i++)	/* Note: <=, not < */
			argv[i] = 0;
	}

	for (i = 0, ap = argv; i < count; i++, ap++)
		if (!xdr_string(xdrs, ap, 10240))
			return (FALSE);

	if (xdrs->x_op == XDR_FREE && argv != NULL) {
		mem_free((char *) argv, (count+1)*sizeof (char **));
		*argvp = NULL;
	}
	return (TRUE);
}

/*
 * xdr_rex_result - process the result of a start or wait operation
 */
xdr_rex_result(xdrs, result)
	XDR *xdrs;
	struct rex_result *result;
{
	return
		xdr_int(xdrs, &result->rlt_stat) &&
		xdr_string(xdrs, &result->rlt_message, 1024);

}

/*
 * xdr_rex_ttymode - process the tty mode information
 */
xdr_rex_ttymode(xdrs, mode)
	XDR *xdrs;
	struct rex_ttymode *mode;
{
  int six = 6;
  int four = 4;
  char *speedp = NULL;
  char *morep = NULL;
  char *yetmorep = NULL;

  if (xdrs->x_op != XDR_FREE) {
  	speedp = &mode->basic.sg_ispeed;
	morep = (char *)&mode->more;
  	yetmorep = (char *)&mode->yetmore;
  }
  return
	xdr_bytes(xdrs, (char **) &speedp, &four, 4) &&
	xdr_short(xdrs, &mode->basic.sg_flags) &&
	xdr_bytes(xdrs, (char **) &morep, &six, 6) &&
	xdr_bytes(xdrs, (char **) &yetmorep, &six, 6) &&
	xdr_u_long(xdrs, &mode->andmore);
}


/*
 * xdr_rex_ttysize - process the tty size information
 */
xdr_rex_ttysize(xdrs, size)
	XDR *xdrs;
	struct ttysize *size;
{
  return
	xdr_int(xdrs, &size->ts_lines) &&
	xdr_int(xdrs, &size->ts_cols);
}
