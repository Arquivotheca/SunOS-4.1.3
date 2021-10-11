#ifndef lint
static	char sccsid[] = "@(#)init_dbx.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is compiled using the "-g" flag to generate the
 * minimum required structure information which is required by
 * dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
