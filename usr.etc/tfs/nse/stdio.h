/*	@(#)stdio.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _NSE_STDIO_H
#define _NSE_STDIO_H

#include <stdio.h>

#ifndef _NSE_TYPES_H
#include <nse/types.h>
#endif

#ifndef _NSE_ERR_H
#include <nse/err.h>
#endif

/*
 * Wrapper routines which call routines in the stdio library and format
 * an error struct if there is an error.
 */
Nse_err		nse_fclose();
Nse_err		nse_fflush();
Nse_err		nse_fgetc();
Nse_err		nse_fgets();
Nse_err		nse_fopen();
Nse_err		nse_fprintf();
Nse_err		nse_fprintf_value();	/* Returns # of chars printed */
Nse_err		nse_fputc();
Nse_err		nse_fputs();
Nse_err		nse_fscanf();
Nse_err		nse_readdir();
Nse_err		nse_safe_write_open();
Nse_err		nse_safe_write_close();
Nse_err		nse_file_open();

#endif _NSE_STDIO_H
