#ifndef lint
static char sccsid[] = "@(#)err.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* these defines should preceed all #include lines in this file */
#define NSE_ERROR_COLLECT
#define NSE_END_ERROR_TAB (char *) 0
#define NSE_BEGIN_ERROR_TAB (char *) 0

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <nse/types.h>
#include <nse/param.h>
#include <nse/util.h>
#include <rpc/rpc.h>

extern long strtol();

extern int sys_nerr;
extern char *sys_errlist[];

static	char		_nse_err_string[MAXPATHLEN];
static	Nse_err_rec	_nse_err = { 0, _nse_err_string };

/* any include file which defines type-independent error messages should be
 * included here (please include comment indicating range of numbers used by
 * the new package - must be negative)
 *
 * to add new packages create the appropriate entries in an include file
 * (use section controlled by #ifdef NSE_ERROR_COLLECT in any include file
 * listed just above as a model) and then include that file above.  Pick an
 * an unused range of error numbers (the pattern should be obvious) and a
 * unique name for the table (be sure to put new name in redefinition of
 * NSE_BEGIN_ERROR_TAB)
 *
 * the base (xx00) for the most negative range of error numbers used by the
 * type independent code (or by type specific code which has not been
 * converted) must be entered into the types configuration files.  The ranges
 * used by unconverted type specific packages must also be entered in that
 * file.
 *
 */
#include <nse/searchlink.h>	/* -701 -> -799 */
#include <nse/tfs_calls.h>	/* -901 -> -999 */
#include <nse/nse_rpc.h>	/* -2201 -> -2299 */
#include <nse/file_err.h>	/* -2501 -> -2599 */

/*
 * put error code and command name into a fixed static error structure and
 * return a pointer to that structure and a pointer to the first free byte in
 * the message area
 */
Nse_err
nse_err_alloc_rec(code, strp)
	int		code;
	char		**strp;
{
	Nse_err		err;
	char		*name;
	int		len;

	err = &_nse_err;
	err->code = code;
	err->str[0] = '\0';
	name = nse_get_cmdname();
	if (name != NULL) {
		sprintf(err->str, "%s: ", name);
	}
	len = strlen(err->str);
	*strp = &err->str[len];
	return err;
}

/*
 * search a linked chain of error tables of the form:
 *
 *	table[0] - "table name"
 *	table[1] - "highest lowest"
 *	table[2] - link to next table
 *	....     - error messages
 *	table[n] - NULL
 *
 * The search can be done in two ways.  If name != NULL, a table in the chain
 * is found where name == "table name".  Otherwise the first table for which
 * the negative error code is in the range [lowest ... highest] is selected.
 * In either case a message is selected from the table by indexing with
 * highest - code.
 *
 * For type independent code, the second method is used.  For type specific
 * code, the first method is used so that the number ranges givn by the
 * type implementors in each table may overlap.
 * 
 * ARGSUSED
 */
