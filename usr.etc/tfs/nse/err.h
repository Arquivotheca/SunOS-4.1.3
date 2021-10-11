/*	@(#)err.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _NSE_ERR_H
#define _NSE_ERR_H

/*
 * Include rpc/types.h to get bool_t, TRUE and FALSE.
 * The window system headers also defined TRUE and FALSE
 * and rpc/types.h does not check so we undefine them here
 * just to be sure.
 */
#ifndef bool_t
#undef TRUE
#undef FALSE
#include <rpc/types.h>
#endif

typedef struct	Nse_err		*Nse_err;
typedef	struct	Nse_err		Nse_err_rec;

struct	Nse_err	{
	int	code;			/* Error number */
	char	*str;			/* Text of message */
};

#define NSE_OK (Nse_err) 0

Nse_err		nse_err_alloc_rec();
void		nse_search_err_table();
Nse_err		nse_err_format();
Nse_err		nse_err_format_errno();
Nse_err		nse_err_format_msg();
Nse_err		nse_err_format_rpc();
void		nse_err_print();
void		nse_err_save();
void		nse_err_destroy();
Nse_err		nse_err_copy();
bool_t		xdr_nse_err_ref();
bool_t		xdr_nse_err();

#endif