void
nse_search_err_table(tables, name, code, str,
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	char	**tables;
	char	*name;
	int	code;
	char	*str;
{
	int	high, low;
	int	i;
	char	*msg;

	while (tables) {
		if ((!tables[0]) || (!tables[1]) ||
		    (!isprint(tables[0][0])) ||
		    (tables[1][0] != '-')) {
			tables = (char **) 0;
			break;
		}
		if (name) {
			if (NSE_STREQ(name, tables[0])) {
				high = (int) strtol(tables[1], &msg, 10);
				low = (int) strtol(msg, (char **) NULL, 10);
				break;
			}
		} else {
			high = (int) strtol(tables[1], &msg, 10);
			low = (int) strtol(msg, (char **) NULL, 10);
			/* sscanf(tables[1], "%d%d", &high, &low); */
			if ((code <= high) && (code >= low)) {
				break;
			}
		}
		tables = (char **) tables[2];
	}
	if (tables) {
		for (i = 0; ((high - i) > code) && tables[i + 3];
			i++);
		if (tables[i+3] && isprint(tables[i+3][0])) {
			msg = tables[3+i];
			sprintf(str, msg, arg1, arg2, arg3, arg4,
			    arg5, arg6, arg7, arg8);
		} else {
			sprintf(str, "Undefined error number %d", 
			    code);
		}
	} else {
		sprintf(str, "Error number %d out of range", code);
	}
}

/*
 * Format an error message and return an err struct.
 * The message has the form:
 *	cmdname: text
 * A template for the text portion is looked up in an error message
 * table, and the caller is expected to pass in parameters suitable
 * for substitution.
 * VARARGS1
 * ARGSUSED
 */
Nse_err
nse_err_format(code, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	int		code;
{
	Nse_err		err;
	char		**tables;
	char		*str;

	err = nse_err_alloc_rec(code, &str);
	if (code == 0) {
		nse_err_destroy(err);
		return NULL;
	} else if (code == -1 || code > 0) {
		sprintf(str, "(%d): unknown failure\n", code);
	} else {
		tables = (char **) NSE_BEGIN_ERROR_TAB;
		nse_search_err_table(tables, (char *) NULL, code, str,
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
	}
	return err;
}

/*
 * Format and errno error message and return an err struct.
 * The caller passing in a template and some arguments and
 * these along with the standard system messags for errnos
 * are used to construct the error message.
 * VARARGS1
 * ARGSUSED
 */
Nse_err
nse_err_format_errno(template, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	char		*template;
{
	Nse_err		err;
	char		*str;

	err = nse_err_alloc_rec(errno, &str);
	sprintf(str, template, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
	strcat(err->str, " failed - ");
	if (errno < sys_nerr && errno > 0) {
		strcat(err->str, sys_errlist[errno]);
	} else {
		str = err->str + strlen(err->str);
		sprintf(str, "Errno %d not recognized", errno);
	}
	return err;
}

/*
 * Build an err struct for a user defined message.
 * The user passes in a template and args.  The
 * error code is irrelevant and thus set to zero.
 * VARARGS1
 * ARGSUSED
 */
Nse_err
nse_err_format_msg(template, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	char		*template;
{
	Nse_err		err;
	char		*str;

	err = nse_err_alloc_rec(0, &str);
	if (template != NULL) {
		sprintf(str, template, arg1, arg2, arg3, arg4, arg5, arg6,
		    arg7, arg8);
	}
	return err;
}

Nse_err
nse_err_format_rpc(code, program, host, rpc_err)
	int		code;
	char		*program;
	char		*host;
	struct rpc_err	*rpc_err;
{
	Nse_err		err;
	char		*str;

	err = nse_err_format(code, program, host);
	switch (code) {
	case NSE_RPC_ERROR:
		strcat(err->str, clnt_sperrno(rpc_err->re_status));
		break;
	case NSE_RPC_CANT_CONNECT:
		rpc_err = &rpc_createerr.cf_error;
		strcat(err->str, clnt_sperrno(rpc_createerr.cf_stat));
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
			strcat(err->str, " - ");
			strcat(err->str, clnt_sperrno(rpc_err->re_status));
		} else {
			rpc_err->re_status = rpc_createerr.cf_stat;
		}
	}
	str = err->str + strlen(err->str);
	switch (rpc_err->re_status) {
	case RPC_CANTSEND:
	case RPC_CANTRECV:
	case RPC_SYSTEMERROR:
		sprintf(str, " - %s", sys_errlist[rpc_err->re_errno]);
		break;
	case RPC_VERSMISMATCH:
	case RPC_PROGVERSMISMATCH:
		sprintf(str, "; low version = %lu, high version = %lu", 
			rpc_err->re_vers.low, rpc_err->re_vers.high);
		break;
	case RPC_AUTHERROR:
		sprintf(str, "; why = %d", (int) rpc_err->re_why);
		break;
	}
	return err;
}

/*
 * Print a message and free the storage for it.
 */
void
nse_err_print(err)
	Nse_err		err;
{
	if (err != NULL) {
		fprintf(stderr, "%s\n", err->str);
	}
}

/*
 * Save an error struct (useful in routines which, when an error is detected,
 * try to clean up by calling other routines which may generate a new error.
 */
void
nse_err_save(err)
	Nse_err		*err;
{
	static Nse_err	local_err;

	if (*err == local_err) {
		return;
	}
	if (local_err) {
		nse_err_destroy(local_err);
	}
	local_err = nse_err_copy(*err);
	*err = local_err;
}

/*
 * Free storage.
 */
void
nse_err_destroy(err)
	Nse_err		err;
{
	if (err != NULL && err != &_nse_err) {
		NSE_DISPOSE(err->str);
		NSE_DISPOSE(err);
	}
}

/*
 * Copy an error struct.
 */
Nse_err
nse_err_copy(from)
	Nse_err		from;
{
	Nse_err		to;

	to = NULL;
	if (from != NULL) {
		to = NSE_NEW(Nse_err);
		to->code = from->code;
		to->str = NSE_STRDUP(from->str);
	}
	return to;
}

/*
 * Xdr a pointer to an Nse_err structure.
 * When decoding, put the results in _nse_err and
 * back patch a pointer to _nse_err into the given arg.
 */
bool_t
xdr_nse_err_ref(xdrs, errp)
	XDR		*xdrs;
	Nse_err		*errp;
{
	if (xdrs->x_op == XDR_FREE && *errp == &_nse_err) {
		return TRUE;		/* Points to static storage */
	}
	if (xdrs->x_op == XDR_DECODE && *errp == NULL) {
		*errp = &_nse_err;
	}
	if (!xdr_pointer(xdrs, (char **)errp, sizeof(struct Nse_err), 
	    xdr_nse_err)) {
		return FALSE;
	}
	return TRUE;
}

/*
 * XDR an Nse_err struct.
 */
bool_t
xdr_nse_err(xdrs, errp)
	XDR		*xdrs;
	Nse_err		errp;
{
	if (!xdr_int(xdrs, &errp->code)) {
		return FALSE;
	}
	if (!xdr_wrapstring(xdrs, &errp->str)) {
		return FALSE;
	}
	return TRUE;
}
